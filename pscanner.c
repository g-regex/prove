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

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "pscanner.h"
#include "token.h"
#include "debug.h"

#define TRUE 1
#define FALSE 0

static FILE *file; /* [prove] source file                */
static int   col;  /* column number of current character */
static int   ch;   /* current character                  */

static void next_char(void); /* advances the "cursor" to the next character */

/* reads an identifier for a <symbol> or <operand> and stores it to token->id */
static void process_string(Token *token);

/* initialises the scanner */
void init_scanner(FILE *f)
{
	file = f;
	cursor.line = 1;
	cursor.col = 0;
	next_char();
}

/*
 * advances the "cursor" to the next token and stores the token information
 * in the provided struct
 */
void next_token(Token *token)
{
	/* skip all whitespace */
	while (isspace(ch)) {
		next_char();
	}

	/* remember token start column (to report the correct error position) */
	cursor.col = col;

	/* get the next token */
	if (ch != EOF) {
		if (isalpha(ch) || isdigit(ch) || isspecial(ch)) {
			/* process a <string> */
			token->type = TOK_SYM;
			process_string(token);
		} else switch (ch) {
			case '[':
				cursor.col = col;
				token->type = TOK_LBRACK;
				strcpy(token->id, "[");
				next_char();
				break;
			case ']':
				cursor.col = col;
				token->type = TOK_RBRACK;
				strcpy(token->id, "]");
				next_char();
				break;
			case '=':
				cursor.col = col;
				next_char();
				if (ch == '>') {
					token->type = TOK_IMPLY;
					strcpy(token->id, "=>");
					next_char();
				} else {
					token->type = TOK_EQ;
					strcpy(token->id, "=");
				}
				break;
			default:
				cursor.col = col;
				/* ERROR */
				//if (!DBG_QUIET_IS_SET) {
					fprintf(stderr, "illegal character '%c' at line %d, column %d\n",
							ch, cursor.line, cursor.col);
				//}
				exit(EXIT_FAILURE);
				token->type = TOK_EOF;
		}
	} else {
		token->type = TOK_EOF;
	}
}

void next_char(void)
{
	static char last_read = '\0';
	if ((ch = fgetc(file)) == EOF) {
		return;
	}

	if (last_read == '\n') {
		cursor.line++;
		col = 0;
	}
	last_read = ch;
	col++;
}

void process_string(Token *token)
{
	char string[MAX_ID_LENGTH+1];
	int i, cmp;

	string[0] = ch;
	cursor.col = col;
	next_char();

	for (i = 1; i != MAX_ID_LENGTH && (isalpha(ch) || isdigit(ch) || isspecial(ch)); i++) {
		string[i] = ch;
		next_char();
	}
	string[i] = '\0';

	/* check that the id length is less than the maximum */
	if (i == MAX_ID_LENGTH && (isalpha(ch) || isdigit(ch) || isspecial(ch))) {
		/* ERROR */
		token->type = TOK_EOF;
	} else {
		/* is the string reserved? */
		cmp = search(string);

		/* if the string is not reserved, it is an operator */
		if (cmp == -1) {
			strcpy(token->id, string);
		} else {
			token->type = get_token_type(cmp);
		}
	}
}
