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

/* graph creation */
void init_pgraph(Pnode** root);
void create_child(Pnode* pnode);
void create_right(Pnode* pnode);
void set_symbol(Pnode* pnode, char* symbol);

/* navigation */
unsigned short int move_right(Pnode** pnode);
unsigned short int move_left(Pnode** pnode);
unsigned short int move_up(Pnode** pnode);
unsigned short int move_down(Pnode** pnode);
unsigned short int wrap_right();
unsigned short int move_and_sum_up(Pnode** pnode);

/* memory deallocation */
void free_graph(Pnode* pnode);

/* verification */
GFlags gflags;  /* accessed by verify.c and proveparser.c */

/* DEBUG */
unsigned short int rn();
unsigned short int iswrapped();
static short int n = 0;

#endif
