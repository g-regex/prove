#include "pgraph.h"
#include "token.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

static Pnode* known_id; /* current identifier in linked list, known from
						   stance of current pnode */
static char* subd_var; /* remembered variable to be substituted */

/* for branch exploration */
typedef struct branch_checkpoint { /* stack for jumping back to parent levels */
	Pnode* pnode;
	struct branch_checkpoint* above;
} BC;
static BC* bc = NULL;
static Pnode* eqfirst; /* temporarily holds the first node of an equality */
static Pnode* eqendwrap; /*temporarily holds node at which to stop wrapping*/

unsigned short int next_in_branch(Pnode* pnode);

void bc_push()
{
	BC* bctos; /* top of stack for branch checkpoint, if descending */
	bctos = (BC*) malloc(sizeof(BC));
	bctos->pnode = reachable;
	bctos->above = bc;
	bc = bctos;
	//printf("push:(%d)", reachable->num);
}

Pnode* bc_pop()
{
	BC* bcold;
	Pnode* pnode;

	bcold = bc;
	pnode = bc->pnode;
	bc = bc->above;
	free(bcold);
	//printf("pop:(%d)", pnode->num);
	return pnode;
}

void init_pgraph(Pnode** root)
{
	*root = (Pnode*) malloc(sizeof(struct Pnode));

	(*root)->parent = (*root)->child
		= (*root)->left = (*root)->right = (*root)->prev_const = NULL;
	(*root)->symbol = NULL;
	(*root)->flags = NFLAG_FRST;
	(*root)->varcount = 0;
	(*root)->var = NULL;

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
	child->varcount = 0;
	child->var = NULL;
	child->symbol = NULL;
	child->flags = NFLAG_FRST;
	if (HAS_FLAG_ASMP(pnode) || !HAS_FFLAGS(pnode)) {
		SET_NFLAG_ASMP(child)
		SET_NFLAG_LOCK(child)
	}
	child->prev_const = pnode->prev_const;

	child->num = n; /* DEBUG */
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
	right->varcount = 0;
	right->var = NULL;

	right->flags = pnode->flags;
	UNSET_NFLAG_NEWC(right)
	UNSET_NFLAG_FRST(right) /* TODO this can be done better */
	if (!HAS_FFLAGS(pnode)) { /*only works for formulae, but that's sufficient*/
		SET_NFLAG_FRST(pnode)
	}
	//if (HAS_FLAG_NEWC(pnode)) {
	if (pnode->var == NULL && !HAS_SYMBOL(pnode)) {
		/* will result in duplicates, but we are lazy */
		right->prev_const = pnode;
	} else {
		right->prev_const = pnode->prev_const;
	}

	if (!HAS_FLAG_IMPL(pnode)) { /* because ASMPs need to trickle down */
		SET_NFLAG_ASMP(pnode)
	}
	if (HAS_FLAG_EQTY(pnode) || HAS_FLAG_FMLA(pnode)) {
		SET_NFLAG_ASMP(right)
	}
	if (HAS_FLAG_LOCK(pnode)) {
		SET_NFLAG_ASMP(right)
		SET_NFLAG_LOCK(right)
	}

	right->num = n; /* DEBUG */
	n++;
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
		var->pnode = (*pnode)->child;
		var->next = oldvar;
		oldvar = var;
	}

	while (move_left(pnode)) {
		cumulative_vc += (*pnode)->varcount;
		(*pnode)->flags |= GET_NFFLAGS((*pnode)->right);

		if (HAS_FLAG_NEWC((*pnode))) {
			cumulative_vc++;
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

void init_known_id(Pnode* pnode)
{
	known_id = pnode;
}

unsigned short int next_known_id()
{ /* TODO skip duplicates */
	if (known_id != NULL) {
		known_id = known_id->prev_const;
	}
	while (known_id != NULL && !CONTAINS_ID(known_id)) {
		known_id = known_id->prev_const;
	}

	/* DEBUG */
	if (known_id != NULL) {
		//printf("{%s}", *(known_id->child->symbol));
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

unsigned short int const_equal(Pnode* p1, Pnode* p2)
{ /* must be given the top left node of the subtrees to compare */
	unsigned short int equal;

	equal = TRUE;
	/* TODO this can be done better */
	if (IS_ID(p1)) {
		//printf(";");
		if (IS_ID(p2)) {
			//printf("$");
			//if (*(p1->symbol) == *(p2->symbol)) printf("!%s;", *(p1->symbol));	
			return (*(p1->symbol) == *(p2->symbol));	
		} else {
			return FALSE;
		}
	} else if (HAS_SYMBOL(p1)) {
		if (HAS_SYMBOL(p2)) {
			//printf("$");
			//if (strcmp(*(p1->symbol), *(p2->symbol)) == 0) printf("X%s;", *(p1->symbol));	
			return (strcmp(*(p1->symbol), *(p2->symbol)) == 0);	
		} else {
			return FALSE;
		}
	}

	if (HAS_CHILD(p1)) {
		//printf("c");
		equal = HAS_CHILD(p2) ? const_equal(p1->child, p2->child) : FALSE;
	}
	if (HAS_RIGHT(p1)) {
		equal = equal &&
			(HAS_RIGHT(p2) ? const_equal(p1->right, p2->right) : FALSE);
	}

	return equal;
}

/* DEBUG */
unsigned short int rn()
{
	return reachable->num;
}

unsigned short int same_as_rchbl(Pnode* pnode)
{
	//printf("{%d}", reachable->num);
	if (!HAS_CHILD(pnode) && !HAS_CHILD(reachable)) {
		/* ERROR this should not happen!! */
		//printf("&");
		return FALSE;
	}
	return const_equal(pnode->child, reachable->child);
}

unsigned short int explore_branch()
{
	if (HAS_FLAG_IMPL(reachable->child) || HAS_FLAG_EQTY(reachable->child)) {
		bc_push();
		reachable = reachable->child;
		return TRUE;
	} else {
		return FALSE;
	}
}

unsigned short int exit_branch()
{
	while (bc->above != NULL) {
		bc_pop();
	}
	reachable = bc_pop();
	//printf("E");
}

unsigned short int check_asmp(Pnode* pnode)
{ /* checks assumption in 'reachable' from the perspective of pnode */
	Pnode* pconst;

	for (pconst = pnode->prev_const; pconst != NULL;
			pconst = pconst->prev_const) {
		//printf(":");
		//if (CONTAINS_ID(pconst)) printf("<%s>", *(pconst->child->symbol));
		//if (HAS_CHILD(pconst)) printf("<C>");
		//if (HAS_SYMBOL(pconst)) printf("V%sV", *(pconst->symbol));
		if (same_as_rchbl(pconst)) {
			//printf("^");
			return TRUE;
		}
	}
	return FALSE;
}

unsigned short int branch_proceed(Pnode* pnode)
{
	while (!move_right(&reachable)) {
		//move up
		if (bc->above == NULL) {
			return FALSE; // last pop is done by branch_exit
		} else {
			reachable = bc_pop();
		}

	}
	if (HAS_SYMBOL(reachable)) {
		return next_in_branch(pnode);
	} else {
		return TRUE;
	}
}

void wrap_right() {
	if (!move_right(&reachable)) {
		reachable = eqfirst;
	}
}

unsigned short int next_in_branch(Pnode* pnode)
{
	if (HAS_SYMBOL(reachable)) {
		if (!move_right(&reachable)) {
			/* FATAL ERROR, must not happen */
			return FALSE;
		} else {
			//printf("Z");
			//return next_in_branch(pnode);
			
			/*if (explore_branch()) {
				return next_in_branch(pnode);
			} else {
				return TRUE;
			}*/
			return TRUE;
		}
	}

	if (explore_branch()) {
		return next_in_branch(pnode);
	}

	if (HAS_FLAG_IMPL(reachable)) {
		if (HAS_FLAG_FRST(reachable)) {
			//printf("\\");
			if (!check_asmp(pnode)) {
				return FALSE;	
			} else if (!move_right(&reachable)) {
				/* FATAL ERROR, must not happen */
				return FALSE;
			} else {
				//printf("Y");
				return next_in_branch(pnode);
			}
		} else {
			/*if (HAS_CHILD(reachable) && (HAS_FLAG_IMPL(reachable->child) || HAS_FLAG_EQTY(reachable->child))) {
				bc_push();
				//printf("J");
				if (!move_down(&reachable)) {
					// FATAL ERROR, must not happen 
					return FALSE;
				} else {
					return next_in_branch(pnode);
				}*/
			if (explore_branch()) {
				return next_in_branch(pnode);
			} else {
				//printf("Q");
				return branch_proceed(pnode);	
			}
		}
	}

	if (HAS_FLAG_EQTY(reachable)) {
		//printf("  >>>  ");
		if (HAS_GFLAG_WRAP) {
			wrap_right();
			if (eqendwrap == reachable) {
				UNSET_GFLAG_WRAP
				/* move up */
				/*if (bc->above == NULL) {
					return FALSE; // last pop is done by branch_exit
				} else {
					reachable = bc_pop();
					return next_in_branch(pnode);
				}*/
				branch_proceed(pnode);
			}
			//return TRUE;
			//if (HAS_SYMBOL(reachable)) printf("'%s'", *(reachable->symbol));
			return FALSE;

		} else {
			if (HAS_FLAG_FRST(reachable)) {
				eqfirst = reachable;
			}

			while (move_right(&reachable)) {
				if (HAS_SYMBOL(reachable)){
					continue;
				} else if (check_asmp(pnode)) {
					SET_GFLAG_WRAP
					eqendwrap = reachable;

					wrap_right();
					if (HAS_SYMBOL(reachable)) { /* assuming correct syntax */
						wrap_right();
					}
					return TRUE;
				}
			}
			return FALSE;
		}
	}
	return FALSE;
}

unsigned short int attempt_explore(Pnode* pnode)
{
	if (explore_branch()) {
		SET_GFLAG_BRCH
		//printf("/");
		if (!next_in_branch(pnode)) {
			//printf("X");
			exit_branch();
			UNSET_GFLAG_BRCH
			return next_reachable_const(pnode);
		}
	} else {
		UNSET_GFLAG_BRCH
	}
	return TRUE;
}

unsigned short int next_reachable_const(Pnode* pnode)
{
	if (HAS_GFLAG_BRCH) {
		if (!next_in_branch(pnode)) {
			exit_branch();
			UNSET_GFLAG_BRCH
			//printf("@(%d)", reachable->num);
		}
		return TRUE;
	} 
	
	if (HAS_GFLAG_SUBD) {
		if (next_known_id()) {
			sub_vars();
			return attempt_explore(pnode);
		} else {
			finish_sub();
			return next_reachable_const(pnode);
		}
	}

	if (move_left(&reachable) || 
			(move_up(&reachable) && move_left(&reachable))) {
		if (HAS_SYMBOL(reachable) ?  move_left(&reachable) :  TRUE) {
			//printf("![%d]", reachable->num);
			if (reachable->var != NULL) {
				init_sub(pnode);
				sub_vars();
			}
			return attempt_explore(pnode);
		}
	}
	return FALSE;
}

/* for debugging */
void print_node_info(Pnode* pnode, unsigned short int ncounter)
{
	Variable* var;

	printf("\n\n");
	printf("Node %d (%d):\n", ncounter, pnode->num);
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
	if (HAS_FLAG_FRST(pnode)) {
		printf(" FRST");
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

		//print_node_info(pnode, ncounter);

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
	//print_node_info(pnode, ncounter);
	if (pnode->var != NULL) {
		free(pnode->var);
	}
	free(pnode);
}
