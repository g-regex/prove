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
		Eqwrapper** eqwrapper, BC** checkpoint, VFlags* vflags);

unsigned short int next_existence(Pnode* perspective, Pnode** pexplorer,
		Eqwrapper** eqwrapper, BC** checkpoint, VFlags* vflags);

void bc_push(Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags);
void bc_pop(Pnode** pnode, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags);
void print_sub(SUB** subd);
void init_sub(Pnode* perspective, VTree* vtree, VFlags* vflags,
		SUB** subd, unsigned short int fwd);
unsigned short int next_sub(Pnode* perspective, SUB* s,
		unsigned short int fwd);
void finish_sub(VFlags* vflags, SUB** subd);

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

/* compares two constant sub-trees;
 * must be given the top left node of the subtrees to compare */
unsigned short int const_equal(Pnode* p1, Pnode* p2)
{
	/* FIXME: segfaulting because of call stack overflow;
	 * use less recursion  */
	unsigned short int equal;

	equal = TRUE;
	mpz_add_ui(comp_count, comp_count, 1);

	if (IS_ID(p1)) {
		if (IS_ID(p2)) {

			/******************************************************************/
			/* BIG TODO!!! We have to decide on how identifiers may be
			 * introduced. Ideally we'd just have one line here:
			 * return (*(p1->symbol) == *(p2->symbol));	
			 *
			 * Instead of:*/

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
		unsigned short int exst)
{
	Pnode* pconst;

	for (pconst = perspective->prev_const; pconst != NULL;
			pconst = pconst->prev_const) {
		if (verify(pconst, pexplorer)) {
			if (exst) {
				DBG_VERIFY(fprintf(stderr, SHELL_MAGENTA "<%d:#%d>",
							(*pexplorer)->num, pconst->num);)
			}
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

unsigned short int ve_recursion(Pnode* pexstart,
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

	while (next_reachable_const(*p_pexplorer /*perspective*/, perspective,
		pexplorer, &eqwrapper, checkpoint, &vflags, subd)) {
		if (verify(*p_pexplorer, pexplorer)) {	

			DBG_EPATH(
					fprintf(stderr, SHELL_BROWN "<%d:%d",
						(*p_pexplorer)->num, (*pexplorer)->num);
					print_sub(subd);
					fprintf(stderr, ">" SHELL_RESET1);
			)

			expl_cp = *p_pexplorer;
			/* FIXME: fail if this returns FALSE - but first find out, why it
			 * fails in the first place */
			if (!next_existence(p_perspective, p_pexplorer, p_eqwrapper,
					p_checkpoint, p_vflags)) {

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
				//SET_GFLAG_VRFD

				/*DBG_VERIFY(
						fprintf(stderr, SHELL_GREEN "<dummy>" SHELL_RESET1);
				)*/

				fprintf(stderr, SHELL_GREEN "<%d:#%d",
						expl_cp->num, (*pexplorer)->num);
				print_sub(subd);
				fprintf(stderr, ">" SHELL_RESET1);

				finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
									subd);

				free(eqwrapper);
				free(pexplorer);
				free(checkpoint);
				free(subd);
				
				return TRUE;
			}

			if (ve_recursion(pexstart, p_perspective,
					p_pexplorer, p_eqwrapper, p_checkpoint, p_vflags,
					FALSE)) {
				success = TRUE;
				*p_pexplorer = expl_cp;
				break;
			} else {
				*p_pexplorer = expl_cp;
			}

		} else {
			DBG_EFAIL(
				fprintf(stderr, SHELL_RED "<%d:%d",
						(*p_pexplorer)->num, (*pexplorer)->num);
				print_sub(subd);
				fprintf(stderr, ">" SHELL_RESET1);
			)
		}
	}

	DBG_VERIFY(if (success) {
		fprintf(stderr, SHELL_GREEN "<%d:#%d",
				(*p_pexplorer)->num, (*pexplorer)->num);
		print_sub(subd);
		fprintf(stderr, ">" SHELL_RESET1);
	} /*else {
		fprintf(stderr, SHELL_RED "<%d>" SHELL_RESET1, (*p_pexplorer)->num);
	}*/)

	finish_verify(pexplorer, &eqwrapper, checkpoint, &vflags,
		subd);

	free(eqwrapper);
	free(pexplorer);
	free(checkpoint);
	free(subd);

	return success;
}

#if 0
Variable* collect_forward_vars(Pnode* pcollector)
{
	Variable* var;
	Variable* newvar;

	var = NULL;

	/* TODO: think about how to handle _variables_ during forward substitution*/
	while (pcollector->num != -1) {
		if (HAS_NFLAG_NEWC(pcollector)) {
			//DBG_VERIFY(fprintf(stderr, SHELL_RED "." SHELL_RESET1);)	
			newvar = (Variable*) malloc(sizeof(Variable));
			newvar->pnode = *(pcollector->child);
			newvar->next = var;
			newvar->flags = VARFLAG_NONE;
			var = newvar;
		}
		pcollector = *(pcollector->right);
	}

	return var;
}
#endif

VTree* collect_forward_vars(Pnode* pcollector)
{
	VTree* vtree;
	VTree* newvtree;

	vtree = NULL;

	/* TODO: think about how to handle _variables_ during forward substitution*/
	while (pcollector->num != -1) {
		if (HAS_NFLAG_NEWC(pcollector)) {
			//DBG_VERIFY(fprintf(stderr, SHELL_RED "." SHELL_RESET1);)	
			newvtree = (VTree*) malloc(sizeof(VTree));
			newvtree->pnode = *(pcollector->child);
			newvtree->right = vtree;
			newvtree->left = NULL;
			newvtree->flags = VARFLAG_NONE;
			vtree = newvtree;
		}
		pcollector = *(pcollector->right);
	}

	return vtree;
}

unsigned short int verify_existence(Pnode* pn, Pnode* pexstart)
{
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
	
	bc_push(pexplorer, &eqwrapper, checkpoint, &vflags);

	if (ve_recursion(pexstart, pn, pexplorer, &eqwrapper, checkpoint,
			&vflags, TRUE)) {
		SET_GFLAG_VRFD
	} else {
		DBG_VERIFY(fprintf(stderr, SHELL_BROWN "<not verified; "
					"trying forward substitution>");)	

		fw_vtree = collect_forward_vars(pexstart);
		if (fw_vtree != NULL) {
			init_sub(pn, fw_vtree, &vflags, subd, TRUE);
			while (next_sub(pn, *subd, TRUE)) {
				print_sub(subd);
				if (ve_recursion(pexstart, pn, pexplorer, &eqwrapper, checkpoint,
						&vflags, TRUE)) {
					SET_GFLAG_VRFD
					break;
				}
			}
			finish_sub(&vflags, subd);
		}
	}
	DBG_VERIFY(fprintf(stderr, SHELL_RESET1);)	
	
	bc_pop(pexplorer, &eqwrapper, checkpoint, &vflags);

	free(eqwrapper);
	free(pexplorer);
	free(checkpoint);
	free(subd);

	return TRUE;
}

/* --- substitution --------------------------------------------------------- */

void init_known_const(Pnode* perspective, SUB* s, unsigned short int fwd)
{
	if (fwd) {
		s->known_const = perspective->prev_id;
	} else {
		s->known_const = perspective->prev_const;
	}
}

/* substitute variable */
unsigned short int sub_var(SUB* s)
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
}

/* substitutes variable(s) by the next known constant/sub-tree */
unsigned short int next_sub(Pnode* perspective, SUB* s,
		unsigned short int fwd)
{
	SUB* s_iter;

	s_iter = s;

	while (s_iter != NULL) {
			if (fwd) {
				s_iter->known_const = s_iter->known_const->prev_id;
			} else {
				s_iter->known_const = s_iter->known_const->prev_const;
			}

			if (s_iter->known_const == NULL) {
				init_known_const(perspective, s_iter, fwd);
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
void init_sub(Pnode* perspective, VTree* vtree, VFlags* vflags,
		SUB** subd, unsigned short int fwd)
{
	SUB* prev;
	SUB* next;

	vtree = init_vtree(vtree);
	prev = *subd;

	/* only substitute, if there is anything to substitute in */
	if (/*vtree != NULL && vtree->pnode != NULL*/ /*DEBUG*/ // &&
			(!fwd && perspective->prev_const != NULL) 
				|| (fwd && perspective->prev_id != NULL)) {

		do {
			//if (!HAS_VARFLAG_LOCK(var->flags)) {
			if (!HAS_VARFLAG_LOCK(vtree->flags) && vtree->pnode != NULL) {
				//SET_VARFLAG_LOCK(var->flags)
				SET_VARFLAG_LOCK(vtree->flags)

				*subd = (SUB*) malloc(sizeof(SUB));
				(*subd)->prev = prev;
				prev = *subd;

				init_known_const(perspective, *subd, fwd);

				//(*subd)->sym = *(var->pnode->symbol);
				(*subd)->sym = *(vtree->pnode->symbol);
				(*subd)->vtree = vtree;

				sub_var(*subd);
			}
			//var = var->next;
			vtree = next_var(vtree);
		//} while (var != NULL);
		} while (vtree != NULL);

		/* reverse list */
		prev = NULL;
		if (*subd != NULL) {
			while ((*subd) != NULL) {
				next = (*subd)->prev;
				(*subd)->prev = prev;
				prev = *subd;
				*subd = next;
			}
			*subd = prev;
		}
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

		*((*subd)->vtree->pnode->symbol) = (*subd)->sym;
		*((*subd)->vtree->pnode->child) = NULL;
		*((*subd)->vtree->pnode->right) = NULL;

		UNSET_VARFLAG_LOCK(((*subd)->vtree->flags))

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
	//bctos->fail = HAS_VFLAG_FAIL(*vflags);
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

	/*if ((*checkpoint)->fail) {
		SET_VFLAG_FAIL(*vflags)
	} else {
		UNSET_VFLAG_FAIL(*vflags)
	}*/

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
		//UNSET_VFLAG_FAIL(*vflags)
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
	if (EXPLORABLE) {
		bc_push(pexplorer, eqwrapper, checkpoint, vflags);
		*pexplorer = *((*pexplorer)->child);
		SET_VFLAG_BRCH(*vflags)
		UNSET_VFLAG_FAIL(*vflags)
		if (!next_in_branch(veri_perspec, pexplorer, eqwrapper, checkpoint,
					vflags)) {
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
		//DBG_EPATH(fprintf(stderr, SHELL_BROWN ">" SHELL_RESET1);)
		POP
		//DBG_EPATH(fprintf(stderr, SHELL_CYAN ">" SHELL_RESET1);)
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
	} else if (!HAS_SYMBOL((*pexplorer)) &&\
			!(HAS_NFLAG_FRST((*pexplorer))\
						|| HAS_VFLAG_FRST(*vflags))) {\
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
		Eqwrapper** eqwrapper, BC** checkpoint, VFlags* vflags)
{
	/* FIXME: too much recursion */
	unsigned short int proceed;
	unsigned short int justfailed;

	do {
		proceed = FALSE;

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
			justfailed = FALSE;
			/* don't check assumptions, when branching through claimed
			 * existence */
			if (!check_asmp(perspective, pexplorer, FALSE)) {
				SET_VFLAG_FAIL((*vflags))
				justfailed = TRUE;
			}

			if ((HAS_NFLAG_FRST((*pexplorer))
						|| HAS_VFLAG_FRST(*vflags))) {
				if (justfailed) {
					/* pop through branch checkpoints until node is not part
					 * of an assumption TODO: add a nice example here */
					POP_PROCEED
				} else if (!move_right(pexplorer)) {
					/* When VFLAG_FRST is set, it might happen that we cannot
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

				/* do not check any assumptions whe branching through claimed
				 * existence */
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


unsigned short int next_existence(Pnode* perspective, Pnode** pexplorer,
		Eqwrapper** eqwrapper, BC** checkpoint, VFlags* vflags)
{
	unsigned short int proceed;

	if (!branch_proceed(pexplorer, eqwrapper, checkpoint, vflags)) {
		return FALSE;
	}
	
	do {
		proceed = FALSE;

		/* skip all assumptions, when checking existence */
		while (HAS_NFLAG_FRST((*pexplorer))
				&& HAS_NFLAG_IMPL((*pexplorer))) {
			if (!branch_proceed(pexplorer, eqwrapper, checkpoint, vflags)) {
				return FALSE;
			}
		}

		/* skip formulators */
		if (HAS_SYMBOL((*pexplorer))) {
			BRANCH_PROCEED
		}

		/* explore EXPLORABLE subbranches */
		if (explore_branch(pexplorer, eqwrapper, checkpoint, vflags)) {
			PROCEED
		}
	} while (proceed);

	return TRUE;
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
						vflags)) {
				exit_branch(pexplorer, eqwrapper, checkpoint, vflags);
			}
			return TRUE;
		} 

		/* substitution */
		if (HAS_VFLAG_SUBD(*vflags)) {

			if (next_sub(sub_perspec, *subd, FALSE)) {
				return attempt_explore(veri_perspec, sub_perspec, pexplorer,
						eqwrapper, checkpoint, vflags, subd);
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
			if ((*pexplorer)->vtree != NULL) {
				init_sub(sub_perspec, (*pexplorer)->vtree,
						vflags, subd, FALSE);
			}
			return attempt_explore(veri_perspec, sub_perspec, pexplorer,
					eqwrapper, checkpoint, vflags, subd);
			/* TODO: remove recursion */
		}
	} while (proceed);

	return FALSE;
}
