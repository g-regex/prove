#ifndef PGRAPH_H
#define PGRAPH_H

typedef enum {
	NFLAG_NONE = 0,
	NFLAG_IMPL = 1,
	NFLAG_EQTY = 2,
	NFLAG_FMLA = 4,
	NFLAG_ASMP = 8,
	NFLAG_LOCK = 16,
	NFLAG_NEWC = 32,
} NFlags;

typedef enum {
	GFLAG_NONE = 0,
	GFLAG_VRFD = 1,
	GFLAG_SUBD = 2
} GFlags;

static GFlags gflags = GFLAG_NONE;

#define HAS_FLAG_IMPL(pnode) (pnode->flags & NFLAG_IMPL)
#define HAS_FLAG_EQTY(pnode) (pnode->flags & NFLAG_EQTY)
#define HAS_FLAG_FMLA(pnode) (pnode->flags & NFLAG_FMLA)
#define HAS_FFLAGS(pnode) (HAS_FLAG_IMPL(pnode) || HAS_FLAG_EQTY(pnode) \
		|| HAS_FLAG_FMLA(pnode))
#define HAS_FLAG_ASMP(pnode) (pnode->flags & NFLAG_ASMP)
#define HAS_FLAG_LOCK(pnode) (pnode->flags & NFLAG_LOCK)
#define HAS_FLAG_NEWC(pnode) (pnode->flags & NFLAG_NEWC)

#define HAS_GFLAG_VRFD (gflags & GFLAG_VRFD)
#define HAS_GFLAG_SUBD (gflags & GFLAG_SUBD)

/* bitmasking the NFFLAGS */
#define GET_NFFLAGS(pnode) (pnode->flags & 7)

#define HAS_CHILD(pnode) (pnode->child != NULL)
#define HAS_RIGHT(pnode) (pnode->right != NULL)
#define HAS_SYMBOL(pnode) (pnode->symbol != NULL)

#define CONTAINS_ID(pnode) \
	(HAS_CHILD(pnode) && pnode->child->symbol != NULL \
	 && pnode->child->right == NULL)
#define IS_ID(pnode) \
	(HAS_SYMBOL(pnode) && pnode->left == NULL \
	 && pnode->right == NULL)

#define IS_INTERNAL(pnode) (pnode->symbol == NULL)

#define SET_NFLAG_IMPL(pnode) pnode->flags |= NFLAG_IMPL;
#define SET_NFLAG_EQTY(pnode) pnode->flags |= NFLAG_EQTY;
#define SET_NFLAG_FMLA(pnode) pnode->flags |= NFLAG_FMLA;
#define SET_NFLAG_ASMP(pnode) pnode->flags |= NFLAG_ASMP;
#define SET_NFLAG_LOCK(pnode) pnode->flags |= NFLAG_LOCK;
#define SET_NFLAG_NEWC(pnode) pnode->flags |= NFLAG_NEWC;

#define SET_GFLAG_VRFD gflags |= GFLAG_VRFD;
#define SET_GFLAG_SUBD gflags |= GFLAG_SUBD;

#define UNSET_NFLAG_ASMP(pnode) pnode->flags &= ~NFLAG_ASMP;
#define UNSET_NFLAG_NEWC(pnode) pnode->flags &= ~NFLAG_NEWC;
#define UNSET_NFLAG_LOCK(pnode) pnode->flags &= ~NFLAG_LOCK;

#define UNSET_GFLAG_VRFD gflags &= ~GFLAG_VRFD;
#define UNSET_GFLAG_SUBD gflags &= ~GFLAG_SUBD;

typedef struct Variable {
	struct Pnode* pnode;
	struct Variable* next;
} Variable;

typedef struct Pnode {
	struct Pnode* parent;
	struct Pnode* child;
	struct Pnode* left;
	struct Pnode* right;
	struct Pnode* prev_const; /* link to previous constant in the tree */
	char** symbol;
	NFlags flags;
	int varcount; /* TODO: remove this */
	Variable* var;
} Pnode;

void init_pgraph(Pnode** root);

static Pnode* reachable;
static Pnode* known_id;
void init_reachable(Pnode* pnode);
unsigned short int next_reachable_const(Pnode* pnode);
void init_known_id(Pnode* pnode);
unsigned short int next_known_id();
unsigned short int substitute_vars();

void create_child(Pnode* pnode);
void create_right(Pnode* pnode);

unsigned short int move_right(Pnode** pnode);
unsigned short int move_down(Pnode** pnode);

unsigned short int move_and_sum_up(Pnode** pnode);

void set_symbol(Pnode* pnode, char* symbol);

void free_graph(Pnode* pnode);


#endif
