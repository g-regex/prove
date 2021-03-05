/* [prove]: A proof verification system using bracketed expressions.
 * Copyright (C) 2020-2021  Gregor Feierabend
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pgraph.h"
#include "verify.h"
#include "debug.h"

#ifdef DVERIFY
#include "stdio.h"
#endif

/* --- preprocessor directives ---------------------------------------------- */
#define TRUE 1
#define FALSE 0

/* explorable are branches, which contain either an implication or an equality,
 * which is not an assumption
 * TODO: verify equalities, which are assumptions - but not by exploration */
#define EXPLORABLE(pexplorer) \
	(HAS_CHILD((*pexplorer)) && (HAS_NFLAG_IMPL((*((*pexplorer)->child)))\
			|| (!HAS_NFLAG_FRST((*pexplorer))\
				&& HAS_NFLAG_EQTY((*((*pexplorer)->child))))))

/* --- function prototypes ---------------------------------------------------*/
unsigned short int check_asmp(Pnode* perspective, Pnode** pexplorer,
		unsigned short int exst);
void finish_sub(VFlags* vflags, SUB** subd);
unsigned short int next_forwards(Pnode* perspective, Pnode** pexplorer,
		Eqwrapper** eqwrapper, BC** checkpoint, VFlags* vflags, unsigned short
		int p_a);

/* --- verification specific movement functions ----------------------------- */
/**
 * @brief Moves pexplorer to the right, if possible. Wrap around to the left, if
 * VFLAG_WRAP is set.
 *
 * @param pexplorer double-pointer to be moved, pointing to a Pnode
 * @param eqwrapper double-pointer to struct containing wrapping information
 * @param vflags	pointer to Vflags specific to current substitution
 *
 * @return 
 */
unsigned short int wrap_right(Pnode** pexplorer, Eqwrapper** eqwrapper,
		VFlags* vflags)
{
	if (!move_right(pexplorer)) {
		if (HAS_VFLAG_WRAP(*vflags)) {
			*pexplorer = (*eqwrapper)->pwrapper;
		} else {
			return FALSE;
		}
	}
	return TRUE;
}

/* --- substitution --------------------------------------------------------- */
/**
 * @brief Initialises known_const field in SUB structure to first eligible
 * constants or ids.
 *
 * @param perspective Pnode from whose perspective constants are known
 * @param s SUB structure to be initialised
 * @param idonly TRUE if only ids are considered eligible constants
 * @param exst TRUE if doing a backwards substitution for existence verification
 * @param exnum Pnode number which must not be exceeded by replaced constants
 * (not carrying the FRST NFLAG)
 *
 * @return 
 */
unsigned short int init_known_const(Pnode* perspective, SUB* s,
		unsigned short int idonly, unsigned short int exst, int exnum)
{
	if (!HAS_VARFLAG_FRST(s->vtree->flags)) {
		idonly = TRUE;
	}

	if (idonly) {
		s->known_const = perspective->prev_id;
	} else {
		s->known_const = perspective->prev_const;
	}

	while (s->known_const != NULL && (exst && 
				!HAS_VARFLAG_FRST(s->vtree->flags) &&
				s->known_const->num < exnum)) {
		if (idonly) {
			s->known_const = s->known_const->prev_id;
		} else {
			s->known_const = s->known_const->prev_const;
		}
	}

	return (s->known_const != NULL);
}

/**
 * @brief Substitutes variable specified in SUB by current value of known_const.
 *
 * @param s SUB structure to be operated on
 */
void sub_var(SUB* s)
{
	/* ATTENTION: This approach to substitution relies on the fact that we only
	 * move rightwards and downwards, while the substitution is in place. */
	if ((*(s->known_const->child))->symbol != NULL) {
		*(s->vtree->pnode->symbol) = *((*(s->known_const->child))->symbol);
	} else {
		*(s->vtree->pnode->symbol) = NULL;
	}

	if ((*(s->known_const->child))->child != NULL) {
		*(s->vtree->pnode->child) = *((*(s->known_const->child))->child);
	} else {
		*(s->vtree->pnode->child) = NULL;
	}

	if ((*(s->known_const->child))->right != NULL) {
		*(s->vtree->pnode->right) = *((*(s->known_const->child))->right);
	} else {
		*(s->vtree->pnode->right) = NULL;
	}

	/* number has to be substituted to ensure correct handling of existential
	 * quantifiers */
	s->vtree->pnode->parent->num = s->known_const->num;
}

/**
 * @brief Performs next substitution of variables specified in SUB. If last
 * eligible constant is reached, recursively perform next substitution on
 * following SUB (in linked list) and restart substitution in SUB.
 *
 * @param perspective Pnode from which constants should be known
 * @param s SUB to be operated on
 * @param idonly TRUE if only ids are considered eligible constants
 * @param exst TRUE if doing a backwards substitution for existence verification
 * @param exnum Pnode number which must not be exceeded by replaced constants
 * (not carrying the FRST NFLAG)
 *
 * @return FALSE if no more substitutions are possible
 */
