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

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include "pscanner.h"
#include "debug.h"
#include "verify.h"
#include "pgraph.h"

#define TRUE 1
#define FALSE 0

#ifdef DEBUG
#define USAGE \
	"Usage: %s --help | "\
	"[--dall | --dparser | --dverify | --dtikz | --dcomplete] "\
	"<filename>\n"

#define HELP \
	"\nGENERAL options:\n\n"\
	"--help\tdisplay this message\n"\
	"\nDEBUGGING options:\n\n"\
	"--dparser  \tactivate debugging output for parser\n"\
	"--dverify  \tactivate debugging output for verification "\
				"(implies --dparser)\n"\
	"--dtikz    \tgenerate TikZ graph representation in ./debug/\n"\
	"--dcomplete\tdo not break verification loop after first success\n"\
	"--dall     \tactivate all debugging options\n"

#else

#define USAGE \
	"Usage: %s --help | <filename>\n"

#define HELP \
	"\nGENERAL options:\n\n"\
	"--help\tdisplay this message\n"\
	"\nDEBUGGING options:\n\n"\
	"--dcomplete\tdo not break verification loop after first success\n"\
	"\nFor more debugging options, compile with full debugging support.\n"

#endif

#define NOSUPPORT \
	fprintf(stderr, "Executable has been compiled without this feature.");\
	exit(EXIT_FAILURE);

Token    token;                     /* current token					*/
Pnode*   pnode;                     /* current node in graph			*/
FILE*    file;                      /* [prove] source file				*/

#ifdef DPARSER
static unsigned short int lvl;      /* indentation level DPARSER		*/
#endif

void parse_expr(void);
int parse_formula(void);
void parse_statement(void);

void expect(TType type);
void check_conflict(Pnode* pnode, TType ttype);

int main(int argc, char *argv[])
{
	struct stat st = {0};			/* for checking directory existence */
	unsigned short int i;

	dbgops = DBG_NONE;
#ifdef DTIKZ
	char* tikzfile;
	char* filename;
#endif

	file = NULL;

	if (argc < 2) {
		fprintf(stderr, USAGE, argv[0]);
		exit(EXIT_FAILURE);
	} else  {
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i], "--help") == 0) {
				printf(USAGE HELP, argv[0]);
				exit(EXIT_SUCCESS);
			} else if (strcmp(argv[i], "--dparser") == 0) {
#ifndef DPARSER
				NOSUPPORT
#endif
				SET_DBG_PARSER
			} else if (strcmp(argv[i], "--dtikz") == 0) {
#ifndef DTIKZ
				NOSUPPORT
#endif
				SET_DBG_TIKZ
			} else if (strcmp(argv[i], "--dverify") == 0) {
#ifndef DVERIFY
				NOSUPPORT
#endif
				SET_DBG_PARSER
				SET_DBG_VERIFY
			} else if (strcmp(argv[i], "--dcomplete") == 0) {
				SET_DBG_COMPLETE
			} else if (strcmp(argv[i], "--dall") == 0) {
				SET_DBG_PARSER
				SET_DBG_TIKZ
				SET_DBG_COMPLETE
				SET_DBG_VERIFY
			} else if (argv[i][0] == '-' && argv[i][1] == '-') {
				fprintf(stderr, "unknown argument '%s', try '--help'\n"
						USAGE, argv[i], argv[0]);
				exit(EXIT_FAILURE);
			} else if (file == NULL) {
				if ((file = fopen(argv[i], "r")) == NULL) {
					fprintf(stderr, "error opening '%s'\n", argv[i]);
					exit(EXIT_FAILURE);
				}
			} else {
				fprintf(stderr,
						"Currently [prove] only supports opening one file at "
						"a time. Refused to open '%s'.\n", argv[i]);
				exit(EXIT_FAILURE);
			}
		}
	}

	TIKZ(
		if (stat("debug", &st) == -1) {
			mkdir("debug", 0700);
		}
	)

#ifdef DPARSER
	lvl = 0;
#endif
	init_scanner(file);
	next_token(&token);

	TIKZ(tikzfile = (char*) malloc(strlen(basename(argv[1]))
				* (sizeof(char) + 5));
	filename = basename(argv[1]);
	sprintf(tikzfile, "debug/%s.tex", strsep(&filename, "."));
	tikz = fopen(tikzfile, "w");
	if (tikz == NULL) {
		fprintf(stderr, "error opening '%s'\n", tikzfile);
		exit(EXIT_FAILURE);
	}
	fprintf(tikz, TIKZ_HEADER TIKZ_GRAPHSCOPE);)

	init_pgraph(&pnode);

	/* <expr>
	 * (loop probably not needed) */
	while (token.type != TOK_EOF) {
		parse_expr();
	}

	TIKZ(fprintf(tikz, TIKZ_ENDSCOPE);)

	free_graph(pnode);

	TIKZ(fprintf(tikz, TIKZ_FOOTER);
	fclose(tikz);)

	fclose(file);

	return EXIT_SUCCESS;
}

