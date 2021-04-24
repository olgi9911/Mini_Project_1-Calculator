#ifndef __CODEGEN__
#define __CODEGEN__

#include "parser.h"
//e.g. reg[0] == 1 -> occupied, == 0 -> empty
extern int reg[8];

// Evaluate the syntax tree
extern int evaluateTree(BTNode *root);

// Print the syntax tree in prefix
extern void printPrefix(BTNode *root);
// Print the ASSEMBLY code
extern void assembly(BTNode *root);

#endif // __CODEGEN__
