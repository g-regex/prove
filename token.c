#include <string.h>
#include "token.h"

typedef struct {
	char   *word;
	TType  type;
} Reserved;

/* representation of reserved keywords */
Reserved reserved[] = {
	{ "=>",		TOK_IMPLY   },
	{ "=",		TOK_EQ		},
};

int isspecial(char ch)
{
	/* permitted special characters */
	char special[] = {'+', '-', '/', '*', '%', '^', '&', '.', '?', '!'};
	int num_special = 10;
	
	for (int i = 0; i < num_special; i++) {
		if (ch == special[i]) {
			return 1;
		}
	}
	return 0;
}

TType get_token_type(int cmp)
{
	return reserved[cmp].type;
}

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

int search(char *pat)
{
	return search_binary(pat, 0,
			(sizeof(reserved) / sizeof(Reserved)) - 1);
}
