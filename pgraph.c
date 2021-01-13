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
#ifdef DPARSER
static short int n = 0;		/* node counter */
#endif

/* --- navigation through graph --------------------------------------------- */

unsigned short int move_right(Pnode** pnode)
{
	if ((*pnode)->right == NULL) {
		return FALSE;
	} else {
		*pnode = (*pnode)->right;
		return TRUE;
	}
}

unsigned short int move_down(Pnode** pnode)
{
	if ((*pnode)->child == NULL) {
		return FALSE;
	} else {
		*pnode = (*pnode)->child;
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

	(*root)->parent = (*root)->child
		= (*root)->left = (*root)->right = (*root)->prev_const = NULL;
	(*root)->symbol = NULL;
	(*root)->flags = NFLAG_FRST;
	(*root)->var = NULL;

	TIKZ(fprintf(tikz, "\\node[draw] (0) at (0pt,0pt) {$0$};\n");
	rightmost_child = 0;
	cur_depth = 1;
	max_depth = 1;)

	(*root)->num = n; /* DEBUG */
	n++;
}

void create_child(Pnode* pnode)
{
	Pnode* child;

	pnode->child = (Pnode*) malloc(sizeof(struct Pnode));

	child = pnode->child;
	child->child = child->left = child->right = NULL;
	child->parent = pnode;
	child->var = NULL;
	child->symbol = NULL;

	/* a newly created child will always be the first (i.e. left-most)
	 * in the current subtree */
	child->flags = NFLAG_FRST;

	/* the assumption status of a node "trickles down" */
	if (HAS_NFLAG_ASMP(pnode) || !HAS_FFLAGS(pnode)) {
		SET_NFLAG_ASMP(child)
		SET_NFLAG_LOCK(child)
	}
	child->prev_const = pnode->prev_const;

	TIKZ(fprintf(tikz, "\\node[draw, below = 40pt of %d]"
			"(%d) {$%d$};\n", pnode->num, n, n);
	fprintf(tikz, "\\draw (%d.south) -- (%d.north);\n", pnode->num, n);
	cur_depth++;
	if (cur_depth > max_depth) {
		max_depth = cur_depth;
	})

	child->num = n; /* DEBUG: pre-order numbering of the nodes */
	n++;
}

void create_right(Pnode* pnode)
{
	Pnode* right;

	pnode->right = (Pnode*) malloc(sizeof(struct Pnode));

	right = pnode->right;
	right->child = right->parent = right->right = NULL;
	right->left = pnode;
	right->symbol = NULL;
	right->var = NULL;

	/* flags are carried over to the right hand side */
	right->flags = pnode->flags;
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
		fprintf(tikz, "\\node[draw, right = " TIKZ_HSPACE "pt] "
				"(%d) at (%d -| %d.east) {\\textrm{%d}};\n",
				n, pnode->num, rightmost_child, n);
		rightmost_child = 0;
	} else {
		fprintf(tikz, "\\node[draw, right = " TIKZ_HSPACE
				"pt of %d] (%d)  {$%d$};\n",
				pnode->num, n, n);
	}
	fprintf(tikz, "\\draw (%d.east) -- (%d.west);\n",
			pnode->num, n);)

	right->num = n; /* DEBUG */
	n++;
}

/* move leftwards through the subtree and create a linked list of all
 * identifiers, which are variables at the parent level; then move up
 * to the parent level */
