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

#ifndef TOKEN_H
#define TOKEN_H

#define MAX_ID_LENGTH 32

#define IS_FORMULATOR(type) \
	(type == TOK_SYM || type == TOK_IMPLY || type == TOK_EQ || type == TOK_REF)
#define IS_IMPL_TYPE_TOK(type) \
	(type == TOK_IMPLY || type == TOK_REF)

/* recognisable tokens */
typedef enum {
	TOK_EOF,     /* end-of-file */

	TOK_LBRACK,  /* left bracket */
	TOK_RBRACK,  /* right bracket */

	TOK_IMPLY,   /* implication ("=>") */
	TOK_REF,     /* reference implication ("ref=>") */

	TOK_EQ,		 /* equality ("=") */
	/*TOK_NOT,*/ /* not ("!") */
	TOK_SYM      /* symbol */
} TType;

typedef struct {
	TType  type;
	union{
		char   id[MAX_ID_LENGTH+1]; /* identifier of the variable */
		int value;
	};
} Token;

int search(char *pat);
unsigned short int isspecial(char ch);
TType get_token_type(int cmp);

#endif /* TOKEN_H */
