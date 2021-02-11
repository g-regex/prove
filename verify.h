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

typedef struct Eqwrapper {
	Pnode* pwrapper;
	Pnode* pendwrap;
} Eqwrapper;


/* stack for branch exploration */
typedef struct branch_checkpoint { /* stack for jumping back to parent levels */
	Pnode* pnode;
	unsigned short int wrap;
	unsigned short int frst;
	Pnode* pwrapper;
	Pnode* pendwrap;
	struct branch_checkpoint* above;
} BC;

unsigned short int verify_universal(Pnode* pn);
unsigned short int verify_existence(Pnode* pn, Pnode* pexstart);

unsigned short int are_equal(Pnode* p1, Pnode* p2);

/* for verification */
void finish_verify();
unsigned short int verify(Pnode* pnode, Pnode** pexplorer);

/* for backtracking */
unsigned short int next_reachable_const(Pnode* pnode, Pnode** pexplorer,
		Eqwrapper** eqwrapper, BC** checkpoint);

#endif