unsigned short int next_sub(Pnode* perspective, SUB* s,
		unsigned short int idonly, unsigned short int exst, int exnum)
{
	SUB* s_iter;
	
	s_iter = s;

	while (s_iter != NULL) {
			if (idonly || !HAS_VARFLAG_FRST(s->vtree->flags)) {
				s_iter->known_const = s_iter->known_const->prev_id;
			} else {
				s_iter->known_const = s_iter->known_const->prev_const;
			}

			if (s_iter->known_const == NULL || (exst && 
						!HAS_VARFLAG_FRST(s_iter->vtree->flags) &&
						s_iter->known_const->num_c < exnum)) {
				init_known_const(perspective, s_iter, idonly, exst, exnum);
				sub_var(s_iter);
				s_iter = s_iter->prev;
			} else {
				sub_var(s_iter);
				return TRUE;
			}
	}
	return FALSE; /* finish substitution */
}

/**
 * @brief Initialises SUB structure for substitution.
 *
 * @param perspective Pnode from whose perspective constants are known
 * @param vtree variable tree holding pointers to Pnodes for substituion
 * @param vflags Vflags corresponding to current substituion
 * @param subd SUB structure to be initialised
 * @param idonly TRUE if only ids are considered eligible constants
 * @param exst TRUE if doing a backwards substitution for existence verification
 * @param exnum Pnode number which must not be exceeded by replaced constants
 * (not carrying the FRST NFLAG)
 *
 * @return FALSE if initialisation fails (due to no available constants to be
 * substituted in)
 */
