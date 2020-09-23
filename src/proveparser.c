#include <string.h>
#include <stdlib.h>
#include "pscanner.h"

#define TRUE 1
#define FALSE 0

#define IS_SYMBOL(type) \
	(type == TOK_IMPLY || type == TOK_SYM)

#define STARTS_STATEMENT(type) \
	(type == TOK_LBRACK)

Token    token;                       /* current token               */
FILE    *file;                        /* [prove] source file         */
static unsigned short int lvl;        /* indentation level           */
static unsigned short int output_lvl; /* print, when on this level   */

void parse_chain(void);
void parse_statement(void);
void parse_operand(void);

void expect(TType type);

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3) {
		printf("usage: %s <filename> [<output_level>]", argv[0]);
		exit(1);
	} else if ((file = fopen(argv[1], "r")) == NULL) {
		printf("error opening '%s'", argv[1]);
		exit(1);
	}

	if (argc == 3) {
		output_lvl = atoi(argv[2]);
	} else {
		output_lvl = 0;
	}

	init_scanner(file);
	next_token(&token);

	lvl = 0;

	/* <expressions> */
	while (token.type != TOK_EOF) {
		parse_chain();
	}

	fclose(file);

	return 0;
}

/* --- parser functions ------------------------------------------------------*/

/*
 * {(<statement>|<symbol>)} <statement> {(<statement>|<symbol>)}
 */
void parse_chain(void)
{
	unsigned short int stmnt_encountered = FALSE;
	unsigned short int proceed = TRUE;
	while (proceed) {
		if (IS_SYMBOL(token.type)) {
			if (lvl == output_lvl) {
				printf("%s", token.id);
			}
			next_token(&token);
		} else if (STARTS_STATEMENT(token.type)) {
			if (lvl == output_lvl - 1) {
				printf("(");
			} else if (lvl == output_lvl) {
				printf(".");
			}
			parse_statement();
			if (lvl == output_lvl - 1) {
				printf(")");
			}
			stmnt_encountered = TRUE;
		} else {
			proceed = FALSE;
		}
	}

	if (!stmnt_encountered) {
		/* ERROR */
		printf("\nexpected <statement> on line %d, column %d\n",
					 cursor.line, cursor.col);
		exit(1);
	}
}

/*
 * "[" (<operand>|<chain>|"false") "]"
 */
void parse_statement(void)
{
	expect(TOK_LBRACK);
	lvl++;

	if (token.type == TOK_FALSE) {
		next_token(&token);
	} else if (token.type == TOK_VAR || token.type == TOK_NUM) {
		parse_operand();
	} else if (STARTS_STATEMENT(token.type) || IS_SYMBOL(token.type)) {
		parse_chain();
	} else {
		printf("\nexpected <operand>, <chain> or \"false\""\
				"on line %d, column %d\n",
					 cursor.line, cursor.col);
		exit(1);
	}

	expect(TOK_RBRACK);
	lvl--;
}

/*
 * <number>| /variable name/
 */
void parse_operand(void)
{
	if (token.type == TOK_NUM || token.type == TOK_VAR) {
		next_token(&token);
	} else {
		/* ERROR */
		printf("\nexpected <operand> on line %d, column %d\n",
				 cursor.line, cursor.col);
		exit(1);
	}
}

/* --- helpers ---------------------------------------------------------------*/

/*
 * checks, whether the current token is of desired type and reports an
 * error otherwise
 */
void expect(TType type)
{
	if (token.type == type) {
		next_token(&token);
	} else {
		/* ERROR */
		printf("\nunexpected token on line %d, column %d\n",
				 cursor.line, cursor.col);
		exit(1);
	}
}
