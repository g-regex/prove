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

typedef enum {
	DBG_NONE = 0,
	DBG_QUIET = 1,
	DBG_TIKZ = 2,
} DBGops;

DBGops dbgops;

#define DBG_QUIET_IS_SET (dbgops & DBG_QUIET)
#define DBG_TIKZ_IS_SET (dbgops & DBG_TIKZ)

#define SET_DBG_QUIET dbgops |= DBG_QUIET;
#define SET_DBG_TIKZ dbgops |= DBG_TIKZ;
