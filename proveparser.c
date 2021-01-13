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

#include <string.h>
#include <stdlib.h>
#include "pscanner.h"
#include "debug.h"
#include "pgraph.h"
#include "tikz.h"

#define TRUE 1
#define FALSE 0

#define DPARSER

#ifdef DPARSER
#define DBG(cmd) \
	if (!DBG_QUIET_IS_SET) { cmd }
#else
#define DBG(cmd)
#endif

Token    token;                     /* current token					*/
Pnode*   pnode;                     /* current node in graph			*/
FILE*    file;                      /* [prove] source file				*/
static unsigned short int lvl;      /* indentation level				*/

void parse_expr(void);
int parse_formula(void);
void parse_statement(void);

void expect(TType type);
void check_conflict(Pnode* pnode, TType ttype);

int main(int argc, char *argv[])
{
	dbgops = DBG_NONE;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <filename> [quiet]\n",
				argv[0]);
		exit(EXIT_FAILURE);
	} else if ((file = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "error opening '%s'\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	lvl = 0;
	init_scanner(file);
	next_token(&token);

	if (argc > 2) {
		SET_DBG_QUIET
	}

	tikz = fopen("latex/graph.tex", "w");
	fprintf(tikz, "%%This file was automatically generated by [prove].\n"
			"\\title{Graph}\n"
			"\\documentclass[landscape, 11pt]{article}\n"
			"\\usepackage{color}\n"
			"\\usepackage{tikz}\n"
			"\\usetikzlibrary{calc}\n"
			"\\usetikzlibrary{positioning}\n"
			"\\usetikzlibrary{arrows.meta}\n"
			"\\usepackage[margin=-5pt, hoffset=0pt, voffset=0pt]{geometry}\n"
			"\\addtolength{\\topmargin}{20pt}\n"
			"\\definecolor{darkmagenta}{rgb}{0.55, 0.0, 0.55}\n"
			"\\definecolor{darkorange}{rgb}{1.0, 0.55, 0.0}\n"
			"\\definecolor{darkpastelgreen}{rgb}{0.01, 0.75, 0.24}\n"
			"\\definecolor{oucrimsonred}{rgb}{0.6, 0.0, 0.0}\n"
			"\\definecolor{darkpowderblue}{rgb}{0.0, 0.2, 0.6}\n"
			"\\definecolor{mediumspringgreen}{rgb}{0.0, 0.98, 0.6}\n"
			"\\definecolor{deepskyblue}{rgb}{0.0, 0.75, 1.0}\n"
			"\\pagenumbering{gobble}\n"
			"\\begin{document}\n");

	fprintf(tikz, "\\begin{figure}[h!]"
		"\\centering\n"
		"\\resizebox{0.9\\hsize}{!}{"
		"\\begin{tikzpicture}[node distance = 1pt, auto]\n"
		"\\begin{scope}["
		"every node/.style={rectangle,inner sep=3pt,minimum width=3pt, minimum height=21pt, text height=5pt,yshift=0pt}, "
		"-{Latex[length=5pt,width=5pt]}]\n");

	init_pgraph(&pnode);

	/* <expr>
	 * (loop probably not needed) */
	while (token.type != TOK_EOF) {
		parse_expr();
	}

	fprintf(tikz, "\\end{scope}\n");
	free_graph(pnode);
	fprintf(tikz, "\\end{tikzpicture} }\n"
			"\\end{figure}\n"
			"\\end{document}");
	fclose(tikz);
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
			DBG(printf("%s", token.id);)
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
				DBG(printf("%s", token.id);)

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
		DBG(printf("%s", token.id);)
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
				DBG(printf("%s", *(pnode->symbol));)
			} else if (token.type == TOK_LBRACK) {
				/* token is a formulator */
				/* check for conflicting flags and report ERROR */
				DBG(printf("%s", *(pnode->symbol));)
				SET_NFLAG_FMLA(pnode)
				parse_expr();
			} else {
				/* formulators must not be mixed/identifiers must not contain = */
				/* ERROR */
			}
		} else if (token.type == TOK_IMPLY) {
			DBG(printf("%s", token.id);)
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
			DBG(printf("%s", token.id);)
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

		DBG(printf("%s", token.id);)
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
			DBG(if(HAS_GFLAG_VRFD) printf("*");)
			DBG(printf("{%d}", pnode->num);)
			while (next_reachable_const(pnode)) {
				DBG(printf("<%d", rn());)
				if(same_as_rchbl(pnode)) {
					/* TODO skip unneccessary cmps */
					DBG(printf("#");)
					SET_GFLAG_VRFD
				}
				DBG(printf(">");)
			}
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
		if (!DBG_QUIET_IS_SET) {
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
		DBG(if (!HAS_NFLAG_ASMP(pnode)) {
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
			if (!DBG_QUIET_IS_SET) {
				fprintf(stderr, "unexpected TOK_IMPLY "
					"on line %d, column %d\n",
						 cursor.line, cursor.col);
			}
			exit(EXIT_FAILURE);
		}
	} else if (ttype == TOK_EQ) {
		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_EQTY(pnode)
		} else if (HAS_NFLAG_EQTY(pnode)) {
			return;
		} else {
			if (!DBG_QUIET_IS_SET) {
				fprintf(stderr, "unexpected TOK_EQ "
					"on line %d, column %d\n",
						 cursor.line, cursor.col);
			}
			exit(EXIT_FAILURE);
		}
	} else if (ttype == TOK_SYM) {
		if (!HAS_FFLAGS(pnode)) {
			SET_NFLAG_FMLA(pnode)
		} else if (HAS_NFLAG_FMLA(pnode)) {
			return;
		} else {
			if (!DBG_QUIET_IS_SET) {
				fprintf(stderr, "unexpected TOK_SYM "
					"on line %d, column %d\n",
						 cursor.line, cursor.col);
			}
			exit(EXIT_FAILURE);
		}
	} else {
		if (!DBG_QUIET_IS_SET) {
			fprintf(stderr, "unexpected error "
				"on line %d, column %d\n",
					 cursor.line, cursor.col);
		}
		exit(EXIT_FAILURE);
	}
}


