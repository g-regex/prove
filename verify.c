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
#include "pgraph.h"
#include "verify.h"
#include "debug.h"
#include <stdio.h>

#ifdef DVERIFY
#include "stdio.h"
#endif

#define TRUE 1
#define FALSE 0

/* explorable are branches, which contain either an implication or an equality,
 * which is not an assumption
 * TODO: verify equalities, which are assumptions - but not by exploration */

#define EXPLORABLE \
	(HAS_CHILD((*pexplorer)) && (HAS_NFLAG_IMPL((*((*pexplorer)->child)))\
			|| (!HAS_NFLAG_FRST((*pexplorer))\
				&& HAS_NFLAG_EQTY((*((*pexplorer)->child))))))

unsigned short int next_in_branch(Pnode* pnode, Pnode** pexplorer,
		Eqwrapper** eqwrapper, BC** checkpoint, VFlags* vflags,
		unsigned short int dbg);
void bc_push(Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags);
void bc_pop(Pnode** pnode, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags);

//static Pnode* eqendwrap; /*temporarily holds node at which to stop wrapping*/

/* --- verification specific movement functions ----------------------------- */

/* move right; if already at the right-most node, move to the left-most node */ 
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

/* --- verification --------------------------------------------------------- */

/* checking circular linked list of equal nodes */
unsigned short int are_equal(Pnode* p1, Pnode* p2)
{
	/*Variable* firsteq;
	Variable* eq_iter;

	firsteq = *(p2->equalto);
	for (eq_iter = firsteq->next; eq_iter != firsteq; eq_iter = eq_iter->next) {
		if (eq_iter == *(p1->equalto)) {
			return TRUE;
		//} else if (IS_ID(eq_iter->pnode) && IS_ID(p1)) { FIXME: SCOPING
		//	if (strcmp(*(eq_iter->pnode->symbol),
		//						*(p1->symbol)) == 0) {
		//		return TRUE;
		//	}
		}
	}*/
	return FALSE;
}

/* compares two constant sub-trees;
 * must be given the top left node of the subtrees to compare */
