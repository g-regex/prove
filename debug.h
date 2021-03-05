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

#ifndef DEBUG_H
#define DEBUG_H

#include "tikz.h"

typedef enum {
	DBG_NONE = 0,
	DBG_TIKZ = 1,		/* generate TikZ graph */
	DBG_PARSER = 2,
	DBG_VERIFY = 4,
	DBG_COMPLETE = 8,	/* do not break loop after first successful
						   verification */
	DBG_FINISH = 16,	/* finish execution, even if verification failed */
	DBG_GRAPH = 32,
	DBG_EQUAL = 64,
	DBG_PATH = 128,
	DBG_FAIL = 256,
	DBG_TMP = 512,
} DBGops;

DBGops dbgops;

/* helpers:
 * DNUM - numerate nodes
 * DEBUG - at least one debugging flag is set
 */

#ifdef DTIKZ
//#define DNUM
#define DEBUG
#define DBG_TIKZ_IS_SET (dbgops & DBG_TIKZ)
#define SET_DBG_TIKZ dbgops |= DBG_TIKZ;
#define TIKZ(cmd) \
	if (DBG_TIKZ_IS_SET) { cmd }
#else
#define TIKZ(cmd)
#define SET_DBG_TIKZ
#endif

#ifdef DVERIFY /* implies DPARSER */
//#define DNUM
#define DEBUG
#ifndef DPARSER
#define DPARSER
#endif
#define DBG_VERIFY_IS_SET (dbgops & DBG_VERIFY)
#define SET_DBG_VERIFY dbgops |= DBG_VERIFY;
#define DBG_VERIFY(cmd) \
	if (DBG_VERIFY_IS_SET) { cmd }
#else
#define DBG_VERIFY(cmd)
#define SET_DBG_VERIFY
#endif

#ifdef DGRAPH /* implies DPARSER */
//#define DNUM
#define DEBUG
#ifndef DPARSER
#define DPARSER
#endif
#define DBG_GRAPH_IS_SET (dbgops & DBG_GRAPH)
#define SET_DBG_GRAPH dbgops |= DBG_GRAPH;
#define DBG_GRAPH(cmd) \
	if (DBG_GRAPH_IS_SET) { cmd }
#else
#define DBG_GRAPH(cmd)
#define SET_DBG_GRAPH
#endif

#ifdef DPARSER /* implies DEQUAL */
//#define DNUM
#define DEBUG
#define DEQUAL
#define DBG_PARSER_IS_SET (dbgops & DBG_PARSER)
#define SET_DBG_PARSER dbgops |= DBG_PARSER;
#define DBG_PARSER(cmd) \
	if (DBG_PARSER_IS_SET) { cmd }
#else
#define DBG_PARSER(cmd)
#define SET_DBG_PARSER
#endif

#ifdef DEQUAL
#define DEBUG
#define DBG_EQUAL_IS_SET (dbgops & DBG_EQUAL)
#define SET_DBG_EQUAL dbgops |= DBG_EQUAL;
#define DBG_EQUAL(cmd) \
	if (DBG_EQUAL_IS_SET) { cmd }
#else
#define DBG_EQUAL(cmd)
#define SET_DBG_EQUAL
#endif

#ifdef DVERIFY
#define DEBUG
#define DBG_PATH_IS_SET (dbgops & DBG_PATH)
#define SET_DBG_PATH dbgops |= DBG_PATH;
#define DBG_PATH(cmd) \
	if (DBG_PATH_IS_SET) { cmd }
#else
#define DBG_PATH(cmd)
#define SET_DBG_PATH
#endif

#ifdef DVERIFY
#define DEBUG
#define DBG_FAIL_IS_SET (dbgops & DBG_FAIL)
#define SET_DBG_FAIL dbgops |= DBG_FAIL;
#define DBG_FAIL(cmd) \
	if (DBG_FAIL_IS_SET) { cmd }
#else
#define DBG_FAIL(cmd)
#define SET_DBG_FAIL
#endif

#ifdef DCOLOUR
#define SHELL_RED "\033[0;31m"
#define SHELL_GREEN "\033[0;32m"
#define SHELL_BROWN "\033[0;33m"
#define SHELL_MAGENTA "\033[0;35m"
#define SHELL_CYAN "\033[0;36m"
#define SHELL_RESET1 "\033[0;0m"
#define SHELL_BOLD "\033[1m"
#define SHELL_RESET2 "\033[0m"
#else
#define SHELL_RED ""
#define SHELL_GREEN ""
#define SHELL_BROWN ""
#define SHELL_MAGENTA ""
#define SHELL_CYAN ""
#define SHELL_RESET1 ""
#define SHELL_BOLD ""
#define SHELL_RESET2 ""

#endif

#ifdef DEBUG
#define DBG_NONE_IS_SET (dbgops == DBG_NONE)
#define DTMP
#else
#define DBG_NONE_IS_SET 1
/*#define DBG_COMPLETE_IS_SET 0
#define DBG_FINISH_IS_SET 0
#define SET_DBG_COMPLETE 
#define SET_DBG_FINISH*/
#endif

#ifdef DTMP
#define DBG_TMP_IS_SET (dbgops & DBG_TMP)
#define SET_DBG_TMP dbgops |= DBG_TMP;
#define DBG_TMP(cmd) \
	if (DBG_TMP_IS_SET) { cmd }
#else
#define DBG_TMP(cmd)
#define SET_DBG_TMP
#endif

#define DBG_COMPLETE_IS_SET (dbgops & DBG_COMPLETE)
#define DBG_FINISH_IS_SET (dbgops & DBG_FINISH)
#define SET_DBG_COMPLETE dbgops |= DBG_COMPLETE;
#define SET_DBG_FINISH dbgops |= DBG_FINISH;

#endif /* DEBUG_H */
