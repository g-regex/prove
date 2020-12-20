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

Token    token;                     /* current token					*/
Pnode*   pnode;                     /* current node in graph			*/
FILE*    file;                      /* [prove] source file				*/
static unsigned short int lvl;      /* indentation level				*/
static short int output_lvl;		/* print, when on this level		*/

void parse_expr(void);
int parse_formula(void);
void parse_statement(void);

void expect(TType type);
void check_conflict(Pnode* pnode, TType ttype);

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

	free_graph(pnode);

	return EXIT_SUCCESS;
}

/* --- parser functions ------------------------------------------------------*/

void parse_expr(void)
{
	if (parse_formula()) {
	} else {
	}
}

int parse_formula(void)
{
	/* TODO perform some check for ERRORS (wrt to EQ and IMP positioning */
	for (int proceed = TRUE; proceed;) {
		if (IS_FORMULATOR(token.type)) {
			DBG(TRUE, token.id)
			set_symbol(pnode, token.id);
			check_conflict(pnode, token.type);

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
				return FALSE;
			} else {
				DBG(TRUE, token.id)

				create_right(pnode);
				move_right(&pnode);

				check_conflict(pnode, token.type);
				set_symbol(pnode, token.id);

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
	Pnode* pcompare;
	unsigned short int found;

	for (int proceed = TRUE; proceed;) {
		lvl++;
		DBG(TRUE, token.id)
		expect(TOK_LBRACK);

		if (HAS_CHILD(pnode) || HAS_SYMBOL(pnode)) {
			create_right(pnode);
			move_right(&pnode);
		}
		create_child(pnode);
		move_down(&pnode);

		if (token.type == TOK_STR) {
			set_symbol(pnode, token.id);	
			next_token(&token);
			if (token.type == TOK_RBRACK) {
				/* token is an identifier */
				DBG(TRUE, *(pnode->symbol))
			} else if (token.type == TOK_LBRACK) {
				/* token is a formulator */
				/* check for conflicting flags and report ERROR */
				DBG(TRUE, *(pnode->symbol))
				SET_NFLAG_FMLA(pnode)
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
				/* statements must not contain only an implication symbol */
				/* ERROR */
			} else if (token.type == TOK_LBRACK) {
				/* only valid option */
				SET_NFLAG_IMPL(pnode)
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
		move_and_sum_up(&pnode);

		/* check whether a new identifier was introduced */
		if (CONTAINS_ID(pnode)) {
			pcompare = pnode->prev_const;
			found = FALSE;
			while (pcompare != NULL) {
				if (CONTAINS_ID(pcompare)) {
					if (strcmp(*(pcompare->child->symbol),
								*(pnode->child->symbol)) == 0) {
						found = TRUE;

						free(*(pnode->child->symbol));
						free(pnode->child->symbol);
						pnode->child->symbol = pcompare->child->symbol;
						break;
					}
				}
				pcompare = pcompare->prev_const;
			}
			if (found == FALSE) {
				if (HAS_FFLAGS(pnode)) {
					/* ERROR new ids must only occur at the beginning
					 * of statements */
				} else {
					SET_NFLAG_NEWC(pnode)
				}
			}
		}

		if (HAS_FLAG_IMPL(pnode) && !HAS_FLAG_ASMP(pnode)) {
			//printf("(");
			init_reachable(pnode);
			while (next_reachable_const(pnode)) {
				//printf("<(%d)", pnode->num);
				//printf(".");
				if(same_as_rchbl(pnode)) { /* TODO skip unneccessary cmps */
					// printf("(%d,%d)", pnode->num, rn()); /* DEBUG "reason" */
				}
				//printf(">");
			}
			//printf(")");
		}

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

/*
 * checks, whether the current formulator type is conflicting with other
 * formulators in the current formula
 */
void check_conflict(Pnode* pnode, TType ttype)
{
	if (ttype == TOK_IMPLY) {
		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_IMPL(pnode)
		} else if (HAS_FLAG_IMPL(pnode)) {
			return;
		} else {
			if (!quiet) {
				fprintf(stderr, "unexpected TOK_IMPLY "
					"on line %d, column %d\n",
						 cursor.line, cursor.col);
			}
			exit(EXIT_FAILURE);
		}
	} else if (ttype == TOK_EQ) {
		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_EQTY(pnode)
		} else if (HAS_FLAG_EQTY(pnode)) {
			return;
		} else {
			if (!quiet) {
				fprintf(stderr, "unexpected TOK_EQ "
					"on line %d, column %d\n",
						 cursor.line, cursor.col);
			}
			exit(EXIT_FAILURE);
		}
	} else if (ttype == TOK_STR) {
		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_FMLA(pnode)
		} else if (HAS_FLAG_FMLA(pnode)) {
			return;
		} else {
			if (!quiet) {
				fprintf(stderr, "unexpected TOK_STR "
					"on line %d, column %d\n",
						 cursor.line, cursor.col);
			}
			exit(EXIT_FAILURE);
		}
	} else {
		if (!quiet) {
			fprintf(stderr, "unexpected error "
				"on line %d, column %d\n",
					 cursor.line, cursor.col);
		}
		exit(EXIT_FAILURE);
	}
}


