#include <string.h>
#include <stdlib.h>
#include "pscanner.h"
#include "debug.h"
#include "pgraph.h"

#define TRUE 1
#define FALSE 0

#define DPARSER

#ifdef DPARSER
#define DBG(enable, msg) \
	if (!quiet && ((output_lvl == -1 && enable) || output_lvl == lvl)) \
	printf("%s", msg);
#else
#define DBG(msg)
#endif

Token    token;                     /* current token               */
Pnode    pnode;                     /* current node in graph       */
FILE    *file;                      /* [prove] source file         */
static unsigned short int lvl;      /* indentation level           */
static short int output_lvl;		/* print, when on this level   */

void parse_expr(void);
int parse_formula(void);
void parse_statement(void);

void expect(TType type);

int main(int argc, char *argv[])
{
	if (argc < 2 && argc > 4) {
		fprintf(stderr, "usage: %s <filename> [<output_level>] [quiet]\n",
				argv[0]);
		exit(EXIT_FAILURE);
	} else if ((file = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "error opening '%s'\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	lvl = 0;
	init_scanner(file);
	init_pgraph(&pnode);
	next_token(&token);

	if (argc == 4) {
		quiet = TRUE;
	}

	if (argc == 3) {
		output_lvl = atoi(argv[2]);
	} else {
		output_lvl = -1;
	}

	/* <expr>
	 * (loop probably not needed) */
	while (token.type != TOK_EOF) {
		parse_expr();
	}

	fclose(file);

	return EXIT_SUCCESS;
}

/* --- parser functions ------------------------------------------------------*/

void parse_expr(void)
{
	if (parse_formula()) {
	} else {
	}
}

/*
 * "[" (<operand>|<chain>|"false") "]"
 */
int parse_formula(void)
{
	for (int proceed = TRUE; proceed;) {
		if (IS_FORMULATOR(token.type)) {
			DBG(TRUE, token.id)
			next_token(&token);
			DBG(FALSE, ".")
			parse_statement();
			if (!IS_FORMULATOR(token.type)) {
				proceed = FALSE;
			}
		} else {
			DBG(FALSE, ".")
			parse_statement();
			if (!IS_FORMULATOR(token.type)) {
				/*if (!quiet) {
					fprintf(stderr, "expected <formulator> "
						"on line %d, column %d\n",
							 cursor.line, cursor.col);
				}
				exit(EXIT_FAILURE);*/
				return FALSE;
			} else {
				DBG(TRUE, token.id)
				next_token(&token);
				if (token.type != TOK_LBRACK) {
					proceed = FALSE;
				}
			}
		}
	}
	return TRUE;
}

void parse_statement(void)
{
	for (int proceed = TRUE; proceed;) {
		lvl++;
		DBG(TRUE, token.id)
		expect(TOK_LBRACK);
		/*if (lvl == output_lvl && !quiet) {
			printf("%s", token.id);
			printf("[");
		}*/

		if (token.type == TOK_STR) {
#ifdef DPARSER
			char prev_id[MAX_ID_LENGTH];
			strcpy(prev_id, token.id); //remember previous token id for debugging
#endif
			next_token(&token);
			if (token.type == TOK_RBRACK) {
				/* token is an identifier */
				DBG(TRUE, prev_id)
			} else if (token.type == TOK_LBRACK) {
				/* token is a formulator */
				DBG(TRUE, prev_id)
				parse_expr();
			} else {
				/* formulators must not be mixed/identifiers must not contain = */
				/* ERROR */
			}
		} else if (token.type == TOK_IMPLY) {
			DBG(TRUE, token.id)
			/* token is an implication symbol */
			next_token(&token);
			if (token.type == TOK_RBRACK) {
				/* implications must not appear at the end */
				/* ERROR */
			} else if (token.type == TOK_LBRACK) {
				/* only valid option */
				parse_expr();
			} else {
				/* formulators must not be mixed/identifiers must not contain = */
				/* ERROR */
			}
		} else if (token.type == TOK_EQ) {
			DBG(TRUE, token.id)
			/* statements must not begin with an equality token */
			/* ERROR */
		} else if (token.type == TOK_LBRACK) {
			parse_expr();
		} else if (token.type == TOK_RBRACK) {
			/* empty statement */
		} else {
			/* cannot go here, undefined behaviour */
			/* ERROR */
		}

		DBG(TRUE, token.id)
		expect(TOK_RBRACK);
		lvl--;

		if (token.type != TOK_LBRACK) {
			proceed = FALSE;
		}
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
		if (!quiet) {
			fprintf(stderr, "unexpected token on line %d, column %d\n",
					 cursor.line, cursor.col);
		}
		exit(EXIT_FAILURE);
	}
}
