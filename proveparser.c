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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include "pscanner.h"
#include "debug.h"
#include "verify.h"
#include "pgraph.h"
#include "error.h"

#ifdef DGMP
#include <gmp.h>
#endif

/* --- preprocessor directives ---------------------------------------------- */
#define TRUE 1
#define FALSE 0

#define NOVERINUM 0

/* --- global variables ----------------------------------------------------- */

#if 0
char* toktype[] = {"TOK_EOF", "TOK_LBRACK", "TOK_RBRACK", "TOK_IMPLY",
	"TOK_REF", "TOK_EQ", /*"TOK_NOT",*/ "TOK_SYM"};
#endif
char* toktype[] = {"end of file", "left bracket", "right bracket", "'=>'",
	"'ref=>'", "'='", "string"};

Token    token;                     /* current token						*/
Pnode*   pnode;                     /* current node in graph				*/
Pnode*   prev_node;					/* remember previous node for equalities*/
FILE*    file;                      /* [prove] source file					*/

static unsigned short int lvl;      /* level/depth of current node in tree	*/


/* --- function prototypes -------------------------------------------------- */
void parse_expr(void);
void parse_formula(void);
void parse_statement(unsigned short int veri_ref);

void expect(TType type);
void check_conflict(Pnode* pnode, TType ttype);

unsigned short int success = EXIT_SUCCESS;
unsigned short int do_veri = TRUE;

/**
 * @brief main function of the [prove]-parser
 *
 * @param argc number of command-line arguments
 * @param argv[] command-line arguments
 *
 * @return 0 or error code
 */
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
			} else if (strcmp(argv[i], "--dequal") == 0) {
#ifndef DPARSER
				NOSUPPORT
#endif
				SET_DBG_EQUAL
			} else if (strcmp(argv[i], "--dpath") == 0) {
#ifndef DPARSER
				NOSUPPORT
#endif
				SET_DBG_VERIFY
				SET_DBG_PATH
			} else if (strcmp(argv[i], "--dfail") == 0) {
#ifndef DPARSER
				NOSUPPORT
#endif
				SET_DBG_VERIFY
				SET_DBG_PATH
				SET_DBG_FAIL
			} else if (strcmp(argv[i], "--dtmp") == 0) {
				SET_DBG_TMP
			} else if (strcmp(argv[i], "--dtikz") == 0) {
#ifndef DTIKZ
				NOSUPPORT
#endif
				SET_DBG_TIKZ
			} else if (strcmp(argv[i], "--dgraph") == 0) {
#ifndef DGRAPH
				NOSUPPORT
#endif
				SET_DBG_PARSER
				SET_DBG_GRAPH
			} else if (strcmp(argv[i], "--dverify") == 0) {
#ifndef DVERIFY
				NOSUPPORT
#endif
				SET_DBG_PARSER
				SET_DBG_VERIFY
			} else if (strcmp(argv[i], "--dcomplete") == 0) {
				SET_DBG_COMPLETE
			} else if (strcmp(argv[i], "--dfinish") == 0) {
				SET_DBG_FINISH
			} else if (strcmp(argv[i], "--dall") == 0) {
				SET_DBG_PARSER
				SET_DBG_FAIL
				SET_DBG_PATH
				SET_DBG_TIKZ
				SET_DBG_COMPLETE
				SET_DBG_FINISH
				SET_DBG_VERIFY
			} else if (strcmp(argv[i], "--noveri") == 0) {
				do_veri = FALSE;
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

	lvl = 0;

#ifdef DGMP
	mpz_init(comp_count);
	mpz_set_ui(comp_count,0);
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
	fprintf(tikz, TIKZ_HEADER TIKZ_LGND TIKZ_GRAPHSCOPE);)

	prev_node = NULL;
	init_pgraph(&pnode);

	parse_expr();
	expect(TOK_EOF);

	TIKZ(fprintf(tikz, TIKZ_ENDSCOPE);)

	free_graph(pnode);

	TIKZ(fprintf(tikz, TIKZ_ENDPIC TIKZ_FOOTER);
	fclose(tikz);)

	fclose(file);

	fprintf(stderr, SHELL_CYAN "Nodes in tree:\t%d",
			get_node_count());
#ifdef DGMP
	fprintf(stderr, "\nNodes compared:\t");
	mpz_out_str(stderr, 10, comp_count);
	mpz_clear(comp_count);
#endif
	fprintf(stderr, "\n" SHELL_RESET1);

	if (!do_veri) {
		return EXIT_SUCCESS;
	} else {
		return success;
	}
}/*}}}*/

