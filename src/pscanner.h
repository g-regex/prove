#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "token.h"

/* current position of token in source file */
typedef struct {
	int line;  /*< line number   */
	int col;   /*< column number */
} Cursor;

Cursor cursor;

void init_scanner(FILE *f);
void next_token(Token *token);

#endif /* SCANNER_H */
