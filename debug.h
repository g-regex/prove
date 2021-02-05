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

#ifdef DPARSER
//#define DNUM
#define DEBUG
#define DBG_PARSER_IS_SET (dbgops & DBG_PARSER)
#define SET_DBG_PARSER dbgops |= DBG_PARSER;
#define DBG_PARSER(cmd) \
	if (DBG_PARSER_IS_SET) { cmd }
#else
#define DBG_PARSER(cmd)
#define SET_DBG_PARSER
#endif

#ifdef DEBUG
#define DBG_NONE_IS_SET (dbgops == DBG_NONE)
#else
#define DBG_NONE_IS_SET 1
/*#define DBG_COMPLETE_IS_SET 0
#define DBG_FINISH_IS_SET 0
#define SET_DBG_COMPLETE 
#define SET_DBG_FINISH*/
#endif

#define DBG_COMPLETE_IS_SET (dbgops & DBG_COMPLETE)
#define DBG_FINISH_IS_SET (dbgops & DBG_FINISH)
#define SET_DBG_COMPLETE dbgops |= DBG_COMPLETE;
#define SET_DBG_FINISH dbgops |= DBG_FINISH;

#endif /* DEBUG_H */
