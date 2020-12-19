#include "pgraph.h"
#include "token.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

void init_pgraph(Pnode** root)
{
	*root = (Pnode*) malloc(sizeof(struct Pnode));

	(*root)->parent = (*root)->child
		= (*root)->left = (*root)->right = (*root)->prev_const = NULL;
	(*root)->symbol = NULL;
	(*root)->flags = NFLAG_NONE;
	(*root)->varcount = 0;
	(*root)->var = NULL;
}

void create_child(Pnode* pnode)
{
	Pnode* child;

	pnode->child = (Pnode*) malloc(sizeof(struct Pnode));

	child = pnode->child;
	child->child = child->left = child->right = NULL;
	child->parent = pnode;
	child->varcount = 0;
	child->var = NULL;
	child->symbol = NULL;
	if (HAS_FLAG_ASMP(pnode) || !HAS_FFLAGS(pnode)) {
		SET_NFLAG_ASMP(child)
		SET_NFLAG_LOCK(child)
	} else {
		child->flags = NFLAG_NONE;
	}
	child->prev_const = pnode->prev_const;
}

void create_right(Pnode* pnode)
{
	Pnode* right;

	pnode->right = (Pnode*) malloc(sizeof(struct Pnode));

	right = pnode->right;
	right->child = right->parent = right->right = NULL;
	right->left = pnode;
	right->symbol = NULL;
	right->varcount = 0;
	right->var = NULL;

	right->flags = pnode->flags;
	UNSET_NFLAG_NEWC(right)
	if (HAS_FLAG_NEWC(pnode)) {
		//SET_NFLAG_ASMP(pnode)
		right->prev_const = pnode;
	} else {
		//if (!HAS_FFLAGS(pnode)) {
		//if (!HAS_FLAG_IMPL(pnode)) {
		//	SET_NFLAG_ASMP(pnode)
		//}
		right->prev_const = pnode->prev_const;
	}

	if (!HAS_FLAG_IMPL(pnode)) {
		SET_NFLAG_ASMP(pnode)
	}
	if (HAS_FLAG_EQTY(pnode) || HAS_FLAG_FMLA(pnode)) {
		SET_NFLAG_ASMP(right)
	}
	if (HAS_FLAG_LOCK(pnode)) {
		SET_NFLAG_ASMP(right)
		SET_NFLAG_LOCK(right)
	}
}

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

void move_rightmost(Pnode** pnode)
{
	while (move_right(pnode));
}

unsigned short int move_and_sum_up(Pnode** pnode)
{
	int cumulative_vc; /* cumulative variable count of adjacent nodes */
	int impl;
	Variable* var;
	Variable* oldvar;

	cumulative_vc = (*pnode)->varcount;
	oldvar = (*pnode)->var;
	var = oldvar;
	if (HAS_FLAG_NEWC((*pnode))) {
		cumulative_vc++;
		var = (Variable*) malloc(sizeof(Variable));
		var->pnode = (*pnode);
		var->next = oldvar;
		oldvar = var;
	}

	while (move_left(pnode)) {
		cumulative_vc += (*pnode)->varcount;
		(*pnode)->flags |= GET_NFFLAGS((*pnode)->right);

		if (HAS_FLAG_NEWC((*pnode))) {
			cumulative_vc++;
			var = (Variable*) malloc(sizeof(Variable));
			var->pnode = (*pnode);
			var->next = oldvar;
			oldvar = var;
		}
	}

	if ((*pnode)->parent == NULL) {
		return FALSE;
	} else {
		*pnode = (*pnode)->parent;
		(*pnode)->varcount = cumulative_vc;
		(*pnode)->var = var;
		return TRUE;
	}
}

void set_symbol(Pnode* pnode, char* symbol)
{
	pnode->symbol = (char**) malloc(sizeof(char*));
	*(pnode->symbol) = (char*) malloc(sizeof(char) * MAX_ID_LENGTH);
	strcpy(*(pnode->symbol), symbol);
}


void init_reachable(Pnode* pnode)
{
	reachable = pnode;
}

void next_reachable()
{
}

void print_node_info(Pnode* pnode, unsigned short int ncounter)
{
	Variable* var;

	printf("\n\n");
	printf("Node %d:\n", ncounter);
	if (pnode->symbol != NULL) {
		printf("\tSymbol: %s\n", *(pnode->symbol));
	}
	printf("\tVarcount: %d\n", pnode->varcount);
	printf("\tNFlags:");
	if (HAS_FLAG_IMPL(pnode)) {
		printf(" IMPL");
	}
	if (HAS_FLAG_EQTY(pnode)) {
		printf(" EQTY");
	}
	if (HAS_FLAG_FMLA(pnode)) {
		printf(" FMLA");
	}
	if (HAS_FLAG_ASMP(pnode)) {
		printf(" ASMP");
	}
	if (HAS_FLAG_NEWC(pnode)) {
		printf(" NEWC");
	}
	if (HAS_FLAG_LOCK(pnode)) {
		printf(" LOCK");
	}
}

void free_graph(Pnode* pnode)
{
	/* function should be able to start at root,
	 * but since the graph creation finishes at the bottom rightmost node,
	 * we can start there */
	unsigned short int ncounter = 0;

	while (pnode->left != NULL || pnode->parent != NULL
			|| pnode->child !=NULL) {
		while (pnode->right !=NULL || pnode->child !=NULL) {
			move_down(&pnode);
			move_rightmost(&pnode);
		}

		print_node_info(pnode, ncounter);

		if (pnode->parent != NULL && HAS_FLAG_NEWC(pnode->parent)) {
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
		ncounter++;
	}
	print_node_info(pnode, ncounter);
	if (pnode->var != NULL) {
		free(pnode->var);
	}
	free(pnode);
}
