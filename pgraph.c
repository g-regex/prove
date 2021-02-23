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

/* --- preprocessor directives -------------------------------------------{{{ */
#define TRUE 1
#define FALSE 0

#define DO(x) ((x) || TRUE)

static short int n = 0;		/* node counter */

/* }}} */

/* --- navigation through graph ------------------------------------------{{{ */

/*{{{*/
/**
 * @brief Moves Pnode pointer to the right.
 *
 * @param pnode Pnode pointer to be moved
 *
 * @return TRUE on success
 */
unsigned short int move_right(Pnode** pnode)
{
	if (!HAS_RIGHT((*pnode))) {
		return FALSE;
	} else {
		*pnode = *((*pnode)->right);
		return TRUE;
	}
}/*}}}*/

/*{{{*/
/**
 * @brief Moves Pnode pointer down.
 *
 * @param pnode Pnode pointer to be moved
 *
 * @return TRUE on success
 */
unsigned short int move_down(Pnode** pnode)
{
	if (!HAS_CHILD((*pnode))) {
		return FALSE;
	} else {
		*pnode = *((*pnode)->child);
		return TRUE;
	}
}/*}}}*/

/*{{{*/
/**
 * @brief Moves Pnode pointer to the left.
 *
 * @param pnode Pnode pointer to be moved
 *
 * @return TRUE on success
 */
unsigned short int move_left(Pnode** pnode)
{
	if ((*pnode)->left == NULL) {
		return FALSE;
	} else {
		*pnode = (*pnode)->left;
		return TRUE;
	}
}/*}}}*/

/*{{{*/
/**
 * @brief Moves Pnode pointer up.
 *
 * @param pnode Pnode pointer to be moved
 *
 * @return TRUE on success
 */
unsigned short int move_up(Pnode** pnode)
{
	if ((*pnode)->parent == NULL) {
		return FALSE;
	} else {
		*pnode = (*pnode)->parent;
		return TRUE;
	}
}/*}}}*/

/* move to the right-most node in the current subtree */
/*{{{*/
/**
 * @brief Move Pnode pointer to the right-most node of current level.
 *
 * @param pnode Pnode pointer to be moved
 */
void move_rightmost(Pnode** pnode)
{
	while (move_right(pnode));
}/*}}}*/

/* }}} */

/* --- graph creation ----------------------------------------------------{{{ */
/*{{{*/
/**
 * @brief Initialises a graph of Pnodes.
 *
 * @param root double pointer for returning address of alloced memory for root
 */
void init_pgraph(Pnode** root)
{
	gflags = GFLAG_NONE;
	*root = (Pnode*) malloc(sizeof(struct Pnode));

	(*root)->parent = //(*root)->above =
		(*root)->left = (*root)->prev_const = (*root)->prev_id = NULL;
	(*root)->child = (*root)->right = NULL;
	(*root)->symbol = NULL;
	(*root)->flags = NFLAG_FRST | NFLAG_TRUE;
	(*root)->vtree = NULL;
	
	TIKZ(fprintf(tikz, TIKZ_STARTNODE);
	rightmost_child = 0;
	cur_depth = 1;
	max_depth = 1;)

//#ifdef DNUM
	(*root)->num = n;
	(*root)->num_c = n;
	n++;
//#endif
}/*}}}*/
/*{{{*/
/**
 * @brief Creates a child for the current node.
 *
 * @param pnode pointer to the current node
 */
void create_child(Pnode* pnode)
{
	Pnode* child;

	pnode->child = (Pnode**) malloc(sizeof(struct Pnode*));
	*(pnode->child) = (Pnode*) malloc(sizeof(struct Pnode));

	child = *(pnode->child);
	child->left = NULL;
	child->child = child->right = NULL;
	child->parent = pnode;
	child->vtree = NULL;
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
	child->prev_id = pnode->prev_id;

	TIKZ(fprintf(tikz, TIKZ_CHILDNODE(pnode->num, n));
	fprintf(tikz, TIKZ_CHILDARROW(pnode->num, n));
	cur_depth++;
	if (cur_depth > max_depth) {
		max_depth = cur_depth;
	})

	child->num = n;
	child->num_c = n;
	n++;
}/*}}}*/
/*{{{*/
/**
 * @brief Creates a node to the right of the current node.
 *
 * @param pnode pointer to the current node
 */