/* --- parser functions ----------------------------------------------------- */
/**
 * @brief parser function for <expr>
 */
void parse_expr(void)
{
	/* maybe the EBNF should be altered a bit,
	 * this seems to be a bit non-sensical */
	parse_formula();
}

/**
 * @brief parser function for <formula>
 */
void parse_formula(void)
{
	int proceed;
	unsigned short int veri_ref;

	veri_ref = FALSE;
	
	if (token.type == TOK_SYM) {
		set_symbol(pnode, token.id);	
		DBG_PARSER(fprintf(stderr, "%s", *(pnode->symbol)););
		next_token(&token);
		if (token.type == TOK_RBRACK) {
			/* token is an identifier */
			return;
		} else if (token.type == TOK_LBRACK /*|| token.type == TOK_NOT*/) {
			/* token is a formulator */
			/* ?check for conflicting flags and report ERROR */
			SET_NFLAG_FMLA(pnode)
			/* continue */
		} else {
			/* formulators must not be mixed/identifiers must not contain = */
			/* ERROR */
			return;
		}
	} else if (IS_IMPL_TYPE_TOK(token.type)) {
		set_symbol(pnode, token.id);	
		DBG_PARSER(fprintf(stderr, SHELL_CYAN "%s" SHELL_RESET1, recall_chars()););
		DBG_PARSER(fprintf(stderr, "%s", token.id););
		/* token is an implication symbol */
		veri_ref = (token.type == TOK_REF);
		next_token(&token);
		DBG_PARSER(fprintf(stderr, SHELL_CYAN "%s" SHELL_RESET1, recall_chars()););
		if (token.type == TOK_RBRACK) {
			/* statements must not contain only an implication symbol */
			/* ERROR */
			return;
		} else if (token.type == TOK_LBRACK /*|| token.type == TOK_NOT*/) {
			/* only valid option */
			SET_NFLAG_IMPL(pnode)
			UNSET_NFLAG_FRST(pnode)
			/* continue */
		} else {
			/* formulators must not be mixed/identifiers must not contain = */
			/* ERROR */
			return;
		}
	} else if (token.type == TOK_EQ) {
		DBG_PARSER(fprintf(stderr, "%s", token.id););
		/* statements must not begin with an equality token */
		/* ERROR */
		return;
	} else if (token.type == TOK_LBRACK /*|| token.type == TOK_NOT*/) {
		/* continue */
	} else if (token.type == TOK_RBRACK) {
		/* empty statement */
		return;
	} else {
		/* cannot go here, undefined behaviour */
		/* ERROR */
		return;
	}

	proceed = TRUE;

	/* TODO perform some check for ERRORS (wrt to EQ and IMP positioning */
	while (proceed) {
		if (IS_FORMULATOR(token.type)) {
			DBG_PARSER(fprintf(stderr, "%s", token.id););
			set_symbol(pnode, token.id);
			check_conflict(pnode, token.type);

			veri_ref = (token.type == TOK_REF);
			next_token(&token);
			parse_statement(veri_ref);
			veri_ref = FALSE;
			if (!IS_FORMULATOR(token.type)) {
				proceed = FALSE;
			}
		} else {
			parse_statement(veri_ref);
			veri_ref = FALSE;
			if (!IS_FORMULATOR(token.type)) {
				return;
			} else {
				DBG_PARSER(fprintf(stderr, "%s", token.id););

				create_right(pnode);
				move_right(&pnode);

				check_conflict(pnode, token.type);
				set_symbol(pnode, token.id);

				veri_ref = (token.type == TOK_REF);
				next_token(&token);
				if (token.type != TOK_LBRACK /*&& token.type != TOK_NOT*/) {
					proceed = FALSE;
				}
			}
		}
	}
	prev_node = NULL;
}

/**
 * @brief parser function for <statement>
 *
 * @param veri_ref TRUE if "ref=>" formulator was used; FIXME: currently
 * enforces id-only substitution, but should just "suggest"
 */