unsigned short int move_and_sum_up(Pnode** pnode)
{
	Variable* var;
	Variable* oldvar;

	/* TIKZ: 
	 * only update rightmost child, if a new right node was created before */
	TIKZ(if (rightmost_child == 0) {
		rightmost_child = (*pnode)->num;
	}
	cur_depth--;)

	oldvar = (*pnode)->var;
	var = oldvar;
	if (HAS_NFLAG_NEWC((*pnode))) {
		var = (Variable*) malloc(sizeof(Variable));
		var->pnode = (*pnode)->child;
		var->next = oldvar;
		oldvar = var;
	}

	while (move_left(pnode)) {
		/* carry flags over from right to left in order to be able to
		 * determine the type of a formula, when reading the first statement,
		 * when traversing the graph at a later stage */
		(*pnode)->flags |= GET_NFFLAGS((*pnode)->right);

		if (HAS_NFLAG_NEWC((*pnode))) {
			var = (Variable*) malloc(sizeof(Variable));
			var->pnode = (*pnode)->child;
			var->next = oldvar;
			oldvar = var;
		}
	}

	if ((*pnode)->parent == NULL) {
		return FALSE;
	} else {
		*pnode = (*pnode)->parent;
		(*pnode)->var = var;
		return TRUE;
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
#ifdef DPARSER

void print_node_info(Pnode* pnode)
{
	Variable* var;

	printf("\n\n");
	printf("Node %d:\n", pnode->num);
	if (pnode->symbol != NULL) {
		printf("\tSymbol: %s\n", *(pnode->symbol));
	}
	printf("\tNFlags:");
	if (HAS_NFLAG_IMPL(pnode)) {
		printf(" IMPL");
	}
	if (HAS_NFLAG_EQTY(pnode)) {
		printf(" EQTY");
	}
	if (HAS_NFLAG_FMLA(pnode)) {
		printf(" FMLA");
	}
	if (HAS_NFLAG_ASMP(pnode)) {
		printf(" ASMP");
	}
	if (HAS_NFLAG_NEWC(pnode)) {
		printf(" NEWC");
	}
	if (HAS_NFLAG_LOCK(pnode)) {
		printf(" LOCK");
	}
	if (HAS_NFLAG_FRST(pnode)) {
		printf(" FRST");
	}
}

void print_flags(Pnode* pnode) {
	if (HAS_NFLAG_IMPL(pnode)) {
		fprintf(tikz, "\\draw[-{Triangle[length=3pt,width=3pt]}, "
				"color=darkmagenta] "
				"([yshift=9pt] %d.east) to "
				"([yshift=9pt, xshift=3pt] %d.east);\n ",
				pnode->num, pnode->num);
	}
	if (HAS_NFLAG_EQTY(pnode)) {
		fprintf(tikz, "\\draw[-{Triangle[length=3pt,width=3pt]}, "
				"color=darkorange] "
				"([yshift=6pt] %d.east) to "
				"([yshift=6pt, xshift=3pt] %d.east);\n ",
				pnode->num, pnode->num);
	}
	if (HAS_NFLAG_FMLA(pnode)) {
		fprintf(tikz, "\\draw[-{Triangle[length=3pt,width=3pt]}, "
				"color=darkpastelgreen] "
				"([yshift=3pt] %d.east) to "
				"([yshift=3pt, xshift=3pt] %d.east);\n ",
				pnode->num, pnode->num);
	}
	if (HAS_NFLAG_ASMP(pnode)) {
		fprintf(tikz, "\\draw[-{Triangle[length=3pt,width=3pt]}, "
				"color=oucrimsonred] "
				"([yshift=0pt] %d.east) to "
				"([yshift=0pt, xshift=3pt] %d.east);\n ",
				pnode->num, pnode->num);
	}
	if (HAS_NFLAG_NEWC(pnode)) {
		fprintf(tikz, "\\draw[-{Triangle[length=3pt,width=3pt]}, "
				"color=darkpowderblue] "
				"([yshift=-3pt] %d.east) to "
				"([yshift=-3pt, xshift=3pt] %d.east);\n ",
				pnode->num, pnode->num);
	}
	if (HAS_NFLAG_LOCK(pnode)) {
		fprintf(tikz, "\\draw[-{Triangle[length=3pt,width=3pt]}, "
				"color=mediumspringgreen] "
				"([yshift=-6pt] %d.east) to "
				"([yshift=-6pt, xshift=3pt] %d.east);\n ",
				pnode->num, pnode->num);
	}
	if (HAS_NFLAG_FRST(pnode)) {
		fprintf(tikz, "\\draw[-{Triangle[length=3pt,width=3pt]}, "
				"color=deepskyblue] "
				"([yshift=-9pt] %d.east) to "
				"([yshift=-9pt, xshift=3pt] %d.east);\n ",
				pnode->num, pnode->num);
	}
}
#endif

/* --- memory deallocation -------------------------------------------------- */

void free_graph(Pnode* pnode)
{
	/* function would be able to start at the root,
	 * but since the graph creation finishes at the bottom rightmost node,
	 * we can start there */

	TIKZ(fprintf(tikz, "\\begin{scope}["
		"every node/.style={rectangle,inner sep=3pt,minimum width=3pt, minimum height=21pt, text height=5pt,yshift=0pt}, "
		"-]\n"
		"\\node (symalign) at (0pt,-%dpt) {};\n", 61 * max_depth);)

	while (pnode->left != NULL || pnode->parent != NULL
			|| pnode->child !=NULL) {
		while (pnode->right !=NULL || pnode->child !=NULL) {
			move_down(&pnode);
			move_rightmost(&pnode);
		}

		//print_node_info(pnode);

		TIKZ(print_flags(pnode);
		if (HAS_SYMBOL(pnode)) {
			fprintf(tikz, "\\node[draw] (s%d) at (symalign -| %d) {$%s$};\n",
					pnode->num, pnode->num, *(pnode->symbol));	
			fprintf(tikz, "\\draw[thin, dash dot, "
					"color=gray] "
					"(%d.south) -- (s%d.north);\n",
					pnode->num, pnode->num);
		})

		if (pnode->parent != NULL && HAS_NFLAG_NEWC(pnode->parent)) {
			free(*(pnode->symbol));
			free(pnode->symbol);
		}

		if (pnode->var != NULL) {
			free(pnode->var);
		}

		if (pnode->left != NULL) {
			move_left(&pnode);
			free(pnode->right);
			pnode->right = NULL;
		} else if (pnode->parent != NULL) {
			move_up(&pnode);
			free(pnode->child);
			pnode->child = NULL;
		}
	}

	//print_node_info(pnode);

	if (pnode->var != NULL) {
		free(pnode->var);
	}

	free(pnode);

	TIKZ(fprintf(tikz, "\\end{scope}\n");)
}