void create_right(Pnode* pnode)
{
	Pnode* right;

	pnode->right = (Pnode**) malloc(sizeof(struct Pnode*));
	*(pnode->right) = (Pnode*) malloc(sizeof(struct Pnode));

	right = *(pnode->right);
	//right->above = pnode->above;
	right->parent = NULL;
	right->child = right->right = NULL;
	right->left = pnode;
	right->symbol = NULL;
	right->vtree = NULL;

	/*
	right->equalto = (Variable**) malloc(sizeof(Variable*));
	*(right->equalto) = (Variable*) malloc(sizeof(Variable));
	(*(right->equalto))->pnode = right;
	(*(right->equalto))->next = *(right->equalto);
	*/

	/* flags are carried over to the right hand side */
	right->flags = pnode->flags; /* | NFLAG_TRUE; */
	UNSET_NFLAG_NEWC(right)
	UNSET_NFLAG_FRST(right) /* TODO this can be done better */
	if (!HAS_FFLAGS(pnode)) {
		SET_NFLAG_FRST(pnode) /* FRST flag is not set for identifiers (i.e. when
								 no right node is created, but it is not needed
								 for them anyway. */
	}

	/* If the current node has no variable children and is no formulator,
	 * let it be the "previous constant" for the next node (linked list). */
	if (pnode->vtree == NULL && !HAS_SYMBOL(pnode)) {
		/* This will result in duplicates, but we are lazy.
		 * It also enables us to "hint" the software, which substitutions
		 * to do first. */
		right->prev_const = pnode;
	} else {
		right->prev_const = pnode->prev_const;
	}

	if (HAS_NFLAG_NEWC(pnode)) {
		/* This will result in duplicates, but we are lazy.
		 * It also enables us to "hint" the software, which substitutions
		 * to do first. */
		right->prev_id = pnode;
	} else {
		right->prev_id = pnode->prev_id;
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

//#ifdef DNUM
	right->num = n; /* DEBUG: pre-order numbering of the nodes */
	right->num_c = n;
	n++;
//#endif
}/*}}}*/
/*{{{*/
/**
 * @brief Creates a dummy node (i.e. temporary ) to the right of the current
 * node to provide a substitution perspective for existence verification.
 *
 * @param pnode pointer to current node
 */
void create_right_dummy(Pnode* pnode)
{
	Pnode* right;

	pnode->right = (Pnode**) malloc(sizeof(struct Pnode*));
	*(pnode->right) = (Pnode*) malloc(sizeof(struct Pnode));

	right = *(pnode->right);
	right->parent = NULL;
	right->child = right->right = NULL;
	right->left = pnode;
	right->symbol = NULL;
	right->vtree = NULL;

	/* flags are carried over to the right hand side */
	right->flags = pnode->flags; /* | NFLAG_TRUE; */
	UNSET_NFLAG_NEWC(right)
	UNSET_NFLAG_FRST(right) /* TODO this can be done better */

	/* If the current node has no variable children and is no formulator,
	 * let it be the "previous constant" for the next node (linked list). */
	if (pnode->vtree == NULL && !HAS_SYMBOL(pnode)) {
		/* This will result in duplicates, but we are lazy.
		 * It also enables us to "hint" the software, which substitutions
		 * to do first. */
		right->prev_const = pnode;
	} else {
		right->prev_const = pnode->prev_const;
	}

	right->prev_id = pnode->prev_id;

	/* All nodes are assumptions in "locked" subtrees. */
	if (HAS_NFLAG_LOCK(pnode)) {
		SET_NFLAG_ASMP(right)
		SET_NFLAG_LOCK(right)
	}

	right->num = -1;
	right->num_c = -1;
}/*}}}*/
/*{{{*/
/**
 * @brief Frees the temporary dummy Pnode.
 *
 * @param pnode pointer to dummy Pnode
 */
void free_right_dummy(Pnode* pnode)
{
	free(*(pnode->right));
	free(pnode->right);
	pnode->right = NULL;
}/*}}}*/
/*{{{*/
/**
 * @brief Moves leftwards through the sub-tree, copying over formulator NFLAGS
 * from right to left and creating a VTree for quick retrieval of variables;
 * then moves up to the parent level.
 *
 * @param pnode pointer to current Pnode
 */
void move_and_sum_up(Pnode** pnode)
{
	VTree* vtree;
	VTree* oldvtree;
	
	/* only update rightmost child, if a new right node was created before */
	TIKZ(
	if (rightmost_child == 0) {
		rightmost_child = (*pnode)->num;
	}
	cur_depth--;
	)

	oldvtree =  (*pnode)->vtree;
	vtree = oldvtree;

	/* carry flags over from right to left in order to be able to
	 * determine the type of a formula, when reading the first statement,
	 * when traversing the graph at a later stage */

	do {
		if (!HAS_NFLAG_LOCK((*pnode)) && !HAS_NFLAG_IMPL((*pnode)) && 
				!HAS_NFLAG_EQTY((*pnode))) {
			UNSET_NFLAG_ASMP((*pnode))
		}

		if (HAS_NFLAG_NEWC((*pnode))) {
			vtree = (VTree*) malloc(sizeof(VTree));
			vtree->parent = NULL;
			vtree->pnode = *((*pnode)->child);
			vtree->right = oldvtree;
			vtree->left = NULL;
			if (oldvtree != NULL) {
				oldvtree->parent = vtree;
				SET_VARFLAG_RGHT(oldvtree->flags)
			}
			vtree->flags = VARFLAG_NONE;
			if (HAS_NFLAG_FRST((*pnode))) {
				SET_VARFLAG_FRST(vtree->flags)
			}
			oldvtree = vtree;
		} else if ((*pnode)->vtree != NULL){
			vtree = (VTree*) malloc(sizeof(VTree));
			vtree->parent = NULL;
			vtree->pnode = NULL;
			vtree->left = (*pnode)->vtree;
			SET_VARFLAG_LEFT((*pnode)->vtree->flags)
			vtree->right = oldvtree;
			if (oldvtree != NULL) {
				oldvtree->parent = vtree;
				SET_VARFLAG_RGHT(oldvtree->flags)
			}
			vtree->flags = VARFLAG_NONE;
			oldvtree = vtree;
		}
	} while (move_left(pnode) &&
			DO((*pnode)->flags |= GET_NFFLAGS((*((*pnode)->right)))));

	if ((*pnode)->parent != NULL) {
		*pnode = (*pnode)->parent;
		(*pnode)->vtree = vtree;
	}
}/*}}}*/
/*{{{*/
/**
 * @brief Sets the symbol field of a Pnode (i.e. when encountering an id or a
 * formualtor)
 *
 * @param pnode pointer to current Pnode
 * @param symbol symbol to be set
 */
void set_symbol(Pnode* pnode, char* symbol)
{
	pnode->symbol = (char**) malloc(sizeof(char*));
	*(pnode->symbol) = (char*) malloc(sizeof(char) * MAX_ID_LENGTH);
	strcpy(*(pnode->symbol), symbol);
}/*}}}*/
/*}}}*/

/* --- VTree creation ----------------------------------------------------{{{ */
/* ------ PREPROCESSOR DIRECTIVES for VTree ------------------------------{{{ */
#define VTREE_MOVE_UP \
	if (vtree->parent != NULL) {\
		vtree = vtree->parent;\
	}
#define VTREE_MOVE_LEFT \
	if (vtree->left != NULL) {\
		vtree = vtree->left;\
	}
/*}}}*/
/*{{{*/
/**
 * @brief Move to the right-most position in VTree to prepare for reverser
 * post-order traversal (optimal for substitution).
 *
 * @param vtree pointer to current position in VTree
 *
 * @return pointer to new position in VTree
 */
VTree* pos_in_vtree(VTree* vtree)
{
	if (vtree == NULL) {
		return NULL;
	}

	while (vtree->right != NULL) {
		vtree = vtree->right;
	}
	return vtree;
}/*}}}*/
/*{{{*/
/**
 * @brief Moves to next position in VTree reverse post-order traversal.
 *
 * @param vtree pointer to current position in VTree
 *
 * @return new position in VTree or NULL, when done
 */
VTree* next_var(VTree* vtree)
{
	if (vtree == NULL || vtree->parent == NULL) {
		return NULL;
	}

	if (HAS_VARFLAG_LEFT(vtree->flags)) {
		VTREE_MOVE_UP
		return vtree;
	} else {
		VTREE_MOVE_UP
		if (vtree->left != NULL) {
			vtree = vtree->left;
			while (vtree->right != NULL) {
				vtree = vtree->right;
			}
		}
	}

	return vtree;
}/*}}}*/
/*}}}*/

/* --- debugging ---------------------------------------------------------{{{ */
/*{{{*/
/**
 * @brief Returns current value of node counter.
 *
 * @return current value of node counter
 */
int get_node_count() {
	return n;
}/*}}}*/

#ifdef DTIKZ
/*{{{*/
/**
 * @brief Add flags to TIKZ graph; to be called when freeing the graph.
 *
 * @param pnode current Pnode
 */
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
		if (!HAS_NFLAG_IMPL(pnode)) {
			fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR_INACT
					TIKZ_FLAG_B(pnode->num, 6));
		} else {
			fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR7 TIKZ_FLAG_B(pnode->num, 6));
		}
	}
	if (pnode->vtree != NULL) {
		fprintf(tikz, TIKZ_FLAG_A TIKZ_COLOR9 TIKZ_FLAG_B(pnode->num, 8));
	}
}/*}}}*/
#endif
/* }}} */

/* --- memory deallocation -----------------------------------------------{{{ */
/*{{{*/
/**
 * @brief Frees a graph of Pnodes.
 *
 * @param pnode Pnode to start with (preferably the bottom right one)
 */
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
			}
		)

		if (pnode->parent != NULL && HAS_NFLAG_NEWC(pnode->parent)) {
			free(*(pnode->symbol));
			free(pnode->symbol);

			free(pnode->child);
			free(pnode->right);
		}

		/* TODO: free linked list of variables */
		/*if (pnode->var != NULL) {
			free(pnode->var);
		}*/

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

	/* TODO: free linked list of variables */
	/*if (pnode->var != NULL) {
		free(pnode->var);
	}*/

	free(pnode);

	TIKZ(fprintf(tikz, TIKZ_ENDSCOPE);)
}/*}}}*/
/* }}} */
