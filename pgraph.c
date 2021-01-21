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

#include "pgraph.h"
#include "verify.h"
#include "token.h"
#include "debug.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

/* DEBUG */
#ifdef DNUM
static short int n = 0;		/* node counter */
#endif

/* --- navigation through graph --------------------------------------------- */

unsigned short int move_right(Pnode** pnode)
{
	if (!HAS_RIGHT((*pnode))) {
		return FALSE;
	} else {
		*pnode = *((*pnode)->right);
		return TRUE;
	}
}

unsigned short int move_down(Pnode** pnode)
{
	if (!HAS_CHILD((*pnode))) {
		return FALSE;
	} else {
		*pnode = *((*pnode)->child);
		return TRUE;
	}
}

unsigned short int move_left(Pnode** pnode)
{
	if ((*pnode)->left == NULL) {
		return FALSE;
	} else {
		*pnode = (*pnode)->left;
		return TRUE;
	}
}

unsigned short int move_up(Pnode** pnode)
{
	if ((*pnode)->parent == NULL) {
		return FALSE;
	} else {
		*pnode = (*pnode)->parent;
		return TRUE;
	}
}

/* move to the right-most node in the current subtree */
void move_rightmost(Pnode** pnode)
{
	while (move_right(pnode));
}

/* --- graph creation ------------------------------------------------------- */

void init_pgraph(Pnode** root)
{
	gflags = GFLAG_NONE;
	*root = (Pnode*) malloc(sizeof(struct Pnode));

	(*root)->parent = 
		(*root)->left = (*root)->prev_const = NULL;
	(*root)->child = (*root)->right = NULL;
	(*root)->symbol = NULL;
	(*root)->flags = NFLAG_FRST | NFLAG_ASMP | NFLAG_TRUE;
	(*root)->var = NULL;

	TIKZ(fprintf(tikz, TIKZ_STARTNODE);
	rightmost_child = 0;
	cur_depth = 1;
	max_depth = 1;)

#ifdef DNUM
	(*root)->num = n; /* DEBUG */
	n++;
#endif
}

void create_child(Pnode* pnode)
{
	Pnode* child;

	pnode->child = (Pnode**) malloc(sizeof(struct Pnode*));
	*(pnode->child) = (Pnode*) malloc(sizeof(struct Pnode));

	child = *(pnode->child);
	child->left = NULL;
	child->child = child->right = NULL;
	child->parent = pnode;
	child->var = NULL;
	child->symbol = NULL;

	/* a newly created child will always be the first (i.e. left-most)
	 * in the current subtree */
	child->flags = NFLAG_FRST | NFLAG_TRUE;

	/* the assumption status of a node "trickles down" */
	if (HAS_NFLAG_ASMP(pnode) || !HAS_FFLAGS(pnode)) {
		SET_NFLAG_ASMP(child)
		SET_NFLAG_LOCK(child)
	}
	child->prev_const = pnode->prev_const;

	TIKZ(fprintf(tikz, TIKZ_CHILDNODE(pnode->num, n));
	fprintf(tikz, TIKZ_CHILDARROW(pnode->num, n));
	cur_depth++;
	if (cur_depth > max_depth) {
		max_depth = cur_depth;
	})

#ifdef DNUM
	child->num = n; /* DEBUG: pre-order numbering of the nodes */
	n++;
#endif
}

void create_right(Pnode* pnode)
{
	Pnode* right;

	pnode->right = (Pnode**) malloc(sizeof(struct Pnode*));
	*(pnode->right) = (Pnode*) malloc(sizeof(struct Pnode));

	right = *(pnode->right);
	right->parent = NULL;
	right->child = right->right = NULL;
	right->left = pnode;
	right->symbol = NULL;
	right->var = NULL;

	/* flags are carried over to the right hand side */
	right->flags = pnode->flags | NFLAG_TRUE;
	UNSET_NFLAG_NEWC(right)
	UNSET_NFLAG_FRST(right) /* TODO this can be done better */
	if (!HAS_FFLAGS(pnode)) {
		SET_NFLAG_FRST(pnode) /* FRST flag is not set for identifiers (i.e. when
								 no right node is created, but it is not needed
								 for them anyway. */
	}

	/* If the current node has no variable children and is no formulator,
	 * let it be the "previous constant" for the next node (linked list). */
	if (pnode->var == NULL && !HAS_SYMBOL(pnode)) {
		/* This will result in duplicates, but we are lazy.
		 * It also enables us to "hint" the software, which substitutions
		 * to do first. */
		right->prev_const = pnode;
	} else {
		right->prev_const = pnode->prev_const;
	}

	/* All nodes in a subtree before the first implication formulator carry the
	 * ASMP flag. That is because all statements before that formulator are
	 * assumptions, when the formula is an implication, and because the ASMP
	 * flag is ignored if the formula is no implication. */
	if (!HAS_NFLAG_IMPL(pnode)) {
		SET_NFLAG_ASMP(pnode)
	}

	/* All nodes in equalities and ordinary formulae are assumptions. */
	if (HAS_NFLAG_EQTY(pnode) || HAS_NFLAG_FMLA(pnode)) {
		SET_NFLAG_ASMP(right)
	}

	/* All nodes are assumptions in "locked" subtrees. */
	if (HAS_NFLAG_LOCK(pnode)) {
		SET_NFLAG_ASMP(right)
		SET_NFLAG_LOCK(right)
	}

	TIKZ(if (rightmost_child != 0) {
		fprintf(tikz, TIKZ_RIGHTTOPNODE(pnode->num, n, rightmost_child));
		rightmost_child = 0;
	} else {
		fprintf(tikz, TIKZ_RIGHTNODE(pnode->num, n));
	}
	fprintf(tikz, TIKZ_RIGHTARROW(pnode->num, n));)

#ifdef DNUM
	right->num = n; /* DEBUG */
	n++;
#endif
}