unsigned short int const_equal(Pnode* p1, Pnode* p2)
{
	/* FIXME: segfaulting because of call stack overflow;
	 * use less recursion  */
	unsigned short int equal;

	equal = TRUE;
	if (IS_ID(p1)) {
		if (IS_ID(p2)) {

			/******************************************************************/
			/* BIG TODO!!! We have to decide on how identifiers may be
			 * introduced. Ideally we'd just have one line here:
			 * return (*(p1->symbol) == *(p2->symbol));	
			 *
			 * Instead of:*/

			 /*if (are_equal(p1->parent, p2->parent)) {
				return TRUE;
			  } else if (are_equal(p2, p1)) { FIXME: SCOPING
				return TRUE;
			 } else */if (*(p1->symbol) != *(p2->symbol)) {
				return (strcmp(*(p1->symbol), *(p2->symbol)) == 0);	
			 } else {
				return TRUE;
			 }
			/******************************************************************/
			/* equal = (*(p1->symbol) == *(p2->symbol)); */
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

/* checks pnode against pexplorer node; this function is basically a safety net,
 * if pnode or pexplorer have no children, which should not happen anyway */
unsigned short int verify(Pnode* pnode, Pnode** pexplorer)
{
	if (!HAS_CHILD(pnode) && !HAS_CHILD((*pexplorer))) {
		/* FATAL ERROR: function should not have been called, if this is true */
		return FALSE;
	}
	return const_equal(*((*pexplorer)->child), *(pnode->child));
	/* TODO: only when other verification fails, take equalities into account */
}

/* checks assumption in pexplorer from the perspective of 'perspective' */
unsigned short int check_asmp(Pnode* perspective, Pnode** pexplorer,
		unsigned short int dbg)
{
	Pnode* pconst;

	for (pconst = perspective->prev_const; pconst != NULL;
			pconst = pconst->prev_const) {
	/*DBG_VERIFY(
			if (dbg) {
				fprintf(stderr, SHELL_CYAN "(%d)" SHELL_RESET1, pconst->num);
				}
			)*/
		if (verify(pconst, pexplorer)) {
			return TRUE;
		}
	}
	return FALSE;
}

#ifdef DVERIFY
void print_sub(SUB** subd)
{
	SUB* sub_iter;

	sub_iter = *subd;

	while (sub_iter != NULL) {
		fprintf(stderr,
			"(%s=%d)", sub_iter->sym, sub_iter->known_const->num);

		sub_iter = sub_iter->prev;
	}
}
#endif

unsigned short int verify_universal(Pnode* pn)
{
	/*if (IS_EMPTY(pn) || (HAS_CHILD(pn) && IS_EMPTY((*(pn->child))))) {
		return HAS_NFLAG_TRUE(pn);
	}*/
	//init_backtrack(pn);
	
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

	DBG_PARSER(fprintf(stderr, SHELL_BOLD "{%d}" SHELL_RESET2, pn->num);)	
	DBG_PARSER(if (HAS_GFLAG_VRFD) fprintf(stderr, "*");)
	if (!HAS_GFLAG_VRFD || DBG_COMPLETE_IS_SET) {
		while (next_reachable_const(pn, pn, pexplorer, &eqwrapper, checkpoint,
					&vflags, subd)) {
			if (verify(pn, pexplorer)) {
				DBG_PARSER(fprintf(stderr, SHELL_GREEN "<#%d",
							(*pexplorer)->num);)
				SET_GFLAG_VRFD

				DBG_VERIFY(print_sub(subd);)
				
				/* if no debugging options are selected and not
				 * explicitly requested, skip unnecessary compares */
				if (DBG_NONE_IS_SET || !DBG_COMPLETE_IS_SET) {
					finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
							subd);
					DBG_PARSER(fprintf(stderr, ">" SHELL_RESET1);) 
					break;
				}
				DBG_PARSER(fprintf(stderr, ">" SHELL_RESET1);) 
			}
			/* DBG_PARSER(fprintf(stderr, "<%d>", rn());) */
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

unsigned short int search_justification(Pnode* pexstart,
		/* parent: */
		Pnode* p_perspective, Pnode** p_pexplorer, Eqwrapper** p_eqwrapper,
		BC** p_checkpoint, VFlags* p_vflags, unsigned short int dbg)
{
	unsigned short int success;
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

	while (next_reachable_const(/*pexstart*/perspective, perspective,
		pexplorer, &eqwrapper, checkpoint, &vflags, subd)) {
		if (verify(*p_pexplorer, pexplorer)) {	

			/* add DBG flag for this */
			/*DBG_VERIFY(
					fprintf(stderr, SHELL_BROWN "<%d:%d",
						(*p_pexplorer)->num, (*pexplorer)->num);
					print_sub(subd);
					fprintf(stderr, ">" SHELL_RESET1);
			)*/

			/* TODO: recursion needed here */
			expl_cp = *p_pexplorer;

			if (*p_pexplorer == p_perspective) {
				SET_GFLAG_VRFD

				DBG_VERIFY(
						fprintf(stderr, SHELL_BROWN "< %d" SHELL_RESET1,
								(*p_pexplorer)->num);
						fprintf(stderr, SHELL_GREEN "<#%d", (*pexplorer)->num);
						print_sub(subd);
						fprintf(stderr, ">" SHELL_RESET1);
				)

				finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
					subd);

				free(eqwrapper);
				free(pexplorer);
				free(checkpoint);
				free(subd);

				return TRUE;
			} else if (!next_in_branch(p_perspective, p_pexplorer, p_eqwrapper,
					p_checkpoint, p_vflags, TRUE)) {
				SET_GFLAG_VRFD

				//DBG_VERIFY(fprintf(stderr, SHELL_RED "F" SHELL_RESET1);)

				DBG_VERIFY(
						fprintf(stderr, SHELL_BROWN " %d" SHELL_RESET1,
							(*p_pexplorer)->num);
						fprintf(stderr, SHELL_GREEN "<#%d", (*pexplorer)->num);
						print_sub(subd);
						fprintf(stderr, ">" SHELL_RESET1);
				)

				finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
									subd);

				free(eqwrapper);
				free(pexplorer);
				free(checkpoint);
				free(subd);
				
				return TRUE;
			}

			if (search_justification(pexstart, p_perspective,
					p_pexplorer, p_eqwrapper, p_checkpoint, p_vflags,
					FALSE)) {
				success = TRUE;
				*p_pexplorer = expl_cp;
				break;
			} else {
				*p_pexplorer = expl_cp;
			}

		} else {
			/* TODO: add DBG flag FAIL */
			/*DBG_VERIFY(fprintf(stderr, SHELL_RED "<%d:%d",
						(*p_pexplorer)->num, (*pexplorer)->num);)
			DBG_VERIFY(print_sub(subd);)
			DBG_VERIFY(fprintf(stderr, ">" SHELL_RESET1);)*/
		}
	}

	DBG_VERIFY(if (success) {
			fprintf(stderr, SHELL_BROWN " %d" SHELL_RESET1,
					(*p_pexplorer)->num);
			fprintf(stderr, SHELL_GREEN "<#%d", (*pexplorer)->num);
			print_sub(subd);
			fprintf(stderr, ">" SHELL_RESET1);
	})

	finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
		subd);

	free(eqwrapper);
	free(pexplorer);
	free(checkpoint);
	free(subd);

	return success;
}

unsigned short int verify_existence(Pnode* pn, Pnode* pexstart)
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
	
	*pexplorer = pexstart;
	*checkpoint = NULL;
	*subd = NULL;
	vflags = VFLAG_NONE;
	
	bc_push(pexplorer, &eqwrapper, checkpoint, &vflags);

	if (search_justification(pexstart, pn, pexplorer, &eqwrapper, checkpoint,
			&vflags, TRUE)) {

		DBG_VERIFY(fprintf(stderr, SHELL_GREEN "<verified>" SHELL_RESET1);)	
		SET_GFLAG_VRFD

		/* TODO: add verification information
		DBG_VERIFY(print_sub();) */
	} else {
		DBG_VERIFY(fprintf(stderr, SHELL_RED "<not verified>" SHELL_RESET1);)	
	}
	bc_pop(pexplorer, &eqwrapper, checkpoint, &vflags);

	free(eqwrapper);
	free(pexplorer);
	free(checkpoint);
	free(subd);

	return TRUE;
}

/* --- substitution --------------------------------------------------------- */

void init_known_const(Pnode* perspective, SUB* s)
{
	s->known_const = perspective->prev_const;
}

/* substitute variable */
unsigned short int sub_var(SUB* s)
{
	/* ATTENTION: This approach to substitution relies on the fact that we only
	 * move rightwards and downwards, while the substitution is in place. */
	if ((*(s->known_const->child))->symbol != NULL) {
		*(s->var->pnode->symbol) = *((*(s->known_const->child))->symbol);
	} else {
		*(s->var->pnode->symbol) = NULL;
	}

	if ((*(s->known_const->child))->child != NULL) {
		*(s->var->pnode->child) = *((*(s->known_const->child))->child);
	} else {
		*(s->var->pnode->child) = NULL;
	}

	if ((*(s->known_const->child))->right != NULL) {
		*(s->var->pnode->right) = *((*(s->known_const->child))->right);
	} else {
		*(s->var->pnode->right) = NULL;
	}

	/*
	if (CONTAINS_ID(s->known_const)) {
		*(s->var->pnode->symbol) = *((*(s->known_const->child))->symbol);
		*(s->var->pnode->child) = NULL;
		*(s->var->pnode->right) = NULL;
	} else {
		*(s->var->pnode->symbol) = NULL;
		// FIXME: check the next 6 lines; missing anything?
		if ((*(s->known_const->child))->child != NULL) {
			*(s->var->pnode->child) = *((*(s->known_const->child))->child);
		}
		if ((*(s->known_const->child))->right != NULL) {
			*(s->var->pnode->right) = *((*(s->known_const->child))->right);
		}
	}
	*/
	//*(s->var->pnode->equalto) = *((*(s->known_const->child))->equalto);
	//DBG_VERIFY(fprintf(stderr,
	//	"\\%s=%d/", s->sym, s->known_const->num);)
}

/* substitutes variable(s) by the next known constant/sub-tree */
unsigned short int next_known_const(Pnode* perspective, SUB* s)
{
	SUB* s_iter;

	s_iter = s;

	while (s_iter != NULL) {
			s_iter->known_const = s_iter->known_const->prev_const;
			if (s_iter->known_const == NULL) {
				init_known_const(perspective, s_iter);
				sub_var(s_iter);
				s_iter = s_iter->prev;
			} else {
				sub_var(s_iter);
				return TRUE;
			}
	}
	return FALSE; /* finish substitution */
}

/* initialise substitution */
void init_sub(Pnode* perspective, Pnode** pexplorer, VFlags* vflags, SUB** subd)
{
	Variable* var;
	SUB* oldsub;

	var = (*pexplorer)->var;
	oldsub = *subd;

	/* only substitute, if there is something to substitute in */
	if (perspective->prev_const != NULL) {
		do {
			if (!var->locked) {
				var->locked = TRUE;

				*subd = (SUB*) malloc(sizeof(SUB));
				(*subd)->prev = oldsub;
				oldsub = *subd;

				init_known_const(perspective, *subd);

				(*subd)->sym = *(var->pnode->symbol);
				//sub->equalto = *(var->pnode->equalto);
				(*subd)->var = var;

				/* FIXME: added this. correct? */
				sub_var(*subd);
			}
			var = var->next;
		} while (var != NULL);

		SET_VFLAG_SUBD(*vflags)
	}
}

/* substitute original variable symbols back in */
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

		*((*subd)->var->pnode->symbol) = (*subd)->sym;
		//*(sub->var->pnode->equalto) = sub->equalto;
		*((*subd)->var->pnode->child) = NULL;
		*((*subd)->var->pnode->right) = NULL;

		(*subd)->var->locked = FALSE;

		free(*subd);
		*subd = prev_sub;
	} while (*subd != NULL);
}

