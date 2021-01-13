/* [prove]: A proof verification system using bracketed expressions.
 * Copyright (C) 2021  Gregor Feierabend
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

#ifdef DPARSER
#ifndef TIKZ_H
#define TIKZ_H

#include <stdio.h>

#define TIKZ_HSPACE "25"

FILE* tikz;
unsigned short int rightmost_child;
unsigned short int max_depth;
unsigned short int cur_depth;
#endif

#endif
