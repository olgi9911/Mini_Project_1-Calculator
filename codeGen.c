#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codeGen.h"

int reg[8] = {};

void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}

int find_id_in_memory(char lex[256]) { //to find the memory location of a "ID"
    for(int i = 0; i < TBLSIZE; i++) {
        if(strcmp(lex, table[i].name) == 0)
            return i * 4; //1 byte = 4 bits for 32-bit CPU
    }
    return 0;
}

int find_empty_reg() {
    for(int i = 0; i <= 7; i++)
        if(reg[i] == 0) return i;
    
    return 0;
}

int find_used_reg() {
    for(int i = 7; i >= 0; i--)
        if(reg[i] == 1) return i;
    
    return 0;
}

int find_var_in_right(BTNode *root) {
    if(root != NULL) {
        if(root -> data == ID) {
            return 1;
        }
        if(find_var_in_right(root -> left)) return 1;
        if(find_var_in_right(root -> right)) return 1;
    }
    return 0;
}

int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;

    if (root != NULL) {
        switch (root->data) {
            case ID:
                retval = getval(root->lexeme);
                break;
            case INT:
                retval = atoi(root->lexeme);
                break;
            case ASSIGN:
                rv = evaluateTree(root->right);
                retval = setval(root->left->lexeme, rv);
                break;
            case ADDSUB:
            case MULDIV:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                } else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                } else if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                } else if (strcmp(root->lexeme, "/") == 0) {
                    if(rv == 0 && !find_var_in_right(root -> right)) {
                        error(DIVZERO);
                    }
                    if(rv != 0) {
                        retval = lv / rv;
                    } else {
                        retval = 0;
                    }
                }
                break;
            case INCDEC: {
                lv = evaluateTree(root -> left);
                //rv = evaluateTree(root -> right);
                if(strcmp(root -> lexeme, "++") == 0) {
                    retval = setval(root -> left -> lexeme, lv + 1);
                } else if(strcmp(root -> lexeme, "--") == 0) {
                    retval = setval(root -> left -> lexeme, lv - 1);
                }
                break;
            }
            case AND:
            case OR:
            case XOR: {
                lv = evaluateTree(root -> left);
                rv = evaluateTree(root -> right);
                if(strcmp(root -> lexeme, "&") == 0) {
                    retval = lv & rv;
                } else if(strcmp(root -> lexeme, "|") == 0) {
                    retval = lv | rv;
                } else if(strcmp(root -> lexeme, "^") == 0) {
                    retval = lv ^ rv;
                }
                break;
            }
            default:
                retval = 0;
        }
    }
    return retval;
}

void assembly(BTNode *root) {
    int used_reg = 0, next_used_reg = 0, empty_reg = 0;
    if(root != NULL) {
        if(root -> data == ASSIGN) {
            assembly(root -> right); //right recursion
        }else {
            assembly(root -> left);
            assembly(root -> right);
        }
        switch (root -> data) {
            case ID:
                empty_reg = find_empty_reg();
                reg[empty_reg] = 1;
                printf("MOV r%d, [%d]\n", empty_reg, find_id_in_memory(root -> lexeme));
                break;
            case INT:
                empty_reg = find_empty_reg();
                reg[empty_reg] = 1;
                printf("MOV r%d, %d\n", empty_reg, root -> val);
                break;
            case ADDSUB:
                used_reg = find_used_reg(); //ID or INT at right
                reg[used_reg] = 0;
                next_used_reg = find_used_reg(); //... at left
                printf("%s r%d, r%d\n", strcmp(root -> lexeme, "+") == 0 ? "ADD" : "SUB", next_used_reg, used_reg);
                break;
            case MULDIV:
                used_reg = find_used_reg();
                reg[used_reg] = 0;
                next_used_reg = find_used_reg();
                printf("%s r%d, r%d\n", strcmp(root -> lexeme, "*") == 0 ? "MUL" : "DIV", next_used_reg, used_reg);
                break;
            case ASSIGN:
                //parse through the right child, then assign(MOV) it to the left child(which is a "ID")
                used_reg = find_used_reg();
                //reg[used_reg] = 0;
                printf("MOV [%d], r%d\n", find_id_in_memory(root -> left -> lexeme), used_reg);
                break;
            case INCDEC: {
                used_reg = find_used_reg(); //find reg of the variable
                reg[empty_reg] = 1;
                empty_reg = find_empty_reg();
                reg[empty_reg] = 1;
                printf("MOV r%d, 1\n", empty_reg);
                printf("%s r%d, r%d\n", strcmp(root ->lexeme, "++") == 0 ? "ADD" : "SUB", used_reg, empty_reg);
                reg[empty_reg] = 0;
                printf("MOV [%d], r%d\n", find_id_in_memory(root -> left -> lexeme), used_reg);
                break;
            }
            case AND:
            case OR:
            case XOR: {
                used_reg = find_used_reg(); //right
                reg[used_reg] = 0; //clear after this opertion
                next_used_reg = find_used_reg(); //left
                if(strcmp(root -> lexeme, "&") == 0) {
                    printf("AND ");
                } else if(strcmp(root -> lexeme, "|") == 0) {
                    printf("OR ");
                } else if(strcmp(root -> lexeme, "^") == 0) {
                    printf("XOR ");
                }
                printf("r%d r%d\n", next_used_reg, used_reg);
                break;
            }
            default:
                break;
        }
    }
    return;
}