/* --- branching ------------------------------------------------------------ */

/* implementation of a "branch checkpoint" stack to easily find the next
 * parent node of the current level to jump back to */
void bc_push(Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags)
{
	BC* bctos; /* top of stack for branch checkpoint, if descending */

	bctos = (BC*) malloc(sizeof(BC));

	bctos->pnode = *pexplorer;
	//bctos->pnode = pexplorer;
	bctos->wrap = HAS_VFLAG_WRAP(*vflags);
	bctos->frst = HAS_VFLAG_FRST(*vflags);
	bctos->fail = HAS_VFLAG_FAIL(*vflags);
	bctos->pwrapper = (*eqwrapper)->pwrapper;
	bctos->pendwrap = (*eqwrapper)->pendwrap;
	bctos->above = *checkpoint;
	*checkpoint = bctos;
}

void bc_pop(Pnode** pnode, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags)
{
	BC* bcold;
	unsigned short int wrap;

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

	if ((*checkpoint)->fail) {
		SET_VFLAG_FAIL(*vflags)
	} else {
		UNSET_VFLAG_FAIL(*vflags)
	}

	(*eqwrapper)->pwrapper = (*checkpoint)->pwrapper;
	(*eqwrapper)->pendwrap = (*checkpoint)->pendwrap;

	*checkpoint = (*checkpoint)->above;
	free(bcold);
}

