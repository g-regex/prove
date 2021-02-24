/* [prove]: A proof verification system using bracketed expressions.
 * Copyright (C) 2021  Gregor Feierabend
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

/* definition of error exit codes */
#define ERR_SYNTAX 2

/* help text */
#ifdef DEBUG
#ifdef MD
#define MDC "`"
#define MDN "\n"
#define MDS "**"
#else
#define MDC ""
#define MDN ""
#define MDS ""
#endif
#define USAGE \
	MDS "Usage:" MDS " " MDC "%s --help | "\
	"[--dall | --dparser | --dverify | --dtikz | --dcomplete] "\
	"<filename>" MDC "\n" MDN

#define HELP \
		"\n" MDS "GENERAL options:" MDS "\n\n" MDN\
	MDC "--help" MDC "\tdisplay this message\n" MDN\
		"\n" MDS "DEBUGGING options:" MDS "\n\n" MDN\
	MDC "--dparser" MDC "  \tactivate debugging output for parser\n" MDN\
	MDC "--dequal" MDC "  \tactivate debugging output for circular linked "\
		"list for equalities\n" MDN\
	MDC "--dgraph" MDC "   \tactivate debugging output for graph creation "\
	 			"(implies " MDC "--dparser" MDC ")\n" MDN\
	MDC "--dverify" MDC "  \tactivate debugging output for verification "\
	 			"(implies " MDC "--dparser" MDC ")\n" MDN\
	MDC "--dtikz" MDC "    \tgenerate TikZ graph representation in "\
		"./debug/\n" MDN\
	MDC "--dcomplete" MDC "\tdo not break verification loop after first "\
		"success\n" MDN\
	MDC "--dfinish" MDC "  \tfinish execution, even if verification "\
		"fails\n" MDN\
	MDC "--dall" MDC "     \tactivate all debugging options\n" MDN

#else

#define USAGE \
	"Usage: %s --help | <filename>\n"

#define HELP \
	"\nGENERAL options:\n\n"\
	"--help\tdisplay this message\n"\
	"\nDEBUGGING options:\n\n"\
	"--dcomplete\tdo not break verification loop after first success\n"\
	"--dfinish  \tfinish execution, even if verification fails\n"\
	"\nFor more debugging options, compile with full debugging support.\n"

#endif

#define NOSUPPORT \
	fprintf(stderr, "Executable has been compiled without this feature.");\
	exit(EXIT_FAILURE);
