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

	(*root)->parent = (*root)->child = (*root)->left = (*root)->right = NULL;
	(*root)->symbol = NULL;
	(*root)->flags = 0;
	(*root)->varcount = 0;
}

void create_child(Pnode* pnode)
{
	Pnode* child;

	pnode->child = (Pnode*) malloc(sizeof(struct Pnode));

	child = pnode->child;
	child->child = child->left = child->right = NULL;
	child->parent = pnode;
	child->symbol = NULL;
	child->flags = FLAG_ASMP;
	child->varcount = 0;
}

void create_right(Pnode* pnode)
{
	Pnode* right;

	pnode->right = (Pnode*) malloc(sizeof(struct Pnode));

	right = pnode->right;
	right->child = right->parent = right->right = NULL;
	right->left = pnode;
	right->symbol = NULL;
	right->flags = pnode->flags;
	right->varcount = 0;
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
	while (move_right(pnode)) {}
}

unsigned short int move_and_sum_up(Pnode** pnode)
{
	int cumulative_vc; /* cumulative variable count of adjacent nodes */
	
	cumulative_vc = (*pnode)->varcount;
	if (IS_CONST_NODE((*pnode))) {
		cumulative_vc++;
	}

	while (move_left(pnode)) {
		cumulative_vc += (*pnode)->varcount;
		if (IS_CONST_NODE((*pnode))) {
			cumulative_vc++;
		}
	}

	if ((*pnode)->parent == NULL) {
		return FALSE;
	} else {
		*pnode = (*pnode)->parent;
		(*pnode)->varcount = cumulative_vc;
		return TRUE;
	}
}

void set_symbol(Pnode* pnode, char* symbol)
{
	pnode->symbol = (char*) malloc(sizeof(char) * MAX_ID_LENGTH);
	strcpy(pnode->symbol, symbol);
}

void print_node_info(Pnode* pnode, unsigned short int ncounter)
{
	printf("\n\n");
	printf("Node %d:\n", ncounter);
	if (pnode->symbol != NULL) {
		printf("\tSymbol: %s\n", pnode->symbol);
	}
	printf("\tFlags:");
	if (HAS_FLAG_IMPL(pnode)) {
		printf(" IMPL");
	}
	if (HAS_FLAG_EQTY(pnode)) {
		printf(" EQTY");
	}
	if (HAS_FLAG_FMTR(pnode)) {
		printf(" FMTR");
	}
	if (HAS_FLAG_ASMP(pnode)) {
		printf(" ASMP");
	}
}

void free_graph(Pnode* pnode)
{
	/* function should be able to start at root,
	 * but since the graph creation finishes at the bottom rightmost node,
	 * we can start there */
	unsigned short int ncounter = 0;

	while (pnode->left != NULL || pnode->parent != NULL) {
		while (pnode->right !=NULL || pnode->child !=NULL) {
			move_down(&pnode);
			move_rightmost(&pnode);
		}

		/* print_node_info(pnode, ncounter); */	

		if (pnode->symbol != NULL) {
			free(pnode->symbol);
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
}