/* The following functions all set the value of pexplorer directly or
 * indirectly and return TRUE in the case of success and FALSE otherwise. */
unsigned short int explore_branch(Pnode** pexplorer, Eqwrapper** eqwrapper,
		BC** checkpoint, VFlags* vflags)
{
	/* OLD less restricted definition of EXPLORABLE:
	 * if (HAS_NFLAG_IMPL((*(reachable->child)))
	 *		|| HAS_NFLAG_EQTY((*(reachable->child)))) {
	 */
	if (EXPLORABLE) {
		if (HAS_NFLAG_FRST((*pexplorer))) {
			bc_push(pexplorer, eqwrapper, checkpoint, vflags);
			*pexplorer = *((*pexplorer)->child);
			SET_VFLAG_FRST(*vflags)
		} else {
			bc_push(pexplorer, eqwrapper, checkpoint, vflags);
			*pexplorer = *((*pexplorer)->child);
		}
		UNSET_VFLAG_FAIL(*vflags)
		UNSET_VFLAG_WRAP(*vflags)
		return TRUE;
	} else {
		return FALSE;
	}
}

void exit_branch(Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags)
{
	while (*checkpoint != NULL) {
		bc_pop(pexplorer, eqwrapper, checkpoint, vflags);
	}
	UNSET_VFLAG_BRCH(*vflags)
	UNSET_VFLAG_FRST(*vflags)
}