/* move leftwards through the subtree and create a linked list of all
 * identifiers, which are variables at the parent level; then move up
 * to the parent level */
unsigned short int move_and_sum_up(Pnode** pnode)
{
	Variable* var;
	Variable* oldvar;
	
	unsigned short int success;

	success = TRUE;

	/* only update rightmost child, if a new right node was created before */
	TIKZ(if (rightmost_child == 0) {
		rightmost_child = (*pnode)->num;
	}
	cur_depth--;)

	oldvar = (*pnode)->var;
	var = oldvar;

	/* carry flags over from right to left in order to be able to
	 * determine the type of a formula, when reading the first statement,
	 * when traversing the graph at a later stage */
	/* TODO: make this a do-while loop; like this it looks dodgy */
#define CARRY_OVER \
	if (HAS_NFLAG_NEWC((*pnode))) {\
		if (!HAS_NFLAG_IMPL((*pnode))) success = FALSE;\
		var = (Variable*) malloc(sizeof(Variable));\
		var->pnode = *((*pnode)->child);\
		var->next = oldvar;\
		oldvar = var;\
		DBG_GRAPH(fprintf(stderr, "/VAR\\");)\
	} else if ((*pnode)->var != NULL){\
		var = (Variable*) malloc(sizeof(Variable));\
		var->pnode = (*pnode)->var->pnode;\
		var->next = (*pnode)->var->next;\
		oldvar = var;\
	}

	CARRY_OVER

	while (move_left(pnode)) {
		(*pnode)->flags |= GET_NFFLAGS((*((*pnode)->right)));
		CARRY_OVER
	}

	if ((*pnode)->parent == NULL) {
		//return FALSE;
		return success;
	} else {
		*pnode = (*pnode)->parent;
		(*pnode)->var = var;
		DBG_GRAPH(
			fprintf(stderr, "/%d:", (*pnode)->num);
			for (;var != NULL; var = var->next)
				fprintf(stderr, "%s,", *(var->pnode->symbol));
			fprintf(stderr, "\\");
		)
		//return TRUE;
		return success;
	}
}

/* set the symbol field of a node (i.e. when encountering an identifier or
 * a formulator) */
void set_symbol(Pnode* pnode, char* symbol)
{
	pnode->symbol = (char**) malloc(sizeof(char*));
	*(pnode->symbol) = (char*) malloc(sizeof(char) * MAX_ID_LENGTH);
	strcpy(*(pnode->symbol), symbol);
}

/* --- debugging ------------------------------------------------------------ */

#ifdef DTIKZ
void print_flags(Pnode* pnode) {
	if (HAS_NFLAG_IMPL(pnode)) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR1 TIKZ_FLAG_B(pnode->num, 0));
	}
	if (HAS_NFLAG_EQTY(pnode)) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR2 TIKZ_FLAG_B(pnode->num, 1));
	}
	if (HAS_NFLAG_FMLA(pnode)) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR3 TIKZ_FLAG_B(pnode->num, 2));
	}
	if (HAS_NFLAG_ASMP(pnode)) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR4 TIKZ_FLAG_B(pnode->num, 3));
	}
	if (HAS_NFLAG_NEWC(pnode)) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR5 TIKZ_FLAG_B(pnode->num, 4));
	}
	if (HAS_NFLAG_LOCK(pnode)) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR6 TIKZ_FLAG_B(pnode->num, 5));
	}
	if (HAS_NFLAG_FRST(pnode)) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR7 TIKZ_FLAG_B(pnode->num, 6));
	}
	if (HAS_NFLAG_TRUE(pnode)) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR8 TIKZ_FLAG_B(pnode->num, 7));
	}
	if (pnode->var != NULL) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR9 TIKZ_FLAG_B(pnode->num, 8));
	}
}
#endif

/* --- memory deallocation -------------------------------------------------- */

void free_graph(Pnode* pnode)
{
	/* function would be able to start at the root,
	 * but since the graph creation finishes at the bottom rightmost node,
	 * we can start there */

	TIKZ(fprintf(tikz, TIKZ_SYMSCOPE(max_depth));)

	while (pnode->left != NULL || pnode->parent != NULL
			|| HAS_CHILD(pnode)) {
		while (HAS_RIGHT(pnode) || HAS_CHILD(pnode)) {
			move_down(&pnode);
			move_rightmost(&pnode);
		}

		TIKZ(print_flags(pnode);
		if (HAS_SYMBOL(pnode)) {
			fprintf(tikz, TIKZ_SYMNODE(pnode->num, *(pnode->symbol)));	
			fprintf(tikz, TIKZ_SYMARROW(pnode->num));
		})

		if (pnode->parent != NULL && HAS_NFLAG_NEWC(pnode->parent)) {
			free(*(pnode->symbol));
			free(pnode->symbol);

			free(pnode->child);
			free(pnode->right);
		}

		//TODO: test for mem leaks
		if (pnode->var != NULL) {
			free(pnode->var);
		}

		if (pnode->left != NULL) {
			move_left(&pnode);
			free(*(pnode->right));
			free(pnode->right);
			pnode->right = NULL;
		} else if (pnode->parent != NULL) {
			move_up(&pnode);
			free(*(pnode->child));
			free(pnode->child);
			pnode->child = NULL;
		}
	}

	TIKZ(print_flags(pnode);)
	if (pnode->var != NULL) {
		free(pnode->var);
	}

	free(pnode);

	TIKZ(fprintf(tikz, TIKZ_ENDSCOPE);)
}
