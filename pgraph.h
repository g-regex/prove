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
	NFLAG_FRST = 64,
	NFLAG_TRUE = 128,
} NFlags;

#define HAS_NFLAG_IMPL(pnode) (pnode->flags & NFLAG_IMPL)
#define HAS_NFLAG_EQTY(pnode) (pnode->flags & NFLAG_EQTY)
#define HAS_NFLAG_FMLA(pnode) (pnode->flags & NFLAG_FMLA)
#define HAS_NFLAG_ASMP(pnode) (pnode->flags & NFLAG_ASMP)
#define HAS_NFLAG_LOCK(pnode) (pnode->flags & NFLAG_LOCK)
#define HAS_NFLAG_NEWC(pnode) (pnode->flags & NFLAG_NEWC)
#define HAS_NFLAG_FRST(pnode) (pnode->flags & NFLAG_FRST)
#define HAS_NFLAG_TRUE(pnode) (pnode->flags & NFLAG_TRUE)

#define SET_NFLAG_IMPL(pnode) pnode->flags |= NFLAG_IMPL;
#define SET_NFLAG_EQTY(pnode) pnode->flags |= NFLAG_EQTY;
#define SET_NFLAG_FMLA(pnode) pnode->flags |= NFLAG_FMLA;
#define SET_NFLAG_ASMP(pnode) pnode->flags |= NFLAG_ASMP;
#define SET_NFLAG_LOCK(pnode) pnode->flags |= NFLAG_LOCK;
#define SET_NFLAG_NEWC(pnode) pnode->flags |= NFLAG_NEWC;
#define SET_NFLAG_FRST(pnode) pnode->flags |= NFLAG_FRST;
#define SET_NFLAG_TRUE(pnode) pnode->flags |= NFLAG_TRUE;

#define UNSET_NFLAG_ASMP(pnode) pnode->flags &= ~NFLAG_ASMP;
#define UNSET_NFLAG_NEWC(pnode) pnode->flags &= ~NFLAG_NEWC;
#define UNSET_NFLAG_LOCK(pnode) pnode->flags &= ~NFLAG_LOCK;
#define UNSET_NFLAG_FRST(pnode) pnode->flags &= ~NFLAG_FRST;
#define UNSET_NFLAG_TRUE(pnode) pnode->flags &= ~NFLAG_TRUE;

#define TOGGLE_NFLAG_TRUE(pnode) \
	if (HAS_NFLAG_TRUE(pnode)) UNSET_NFLAG_TRUE(pnode)\
	else SET_NFLAG_TRUE(pnode)

#define HAS_FFLAGS(pnode) (HAS_NFLAG_IMPL(pnode) || HAS_NFLAG_EQTY(pnode) \
		|| HAS_NFLAG_FMLA(pnode))
/* bitmasking the NFFLAGS */
#define GET_NFFLAGS(pnode) (pnode->flags & 7)

typedef enum {
	GFLAG_NONE = 0,
	GFLAG_VRFD = 1,
	GFLAG_PSTP = 2,
} GFlags;

GFlags gflags;  /* accessed by verify.c and proveparser.c */

#define HAS_GFLAG_VRFD (gflags & GFLAG_VRFD)
#define HAS_GFLAG_PSTP (gflags & GFLAG_PSTP)

#define HAS_CHILD(pnode) (pnode->child != NULL && *(pnode->child) != NULL)
#define HAS_RIGHT(pnode) (pnode->right != NULL && *(pnode->right) != NULL)
#define HAS_SYMBOL(pnode) (pnode->symbol != NULL && *(pnode->symbol) != NULL)

#define SET_GFLAG_VRFD gflags |= GFLAG_VRFD;
#define SET_GFLAG_PSTP gflags |= GFLAG_PSTP;

#define UNSET_GFLAG_VRFD gflags &= ~GFLAG_VRFD;
#define UNSET_GFLAG_PSTP gflags &= ~GFLAG_PSTP;

typedef enum {
	VARFLAG_NONE = 0,
	VARFLAG_LOCK = 1,
	VARFLAG_LEFT = 2,
	VARFLAG_RGHT = 4,
} VarFlags;

#define HAS_VARFLAG_LOCK(flags) (flags & VARFLAG_LOCK)
#define HAS_VARFLAG_LEFT(flags) (flags & VARFLAG_LEFT)
#define HAS_VARFLAG_RGHT(flags) (flags & VARFLAG_RGHT)

#define SET_VARFLAG_LOCK(flags) flags |= VARFLAG_LOCK;
#define SET_VARFLAG_LEFT(flags) flags |= VARFLAG_LEFT;
#define SET_VARFLAG_RGHT(flags) flags |= VARFLAG_RGHT;

#define UNSET_VARFLAG_LOCK(flags) flags &= ~VARFLAG_LOCK;
#define UNSET_VARFLAG_LEFT(flags) flags &= ~VARFLAG_LEFT;
#define UNSET_VARFLAG_RGHT(flags) flags &= ~VARFLAG_RGHT;

typedef struct VTree {
	struct Pnode* pnode;
	struct VTree* left;
	struct VTree* right;
	struct VTree* parent;
	VarFlags flags;
} VTree;

typedef struct Pnode {
	struct Pnode* parent;
	/*struct Pnode* above;*//*pointing to the parent of the leftmost node in the
							current statement */
	struct Pnode** child;
	struct Pnode* left;
	struct Pnode** right;
	char** symbol; /* using a double pointer to let known identifiers
					  point to the same char* in memory */
	NFlags flags;

	struct Pnode* prev_const; /* link to previous constant sub-tree */
	struct Pnode* prev_id; /* link to previous constant id */

	struct Pnode* vleft;
	struct Pnode* vright;
	struct Pnode* vparent;
	VarFlags varflags;

	int num; /* number of the current node in pre-order traversal of the tree */

	VTree* vtree;
} Pnode;

#define CONTAINS_ID(pnode) \
	(HAS_CHILD(pnode) && HAS_SYMBOL((*(pnode->child))) \
	 && !HAS_RIGHT((*(pnode->child))))
#define IS_ID(pnode) \
	(HAS_SYMBOL(pnode) && pnode->left == NULL \
	 && !HAS_RIGHT(pnode))
#define IS_EMPTY(pnode) \
	(!HAS_CHILD(pnode) && !HAS_SYMBOL(pnode))

#define IS_INTERNAL(pnode) (pnode->symbol == NULL)

/* graph creation */
void init_pgraph(Pnode** root);
void create_child(Pnode* pnode);
void create_right(Pnode* pnode);
void create_right_dummy(Pnode* pnode);
void free_right_dummy(Pnode* pnode);
void set_symbol(Pnode* pnode, char* symbol);
void equate(Pnode* p1, Pnode* p2);

/* navigation */
unsigned short int move_right(Pnode** pnode);
void move_rightmost(Pnode** pnode);
unsigned short int move_left(Pnode** pnode);
unsigned short int move_up(Pnode** pnode);
unsigned short int move_down(Pnode** pnode);
unsigned short int wrap_right();

void move_and_sum_up(Pnode** pnode);

/* memory deallocation */
void free_graph(Pnode* pnode);

VTree* init_vtree(VTree* vtree);
VTree* next_var(VTree* vtree);

int get_node_count();

#endif
