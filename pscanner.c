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

/* reads a <number> token and stores its integer value to token->value */
static void process_number(Token *token);
/* reads an identifier for a <symbol> or <operand> and stores it to token->id */
static void process_word(Token *token);

/* initialises the scanner */
void init_scanner(FILE *f)
{
	quiet = FALSE;
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
		if (isalpha(ch) && ch != '\\') {
			/* process an <operand> */
			token->type = TOK_VAR;
			process_word(token);
		} else if (isdigit(ch)) {

			/* process a <number> */
			process_number(token);

		} else switch (ch) {
			case '[':
				cursor.col = col;
				token->type = TOK_LBRACK;
				next_char();
				break;
			case ']':
				cursor.col = col;
				token->type = TOK_RBRACK;
				next_char();
				break;
			case ':':
				cursor.col = col;
				token->type = TOK_IMPLY;
				strcpy(token->id, ":");
				next_char();
				break;
			case '\\':
				/* process a <symbol> */
				cursor.col = col;
				next_char();
				if (!isalpha(ch)) {
					/* ERROR */
					if (!quiet) {
						printf("\nillegal character '%c' at line %d, column %d\n",
								ch, cursor.line, cursor.col);
					}
					exit(EXIT_FAILURE);
				}
				token->type = TOK_SYM;
				process_word(token);
				break;
			default:
				cursor.col = col;
				/* ERROR */
				if (!quiet) {
					printf("\nillegal character '%c' at line %d, column %d\n",
							ch, cursor.line, cursor.col);
				}
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

	/* debugging output */
	/*if (!isspace(ch)) {
		printf("%c", (char) ch);
	} else {
		printf(".");
	}*/

	if (last_read == '\n') {
		cursor.line++;
		col = 0;
	}
	last_read = ch;
	col++;
}

void process_word(Token *token)
{
	char word[MAX_ID_LENGTH+1];
	int i, cmp;

	word[0] = ch;
	cursor.col = col;
	next_char();

	for (i = 1; i != MAX_ID_LENGTH && (isalpha(ch) || isdigit(ch) || ch == '_'); i++) {
		word[i] = ch;
		next_char();
	}
	word[i] = '\0';

	/* check that the id length is less than the maximum */
	if (i == MAX_ID_LENGTH && (isalpha(ch) || ch == '_')) {
		/* ERROR */
		token->type = TOK_EOF;
	} else {
		/* is the word reserved? */
		cmp = search(word);

		/* if the word is not reserved, it is an operator */
		if (cmp == -1) {
			strcpy(token->id, word);
		} else {
			token->type = get_token_type(cmp);
		}
	}
}

void process_number(Token *token)
{
	int d;

	token->type = TOK_NUM;
	token->value = 0;
	while (isdigit(ch)) {
		d = atoi((char*) &ch);
		if (token->value > (INT_MAX - d) / 10) {
			/* ERROR */
			if (!quiet) {
				printf("\nnumber too large\n");
			}
			exit(EXIT_FAILURE);
		} else {
			token->value = 10 * token->value + d;
		}
		next_char();
	}

}
