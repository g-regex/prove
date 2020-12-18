#ifndef PGRAPH_H
#define PGRAPH_H

typedef enum {
	FLAG_NONE = 0,
	FLAG_IMPL = 1,
	FLAG_EQTY = 2,
	FLAG_FMLA = 4,
	FLAG_ASMP = 8,
	FLAG_LOCK = 16,
	FLAG_NEWC = 32,
} Flags;

#define HAS_FLAG_IMPL(pnode) (pnode->flags & FLAG_IMPL)
#define HAS_FLAG_EQTY(pnode) (pnode->flags & FLAG_EQTY)
#define HAS_FLAG_FMLA(pnode) (pnode->flags & FLAG_FMLA)
#define HAS_FFLAGS(pnode) (HAS_FLAG_IMPL(pnode) || HAS_FLAG_EQTY(pnode) \
		|| HAS_FLAG_FMLA(pnode))
#define HAS_FLAG_ASMP(pnode) (pnode->flags & FLAG_ASMP)
#define HAS_FLAG_LOCK(pnode) (pnode->flags & FLAG_LOCK)
#define HAS_FLAG_NEWC(pnode) (pnode->flags & FLAG_NEWC)

#define HAS_CHILD(pnode) (pnode->child != NULL)
#define HAS_SYMBOL(pnode) (pnode->symbol != NULL)

#define CONTAINS_ID(pnode) \
	(HAS_CHILD(pnode) && pnode->child->symbol != NULL \
	 && pnode->child->right == NULL)
#define IS_ID(pnode) \
	(HAS_SYMBOL(pnode) && pnode->left == NULL \
	 && pnode->right == NULL)



#define SET_IMPL(pnode) pnode->flags |= FLAG_IMPL;
#define SET_EQTY(pnode) pnode->flags |= FLAG_EQTY;
#define SET_FMLA(pnode) pnode->flags |= FLAG_FMLA;
#define SET_ASMP(pnode) pnode->flags |= FLAG_ASMP;
#define SET_LOCK(pnode) pnode->flags |= FLAG_LOCK;
#define SET_NEWC(pnode) pnode->flags |= FLAG_NEWC;

#define UNSET_ASMP(pnode) pnode->flags &= ~FLAG_ASMP;
#define UNSET_NEWC(pnode) pnode->flags &= ~FLAG_NEWC;
#define UNSET_LOCK(pnode) pnode->flags &= ~FLAG_LOCK;

typedef struct Pnode {
	struct Pnode* parent;
	struct Pnode* child;
	struct Pnode* left;
	struct Pnode* right;
	struct Pnode* prev_const; /* link to previous constant in the tree */
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