unsigned short int attempt_explore(Pnode* veri_perspec, Pnode* sub_perspec,
		Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags, SUB** subd)
{
	/* OLD implementation: if (explore_branch()) { */
	if (EXPLORABLE) {
		bc_push(pexplorer, eqwrapper, checkpoint, vflags);
		*pexplorer = *((*pexplorer)->child);
		SET_VFLAG_BRCH(*vflags)
		//DBG_VERIFY(fprintf(stderr, "~");)
		if (!next_in_branch(veri_perspec, pexplorer, eqwrapper, checkpoint,
					vflags, FALSE)) {
			exit_branch(pexplorer, eqwrapper, checkpoint, vflags);
			return next_reachable_const(veri_perspec, sub_perspec,
					pexplorer, eqwrapper, checkpoint, vflags, subd);
		}
	} else {
		UNSET_VFLAG_BRCH(*vflags)
	}
	return TRUE;
}

#define POP \
	if ((*checkpoint)->above == NULL) {\
		return FALSE; /* last pop is done by exit_branch */\
	} else {\
		bc_pop(pexplorer, eqwrapper, checkpoint, vflags);\
	}

/* move up in branch in proceed with exploration to the right, if possible */
unsigned short int branch_proceed(Pnode** pexplorer, Eqwrapper** eqwrapper,
		BC** checkpoint, VFlags* vflags)
{
	/* FIXME: check, whether GFLAG_FRST is set properly */
	while (!wrap_right(pexplorer, eqwrapper, vflags)) {
		POP
		if (wrap_right(pexplorer, eqwrapper, vflags)) {
			break;
		}
	}
	return TRUE;
}

#define PROCEED \
	proceed = TRUE;\
	continue;

#define BRANCH_PROCEED \
	if (!branch_proceed(pexplorer, eqwrapper, checkpoint, vflags)) {\
		return FALSE;\
	} else if (!HAS_SYMBOL((*pexplorer))) {\
		return TRUE;\
	} else {\
		PROCEED\
	}

#define POP_PROCEED \
	do {\
		POP\
	} while (HAS_NFLAG_IMPL((*pexplorer))\
				&& HAS_NFLAG_FRST((*pexplorer)));\
	BRANCH_PROCEED\

#define SKIP_FORMULATORS \
	do {\
		wrap_right(pexplorer, eqwrapper, vflags);\
	} while (HAS_SYMBOL((*pexplorer)));

