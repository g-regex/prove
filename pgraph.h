#ifndef PGRAPH_H
#define PGRAPH_H

typedef enum {
	FLAG_START = 0,
	FLAG_IMPL = 1,
	FLAG_EQTY = 2,
	FLAG_FMTR = 4,
	FLAG_ASMP = 8
} Flags;

#define IS_CONST_NODE(pnode) (pnode->child->symbol != NULL)
#define IS_LEAF_NODE(pnode) (pnode->symbol != NULL)
#define IS_LEFTMOST_NODE(pnode) (pnode->left == NULL)
#define IS_START_NODE(pnode) (pnode->flags == FLAG_START)

#define SET_IMPL(pnode) (pnode->flags &= FLAG_IMPL)
#define SET_EQTY(pnode) (pnode->flags &= FLAG_EQTY)
#define SET_FMTR(pnode) (pnode->flags &= FLAG_FMTR)
#define SET_ASMP(pnode) (pnode->flags &= FLAG_ASMP)
#define UNSET_ASMP(pnode) (pnode->flags &= ~FLAG_ASMP)

typedef struct Pnode {
	struct pnode* parent;
	struct pnode* child;
	struct pnode* left;
	struct pnode* right;
	char* symbol;
	Flags flags;
	int varcount;
} Pnode;

void init_pgraph(Pnode* root);

#endif
