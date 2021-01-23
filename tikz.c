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

#include <stdio.h>
#include <sys/stat.h>
#define DTIKZ
#include "tikz.h"

int main(int argc, char *argv)
{
	struct stat st = {0};			/* for checking directory existence */
	FILE *file;

	if (stat("debug", &st) == -1) {
		mkdir("debug", 0700);
	}

	file = fopen("debug/legend.tex", "w");

	if (file != NULL) {
		fprintf(file, TIKZ_HEADER TIKZ_LGND_V);
		fprintf(file, TIKZ_FOOTER_LGND);
	}

	fclose(file);
	return 0;
}
