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

#define TRUE 1
#define FALSE 0

unsigned short int next_in_branch(Pnode* pnode);

static Pnode* eqfirst;			/* temporarily holds the first node of an equality */
static Pnode* reachable;

/* for substitution */
static Pnode* known_id; /* current identifier in linked list, known from
						   stance of current pnode */
static char* subd_var; /* remembered variable to be substituted */

/* for branch exploration */
typedef struct branch_checkpoint { /* stack for jumping back to parent levels */
	Pnode* pnode;
	unsigned short int wrap;
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

/* compares two constant subtrees;
 * must be given the top left node of the subtrees to compare */
unsigned short int const_equal(Pnode* p1, Pnode* p2)
{
	unsigned short int equal;

	equal = TRUE;
	/* TODO this can be done better */
	if (IS_ID(p1)) {
		if (IS_ID(p2)) {

			/******************************************************************/
			/* BIG TODO!!! We have to decide on how identifiers may be
			 * introduced. Ideally we'd just have one line here:
			 * return (*(p1->symbol) == *(p2->symbol));	*/

			 if (*(p1->symbol) != *(p2->symbol)) {
				return (strcmp(*(p1->symbol), *(p2->symbol)) == 0);	
			 } else {
				return TRUE;
			 }
			/******************************************************************/

		} else {
			return FALSE;
		}
	} else if (HAS_SYMBOL(p1)) { /* this is for formulators */
		if (HAS_SYMBOL(p2)) {
			return (strcmp(*(p1->symbol), *(p2->symbol)) == 0);	
		} else {
			return FALSE;
		}
	}

	if (HAS_CHILD(p1)) {
		equal = HAS_CHILD(p2) ? const_equal(p1->child, p2->child) : FALSE;
	}
	if (HAS_RIGHT(p1)) {
		equal = equal &&
			(HAS_RIGHT(p2) ? const_equal(p1->right, p2->right) : FALSE);
	}

	return equal;
}

/* checks pnode against reachable node; this function is basically a safety net,
 * if pnode or reachable have no children, which should not happen anyway */
unsigned short int same_as_rchbl(Pnode* pnode)
{
	if (!HAS_CHILD(pnode) && !HAS_CHILD(reachable)) {
		/* FATAL ERROR: function should not have been called, if this is true */
		return FALSE;
	}
	return const_equal(pnode->child, reachable->child);
}

/* checks assumption in 'reachable' from the perspective of pnode */
unsigned short int check_asmp(Pnode* pnode)
{
	Pnode* pconst;

	for (pconst = pnode->prev_const; pconst != NULL;
			pconst = pconst->prev_const) {
		if (same_as_rchbl(pconst)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* --- substitution --------------------------------------------------------- */

void init_known_id(Pnode* pnode)
{
	known_id = pnode;
}

unsigned short int next_known_id()
{
	/* TODO skip duplicates */
	if (known_id != NULL) {
		known_id = known_id->prev_const;
	}
	while (known_id != NULL && !CONTAINS_ID(known_id)) {
		known_id = known_id->prev_const;
	}

	return (known_id != NULL);
}

void init_sub(Pnode* pnode)
{
	init_known_id(pnode);
	next_known_id(); /* TODO add error check */

	subd_var = *(reachable->var->pnode->symbol);
	SET_GFLAG_SUBD
}

unsigned short int sub_vars()
{
	*(reachable->var->pnode->symbol) = *(known_id->child->symbol);
}

void finish_sub()
{
	*(reachable->var->pnode->symbol) = subd_var;
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
	if (HAS_NFLAG_IMPL(reachable->child) || HAS_NFLAG_EQTY(reachable->child)) {
		bc_push();
		reachable = reachable->child;
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
}

unsigned short int attempt_explore(Pnode* pnode)
{
	if (explore_branch()) {
		SET_GFLAG_BRCH
		if (!next_in_branch(pnode)) {
			exit_branch();
			UNSET_GFLAG_BRCH
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
			if (!check_asmp(pnode)) {
				return FALSE;	
			} else if (!move_right(&reachable)) {
				/* FATAL ERROR, must not happen
				 * (node with FRST flag cannot be the last one) */
				return FALSE;
			} else {
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

				if (HAS_SYMBOL(reachable)){
					continue;
				} else if (check_asmp(pnode)) {
					SET_GFLAG_WRAP
					eqendwrap = reachable;

					do {
						wrap_right();
					} while (HAS_SYMBOL(reachable));

					return TRUE;
				}

			} while (move_right(&reachable));

			return FALSE;
		}
	}

	return FALSE;
}

/* --- backtracking --------------------------------------------------------- */

void init_reachable(Pnode* pnode)
{
	reachable = pnode;
}

unsigned short int next_reachable_const(Pnode* pnode)
{
	/* branch exploration */
	if (HAS_GFLAG_BRCH) {
		if (!next_in_branch(pnode)) {
			exit_branch();
			UNSET_GFLAG_BRCH
		}
		return TRUE;
	} 

	/* substitution */
	if (HAS_GFLAG_SUBD) {
		if (next_known_id()) {
			sub_vars();
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
				sub_vars();
			}
			return attempt_explore(pnode);
		}
	}
	return FALSE;
}

/* --- debugging ------------------------------------------------------------ */

unsigned short int rn()
{
	return reachable->num;
}
