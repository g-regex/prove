#ifndef PGRAPH_H
#define PGRAPH_H

typedef enum {
	FLAG_START = 0,
	FLAG_IMPL = 1,
	FLAG_EQTY = 2,
	FLAG_FMTR = 4,
	FLAG_ASMP = 8
} Flags;

#define IS_CONST_NODE(pnode) \
	(pnode->child != NULL && pnode->child->symbol != NULL)
#define IS_FORMULATOR_NODE(pnode) \
	(pnode->symbol != NULL)
#define IS_LEAF_NODE(pnode) (pnode->symbol != NULL)
#define IS_LEFTMOST_NODE(pnode) (pnode->left == NULL)

#define HAS_FLAG_IMPL(pnode) (pnode->flags & FLAG_IMPL)
#define HAS_FLAG_EQTY(pnode) (pnode->flags & FLAG_EQTY)
#define HAS_FLAG_FMTR(pnode) (pnode->flags & FLAG_FMTR)
#define HAS_FFLAGS(pnode) (HAS_FLAG_IMPL(pnode) || HAS_FLAG_EQTY(pnode) \
		|| HAS_FLAG_FMTR(pnode))

#define HAS_FLAG_ASMP(pnode) (pnode->flags & FLAG_ASMP)

#define SET_IMPL(pnode) pnode->flags |= FLAG_IMPL;
#define SET_EQTY(pnode) pnode->flags |= FLAG_EQTY;
#define SET_FMTR(pnode) pnode->flags |= FLAG_FMTR;
#define SET_ASMP(pnode) pnode->flags |= FLAG_ASMP;

#define UNSET_ASMP(pnode) pnode->flags &= ~FLAG_ASMP;

typedef struct Pnode {
	struct Pnode* parent;
	struct Pnode* child;
	struct Pnode* left;
	struct Pnode* right;
	char* symbol;
	Flags flags;
	int varcount;
} Pnode;

void init_pgraph(Pnode** root);

void create_child(Pnode* pnode);
void create_right(Pnode* pnode);

unsigned short int move_right(Pnode** pnode);
unsigned short int move_down(Pnode** pnode);

unsigned short int move_and_sum_up(Pnode** pnode);

void set_symbol(Pnode* pnode, char* symbol);

void free_graph(Pnode* pnode);

#endif
