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

#define HAS_NFLAG_IMPL(pnode) (pnode->flags & NFLAG_IMPL)
#define HAS_NFLAG_EQTY(pnode) (pnode->flags & NFLAG_EQTY)
#define HAS_NFLAG_FMLA(pnode) (pnode->flags & NFLAG_FMLA)
#define HAS_NFLAG_ASMP(pnode) (pnode->flags & NFLAG_ASMP)
#define HAS_NFLAG_LOCK(pnode) (pnode->flags & NFLAG_LOCK)
#define HAS_NFLAG_NEWC(pnode) (pnode->flags & NFLAG_NEWC)
#define HAS_NFLAG_FRST(pnode) (pnode->flags & NFLAG_FRST)

#define SET_NFLAG_IMPL(pnode) pnode->flags |= NFLAG_IMPL;
#define SET_NFLAG_EQTY(pnode) pnode->flags |= NFLAG_EQTY;
#define SET_NFLAG_FMLA(pnode) pnode->flags |= NFLAG_FMLA;
#define SET_NFLAG_ASMP(pnode) pnode->flags |= NFLAG_ASMP;
#define SET_NFLAG_LOCK(pnode) pnode->flags |= NFLAG_LOCK;
#define SET_NFLAG_NEWC(pnode) pnode->flags |= NFLAG_NEWC;
#define SET_NFLAG_FRST(pnode) pnode->flags |= NFLAG_FRST;

#define UNSET_NFLAG_ASMP(pnode) pnode->flags &= ~NFLAG_ASMP;
#define UNSET_NFLAG_NEWC(pnode) pnode->flags &= ~NFLAG_NEWC;
#define UNSET_NFLAG_LOCK(pnode) pnode->flags &= ~NFLAG_LOCK;
#define UNSET_NFLAG_FRST(pnode) pnode->flags &= ~NFLAG_FRST;

#define HAS_FFLAGS(pnode) (HAS_NFLAG_IMPL(pnode) || HAS_NFLAG_EQTY(pnode) \
		|| HAS_NFLAG_FMLA(pnode))
/* bitmasking the NFFLAGS */
#define GET_NFFLAGS(pnode) (pnode->flags & 7)

typedef enum {
	GFLAG_NONE = 0,
	GFLAG_VRFD = 1,
	GFLAG_SUBD = 2,
	GFLAG_BRCH = 4,
	GFLAG_WRAP = 8
} GFlags;

#define HAS_GFLAG_VRFD (gflags & GFLAG_VRFD)
#define HAS_GFLAG_SUBD (gflags & GFLAG_SUBD)
#define HAS_GFLAG_BRCH (gflags & GFLAG_BRCH)
#define HAS_GFLAG_WRAP (gflags & GFLAG_WRAP)

#define HAS_CHILD(pnode) (pnode->child != NULL)
#define HAS_RIGHT(pnode) (pnode->right != NULL)
#define HAS_SYMBOL(pnode) (pnode->symbol != NULL)

#define SET_GFLAG_VRFD gflags |= GFLAG_VRFD;
#define SET_GFLAG_SUBD gflags |= GFLAG_SUBD;
#define SET_GFLAG_BRCH gflags |= GFLAG_BRCH;
#define SET_GFLAG_WRAP gflags |= GFLAG_WRAP;

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
	char** symbol; /* using a double pointer to let known identifiers
					  point to the same char* in memory */
	NFlags flags;
	int num; /* number of the current node in pre-order traversal of the tree */
	Variable* var; /* link to the first variable in
					  sub-tree (for substitution) */
} Pnode;

#define CONTAINS_ID(pnode) \
	(HAS_CHILD(pnode) && pnode->child->symbol != NULL \
	 && pnode->child->right == NULL)
#define IS_ID(pnode) \
	(HAS_SYMBOL(pnode) && pnode->left == NULL \
	 && pnode->right == NULL)

#define IS_INTERNAL(pnode) (pnode->symbol == NULL)

static short int n = 0; /* DEBUG */

/* for verification */
unsigned short int same_as_rchbl(Pnode* pnode);

/* for backtracking */
static GFlags gflags = GFLAG_NONE;
static Pnode* reachable;
void init_reachable(Pnode* pnode);
unsigned short int next_reachable_const(Pnode* pnode);

/* graph creation */
void init_pgraph(Pnode** root);
void create_child(Pnode* pnode);
void create_right(Pnode* pnode);
void set_symbol(Pnode* pnode, char* symbol);

/* navigation */
unsigned short int move_right(Pnode** pnode);
unsigned short int move_down(Pnode** pnode);
unsigned short int move_and_sum_up(Pnode** pnode);

/* memory deallocation */
void free_graph(Pnode* pnode);

/* DEBUG */
unsigned short int rn();
unsigned short int iswrapped();

#endif
