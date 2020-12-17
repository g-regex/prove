#include "pgraph.h"
#include <stddef.h>

#define TRUE 1
#define FALSE 0

void init_pgraph(Pnode* root)
{
	root->parent = root->child = root->left = root->right = NULL;
	root->symbol = NULL;
	root->flags = 0;
	root->varcount = 0;
}