unsigned short int init_sub(Pnode* perspective, VTree* vtree, VFlags* vflags,
		SUB** subd, unsigned short int idonly, unsigned short int exst,
		int exnum)
{
	SUB* prev;

	vtree = pos_in_vtree(vtree);
	prev = *subd;

	/* only substitute, if there is anything to substitute in */
	if ((!idonly && perspective->prev_const != NULL) 
				|| (idonly && perspective->prev_id != NULL)) {

		do {
			/* - do not substitute locked variables
			 * - do not attempt to substitute if node in vtree is holding a
			 *   branch
			 */
			if (!HAS_VARFLAG_LOCK(vtree->flags) && vtree->pnode != NULL) {
				SET_VARFLAG_LOCK(vtree->flags)

				*subd = (SUB*) malloc(sizeof(SUB));
				(*subd)->prev = prev;
				prev = *subd;

				(*subd)->sym = *(vtree->pnode->symbol);
				(*subd)->num = vtree->pnode->parent->num;
				(*subd)->vtree = vtree;

				if (!init_known_const(perspective, *subd, idonly, exst,
							exnum)) {
					finish_sub(vflags, subd);
					return FALSE;
				}

				sub_var(*subd);
			}
			vtree = next_var(vtree);
		} while (vtree != NULL);

		SET_VFLAG_SUBD(*vflags)
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
 * @brief Substitutes original identifiers back into variable Pnodes.
 *
 * @param vflags Vflags corresponding to current substitution
 * @param subd SUB structure holding information of current substitution
 */
void finish_sub(VFlags* vflags, SUB** subd)
{
	SUB* prev_sub;

	UNSET_VFLAG_SUBD(*vflags)

	//TODO: beautify
	if (*subd == NULL) {
		return;
	}

	do {
		prev_sub = (*subd)->prev;

		*((*subd)->vtree->pnode->symbol) = (*subd)->sym;
		(*subd)->vtree->pnode->parent->num = (*subd)->num;
		*((*subd)->vtree->pnode->child) = NULL;
		*((*subd)->vtree->pnode->right) = NULL;

		UNSET_VARFLAG_LOCK(((*subd)->vtree->flags))

		free(*subd);
		*subd = prev_sub;
	} while (*subd != NULL);
}

#ifdef DVERIFY
/**
 * @brief Prints debugging information about substitution.
 *
 * @param subd SUB structure holding information about substitution
 */
void print_sub(SUB** subd)
{
	SUB* sub_iter;

	sub_iter = *subd;

	while (sub_iter != NULL) {
		if (sub_iter->known_const->num_c != sub_iter->known_const->num) {
			fprintf(stderr,
				"(%s=%d<-%d)", sub_iter->sym, sub_iter->known_const->num_c,
				sub_iter->known_const->num);
		} else {
			fprintf(stderr,
				"(%s=%d)", sub_iter->sym, sub_iter->known_const->num_c);
		}
		if (HAS_VARFLAG_FRST(sub_iter->vtree->flags)) {
			fprintf(stderr, "*");
		}

		sub_iter = sub_iter->prev;
	}
}
#endif

/* --- tree exploration ----------------------------------------------------- */
/**
 * @brief Pushes a new branch checkpoint on a stack.
 *
 * @param pexplorer Pnode to be saved in checkpoint
 * @param eqwrapper Wrapping information to be saved
 * @param checkpoint Checkpoint structure holding the stack
 * @param vflags VFlags holding verification information about current level
 */
void bc_push(Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags)
{
	BC* bctos; /* top of stack for branch checkpoint, if descending */

	bctos = (BC*) malloc(sizeof(BC));

	bctos->pnode = *pexplorer;
	bctos->wrap = HAS_VFLAG_WRAP(*vflags);
	bctos->frst = HAS_VFLAG_FRST(*vflags);
	bctos->pwrapper = (*eqwrapper)->pwrapper;
	bctos->pendwrap = (*eqwrapper)->pendwrap;
	bctos->above = *checkpoint;
	*checkpoint = bctos;
}

/**
 * @brief Pops information from the BC stack and copies it over to the
 * corresponding variables.
 *
 * @param pnode Pnode pointer to be replaced by pointer stored in checkpoint
 * @param eqwrapper Wrapping information to be restored for level moving to
 * @param checkpoint BC stack to be popped from
 * @param vflags Vflags to be restored
 */
unsigned short int bc_pop(Pnode** pnode, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags, unsigned short int force)
{
	BC* bcold;
	unsigned short int wrap;

	if (!force && (*checkpoint)->above == NULL) {
		return FALSE;
	}

	bcold = *checkpoint;

	*pnode = (*checkpoint)->pnode;
	if ((*checkpoint)->wrap) {
		SET_VFLAG_WRAP(*vflags)
	} else {
		UNSET_VFLAG_WRAP(*vflags)
	}

	if ((*checkpoint)->frst) {
		SET_VFLAG_FRST(*vflags)
	} else {
		UNSET_VFLAG_FRST(*vflags)
	}

	(*eqwrapper)->pwrapper = (*checkpoint)->pwrapper;
	(*eqwrapper)->pendwrap = (*checkpoint)->pendwrap;

	*checkpoint = (*checkpoint)->above;
	free(bcold);

	return TRUE;
}

/**
 * @brief Moves pexplorer to its child node, if it is considered EXPLORABLE.
 *
 * @param pexplorer Pnode pointer to be moved
 * @param eqwrapper Wrapping information
 * @param checkpoint BC stack to store information of current level
 * @param vflags verification flags to be saved
 *
 * @return 
 */
unsigned short int explore_branch(Pnode** pexplorer, Eqwrapper** eqwrapper,
		BC** checkpoint, VFlags* vflags)
{
	if (EXPLORABLE(pexplorer)) {
		if (HAS_NFLAG_FRST((*pexplorer))) {
			bc_push(pexplorer, eqwrapper, checkpoint, vflags);
			*pexplorer = *((*pexplorer)->child);
			SET_VFLAG_FRST(*vflags)
		} else {
			bc_push(pexplorer, eqwrapper, checkpoint, vflags);
			*pexplorer = *((*pexplorer)->child);
		}
		UNSET_VFLAG_WRAP(*vflags)
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
 * @brief Moves pexplorer back to parent level and restores all relevant
 * information.
 *
 * @param pexplorer Pnode pointer to be moved
 * @param eqwrapper Wrapping information to be overwritten by bc_pop
 * @param checkpoint BC stack to retrieve information about parent level
 * @param vflags verification flags to be restored
 */
void exit_branch(Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags)
{
	while (*checkpoint != NULL) {
		bc_pop(pexplorer, eqwrapper, checkpoint, vflags, TRUE);
	}
	UNSET_VFLAG_BRCH(*vflags)
	UNSET_VFLAG_FRST(*vflags)
}

/**
 * @brief Move up in branch and proceed with exploration to the right, if
 * possible.
 *
 * @param pexplorer Pnode pointer to be moved
 * @param eqwrapper Wrapping information of current level
 * @param checkpoint BC stack to pop from
 * @param vflags verification flags to be overwritten by bc_pop
 *
 * @return 
 */
unsigned short int branch_proceed(Pnode** pexplorer, Eqwrapper** eqwrapper,
		BC** checkpoint, VFlags* vflags)
{
	if (!wrap_right(pexplorer, eqwrapper, vflags)) {
		if (!bc_pop(pexplorer, eqwrapper, checkpoint, vflags, FALSE)) {
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * @brief Processes statements in implications and equalities, which are
 * assumptions and leaves pexplorer at the first node, which is not assumed
 * anymore.
 *
 * @param veri_persp Pnode from whose perspective assumptions are to be verified
 * @param pexplorer Pointer to current position of explorer in the tree
 * @param eqwrapper Structure storing wrapping information
 * @param checkpoint Checkpoint for (re)storing level information
 * @param vflags verification flags
 *
 * @return FALSE if the assumption(s) could not be verified
 */
unsigned short int process_assumptions(Pnode* veri_persp, Pnode** pexplorer,
		Eqwrapper** eqwrapper, BC** checkpoint, VFlags* vflags)
{
	if (HAS_NFLAG_IMPL((*pexplorer))) {
		do {
			/* skip formulators */
			if (HAS_SYMBOL((*pexplorer))) {
				//BRANCH_PROCEED
				if (!branch_proceed(pexplorer, eqwrapper, checkpoint, vflags)) {\
					return FALSE;\
				} else if (!HAS_SYMBOL((*pexplorer)) &&\
						!POS_FRST(pexplorer, vflags)) {\
					return TRUE;\
				}
				continue;
			}

			if (!POS_FRST(pexplorer, vflags)) {
				break;
			}

			if (!check_asmp(veri_persp, pexplorer, FALSE)) {
				SET_VFLAG_FAIL((*vflags))
				/* pop through branch checkpoints until node is not part
				 * of an assumption TODO: add a nice example here */
				do {
					if (!bc_pop(pexplorer, eqwrapper, checkpoint, vflags, FALSE)) {
						return FALSE;
					}
				} while (HAS_NFLAG_IMPL((*pexplorer))
						&& POS_FRST(pexplorer, vflags));
				break;
			} else if (!move_right(pexplorer)) {
				if (!bc_pop(pexplorer, eqwrapper, checkpoint, vflags, FALSE)) {
					return FALSE;
				}
			} else {
				return next_forwards(veri_persp, pexplorer, eqwrapper,
					checkpoint, vflags, TRUE);
			}

		} while (TRUE);
	} else if (HAS_NFLAG_EQTY((*pexplorer))) {
		/* TODO: Handle equality as such (i.e. equalities create new
		 * substitution rules). Currently equalities are just two-sided
		 * implications. */

		(*eqwrapper)->pwrapper = *pexplorer;

		do {
			/* loop through nodes in equality to find a valid assumption
			 * and stop at the first reachable _after_ it */
			if (HAS_SYMBOL((*pexplorer))){ /* skip "=" */
				continue;
			} else if (check_asmp(veri_persp, pexplorer, FALSE)) {
				SET_VFLAG_WRAP(*vflags)
				(*eqwrapper)->pendwrap = *pexplorer;

				/* skip formulators */
				do {
					wrap_right(pexplorer, eqwrapper, vflags);
				} while (HAS_SYMBOL((*pexplorer)));

				if (explore_branch(pexplorer, eqwrapper, checkpoint, vflags)) {
					while (explore_branch(pexplorer, eqwrapper, checkpoint, vflags)) {};
					return next_forwards(veri_persp, pexplorer, eqwrapper,
						checkpoint, vflags, TRUE);
				}
				return TRUE;
			}

		} while (move_right(pexplorer));

		if (!bc_pop(pexplorer, eqwrapper, checkpoint, vflags, FALSE)) {
			return FALSE;
		}
	}
}

/**
 * @brief Skips all assumptions and leaves the explorer at the first Pnode,
 * which is not an assumption (in forwards direction).
 *
 * @param pexplorer pointer to current position of explorer
 * @param eqwrapper structure for (re)storing wrapping information
 * @param checkpoint structure for (re)storing level information
 * @param vflags verification flags
 *
 * @return FALSE, if no Pnode left in branch to leave explorer at
 */
unsigned short int skip_assumptions(Pnode** pexplorer, Eqwrapper** eqwrapper,
		BC** checkpoint, VFlags* vflags)
{
	while (HAS_NFLAG_FRST((*pexplorer))
			&& HAS_NFLAG_IMPL((*pexplorer))) {
		if (!branch_proceed(pexplorer, eqwrapper, checkpoint,
					vflags)) {
			return FALSE;
		}
	}

	/* skip following formulator */
	if (HAS_SYMBOL((*pexplorer))) {
		if (!branch_proceed(pexplorer, eqwrapper, checkpoint,
					vflags)) {
			return FALSE;
		}
	}

	return TRUE;
}

/**
 * @brief Moves explorer to next Pnode in tree in forwards direction.
 *
 * @param veri_persp Pnode from whose position verification is to be performed
 * @param pexplorer pointer to current position of explorer
 * @param eqwrapper structure for (re)storing wrapping information
 * @param checkpoint structure for (re)storing level information
 * @param vflags verification flags
 * @param p_a TRUE if assumptions are to be processed
 *
 * @return TRUE, if explorer could successfully be moved
 */
unsigned short int next_forwards(Pnode* veri_persp, Pnode** pexplorer,
		Eqwrapper** eqwrapper, BC** checkpoint, VFlags* vflags, unsigned short
		int p_a)
{
	if (p_a && !HAS_VFLAG_WRAP((*vflags)) && 
			(HAS_NFLAG_EQTY((*pexplorer)) ||
			 (HAS_NFLAG_IMPL((*pexplorer)) && POS_FRST(pexplorer, vflags)))) {

		/* process assumptions, if
		 *  - asked to do so
		 *  - assumption in equality has not yet been verified
		 *  - node is in eligible position */

		while (explore_branch(pexplorer, eqwrapper, checkpoint, vflags)) {};
		return process_assumptions(veri_persp, pexplorer, eqwrapper,
				checkpoint, vflags);

	} else if ((*eqwrapper)->pendwrap == *pexplorer ||
			!wrap_right(pexplorer, eqwrapper, vflags)) {

		/* move/wrap right or finish wrapping */

		if (!bc_pop(pexplorer, eqwrapper, checkpoint, vflags, FALSE)) {
			return FALSE;
		}

	} else if (!HAS_NFLAG_FRST((*pexplorer))) {

		/* explore everything after the first "=>" of the current formula */

		while (explore_branch(pexplorer, eqwrapper, checkpoint, vflags)) {};
	
		do {
			do {
				/* skip formulators */
				if (HAS_SYMBOL((*pexplorer))) {
					if (!branch_proceed(pexplorer, eqwrapper, checkpoint,
								vflags)) {
						return FALSE;
					}
					continue;
				}

				/* explore EXPLORABLE subbranches */
				if (EXPLORABLE(pexplorer)) {
					while (explore_branch(pexplorer, eqwrapper, checkpoint,
								vflags)) {};
					continue;
				}

				break;
			} while (TRUE);

			/* skip all assumptions (after further exploration),
			 * when asked to do so */
			if (!p_a && !skip_assumptions(pexplorer, eqwrapper, checkpoint,
						vflags)) {
				return FALSE;
			}

			break;
		} while (TRUE);
	} else if (!p_a && !skip_assumptions(pexplorer, eqwrapper, checkpoint,
						vflags)) {

		/* skip all assumptions, when asked to do so */

		return FALSE;
	}

	return TRUE;
}

/**
 * @brief Attempts to explore a new branch (as opposed to trying to explore a
 * sub-branch)
 *
 * @param veri_perspec Pnode from whose perspective assumptions are verified
 * @param sub_perspec Pnode from whose perspective substitutions are done
 * @param pexplorer Pnode pointer exploring the branch
 * @param eqwrapper Wrapping information
 * @param checkpoint BC stack holding information corresponding to a level
 * @param vflags verification flags
 * @param subd substitution information
 * @param idonly TRUE if only ids are considered eligible constants
 * @param exst TRUE if doing a backwards substitution for existence verification
 * @param exnum Pnode number which must not be exceeded by replaced constants
 * (not carrying the FRST NFLAG)
 * @param do_sub TRUE, if substitutions are to be made
 * @param p_a TRUE, if assumptions are to be processed
 *
 * @return FALSE, if attempt fails
 */
unsigned short int attempt_explore(Pnode* veri_perspec, Pnode* sub_perspec,
		Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags, SUB** subd, unsigned short int idonly,
		unsigned short int exst, int exnum, unsigned short int do_sub,
		unsigned short int p_a)
{
	if (EXPLORABLE(pexplorer)) {
		bc_push(pexplorer, eqwrapper, checkpoint, vflags);
		*pexplorer = *((*pexplorer)->child);
		SET_VFLAG_BRCH(*vflags)
		UNSET_VFLAG_FAIL(*vflags)
		if (!next_forwards(veri_perspec, pexplorer, eqwrapper, checkpoint,
					vflags, p_a)) {
			exit_branch(pexplorer, eqwrapper, checkpoint, vflags);
			return next_backwards(veri_perspec, sub_perspec,
					pexplorer, eqwrapper, checkpoint, vflags, subd, idonly,
					exst, exnum, do_sub, p_a);
		}
	} else {
		UNSET_VFLAG_BRCH(*vflags)
	}
	return TRUE;
}

/**
 * @brief Moves pexplorer to next reachable constant sub-tree, using branching
 * and substitution if necessary.
 *
 * @param veri_perspec Pnode from whose perspective assumptions are verified
 * @param sub_perspec Pnode from whose perspective substitutions are done
 * @param pexplorer Pnode pointer exploring the branch
 * @param eqwrapper Wrapping information
 * @param checkpoint BC stack holding information corresponding to a level
 * @param vflags verification flags
 * @param subd substitution information
 * @param idonly TRUE if only ids are considered eligible constants
 * @param exst TRUE if doing a backwards substitution for existence verification
 * @param exnum Pnode number which must not be exceeded by replaced constants
 * (not carrying the FRST NFLAG)
 * @param do_sub TRUE, if substitutions are to be made
 * @param p_a TRUE, if assumptions are to be processed
 *
 * @return FALSE, if no reachable sub-trees are left
 */
unsigned short int next_backwards(Pnode* veri_perspec,
		Pnode* sub_perspec,
		Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags, SUB** subd, unsigned short int idonly,
		unsigned short int exst, int exnum, unsigned short int do_sub,
		unsigned short int p_a)
{
	do {
		/* branch exploration */
		if (HAS_VFLAG_BRCH(*vflags)) {
			if (!next_forwards(veri_perspec, pexplorer, eqwrapper, checkpoint,
						vflags, p_a)) {
				exit_branch(pexplorer, eqwrapper, checkpoint, vflags);
			}
			return TRUE;
		} 

		/* substitution */
		if (do_sub && HAS_VFLAG_SUBD(*vflags)) {

			if (next_sub(sub_perspec, *subd, idonly, exst, exnum)) {
				return attempt_explore(veri_perspec, sub_perspec, pexplorer,
						eqwrapper, checkpoint, vflags, subd, idonly, exst,
						exnum, do_sub, p_a);
			} else {
				finish_sub(vflags, subd);
				continue;
			}
		}

		/* backtracking */
		do {
			if (move_left(pexplorer)) {
				break;
			} else if (!move_up(pexplorer)) {
				return FALSE;
			}
		} while (TRUE);

		if (HAS_SYMBOL((*pexplorer))) {
			continue;
		} else {
			if (do_sub && (*pexplorer)->vtree != NULL) {
				init_sub(sub_perspec, (*pexplorer)->vtree,
						vflags, subd, idonly, exst, exnum);
			}
			return attempt_explore(veri_perspec, sub_perspec, pexplorer,
					eqwrapper, checkpoint, vflags, subd, idonly,
					exst, exnum, do_sub, p_a);
		}
		break;
	} while (TRUE);

	return FALSE;
}

/* --- verification --------------------------------------------------------- */

/**
 * @brief Compares two constant sub-trees.
 *
 * @param p1 top left Pnode of one sub-tree
 * @param p2 top left Pnode of another sub-tree
 *
 * @return TRUE, if the two sub-trees are similar
 */
unsigned short int const_equal(Pnode* p1, Pnode* p2)
{
	/* FIXME: segfaulting because of call stack overflow;
	 * use less recursion  */
	unsigned short int equal;

	equal = TRUE;
#ifdef DGMP
	mpz_add_ui(comp_count, comp_count, 1);
#endif

	if (IS_ID(p1)) {
		if (IS_ID(p2)) {

			/******************************************************************/
			/* BIG TODO!!! We have to decide on how identifiers may be
			 * introduced. Ideally we would just have one line here:
			 */ /*return (*(p1->symbol) == *(p2->symbol));	
			 *
			 * Instead of (currently needed for verify_cases):*/

			 if (*(p1->symbol) != *(p2->symbol)) {
				return (strcmp(*(p1->symbol), *(p2->symbol)) == 0);	
			 } else {
				return TRUE;
			 }
		} else {
			return FALSE;
		}
	} else if (HAS_SYMBOL(p1)) { /* this is for formulators */
		if (HAS_SYMBOL(p2)) {
			equal = (strcmp(*(p1->symbol), *(p2->symbol)) == 0);	
		} else {
			return FALSE;
		}
	}

	if (equal && HAS_CHILD(p1)) {
		equal = HAS_CHILD(p2) ? const_equal(*(p1->child), *(p2->child)) : FALSE;
	}
	if (equal && HAS_RIGHT(p1)) {
		equal = HAS_RIGHT(p2) ? const_equal(*(p1->right), *(p2->right)) : FALSE;
	}

	equal &= (IS_EMPTY(p1) == IS_EMPTY(p2));
	
	return equal;
}

/**
 * @brief Compares two sub-trees with each other by calling const_equal. The
 * function is basically just a safety net for the case, when pnode or pexplorer
 * have no children (which should not happen anyway).
 *
 * @param pnode top left Pnode of one sub-tree
 * @param pexplorer top left Pnode of another sub-tree
 *
 * @return TRUE, if the two sub-trees are similar
 *
 * @return 
 */
unsigned short int verify(Pnode* pnode, Pnode** pexplorer)
{
	if (!HAS_CHILD(pnode) && !HAS_CHILD((*pexplorer))) {
		/* FATAL ERROR: function should not have been called, if this is true */
		return FALSE;
	}
	return const_equal(*((*pexplorer)->child), *(pnode->child));
	/* TODO: only when other verification fails, take equalities into account */
}

/**
 * @brief Tries to verify an assumption
 *
 * @param perspective Pnode pointer from whose perspective verification is done
 * @param pexplorer pointer to Pnode to be verified
 * @param exst TRUE, if existentence verification is to be performed
 *
 * @return TRUE, if verification was successful
 */
unsigned short int check_asmp(Pnode* perspective, Pnode** pexplorer,
		unsigned short int exst)
{
	Pnode* pconst;

	for (pconst = perspective->prev_const; pconst != NULL;
			pconst = pconst->prev_const) {
		if (verify(pconst, pexplorer)) {
			if (exst) {
				DBG_VERIFY(fprintf(stderr, SHELL_MAGENTA "<%d:%d>",
							(*pexplorer)->num_c, pconst->num_c););
			}
			return TRUE;
		}
	}
	return FALSE;
}

#if 0
/* REDUNDANT? */
/**
 * @brief Triggers universal verification of a Pnode
 *
 * @param pn Pnode to be verified
 *
 * @return TRUE, if verification was successful
 */
unsigned short int verify_universal(Pnode* pn)
{
	//TODO: pack these in one struct
	Eqwrapper* eqwrapper;
	Pnode** pexplorer;
	BC** checkpoint;
	SUB** subd;
	VFlags vflags;

	eqwrapper = (Eqwrapper*) malloc(sizeof(Eqwrapper));
	pexplorer = (Pnode**) malloc(sizeof(Pnode*));
	checkpoint = (BC**) malloc(sizeof(BC*));
	subd = (SUB**) malloc(sizeof(SUB*));
	
	*pexplorer = pn;
	*checkpoint = NULL;
	*subd = NULL;
	vflags = VFLAG_NONE;

	DBG_PARSER(fprintf(stderr, SHELL_BOLD "{%d}" SHELL_RESET2, pn->num_c););	
	DBG_PARSER(if (HAS_GFLAG_VRFD) fprintf(stderr, "*"););
	if (!HAS_GFLAG_VRFD || DBG_COMPLETE_IS_SET) {
		while (next_backwards(pn, pn, pexplorer, &eqwrapper, checkpoint,
					&vflags, subd, FALSE, FALSE, 0)) {
			if (verify(pn, pexplorer)) {
				DBG_PARSER(fprintf(stderr, SHELL_GREEN "<#%d",
							(*pexplorer)->num_c););
				SET_GFLAG_VRFD

				DBG_VERIFY(print_sub(subd););
				
				/* if no debugging options are selected and not
				 * explicitly requested, skip unnecessary compares */
				if (DBG_NONE_IS_SET || !DBG_COMPLETE_IS_SET) {
					finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
							subd);
					DBG_PARSER(fprintf(stderr, ">" SHELL_RESET1);); 
					break;
				}
				DBG_PARSER(fprintf(stderr, ">" SHELL_RESET1);); 
			}
			/* DBG_PARSER(fprintf(stderr, "<%d>", rn());); */
		}
	}

	free(eqwrapper);
	free(pexplorer);
	free(checkpoint);
	free(subd);

	if (!HAS_GFLAG_VRFD) {	
		/*TOGGLE_NFLAG_TRUE(pn)
		if (!HAS_NFLAG_TRUE(pn)) {*/
			return FALSE;
		/*} else {
			SET_GFLAG_VRFD
		}*/
	}
	return TRUE;
}
#endif

/**
 * @brief Recursive verification of quantifiers (helper function for
 * verify_quantifiers).
 *
 * @param pexstart Pnode at the beginning of the list
 * @param p_perspective perspective taken, while moving through the list
 * @param p_pexplorer Pnode advancing through the list
 * @param p_eqwrapper Wrapping information corresponding to this movement
 * @param p_checkpoint BC stack for recursive exploration
 * @param p_vflags verification flags
 * @param dbg turn debugging on (deprecated)
 * @param idonly TRUE, if only ids are considered eligible for substitution
 * @param exnum Pnode number which must not be exceeded by replaced constants
 * (not carrying the FRST NFLAG)
 * @param carry_over TRUE, if node has been verified and status of successful
 * verification is to be carried over to parent level
 *
 * @return TRUE, if verification of current node in list was successful
 */
unsigned short int vq_recurs(Pnode* pexstart,
		Pnode* p_perspective, Pnode** p_pexplorer, Eqwrapper** p_eqwrapper,
		BC** p_checkpoint, VFlags* p_vflags, unsigned short int dbg,
		unsigned short int idonly, int exnum, unsigned short int carry_over)
{
	unsigned short int success;
	unsigned short int rightmost;
	Pnode* expl_cp; /* checkpoint for parent explorer */
	Pnode* perspective; /* perspective for validation of next_reachable */

	Eqwrapper* eqwrapper;
	Pnode** pexplorer;
	BC** checkpoint;
	SUB** subd;
	VFlags vflags;

	eqwrapper = (Eqwrapper*) malloc(sizeof(Eqwrapper));
	pexplorer = (Pnode**) malloc(sizeof(Pnode*));
	checkpoint = (BC**) malloc(sizeof(BC*));
	subd = (SUB**) malloc(sizeof(SUB*));
	
	*pexplorer = pexstart;
	*checkpoint = NULL;
	*subd = NULL;

	vflags = VFLAG_NONE;
	success = FALSE;
	perspective = *p_pexplorer;

	move_rightmost(&perspective);

	while (next_backwards(*p_pexplorer /*perspective*/, perspective,
		pexplorer, &eqwrapper, checkpoint, &vflags, subd, idonly, TRUE,
		exnum, TRUE, TRUE)) {
		if (carry_over || verify(*p_pexplorer, pexplorer)) {	

			DBG_PATH(
					fprintf(stderr, SHELL_MAGENTA "<%d:%d",
						(*p_pexplorer)->num_c, (*pexplorer)->num_c);
					print_sub(subd);
					fprintf(stderr, ">" SHELL_RESET1);
			);

			expl_cp = *p_pexplorer;

			rightmost = !HAS_RIGHT((*p_pexplorer));

			if (!next_forwards(NULL, p_pexplorer, p_eqwrapper, p_checkpoint, p_vflags, FALSE)) {

				*p_pexplorer = expl_cp;

				finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
									subd);

				free(eqwrapper);
				free(pexplorer);
				free(checkpoint);
				free(subd);
				
				return FALSE;
			}

			/* if the dummy node has been reached, verification has been
			 * successful */
			if ((*p_pexplorer)->num == -1) {
				DBG_VERIFY(if (carry_over) {
						fprintf(stderr, SHELL_GREEN "<%d:co>" SHELL_RESET1,
								expl_cp->num_c);
						} else {
						fprintf(stderr, SHELL_GREEN "<%d:%d",
								expl_cp->num_c, (*pexplorer)->num_c);
						print_sub(subd);
						fprintf(stderr, ">" SHELL_RESET1);
						});

				finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
									subd);

				free(eqwrapper);
				free(pexplorer);
				free(checkpoint);
				free(subd);
				
				return TRUE;
			}

			if (vq_recurs(pexstart, p_perspective,
					p_pexplorer, p_eqwrapper, p_checkpoint, p_vflags,
					FALSE, idonly, exnum, rightmost)) {
				success = TRUE;
				*p_pexplorer = expl_cp;
				break;
			} else {
				*p_pexplorer = expl_cp;
			}

		} else {
			DBG_FAIL(
				fprintf(stderr, SHELL_RED "<%d:%d",
						(*p_pexplorer)->num_c, (*pexplorer)->num_c);
				print_sub(subd);
				fprintf(stderr, ">" SHELL_RESET1);
			);
		}
	}

	DBG_VERIFY(if (carry_over) {
			fprintf(stderr, SHELL_GREEN "<%d:co>" SHELL_RESET1,
					(*p_pexplorer)->num_c);
			} else if (success) {
			fprintf(stderr, SHELL_GREEN "<%d:%d",
					(*p_pexplorer)->num_c, (*pexplorer)->num_c);
			print_sub(subd);
			fprintf(stderr, ">" SHELL_RESET1);
			});

	finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
		subd);

	free(eqwrapper);
	free(pexplorer);
	free(checkpoint);
	free(subd);

	return success;
}

/**
 * @brief Collects variables for forward substituion during existence
 * verification - currently only at top-most level.
 *
 * @param pcollector Pnode to start collection at
 *
 * @return pointer to VTree holding the collected variables
 */
VTree* collect_forward_vars(Pnode* pcollector)
{
	VTree* vtree;
	VTree* newvtree;

	vtree = NULL;

	/* TODO: think about how to handle _variables_ during forward substitution*/
	while (pcollector->num != -1) {
		if (HAS_NFLAG_NEWC(pcollector)) {
			//DBG_VERIFY(fprintf(stderr, SHELL_RED "." SHELL_RESET1););	
			newvtree = (VTree*) malloc(sizeof(VTree));
			newvtree->pnode = *(pcollector->child);
			newvtree->right = vtree;
			newvtree->left = NULL;
			newvtree->flags = VARFLAG_NONE;
			if (vtree != NULL) {
				vtree->parent = newvtree;
				SET_VARFLAG_RGHT(vtree->flags)
			}
			vtree = newvtree;
		}
		pcollector = *(pcollector->right);
	}

	return vtree;
}

/**
 * @brief Triggers quantifier verification of a list of Pnodes.
 *
 * @param pn Pnode to be verified
 * @param pexstart Pnode at the beginning of the list
 * @param idonly TRUE, if only ids are considered eligible for substitution
 *
 * @return TRUE, if verification was successful
 */
unsigned short int verify_quantifiers(Pnode* pn, Pnode* pexstart,
		unsigned short int idonly)
{
	int exnum;

	//TODO: pack these in one struct
	Eqwrapper* eqwrapper;
	Pnode** pexplorer;
	BC** checkpoint;
	SUB** subd;
	VFlags vflags;
	VTree* fw_vtree;

	eqwrapper = (Eqwrapper*) malloc(sizeof(Eqwrapper));
	pexplorer = (Pnode**) malloc(sizeof(Pnode*));
	checkpoint = (BC**) malloc(sizeof(BC*));
	subd = (SUB**) malloc(sizeof(SUB*));
	
	*pexplorer = pexstart;
	*checkpoint = NULL;
	*subd = NULL;
	vflags = VFLAG_NONE;
	exnum = pexstart->num;

	bc_push(pexplorer, &eqwrapper, checkpoint, &vflags);

	if (vq_recurs(pexstart, pn, pexplorer, &eqwrapper, checkpoint,
			&vflags, TRUE, idonly, exnum, FALSE)) {
		SET_GFLAG_VRFD
	} else {
		DBG_VERIFY(fprintf(stderr, SHELL_BROWN "<not verified; "
					"trying forward substitution>"););	

		fw_vtree = collect_forward_vars(pexstart);
		if (fw_vtree != NULL) {
			if (init_sub(pexstart, fw_vtree, &vflags, subd, TRUE, FALSE,
						exnum)) {
				do {
					DBG_VERIFY(
							fprintf(stderr, SHELL_BROWN "<");
							print_sub(subd);
							fprintf(stderr, ">" SHELL_RESET1);
							);
					if (vq_recurs(pexstart, pn, pexplorer, &eqwrapper,
								checkpoint, &vflags, TRUE, idonly, exnum,
								FALSE)) {
						SET_GFLAG_VRFD
						break;
					}
				} while (next_sub(pn, *subd, TRUE, FALSE, 0));
				finish_sub(&vflags, subd);
			}
		}
	}
	DBG_VERIFY(fprintf(stderr, SHELL_RESET1););	
	
	bc_pop(pexplorer, &eqwrapper, checkpoint, &vflags, TRUE);

	free(eqwrapper);
	free(pexplorer);
	free(checkpoint);
	free(subd);

	return HAS_GFLAG_VRFD;
}

/**
 * @brief Attempts to verify a Pnode based on the consideration of different
 * cases.
 *
 * @param pn Pnode to be verified
 *
 * @return TRUE on success
 */
unsigned short int verify_cases(Pnode* pn)
{
	Eqwrapper* eqwrapper;
	Pnode** pexplorer;
	BC** checkpoint;
	VFlags vflags;
	SUB** subd;

	eqwrapper = (Eqwrapper*) malloc(sizeof(Eqwrapper));
	pexplorer = (Pnode**) malloc(sizeof(Pnode*));
	checkpoint = (BC**) malloc(sizeof(BC*));
	subd = (SUB**) malloc(sizeof(SUB*));

	*pexplorer = pn;
	*checkpoint = NULL;
	*subd = NULL;
	vflags = VFLAG_NONE;
	
	while (next_backwards(NULL, pn, pexplorer, &eqwrapper, checkpoint,
				&vflags, subd, FALSE, FALSE, 0, FALSE, FALSE)) {

		/* TODO: Actual implementation. Currently only prints debugging
		 * information about where the same conclusion has been made before. */
		if (verify(pn, pexplorer)) {
			DBG_PATH(
					fprintf(stderr, SHELL_MAGENTA "<%d>",
						(*pexplorer)->num_c);
					);
		}

	}

	free(pexplorer);

	return FALSE;
}

/**
 * @brief Cleanly finishes verification by exiting explored branches and
 * finishing substitution.
 *
 * @param pexplorer Pnode pointer moved backwards through branch to be exited
 * @param eqwrapper Wrapping information to be restored (and discarded)
 * @param checkpoint BC stack used to move back through branch
 * @param vflags verification flags
 * @param subd substitution information
 */
void finish_verify(Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags, SUB** subd)
{
	if (HAS_VFLAG_BRCH(*vflags)) {
		exit_branch(pexplorer, eqwrapper, checkpoint, vflags);
	}
	if (HAS_VFLAG_SUBD(*vflags)) {
		finish_sub(vflags, subd);
	}
}