void parse_statement(unsigned short int veri_ref)
{
	Pnode* ptmp;
	Pnode* pexstart;				/* to remember first node for verifying
									   existence */
	int proceed;
	unsigned short int found;		/* indicating whether an identifier has been
									   found during backtracking */

	proceed = TRUE;

	pexstart = NULL; /* NULLed to make sure that existence will only be
						verified on the same level */

	while (proceed) {
		found = FALSE;
		lvl++;
		DBG_PARSER(fprintf(stderr, SHELL_CYAN "%s" SHELL_RESET1,
					recall_chars()););
		DBG_PARSER(fprintf(stderr, "%s", token.id););

		expect(TOK_LBRACK);
		DBG_PARSER(fprintf(stderr, SHELL_CYAN "%s" SHELL_RESET1,
					recall_chars()););
		if (HAS_GFLAG_VRFD) {
			UNSET_GFLAG_VRFD
		}

		if (HAS_CHILD(pnode) || HAS_SYMBOL(pnode)) {
			create_right(pnode);
			move_right(&pnode);
		}
		create_child(pnode);
		move_down(&pnode);

		parse_expr();

		DBG_PARSER(fprintf(stderr, SHELL_CYAN "%s" SHELL_RESET1,
					recall_chars()););
		DBG_PARSER(fprintf(stderr, "%s", token.id););
		expect(TOK_RBRACK);
		lvl--;

		move_and_sum_up(&pnode);

		if (HAS_NFLAG_EQTY(pnode)) {
			/* TODO: add FATAL ERROR, if inexistent */
			prev_node = pnode->left->left; 
		}

		/* check whether a new identifier was introduced */
		if (CONTAINS_ID(pnode)) {
			ptmp = pnode->prev_const;
			while (ptmp != NULL) {
				if (CONTAINS_ID(ptmp)) {
					if (strcmp(*((*(ptmp->child))->symbol),
								*((*(pnode->child))->symbol)) == 0) {
						found = TRUE;
						/*equate(ptmp, pnode);*/
						free(*((*(pnode->child))->symbol));
						free((*(pnode->child))->symbol);
						(*(pnode->child))->symbol =
							(*(ptmp->child))->symbol;
						(*(pnode->child))->child =
							(*(ptmp->child))->child;
						(*(pnode->child))->right =
							(*(ptmp->child))->right;
						break;
					}
				}
				ptmp = ptmp->prev_const;
			}

			if (found == FALSE) {
				DBG_PARSER(fprintf(stderr, SHELL_MAGENTA "*" SHELL_RESET1);)

				SET_NFLAG_NEWC(pnode)

				(*(pnode->child))->child =
					(Pnode**) malloc(sizeof(struct Pnode*));
				*((*(pnode->child))->child) = NULL;
				(*(pnode->child))->right =
					(Pnode**) malloc(sizeof(struct Pnode*));
				*((*(pnode->child))->right) = NULL;
			}
		}

		/* postpone verification for existence */
		/* FIXME: put this at a better place */
		if (pnode->left != NULL && HAS_SYMBOL(pnode->left) && !HAS_GFLAG_VRFD &&
				HAS_NFLAG_IMPL(pnode) && !HAS_NFLAG_ASMP(pnode)
					&& !HAS_GFLAG_PSTP) {
			SET_GFLAG_PSTP
			pexstart = pnode;
			DBG_VERIFY(fprintf(stderr, SHELL_BOLD "{%d}>"
						SHELL_RESET2, pnode->num););
		}

		/* TODO: handle equalities */
		if (HAS_NFLAG_EQTY(pnode)) {
			/*equate(prev_node, pnode);*/
			prev_node = pnode;
		}

		/* FIXME: Has this become redundant? */
#if 0
		if (HAS_NFLAG_IMPL(pnode) && !HAS_NFLAG_ASMP(pnode)
				&& !HAS_GFLAG_VRFD && !HAS_GFLAG_PSTP) {
			/* universal verification is triggered here */
			if (pnode->num > NOVERINUM && /* DEBUG!!!! */
					do_veri && !verify_universal(pnode)) {
				fprintf(stderr,
						SHELL_RED
						"verification failed on line %d, column %d"
						SHELL_RESET1
						"\n",
						 cursor.line, cursor.col);
				if (!DBG_FINISH_IS_SET) {
					exit(EXIT_FAILURE);
				} else {
					success = EXIT_FAILURE;
				}
			}
		}
#endif

		if (token.type != TOK_LBRACK /*&& token.type != TOK_NOT*/) {
			proceed = FALSE;

			/* trigger quantifier verification here */
			if (HAS_NFLAG_IMPL(pnode) && !HAS_NFLAG_ASMP(pnode)
					&& HAS_GFLAG_PSTP && pexstart != NULL) {
				DBG_VERIFY(
						if (pnode->num != pexstart->num) {
							fprintf(stderr, SHELL_BOLD "<{%d}" SHELL_RESET2,
								pnode->num);
						} else {
							fprintf(stderr, SHELL_BOLD "|" SHELL_RESET2);
						}
				);
				create_right_dummy(pnode);

				/* TODO: Introduce precedence for verification functions.
				 * Implement this through an array of function pointers.
				 * Currently "reference implication" _enforces_ id-only
				 * substitution, but it should be handled as a _hint_ only
				 * without changing the actual meaning of an implication. */
				if (veri_ref) {
					DBG_PARSER(fprintf(stderr, SHELL_MAGENTA "<REF>"
								SHELL_RESET1);)
				}

				if (pnode->num > NOVERINUM && /* DEBUG!!!! */
					do_veri && !verify_quantifiers(*(pnode->right), pexstart,
						veri_ref)) {
					if (lvl != 0) {
						fprintf(stderr,
								SHELL_BROWN
								"<verification postponed to parent level>"
								SHELL_RESET1
								"\n");
					} else {
						/* TODO: Try different verification strategies here.
						 * Next strategy to implement: Consideration of cases.
						 **/
						if (pnode->num != pexstart->num) {
							fprintf(stderr,
									SHELL_RED
									"verification failed on line %d, column %d"
									SHELL_RESET1
									"\n",
									cursor.line, cursor.col);
							if (!DBG_FINISH_IS_SET) {
								exit(EXIT_FAILURE);
							} else {
								success = EXIT_FAILURE;
							}
						} else {
							fprintf(stderr,
									SHELL_BROWN
									"<trying case-based verification>"
									SHELL_RESET1);
							if (!verify_cases(pnode)) {
								fprintf(stderr,
										SHELL_RED
										"verification failed on line %d, "
										"column %d" SHELL_RESET1 "\n",
										cursor.line, cursor.col);
								if (!DBG_FINISH_IS_SET) {
									exit(EXIT_FAILURE);
								} else {
									success = EXIT_FAILURE;
								}
							}
						}
					}
				}	
				free_right_dummy(pnode);

				UNSET_GFLAG_PSTP
			}
		}
		DBG_PARSER(fprintf(stderr, SHELL_CYAN "%s" SHELL_RESET1,
					recall_chars());); 
	}
}

