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
	NFLAG_FRST = 64
} NFlags;

typedef enum {
	GFLAG_NONE = 0,
	GFLAG_VRFD = 1,
	GFLAG_SUBD = 2,
	GFLAG_BRCH = 4,
	GFLAG_WRAP = 8
} GFlags;

#define HAS_FLAG_IMPL(pnode) (pnode->flags & NFLAG_IMPL)
#define HAS_FLAG_EQTY(pnode) (pnode->flags & NFLAG_EQTY)
#define HAS_FLAG_FMLA(pnode) (pnode->flags & NFLAG_FMLA)
#define HAS_FFLAGS(pnode) (HAS_FLAG_IMPL(pnode) || HAS_FLAG_EQTY(pnode) \
		|| HAS_FLAG_FMLA(pnode))
#define HAS_FLAG_ASMP(pnode) (pnode->flags & NFLAG_ASMP)
#define HAS_FLAG_LOCK(pnode) (pnode->flags & NFLAG_LOCK)
#define HAS_FLAG_NEWC(pnode) (pnode->flags & NFLAG_NEWC)
#define HAS_FLAG_FRST(pnode) (pnode->flags & NFLAG_FRST)

#define HAS_GFLAG_VRFD (gflags & GFLAG_VRFD)
#define HAS_GFLAG_SUBD (gflags & GFLAG_SUBD)
#define HAS_GFLAG_BRCH (gflags & GFLAG_BRCH)
#define HAS_GFLAG_WRAP (gflags & GFLAG_WRAP)

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
#define SET_NFLAG_FRST(pnode) pnode->flags |= NFLAG_FRST;

#define SET_GFLAG_VRFD gflags |= GFLAG_VRFD;
#define SET_GFLAG_SUBD gflags |= GFLAG_SUBD;
#define SET_GFLAG_BRCH gflags |= GFLAG_BRCH;
#define SET_GFLAG_WRAP gflags |= GFLAG_WRAP;

#define UNSET_NFLAG_ASMP(pnode) pnode->flags &= ~NFLAG_ASMP;
#define UNSET_NFLAG_NEWC(pnode) pnode->flags &= ~NFLAG_NEWC;
#define UNSET_NFLAG_LOCK(pnode) pnode->flags &= ~NFLAG_LOCK;
#define UNSET_NFLAG_FRST(pnode) pnode->flags &= ~NFLAG_FRST;

#define UNSET_GFLAG_VRFD gflags &= ~GFLAG_VRFD;
#define UNSET_GFLAG_SUBD gflags &= ~GFLAG_SUBD;
#define UNSET_GFLAG_BRCH gflags &= ~GFLAG_BRCH;
#define UNSET_GFLAG_WRAP gflags &= ~GFLAG_WRAP;

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
	int num;	/* DEBUG */
	Variable* var;
} Pnode;

static short int n = 0; /* DEBUG */

void init_pgraph(Pnode** root);

/* for verification */
static GFlags gflags = GFLAG_NONE;
static Pnode* reachable;
void init_reachable(Pnode* pnode);
unsigned short int next_reachable_const(Pnode* pnode);
unsigned short int same_as_rchbl(Pnode* pnode);

/* graph creation */
void create_child(Pnode* pnode);
void create_right(Pnode* pnode);

unsigned short int move_right(Pnode** pnode);
unsigned short int move_down(Pnode** pnode);
unsigned short int move_and_sum_up(Pnode** pnode);

void set_symbol(Pnode* pnode, char* symbol);

/* graph destruction */
void free_graph(Pnode* pnode);

/* DEBUG */
unsigned short int rn();
unsigned short int iswrapped();

#endif
