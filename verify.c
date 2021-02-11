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
		Eqwrapper** eqwrapper);

/* stack for substitution */
typedef struct substitution_status {
	Pnode* known_const;	/* currently used constant sub-tree for substitution  */
	char* sym;			/* symbol of substituted variable */
	Variable* var;		/* substituted variable */
	//Variable* equalto;
	struct substitution_status* prev;
} SUB;
static SUB* sub = NULL;

/* stack for branch exploration */
typedef struct branch_checkpoint { /* stack for jumping back to parent levels */
	Pnode* pnode;
	unsigned short int wrap;
	unsigned short int frst;
	Pnode* pwrapper;
	Pnode* pendwrap;
	struct branch_checkpoint* above;
} BC;
static BC* bc = NULL;
//static Pnode* eqendwrap; /*temporarily holds node at which to stop wrapping*/

/* --- verification specific movement functions ----------------------------- */

/* move right; if already at the right-most node, move to the left-most node */ 
unsigned short int wrap_right(Pnode** pexplorer, Eqwrapper** eqwrapper)
{
	if (!move_right(pexplorer)) {
		if (HAS_GFLAG_WRAP) {
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
unsigned short int check_asmp(Pnode* perspective, Pnode** pexplorer)
{
	Pnode* pconst;

	for (pconst = perspective->prev_const; pconst != NULL;
			pconst = pconst->prev_const) {
		if (verify(pconst, pexplorer)) {
			return TRUE;
		}
	}
	return FALSE;
}

#ifdef DVERIFY
void print_sub()
{
	SUB* sub_iter;

	sub_iter = sub;

	while (sub_iter != NULL) {
		fprintf(stderr,
			"(%s=%d)", sub_iter->sym, sub_iter->known_const->num);

		sub_iter = sub_iter->prev;
	}
}
#endif

unsigned short int trigger_verify(Pnode* pn, Pnode** pexplorer)
{
	/*if (IS_EMPTY(pn) || (HAS_CHILD(pn) && IS_EMPTY((*(pn->child))))) {
		return HAS_NFLAG_TRUE(pn);
	}*/
	//init_backtrack(pn);
	Eqwrapper* eqwrapper;
	eqwrapper = (Eqwrapper*) malloc(sizeof(Eqwrapper));
	
	*pexplorer = pn;
	DBG_PARSER(fprintf(stderr, SHELL_BOLD "{%d}" SHELL_RESET2, pn->num);)	
	DBG_PARSER(if(HAS_GFLAG_VRFD) fprintf(stderr, "*");)
	if (!HAS_GFLAG_VRFD || DBG_COMPLETE_IS_SET) {
		while (next_reachable_const(pn, pexplorer, &eqwrapper)) {
			if(verify(pn, pexplorer)) {
				DBG_PARSER(fprintf(stderr, SHELL_GREEN "<#%d",
							(*pexplorer)->num);)
				SET_GFLAG_VRFD

				DBG_VERIFY(print_sub();)
				
				/* if no debugging options are selected and not
				 * explicitly requested, skip unnecessary compares */
				if (DBG_NONE_IS_SET || !DBG_COMPLETE_IS_SET) {
					finish_verify(pexplorer, &eqwrapper);
					DBG_PARSER(fprintf(stderr, ">" SHELL_RESET1);) 
					break;
				}
				DBG_PARSER(fprintf(stderr, ">" SHELL_RESET1);) 
			}
			/* DBG_PARSER(fprintf(stderr, "<%d>", rn());) */
		}
	}

	free(eqwrapper);

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
	if (s != NULL) {
		if (s->known_const != NULL) {
			s->known_const = s->known_const->prev_const;
			if (s->known_const == NULL) {
				if (next_known_const(perspective, s->prev)) {
					init_known_const(perspective, s);
					sub_var(s);
					return TRUE;
				} else {
					return FALSE;
				}
			} else {
				sub_var(s);
				return TRUE;
			}
		} else {
			return FALSE; /* FATAL ERROR: should not happen */
		}
	} else {
		return FALSE; /* finish substitution */
	}
}

/* initialise substitution */
void init_sub(Pnode* perspective, Pnode** pexplorer)
{
	Variable* var;
	SUB* oldsub;

	var = (*pexplorer)->var;
	oldsub = sub;

	/* only substitute, if there is something to substitute in */
	if (perspective->prev_const != NULL) {
		do {
			sub = (SUB*) malloc(sizeof(SUB));
			sub->prev = oldsub;
			oldsub = sub;

			init_known_const(perspective, sub);

			sub->sym = *(var->pnode->symbol);
			//sub->equalto = *(var->pnode->equalto);
			sub->var = var;

			/* FIXME: added this. correct? */
			sub_var(sub);

			var = var->next;
		} while (var != NULL);

		SET_GFLAG_SUBD
	}
}

/* substitute original variable symbols back in */
void finish_sub()
{
	SUB* prev_sub;

	do {
		prev_sub = sub->prev;

		*(sub->var->pnode->symbol) = sub->sym;
		//*(sub->var->pnode->equalto) = sub->equalto;
		*(sub->var->pnode->child) = NULL;
		*(sub->var->pnode->right) = NULL;

		free(sub);
		sub = prev_sub;
	} while (sub != NULL);

	UNSET_GFLAG_SUBD
}

/* --- branching ------------------------------------------------------------ */

/* implementation of a "branch checkpoint" stack to easily find the next
 * parent node of the current level to jump back to */
void bc_push(Pnode** pexplorer, Eqwrapper** eqwrapper)
{
	BC* bctos; /* top of stack for branch checkpoint, if descending */

	bctos = (BC*) malloc(sizeof(BC));

	bctos->pnode = *pexplorer;
	//bctos->pnode = pexplorer;
	bctos->wrap = HAS_GFLAG_WRAP;
	bctos->frst = HAS_GFLAG_FRST;
	bctos->pwrapper = (*eqwrapper)->pwrapper;
	bctos->pendwrap = (*eqwrapper)->pendwrap;
	bctos->above = bc;
	bc = bctos;
}

void bc_pop(Pnode** pnode, Eqwrapper** eqwrapper)
{
	BC* bcold;
	unsigned short int wrap;

	bcold = bc;

	*pnode = bc->pnode;
	if (bc->wrap) {
		SET_GFLAG_WRAP
	} else {
		UNSET_GFLAG_WRAP
	}

	if (bc->frst) {
		SET_GFLAG_FRST
	} else {
		UNSET_GFLAG_FRST
	}

	(*eqwrapper)->pwrapper = bc->pwrapper;
	(*eqwrapper)->pendwrap = bc->pendwrap;

	bc = bc->above;
	free(bcold);
}

/* The following functions all set the value of pexplorer directly or
 * indirectly and return TRUE in the case of success and FALSE otherwise. */
unsigned short int explore_branch(Pnode** pexplorer, Eqwrapper** eqwrapper)
{
	/* OLD less restricted definition of EXPLORABLE:
	 * if (HAS_NFLAG_IMPL((*(reachable->child)))
	 *		|| HAS_NFLAG_EQTY((*(reachable->child)))) {
	 */
	if (EXPLORABLE) {
		if (HAS_NFLAG_FRST((*pexplorer))) {
			bc_push(pexplorer, eqwrapper);
			*pexplorer = *((*pexplorer)->child);
			SET_GFLAG_FRST
		} else {
			bc_push(pexplorer, eqwrapper);
			*pexplorer = *((*pexplorer)->child);
		}
		return TRUE;
	} else {
		return FALSE;
	}
}

void exit_branch(Pnode** pexplorer, Eqwrapper** eqwrapper)
{
	while (bc != NULL) {
		bc_pop(pexplorer, eqwrapper);
	}
	UNSET_GFLAG_BRCH
	UNSET_GFLAG_FRST
}

unsigned short int attempt_explore(Pnode* pnode, Pnode** pexplorer,
		Eqwrapper** eqwrapper)
{
	/* OLD implementation: if (explore_branch()) { */
	if (EXPLORABLE) {
		bc_push(pexplorer, eqwrapper);
		*pexplorer = *((*pexplorer)->child);
		SET_GFLAG_BRCH
		//DBG_VERIFY(fprintf(stderr, "~");)
		if (!next_in_branch(pnode, pexplorer, eqwrapper)) {
			exit_branch(pexplorer, eqwrapper);
			return next_reachable_const(pnode, pexplorer, eqwrapper);
		}
	} else {
		UNSET_GFLAG_BRCH
	}
	return TRUE;
}

#define POP \
	if (bc->above == NULL) {\
		return FALSE; /* last pop is done by exit_branch */\
	} else {\
		bc_pop(pexplorer, eqwrapper);\
	}

/* move up in branch in proceed with exploration to the right, if possible */
unsigned short int branch_proceed(Pnode** pexplorer, Eqwrapper** eqwrapper)
{
	/* FIXME: check, whether GFLAG_FRST is set properly */
	while (!wrap_right(pexplorer, eqwrapper)) {
		POP
		if (wrap_right(pexplorer, eqwrapper)) {
			break;
		}
	}
	return TRUE;
}

#define BRANCH_PROCEED \
	if (!branch_proceed(pexplorer, eqwrapper)) {\
		return FALSE;\
	} else if (!HAS_SYMBOL((*pexplorer))) {\
		return TRUE;\
	} else {\
		proceed = TRUE;\
		continue;\
	}

#define PROCEED \
	proceed = TRUE;\
	continue;

#define SKIP_FORMULATORS \
	do {\
		wrap_right(pexplorer, eqwrapper);\
	} while (HAS_SYMBOL((*pexplorer)));

/* set pexplorer to the next valid value or return FALSE */
unsigned short int next_in_branch(Pnode* perspective, Pnode** pexplorer,
		Eqwrapper** eqwrapper)
{
	/* FIXME: too much recursion */
	unsigned short int proceed;

	do {
		proceed = FALSE;

		/* skip formulators */
		if (HAS_SYMBOL((*pexplorer))) {
			BRANCH_PROCEED
		}

		/* explore EXPLORABLE subbranches */
		if (explore_branch(pexplorer, eqwrapper)) {
			PROCEED
		}

		if (HAS_NFLAG_IMPL((*pexplorer))) {
			/* if processing an assumption, verify it */
			if (HAS_NFLAG_FRST((*pexplorer)) || HAS_GFLAG_FRST) {
				if (!check_asmp(perspective, pexplorer)) {
					/* pop through branch checkpoints until node is not part
					 * of an assumption TODO: add a nice example here */
					do {
						POP
					} while (HAS_NFLAG_IMPL((*pexplorer))
								&& HAS_NFLAG_FRST((*pexplorer)));

					BRANCH_PROCEED
				} else if (!move_right(pexplorer)) {
					/* When GFLAG_FRST is set, it might happen that we cannot
					 * move right and have to pop back to proceed with the next
					 * node at a lower level */
					BRANCH_PROCEED
				} else {
					PROCEED
				}
			} else {
				if (explore_branch(pexplorer, eqwrapper)) {
					PROCEED
				} else {
					BRANCH_PROCEED
				}
			}
		} else if (HAS_NFLAG_EQTY((*pexplorer))) {
			if (HAS_GFLAG_WRAP) {
				/* proceed with branch after cycling through equality */
				if ((*eqwrapper)->pendwrap == *pexplorer) {
					UNSET_GFLAG_WRAP
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
					} else if (check_asmp(perspective, pexplorer)) {
						SET_GFLAG_WRAP
						(*eqwrapper)->pendwrap = *pexplorer;

						SKIP_FORMULATORS
						return TRUE;
					}

				} while (move_right(pexplorer));

				POP
				wrap_right(pexplorer, eqwrapper);
				BRANCH_PROCEED
			}
		}
	} while (proceed);

	return FALSE;
}

/* --- backtracking --------------------------------------------------------- */

/* finish verification of current node cleanly */
void finish_verify(Pnode** pexplorer, Eqwrapper** eqwrapper)
{
	if (HAS_GFLAG_BRCH) {
		exit_branch(pexplorer, eqwrapper);
	}
	if (HAS_GFLAG_SUBD) {
		finish_sub();
	}
}

unsigned short int next_reachable_const(Pnode* pnode, Pnode** pexplorer,
		Eqwrapper** eqwrapper)
{
	unsigned short int proceed;

	do {
		proceed = FALSE;

		/* branch exploration */
		if (HAS_GFLAG_BRCH) {
			if (!next_in_branch(pnode, pexplorer, eqwrapper)) {
				exit_branch(pexplorer, eqwrapper);
			}
			return TRUE;
		} 

		/* substitution */
		if (HAS_GFLAG_SUBD) {
			if (next_known_const(pnode, sub)) {
				return attempt_explore(pnode, pexplorer, eqwrapper); /* TODO: remove recursion */
			} else {
				finish_sub();
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
				init_sub(pnode, pexplorer);
			}
			return attempt_explore(pnode, pexplorer, eqwrapper); /* TODO: remove recursion */
		}
	} while (proceed);
}
