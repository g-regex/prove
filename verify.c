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

unsigned short int next_in_branch(Pnode* pnode);

static Pnode* eqfirst; /* temporarily holds the first node of an equality */
static Pnode* reachable;

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
	Pnode* eqfirst;
	Pnode* eqendwrap;
	struct branch_checkpoint* above;
} BC;
static BC* bc = NULL;
static Pnode* eqendwrap; /*temporarily holds node at which to stop wrapping*/

/* for verification */
void finish_verify();

/* for backtracking */
void init_backtrack(Pnode* pnode);
unsigned short int next_reachable_const(Pnode* pnode);

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

unsigned short int are_equal(Pnode* p1, Pnode* p2)
{
	Variable* firsteq;
	Variable* eq_iter;

	//DBG_PARSER(fprintf(stderr, "(%d==%d?)", p1->num, p2->num);)

	/* start with checking linked list of equal nodes */
	firsteq = p2->equalto;
	for (eq_iter = firsteq->next; eq_iter != firsteq; eq_iter = eq_iter->next) {
		//DBG_PARSER(fprintf(stderr, "+");)
		/*DBG_PARSER(fprintf(stderr, "(eqiter: %d)(p1/%d: %d)",
				eq_iter->pnode->num,
				p1->num,
				p1->equalto->pnode->num
				);)*/
		if (eq_iter == p1->equalto) {
			//DBG_PARSER(fprintf(stderr, "!");)
			return TRUE;
		} else if (CONTAINS_ID(eq_iter->pnode) && CONTAINS_ID(p1->equalto->pnode)) {
			//DBG_PARSER(fprintf(stderr, "Z");)
			if (strcmp(*((*(eq_iter->pnode->child))->symbol),
						*((*(p1->equalto->pnode->child))->symbol)) == 0) {
			//DBG_PARSER(fprintf(stderr, "&");)
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

	if (are_equal(p1, p2)) {
		return TRUE;
	}

	/*if (HAS_NFLAG_TRUE(p1) != HAS_NFLAG_TRUE(p2)) {
		return FALSE;
	}*/

	equal = TRUE;
	if (IS_ID(p1)) {
		if (IS_ID(p2)) {

			/******************************************************************/
			/* BIG TODO!!! We have to decide on how identifiers may be
			 * introduced. Ideally we'd just have one line here:
			 * return (*(p1->symbol) == *(p2->symbol));	
			 *
			 * Instead of:*/

			 if (*(p1->symbol) != *(p2->symbol)) {
				//unsigned short int r = (strcmp(*(p1->symbol), *(p2->symbol)) == 0);	
				//DBG_VERIFY(fprintf(stderr, "+%d,%s,%s", r, *(p1->symbol), *(p2->symbol));)
				return (strcmp(*(p1->symbol), *(p2->symbol)) == 0);	
				//return r;
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
		return FALSE;
	}
	if (are_equal(pnode, reachable)) {
		return TRUE;
	}
	return const_equal(*(pnode->child), *(reachable->child));
}

/* checks assumption in 'reachable' from the perspective of pnode */
unsigned short int check_asmp(Pnode* pnode)
{
	Pnode* pconst;

	DBG_VERIFY(fprintf(stderr, ":");)

	for (pconst = pnode->prev_const; pconst != NULL;
			pconst = pconst->prev_const) {
		if (verify(pconst)) {
			DBG_VERIFY(fprintf(stderr, "(%d)", pconst->num);)
			return TRUE;
		} else {
			DBG_VERIFY(fprintf(stderr, "%d,", pconst->num);)
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
	DBG_PARSER(if(HAS_GFLAG_VRFD) fprintf(stderr, "*");)
	DBG_PARSER(fprintf(stderr, "{%d}", pn->num);)
	if (!HAS_GFLAG_VRFD || DBG_COMPLETE_IS_SET) {
		while (next_reachable_const(pn)) {
			DBG_PARSER(fprintf(stderr, "<%d", rn());)
			if(verify(pn)) {
				DBG_PARSER(fprintf(stderr, "#");)
				SET_GFLAG_VRFD
				
				/* if no debugging options are selected and not
				 * explicitly requested, skip unnecessary compares */
				if (DBG_NONE_IS_SET || !DBG_COMPLETE_IS_SET) {
					finish_verify();
					DBG_PARSER(fprintf(stderr, ">");)
					break;
				}
			}
			DBG_PARSER(fprintf(stderr, ">");)
		}
	}

	if (!HAS_GFLAG_VRFD) {	
		TOGGLE_NFLAG_TRUE(pn)
		/*if (!HAS_NFLAG_TRUE(pn)) {*/
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
		*(s->var->pnode->child) = *((*(s->known_const->child))->child);
		*(s->var->pnode->right) = *((*(s->known_const->child))->right);
	}
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

					DBG_VERIFY(fprintf(stderr,
						"\\%s=%d/", s->sym, s->known_const->num);)
					return TRUE;
				} else {
					return FALSE;
				}
			} else {
				sub_var(s);
				DBG_VERIFY(fprintf(stderr,
					"\\%s=%d/", s->sym, s->known_const->num);)
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

/* substitute original variable symbols back in */
void finish_sub()
{
	SUB* prev_sub;

	do {
		prev_sub = sub->prev;

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
	eqfirst = bc->eqfirst;
	eqendwrap = bc->eqendwrap;

	bc = bc->above;
	free(bcold);
}

/* The following functions all set the value of "reachable" directly or
 * indirectly and return TRUE in the case of success and FALSE otherwise. */
unsigned short int explore_branch()
{
	if (HAS_NFLAG_IMPL((*(reachable->child))) || HAS_NFLAG_EQTY((*(reachable->child)))) {
		bc_push();
		reachable = *(reachable->child);
		return TRUE;
	} else {
		return FALSE;
	}
}

unsigned short int exit_branch()
{
	while (bc != NULL) {
		bc_pop(&reachable);
	}
	UNSET_GFLAG_BRCH
}

unsigned short int attempt_explore(Pnode* pnode)
{
	if (explore_branch()) {
		SET_GFLAG_BRCH
		DBG_VERIFY(fprintf(stderr, "~");)
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
	do {
		if (bc->above == NULL) {
			return FALSE; /* last pop is done by exit_branch */
		} else {
			bc_pop(&reachable);
			if (HAS_GFLAG_WRAP) {
				SET_GFLAG_WRAP
				wrap_right();
				break;
			}
		}
	} while (!wrap_right());

	return HAS_SYMBOL(reachable) ? next_in_branch(pnode) : TRUE;
}

/* set "reachable" to the next valid value or return FALSE */
unsigned short int next_in_branch(Pnode* pnode)
{
	if (HAS_SYMBOL(reachable)) {
		return wrap_right(&reachable);
	}

	if (explore_branch()) {
		return next_in_branch(pnode);
	}

	if (HAS_NFLAG_IMPL(reachable)) {
		if (HAS_NFLAG_FRST(reachable)) {
			DBG_VERIFY(fprintf(stderr, "(%d", reachable->num);)
			if (!check_asmp(pnode)) {
				DBG_VERIFY(fprintf(stderr, ":F)");)
				return FALSE;	
			} else if (!move_right(&reachable)) {
				DBG_VERIFY(fprintf(stderr, ":X)");)
				/* FATAL ERROR, must not happen
				 * (node with FRST flag cannot be the last one) */
				return FALSE;
			} else {
				DBG_VERIFY(fprintf(stderr, ":>)");)
				return next_in_branch(pnode);
			}
		} else {
			if (explore_branch()) {
				return next_in_branch(pnode);
			} else {
				return branch_proceed(pnode);	
			}
		}
	} else if (HAS_NFLAG_EQTY(reachable)) {
		if (HAS_GFLAG_WRAP) {
			if (eqendwrap == reachable) {
				UNSET_GFLAG_WRAP
				return branch_proceed(pnode);
			}

			do {
				wrap_right();
			} while (HAS_SYMBOL(reachable));

			return TRUE;
		} else {
			if (HAS_NFLAG_FRST(reachable)) {
				eqfirst = reachable;
			}

			do {

				DBG_VERIFY(fprintf(stderr, "(%d", reachable->num);)
				if (HAS_SYMBOL(reachable)){
					DBG_VERIFY(fprintf(stderr, ":S)");)
					continue;
				} else if (check_asmp(pnode)) {
					DBG_VERIFY(fprintf(stderr, ":>>)");)
					SET_GFLAG_WRAP
					eqendwrap = reachable;

					do {
						wrap_right();
					} while (HAS_SYMBOL(reachable));

					return TRUE;
				} else {
					DBG_VERIFY(fprintf(stderr, ":E)");)
				}

			} while (move_right(&reachable));

			return FALSE;
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
	if (move_left(&reachable) || 
			(move_up(&reachable) && move_left(&reachable))) {
		if (HAS_SYMBOL(reachable) ?  move_left(&reachable) :  TRUE) {
			if (reachable->var != NULL) {
				init_sub(pnode);
			}
			return attempt_explore(pnode);
		}
	}
	return FALSE;
}

/* --- debugging ------------------------------------------------------------ */

#ifdef DNUM
unsigned short int rn()
{
	return reachable->num;
}
#endif
