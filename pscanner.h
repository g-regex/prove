/* [prove]: A proof verification system using bracketed expressions.
 * Copyright (C) 2020  Gregor Feierabend
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

#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "token.h"

/* current position of token in source file */
typedef struct {
	int line;  /*< line number   */
	int col;   /*< column number */
} Cursor;

Cursor cursor;

void init_scanner(FILE *f);
void next_token(Token *token);

#endif /* SCANNER_H */