/* --- parser functions ------------------------------------------------------*/

void parse_expr(void)
{
	/* maybe the EBNF should be altered a bit,
	 * this seems to be a bit non-sensical */
	if (parse_formula()) {
	} else {
	}
}

int parse_formula(void)
{
	/* TODO perform some check for ERRORS (wrt to EQ and IMP positioning */
	for (int proceed = TRUE; proceed;) {
		if (IS_FORMULATOR(token.type)) {
			DBG_PARSER(printf("%s", token.id);)
			set_symbol(pnode, token.id);
			check_conflict(pnode, token.type);

			next_token(&token);
			parse_statement();
			if (!IS_FORMULATOR(token.type)) {
				proceed = FALSE;
			}
		} else {
			parse_statement();
			if (!IS_FORMULATOR(token.type)) {
				return FALSE;
			} else {
				DBG_PARSER(printf("%s", token.id);)

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
#ifdef DPARSER
		lvl++;
#endif
		DBG_PARSER(printf("%s", token.id);)
		expect(TOK_LBRACK);
		if (HAS_GFLAG_VRFD) {
			UNSET_GFLAG_VRFD
		}

		if (HAS_CHILD(pnode) || HAS_SYMBOL(pnode)) {
			create_right(pnode);
			move_right(&pnode);
		}
		create_child(pnode);
		move_down(&pnode);

		if (token.type == TOK_SYM) {
			set_symbol(pnode, token.id);	
			next_token(&token);
			if (token.type == TOK_RBRACK) {
				/* token is an identifier */
				DBG_PARSER(printf("%s", *(pnode->symbol));)
			} else if (token.type == TOK_LBRACK) {
				/* token is a formulator */
				/* check for conflicting flags and report ERROR */
				DBG_PARSER(printf("%s", *(pnode->symbol));)
				SET_NFLAG_FMLA(pnode)
				parse_expr();
			} else {
				/* formulators must not be mixed/identifiers must not contain = */
				/* ERROR */
			}
		} else if (token.type == TOK_IMPLY) {
			DBG_PARSER(printf("%s", token.id);)
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
			DBG_PARSER(printf("%s", token.id);)
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

		DBG_PARSER(printf("%s", token.id);)
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

		/* verification is triggered here */
		if (HAS_NFLAG_IMPL(pnode) && !HAS_NFLAG_ASMP(pnode)) {
			init_reachable(pnode);
			DBG_PARSER(if(HAS_GFLAG_VRFD) printf("*");)
			DBG_PARSER(printf("{%d}", pnode->num);)
			if (!HAS_GFLAG_VRFD || DBG_COMPLETE_IS_SET) {
				while (next_reachable_const(pnode)) {
					DBG_PARSER(printf("<%d", rn());)
					if(same_as_rchbl(pnode)) {
						DBG_PARSER(printf("#");)
						SET_GFLAG_VRFD
						
						/* if compiled without debugging support, which might
						 * print to the terminal, skip unnecessary compares */
						if (DBG_NONE_IS_SET || !DBG_COMPLETE_IS_SET) {
							finish_verify();
							DBG_PARSER(printf(">");)
							break;
						}
					}
					DBG_PARSER(printf(">");)
				}
			}

			if (!HAS_GFLAG_VRFD) {
				fprintf(stderr, "verification failed on line %d, column %d\n",
						 cursor.line, cursor.col);
				exit(EXIT_FAILURE);
			}
		}

#ifdef DPARSER
		lvl--;
#endif

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
		fprintf(stderr, "unexpected token on line %d, column %d\n",
				 cursor.line, cursor.col);
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
		/* indent assumptions in debugging output to improve readability */
		DBG_PARSER(if (!HAS_NFLAG_ASMP(pnode)) {
			printf("\n");
			for (int i = 0; i < lvl; i++ ) {
				printf("\t");
			}
		})

		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_IMPL(pnode)
		} else if (HAS_NFLAG_IMPL(pnode)) {
			return;
		} else {
			fprintf(stderr, "unexpected TOK_IMPLY "
				"on line %d, column %d\n", cursor.line, cursor.col);
			exit(EXIT_FAILURE);
		}
	} else if (ttype == TOK_EQ) {
		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_EQTY(pnode)
		} else if (HAS_NFLAG_EQTY(pnode)) {
			return;
		} else {
			fprintf(stderr, "unexpected TOK_EQ "
				"on line %d, column %d\n", cursor.line, cursor.col);
			exit(EXIT_FAILURE);
		}
	} else if (ttype == TOK_SYM) {
		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_FMLA(pnode)
		} else if (HAS_NFLAG_FMLA(pnode)) {
			return;
		} else {
			fprintf(stderr, "unexpected TOK_SYM "
				"on line %d, column %d\n", cursor.line, cursor.col);
			exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stderr, "unexpected error "
			"on line %d, column %d\n", cursor.line, cursor.col);
		exit(EXIT_FAILURE);
	}
}


