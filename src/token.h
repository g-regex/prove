#ifndef TOKEN_H
#define TOKEN_H

#define MAX_ID_LENGTH 32

/* recognisable tokens */
typedef enum {

	TOK_EOF,     /* end-of-file */

	TOK_LBRACK,  /* left bracket */
	TOK_RBRACK,  /* right bracket */

	/* operands */
	TOK_VAR,     /* variable */
	TOK_NUM,     /* number */

	/* symbols */
	TOK_IMPLY,   /* implication symbol */
	TOK_SYM,     /* other symbol */

	TOK_FALSE, /*< false */

} TType;

typedef struct {
	TType  type;
	union{
		char   id[MAX_ID_LENGTH+1]; /* identifier of the variable */
		int value;
	};
} Token;

int search(char *pat);
TType get_token_type(int cmp);

#endif /* TOKEN_H */
