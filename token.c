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
#include "token.h"

#define TRUE 1
#define FALSE 0

typedef struct {
	char   *word;
	TType  type;
} Reserved;

/* representation of reserved keywords */
Reserved reserved[] = {
	{ "=>",		TOK_IMPLY   },
	{ "ref=>",	TOK_REF     },
	{ "=",		TOK_EQ		},
};

/**
 * @brief Checks whether a character is a legal special character according to
 * the EBNF.
 *
 * @param ch character to be checked
 *
 * @return TRUE, if character is a legal special character
 */
unsigned short int isspecial(char ch)
{
	/* permitted special characters */
	char special[] = {'+', '-', '/', '*', '%', '^', '&', '.', '?', ':', '!',
		'_'};
	int num_special = 12;
	
	for (int i = 0; i < num_special; i++) {
		if (ch == special[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @brief Gets the token type of a reserved string.
 *
 * @param cmp index of the reserved token
 *
 * @return type of reserved token
 */
TType get_token_type(int cmp)
{
	return reserved[cmp].type;
}

/**
 * @brief Performs a binary search through the array of reserved token strings.
 *
 * @param pat pattern to be searched for
 * @param min starting position in array
 * @param max end position in array
 *
 * @return token position in array on success and -1 otherwise
 */
int search_binary(char *pat, int min, int max)
{
	int mid = min + ((max - min) / 2);

	if (min >= max) {
		if (strcmp(pat, reserved[mid].word) == 0) {
			return mid;
		} else {
			return -1;
		}
	} else {
		if (strcmp(pat, reserved[mid].word) < 0) {
			return search_binary(pat, min, mid - 1);
		} else if (strcmp(pat, reserved[mid].word) > 0) {
			return search_binary(pat, mid + 1, max);
		} else {
			return mid;
		}
	}
}

/**
 * @brief Wrapper function for search_binary().
 *
 * @param pat pattern to be searched for
 *
 * @return whatever search_binary returns
 */
int search(char *pat)
{
	return search_binary(pat, 0,
			(sizeof(reserved) / sizeof(Reserved)) - 1);
}
