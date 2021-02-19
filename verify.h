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
#ifndef VERIFY_H
#define VERIFY_H

#include "pgraph.h"
#include <gmp.h>

typedef enum {
	VFLAG_NONE = 0,
	VFLAG_SUBD = 1,
	VFLAG_BRCH = 2,
	VFLAG_WRAP = 4,
	VFLAG_FRST = 8, /* deprecated */
	VFLAG_FAIL = 16,
} VFlags;

#define HAS_VFLAG_SUBD(vflags) (vflags & VFLAG_SUBD)
#define HAS_VFLAG_BRCH(vflags) (vflags & VFLAG_BRCH)
#define HAS_VFLAG_WRAP(vflags) (vflags & VFLAG_WRAP)
#define HAS_VFLAG_FRST(vflags) (vflags & VFLAG_FRST)
#define HAS_VFLAG_FAIL(vflags) (vflags & VFLAG_FAIL)

#define SET_VFLAG_SUBD(vflags) vflags |= VFLAG_SUBD;
#define SET_VFLAG_BRCH(vflags) vflags |= VFLAG_BRCH;
#define SET_VFLAG_WRAP(vflags) vflags |= VFLAG_WRAP;
#define SET_VFLAG_FRST(vflags) vflags |= VFLAG_FRST;
#define SET_VFLAG_FAIL(vflags) vflags |= VFLAG_FAIL;

#define UNSET_VFLAG_SUBD(vflags) vflags &= ~VFLAG_SUBD;
#define UNSET_VFLAG_BRCH(vflags) vflags &= ~VFLAG_BRCH;
#define UNSET_VFLAG_WRAP(vflags) vflags &= ~VFLAG_WRAP;
#define UNSET_VFLAG_FRST(vflags) vflags &= ~VFLAG_FRST;
#define UNSET_VFLAG_FAIL(vflags) vflags &= ~VFLAG_FAIL;

typedef struct Eqwrapper {
	Pnode* pwrapper;
	Pnode* pendwrap;
} Eqwrapper;


/* stack for branch exploration */
typedef struct branch_checkpoint { /* stack for jumping back to parent levels */
	Pnode* pnode;
	unsigned short int wrap;
	unsigned short int frst;
	unsigned short int fail;
	Pnode* pwrapper;
	Pnode* pendwrap;
	struct branch_checkpoint* above;
} BC;

/* stack for substitution */
typedef struct substitution_status {
	Pnode* known_const;	/* currently used constant sub-tree for substitution  */
	char* sym;			/* symbol of substituted variable */
	Variable* var;		/* substituted variable */
	VTree* vtree;		/* substituted variable */
	//Variable* equalto;
	struct substitution_status* prev;
	struct substitution_status* next;
} SUB;

unsigned short int verify_universal(Pnode* pn);
unsigned short int verify_existence(Pnode* pn, Pnode* pexstart);

unsigned short int are_equal(Pnode* p1, Pnode* p2);

/* for verification */
void finish_verify();
unsigned short int verify(Pnode* pnode, Pnode** pexplorer);

/* for backtracking */
unsigned short int next_reachable_const(Pnode* veri_perspec, Pnode* sub_perspec,
		Pnode** pexplorer, Eqwrapper** eqwrapper, BC** checkpoint,
		VFlags* vflags, SUB** subd);

mpz_t comp_count;
#endif