/* set pexplorer to the next valid value or return FALSE */
unsigned short int next_in_branch(Pnode* perspective, Pnode** pexplorer,
		Eqwrapper** eqwrapper, BC** checkpoint, VFlags* vflags,
		unsigned short int exst)
{
	/* FIXME: too much recursion */
	unsigned short int proceed;

	do {
		proceed = FALSE;

		while (exst && HAS_NFLAG_FRST((*pexplorer))
				&& HAS_NFLAG_IMPL((*pexplorer))) {
			if (!branch_proceed(pexplorer, eqwrapper, checkpoint, vflags)) {
				return FALSE;
			}
		}

		/* skip formulators */
		if (HAS_SYMBOL((*pexplorer))) {
			if (HAS_NFLAG_IMPL((*pexplorer))) {
				//UNSET_VFLAG_FAIL((*vflags))
				if (HAS_VFLAG_FAIL((*vflags))) {
					POP_PROCEED
				} else {
					BRANCH_PROCEED
				}
			} else {
				BRANCH_PROCEED
			}
		}

		/* explore EXPLORABLE subbranches */
		if (explore_branch(pexplorer, eqwrapper, checkpoint, vflags)) {
			PROCEED
		}

		if (HAS_NFLAG_IMPL((*pexplorer))) {
			/* if processing an assumption, verify it */
			if (!check_asmp(perspective, pexplorer, FALSE)) {
				SET_VFLAG_FAIL((*vflags))
			}

			if (!exst
					&& (HAS_NFLAG_FRST((*pexplorer))
						/*|| HAS_VFLAG_FRST(*vflags)*/)) {
				if (HAS_VFLAG_FAIL((*vflags))) {
					/* pop through branch checkpoints until node is not part
					 * of an assumption TODO: add a nice example here */
					POP_PROCEED
				} else if (!move_right(pexplorer)) {
					/* When GFLAG_FRST is set, it might happen that we cannot
					 * move right and have to pop back to proceed with the next
					 * node at a lower level */
					BRANCH_PROCEED
				} else {
					PROCEED
				}
			} else {
				if (explore_branch(pexplorer, eqwrapper, checkpoint, vflags)) {
					PROCEED
				} else {
					//BRANCH_PROCEED
					if (!branch_proceed(pexplorer, eqwrapper, checkpoint, vflags)) {
						return FALSE;
					} else if (explore_branch(pexplorer, eqwrapper, checkpoint,
								vflags)) {
						//return TRUE;
						PROCEED
					} else if (!HAS_SYMBOL((*pexplorer))) {\
						return TRUE;\
					} else {
						proceed = TRUE;
						continue;
					}
				}
			}
		} else if (HAS_NFLAG_EQTY((*pexplorer))) {
			if (HAS_VFLAG_WRAP(*vflags)) {
				/* proceed with branch after cycling through equality */
				if ((*eqwrapper)->pendwrap == *pexplorer) {
					UNSET_VFLAG_WRAP(*vflags)
					BRANCH_PROCEED
				}
				SKIP_FORMULATORS
				return TRUE;
			} else {
				/* remember first node in equality for wrapping */
				if (HAS_NFLAG_FRST((*pexplorer))) {
					(*eqwrapper)->pwrapper = *pexplorer;
				}

				do {
					/* loop through nodes in equality to find a valid assumption
					 * and stop at the first reachable _after_ it */

					if (HAS_SYMBOL((*pexplorer))){ /* skip "=" */
						continue;
					} else if (check_asmp(perspective, pexplorer, FALSE)) {
						SET_VFLAG_WRAP(*vflags)
						(*eqwrapper)->pendwrap = *pexplorer;

						SKIP_FORMULATORS
						return TRUE;
					}

				} while (move_right(pexplorer));

				POP
				wrap_right(pexplorer, eqwrapper, vflags);
				BRANCH_PROCEED
			}
		}
	} while (proceed);

	return FALSE;
}

/* --- backtracking --------------------------------------------------------- */

/* finish verification of current node cleanly */
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

unsigned short int next_reachable_const(Pnode* veri_perspec, Pnode* sub_perspec,
		Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags, SUB** subd)
{
	unsigned short int proceed;

	do {
		proceed = FALSE;

		/* branch exploration */
		if (HAS_VFLAG_BRCH(*vflags)) {
			if (!next_in_branch(veri_perspec, pexplorer, eqwrapper, checkpoint,
						vflags, FALSE)) {
				exit_branch(pexplorer, eqwrapper, checkpoint, vflags);
			}
			return TRUE;
		} 

		/* substitution */
		if (HAS_VFLAG_SUBD(*vflags)) {
			if (next_known_const(sub_perspec, *subd)) {
				return attempt_explore(veri_perspec, sub_perspec, pexplorer, eqwrapper, checkpoint,
						vflags, subd);
				/* TODO: remove recursion */
			} else {
				finish_sub(vflags, subd);
				proceed = TRUE;
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
			proceed = TRUE;
			continue;
		} else {
			if ((*pexplorer)->var != NULL) {
				init_sub(sub_perspec, pexplorer, vflags, subd);
			}
			return attempt_explore(veri_perspec, sub_perspec, pexplorer,
					eqwrapper, checkpoint, vflags, subd);
			/* TODO: remove recursion */
		}
	} while (proceed);

	return FALSE;
}
