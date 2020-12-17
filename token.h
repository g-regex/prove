#ifndef TOKEN_H
#define TOKEN_H

#define MAX_ID_LENGTH 32

#define IS_FORMULATOR(type) \
	(type == TOK_STR || type == TOK_IMPLY || type == TOK_EQ)

/* recognisable tokens */
typedef enum {
	TOK_EOF,     /* end-of-file */

	TOK_LBRACK,  /* left bracket */
	TOK_RBRACK,  /* right bracket */

	TOK_IMPLY,   /* implication */
	TOK_EQ,		 /* equality */
	TOK_STR      /* string */
} TType;

typedef struct {
	TType  type;
	union{
		char   id[MAX_ID_LENGTH+1]; /* identifier of the variable */
		int value;
	};
} Token;

int search(char *pat);
int isspecial(char ch);
TType get_token_type(int cmp);

#endif /* TOKEN_H */