/* --- helpers -------------------------------------------------------------- */

/**
 * @brief Checks, whether the current token is of desired type and reports an
 * error otherwise.
 *
 * @param type type of the current token
 */
void expect(TType type)
{
	if (token.type == type) {
		next_token(&token);
	} else {
		/* ERROR */
		fprintf(stderr, "unexpected token on line %d, column %d; expected %s, "
				"but found %s\n",
				 cursor.line, cursor.col, toktype[type], toktype[token.type]);
		exit(ERR_SYNTAX);
	}
}/*}}}*/

/**
 * @brief Checks whether the current formulator type is conflicting with other
 * formulators in the currently processed formula.
 *
 * @param pnode pointer to current node
 * @param ttype type of currently processed token
 */
void check_conflict(Pnode* pnode, TType ttype)
{
	if (IS_IMPL_TYPE_TOK(ttype)) {
		/* indent assumptions in debugging output to improve readability */
		/* OUTPUT DBG_PARSER(if (!HAS_NFLAG_ASMP(pnode)) {
			fprintf(stderr, "\n");
			for (int i = 0; i < lvl; i++ ) {
				fprintf(stderr, "\t");
			}
		}); */

		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_IMPL(pnode)
		} else if (HAS_NFLAG_IMPL(pnode)) {
			return;
		} else {
			/*fprintf(stderr, "unexpected IMPL_TYPE_TOK "*/
			fprintf(stderr, "unexpected implication token "
				"on line %d, column %d\n", cursor.line, cursor.col);
			exit(ERR_SYNTAX);
		}
	} else if (ttype == TOK_EQ) {
		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_EQTY(pnode)
			prev_node = pnode->left; /* TODO: add FATAL ERROR, if inexistent */
		} else if (HAS_NFLAG_EQTY(pnode)) {
			/* TODO: maybe only allow equalities of the form [...]=[...] */
			return;
		} else {
			/*fprintf(stderr, "unexpected TOK_EQ "*/
			fprintf(stderr, "unexpected '=' "
				"on line %d, column %d\n", cursor.line, cursor.col);
			exit(ERR_SYNTAX);
		}
	} else if (ttype == TOK_SYM) {
		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_FMLA(pnode)
		} else if (HAS_NFLAG_FMLA(pnode)) {
			return;
		} else {
			/*fprintf(stderr, "unexpected TOK_SYM "*/
			fprintf(stderr, "unexpected string "
				"on line %d, column %d\n", cursor.line, cursor.col);
			exit(ERR_SYNTAX);
		}
	} else {
		fprintf(stderr, "unexpected error "
			"on line %d, column %d\n", cursor.line, cursor.col);
		exit(ERR_SYNTAX);
	}
}/*}}}*/
