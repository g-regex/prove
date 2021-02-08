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

#define EXPLORABLE \
	(HAS_CHILD(reachable) && (HAS_NFLAG_IMPL((*(reachable->child)))\
			|| (!HAS_NFLAG_FRST(reachable)\
				&& HAS_NFLAG_EQTY((*(reachable->child))))))

unsigned short int next_in_branch(Pnode* pnode);

static Pnode* eqfirst; /* temporarily holds the first node of an equality */

/* stack for substitution */
typedef struct substitution_status {
	Pnode* known_const;	/* currently used constant sub-tree for substitution  */
	char* sym;			/* symbol of substituted variable */
	Variable* var;		/* substituted variable */
	struct substitution_status* prev;
} SUB;
static SUB* sub = NULL;

/* stack for branch exploration */
typedef struct branch_checkpoint { /* stack for jumping back to parent levels */
	Pnode* pnode;
	unsigned short int wrap;
	unsigned short int frst;
	Pnode* eqfirst;
	Pnode* eqendwrap;
	struct branch_checkpoint* above;
} BC;
static BC* bc = NULL;
static Pnode* eqendwrap; /*temporarily holds node at which to stop wrapping*/

/* --- verification specific movement functions ----------------------------- */

/* move right; if already at the right-most node, move to the left-most node */ 
unsigned short int wrap_right()
{
	if (!move_right(&reachable)) {
		if (HAS_GFLAG_WRAP) {
			reachable = eqfirst;
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
	Variable* firsteq;
	Variable* eq_iter;

	firsteq = p2->equalto;
	for (eq_iter = firsteq->next; eq_iter != firsteq; eq_iter = eq_iter->next) {
		if (eq_iter == p1->equalto) {
			return TRUE;
		} else if (IS_ID(eq_iter->pnode) && IS_ID(p1)) { /* FIXME: SCOPING */
			if (strcmp(*(eq_iter->pnode->symbol),
								*(p1->symbol)) == 0) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* compares two constant sub-trees;
 * must be given the top left node of the subtrees to compare */
unsigned short int const_equal(Pnode* p1, Pnode* p2)
{
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

			 if (are_equal(p1, p2)) {
				return TRUE;
			 } else if (are_equal(p2, p1)) { /* FIXME: SCOPING */
				return TRUE;
			 } else if (*(p1->symbol) != *(p2->symbol)) {
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
		/* FIXME: segfaulting because of call stack overflow;
		 * use less recursion  */
		equal = HAS_CHILD(p2) ? const_equal(*(p1->child), *(p2->child)) : FALSE;
	}
	if (equal && HAS_RIGHT(p1)) {
		equal = HAS_RIGHT(p2) ? const_equal(*(p1->right), *(p2->right)) : FALSE;
	}

	equal &= (IS_EMPTY(p1) == IS_EMPTY(p2));

	if (equal) {
		/* equate(p1, p2); This is not working due to the possible occurrence
		 * of variables. TODO: make this work to improve performance */
		/*if (HAS_NFLAG_NEWC(p1)) {
			equate(p1, p2);
		}*/

		return TRUE;
	} else {
		return FALSE;
	}
}

/* checks pnode against reachable node; this function is basically a safety net,
 * if pnode or reachable have no children, which should not happen anyway */
unsigned short int verify(Pnode* pnode)
{
	if (!HAS_CHILD(pnode) && !HAS_CHILD(reachable)) {
		/* FATAL ERROR: function should not have been called, if this is true */
		//DBG_VERIFY(fprintf(stderr, "X");)
		return FALSE;
	}
	if (are_equal(reachable, pnode)) {
		return TRUE;
	}
	return const_equal(*(reachable->child), *(pnode->child));
}

/* checks assumption in 'reachable' from the perspective of pnode */
unsigned short int check_asmp(Pnode* pnode)
{
	Pnode* pconst;

	for (pconst = pnode->prev_const; pconst != NULL;
			pconst = pconst->prev_const) {
		if (verify(pconst)) {
			return TRUE;
		}
	}
	return FALSE;
}

unsigned short int trigger_verify(Pnode* pn)
{
	/*if (IS_EMPTY(pn) || (HAS_CHILD(pn) && IS_EMPTY((*(pn->child))))) {
		return HAS_NFLAG_TRUE(pn);
	}*/
	init_backtrack(pn);
	DBG_PARSER(fprintf(stderr, SHELL_BOLD "{%d}" SHELL_RESET2, pn->num);)	
	DBG_PARSER(if(HAS_GFLAG_VRFD) fprintf(stderr, "*");)
	if (!HAS_GFLAG_VRFD || DBG_COMPLETE_IS_SET) {
		while (next_reachable_const(pn)) {
			if(verify(pn)) {
				DBG_PARSER(fprintf(stderr, SHELL_GREEN "<#%d", rn());)
				SET_GFLAG_VRFD
				
				/* if no debugging options are selected and not
				 * explicitly requested, skip unnecessary compares */
				if (DBG_NONE_IS_SET || !DBG_COMPLETE_IS_SET) {
					finish_verify();
					DBG_PARSER(fprintf(stderr, ">" SHELL_RESET1);) 
					break;
				}
				DBG_PARSER(fprintf(stderr, ">" SHELL_RESET1);) 
			}
			/* DBG_PARSER(fprintf(stderr, "<%d>", rn());) */
		}
	}

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

void init_known_const(Pnode* pnode, SUB* s)
{
	s->known_const = pnode->prev_const;
}

/* substitute variable */
unsigned short int sub_var(SUB* s)
{
	if (CONTAINS_ID(s->known_const)) {
		*(s->var->pnode->symbol) = *((*(s->known_const->child))->symbol);
		*(s->var->pnode->child) = NULL;
		*(s->var->pnode->right) = NULL;
	} else {
		*(s->var->pnode->symbol) = NULL;
		/* FIXME: check the next 6 lines; missing anything? */
		if ((*(s->known_const->child))->child != NULL) {
			*(s->var->pnode->child) = *((*(s->known_const->child))->child);
		}
		if ((*(s->known_const->child))->right != NULL) {
			*(s->var->pnode->right) = *((*(s->known_const->child))->right);
		}
	}
	//DBG_VERIFY(fprintf(stderr,
	//	"\\%s=%d/", s->sym, s->known_const->num);)
}

/* substitutes variable(s) by the next known constant/sub-tree */
unsigned short int next_known_const(Pnode* pnode, SUB* s)
{
	if (s != NULL) {
		if (s->known_const != NULL) {
			s->known_const = s->known_const->prev_const;
			if (s->known_const == NULL) {
				if (next_known_const(pnode, s->prev)) {
					init_known_const(pnode, s);
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
void init_sub(Pnode* pnode)
{
	Variable* var;
	SUB* oldsub;

	var = reachable->var;
	oldsub = sub;

	/* only substitute, if there is something to substitute in */
	if (pnode->prev_const != NULL) {
		do {
			sub = (SUB*) malloc(sizeof(SUB));
			sub->prev = oldsub;
			oldsub = sub;

			init_known_const(pnode, sub);

			sub->sym = *(var->pnode->symbol);
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

		/* TODO: function print_sub for debugging with DFLAG DCOMPLETE */
		DBG_VERIFY(
				if (HAS_GFLAG_VRFD && !DBG_COMPLETE_IS_SET) {
					fprintf(stderr,
						"(%s=%d)", sub->sym, sub->known_const->num);
				}
		)

		*(sub->var->pnode->symbol) = sub->sym;
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
void bc_push()
{
	BC* bctos; /* top of stack for branch checkpoint, if descending */

	bctos = (BC*) malloc(sizeof(BC));

	bctos->pnode = reachable;
	bctos->wrap = HAS_GFLAG_WRAP;
	bctos->frst = HAS_GFLAG_FRST;
	bctos->eqfirst = eqfirst;
	bctos->eqendwrap = eqendwrap;
	bctos->above = bc;
	bc = bctos;
}

void bc_pop(Pnode** pnode)
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

	eqfirst = bc->eqfirst;
	eqendwrap = bc->eqendwrap;

	bc = bc->above;
	free(bcold);
}

/* The following functions all set the value of "reachable" directly or
 * indirectly and return TRUE in the case of success and FALSE otherwise. */
unsigned short int explore_branch()
{
	/* OLD less restricted definition of EXPLORABLE:
	 * if (HAS_NFLAG_IMPL((*(reachable->child)))
	 *		|| HAS_NFLAG_EQTY((*(reachable->child)))) {
	 */
	if (EXPLORABLE) {
		if (HAS_NFLAG_FRST(reachable)) {
			bc_push();
			reachable = *(reachable->child);
			SET_GFLAG_FRST
		} else {
			bc_push();
			reachable = *(reachable->child);
		}
		return TRUE;
	} else {
		return FALSE;
	}
}

void exit_branch()
{
	while (bc != NULL) {
		bc_pop(&reachable);
	}
	UNSET_GFLAG_BRCH
	UNSET_GFLAG_FRST
}

unsigned short int attempt_explore(Pnode* pnode)
{
	/* OLD implementation: if (explore_branch()) { */
	if (EXPLORABLE) {
		bc_push();
		reachable = *(reachable->child);
		SET_GFLAG_BRCH
		//DBG_VERIFY(fprintf(stderr, "~");)
		if (!next_in_branch(pnode)) {
			exit_branch();
			return next_reachable_const(pnode);
		}
	} else {
		UNSET_GFLAG_BRCH
	}
	return TRUE;
}

/* move up in branch in proceed with exploration to the right, if possible */
unsigned short int branch_proceed(Pnode* pnode)
{
	while (!wrap_right()) {
		if (bc->above == NULL) {
			return FALSE; /* last pop is done by exit_branch */
		} else {
			bc_pop(&reachable);
			if (wrap_right()) {
				break;
			}
		}
	}

	/* FIXME: check, whether GFLAG_FRST is set properly
	 * if (HAS_GFLAG_FRST) {
	 *	DBG_VERIFY(fprintf(stderr, "*(%d)", rn());)
	 *} */
	return HAS_SYMBOL(reachable) ? next_in_branch(pnode) : TRUE;
}

/* set "reachable" to the next valid value or return FALSE */
unsigned short int next_in_branch(Pnode* pnode)
{
	/* FIXME: too much recursion */

	/* skip formulators */
	if (HAS_SYMBOL(reachable)) {
		return branch_proceed(pnode);	
	}

	if (explore_branch()) {
		return next_in_branch(pnode);
	}

	if (HAS_NFLAG_IMPL(reachable)) {
		//DBG_VERIFY(fprintf(stderr, "(%d", reachable->num);)
		if (HAS_NFLAG_FRST(reachable) || HAS_GFLAG_FRST) {
			if (!check_asmp(pnode)) {
				//DBG_VERIFY(fprintf(stderr, ":F)");)

				/* c/p from branch proceed -> optimise */
				do {
					if (bc->above == NULL) {
						return FALSE; /* last pop is done by exit_branch */
					} else {
						bc_pop(&reachable);
					}
				} while (HAS_NFLAG_IMPL(reachable)
							&& HAS_NFLAG_FRST(reachable));
				return branch_proceed(pnode);
			} else if (!move_right(&reachable)) {
				//DBG_VERIFY(fprintf(stderr, ":R)");)
				/* OLD: FATAL ERROR, must not happen
				 * (node with FRST flag cannot be the last one) */
				// CORRECTION: can happen with GFLAG FRST
				return branch_proceed(pnode);
			} else {
				//DBG_VERIFY(fprintf(stderr, ":>)");)
				return next_in_branch(pnode);
			}
		} else {
			//DBG_VERIFY(fprintf(stderr, ":X)");)
			if (explore_branch()) {
				return next_in_branch(pnode);
			} else {
				//DBG_PARSER(fprintf(stderr, ".");)
				return branch_proceed(pnode);	
			}
		}
	} else if (HAS_NFLAG_EQTY(reachable)) {
		if (HAS_GFLAG_WRAP) {
			if (eqendwrap == reachable) {
				UNSET_GFLAG_WRAP
				return branch_proceed(pnode);
			}

			/*OLD: do {
				wrap_right();
			} while (HAS_SYMBOL(reachable));
			return TRUE;*/

			/* NEW */
			if (are_equal(reachable, eqendwrap)) {
				//DBG_VERIFY(fprintf(stderr, "T", reachable->num);)
				do {
					wrap_right();
				} while (HAS_SYMBOL(reachable));
				return TRUE;
			} else {
				return FALSE;
			}
			/* --- */
		} else {
			if (HAS_NFLAG_FRST(reachable)) {
				eqfirst = reachable;
			}

			do {

				//DBG_VERIFY(fprintf(stderr, "(%d", reachable->num);)
				if (HAS_SYMBOL(reachable)){
					//DBG_VERIFY(fprintf(stderr, ":S)");)
					continue;
				} else if (check_asmp(pnode)) {
					//DBG_VERIFY(fprintf(stderr, ":>>)");)
					SET_GFLAG_WRAP
					eqendwrap = reachable;

					do {
						wrap_right();
					} while (HAS_SYMBOL(reachable));
					return TRUE;
					/*if (are_equal(reachable, eqendwrap)) {
						DBG_VERIFY(fprintf(stderr, "T*", reachable->num);)
						return TRUE;
					} else {
						return FALSE;
					}*/
				} else {
					//DBG_VERIFY(fprintf(stderr, ":E)");)
				}

			} while (move_right(&reachable));

			/* c/p from branch proceed -> optimise */
			if (bc->above == NULL) {
				return FALSE; /* last pop is done by exit_branch */
			} else {
				bc_pop(&reachable);
				if (HAS_GFLAG_WRAP) {
					wrap_right(); /* FIXME: is this, what we want? */
				}
			}
			return branch_proceed(pnode);
			/* --- */
			/*return FALSE;*/
		}
	}

	return FALSE;
}

/* --- backtracking --------------------------------------------------------- */

void init_backtrack(Pnode* pnode)
{
	reachable = pnode;
}

/* finish verification of current node cleanly */
void finish_verify()
{
	if (HAS_GFLAG_BRCH) {
		exit_branch();
	}
	if (HAS_GFLAG_SUBD) {
		finish_sub();
	}
}

unsigned short int next_reachable_const(Pnode* pnode)
{
	/* branch exploration */
	if (HAS_GFLAG_BRCH) {
		if (!next_in_branch(pnode)) {
			exit_branch();
		}
		return TRUE;
	} 

	/* substitution */
	if (HAS_GFLAG_SUBD) {
		if (next_known_const(pnode, sub)) {
			return attempt_explore(pnode);
		} else {
			finish_sub();
			return next_reachable_const(pnode);
		}
	}

	/* backtracking */
#if 0
	OLD: backtracking was struggling when first statements
	in a formula were nested

	if (move_left(&reachable) || 
			(move_up(&reachable) && (move_left(&reachable) || TRUE))) {
		/*if (HAS_SYMBOL(reachable) ?  move_left(&reachable) :  TRUE) {
			if (reachable->var != NULL) {
				init_sub(pnode);
			}
			return attempt_explore(pnode);
		}*/
		DBG_PARSER(fprintf(stderr, ";%d", reachable->num);)
		if (HAS_SYMBOL(reachable)) {
			return next_reachable_const(pnode);
		} else {
			if (reachable->var != NULL) {
				init_sub(pnode);
			}
			return attempt_explore(pnode);
		}
	}
	return FALSE;
#endif

	/* NEW */
	do {
		if (move_left(&reachable)) {
			break;
		} else if (!move_up(&reachable)) {
			return FALSE;
		}
	} while (TRUE);

	if (HAS_SYMBOL(reachable)) {
		return next_reachable_const(pnode);
	} else {
		if (reachable->var != NULL) {
			init_sub(pnode);
		}
		return attempt_explore(pnode);
	}
	/* --- */
}

/* --- debugging ------------------------------------------------------------ */

//#ifdef DNUM
unsigned short int rn()
{
	return reachable->num;
}
//#endif
