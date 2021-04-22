#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "codeGen.h"

int sbcount = 0;

Symbol table[TBLSIZE], curr_table[TBLSIZE]; // store variables in the table before the "assign" operation

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    sbcount = 3;
}

int getval(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0)
            return table[i].val;

    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    
    strcpy(table[sbcount].name, str);
    table[sbcount].val = 0;
    sbcount++;
    return 0;
}

int setval(char *str, int val) {
    int i = 0;

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            table[i].val = val;
            return val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    
    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    sbcount++;
    return val;
}

int find_new_var_right(BTNode *root) {
    if(root != NULL) {
        if(root -> data == ID) {
            int found = 0;
            for(int i = 0; i < TBLSIZE; i++) {
                if(strcmp(root -> lexeme, curr_table[i].name) == 0) {
                    found = 1;
                }
            }
            if(!found){
                //new_var_in_right = 1;
                return 1;
            }
        }
        if(find_new_var_right(root -> left)) return 1;
        if(find_new_var_right(root -> right)) return 1;
    }
    return 0;
}

BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)calloc(1, sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    //node->left = NULL;
    //node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// statement := ENDFILE | END | assign_expr END
void statement(void) {
    for(int i = 0; i <= 7; i++) {
        reg[i] = 0;
    }
    BTNode *retp = NULL;
    if (match(ENDFILE)) { //store x, y, z into r0, r1, r2 respectively after EOF
        for(int i = 0; i <= 2; i++){
            printf("MOV r%d, [%d]\n", i, i * 4);
        }
        printf("EXIT 0\n");
        exit(0);
    } else if (match(END)) { //input is empty
        advance();
    } else {
        //retp = expr(); //parsing starts here
        retp = assign_expr();
        if (match(END)) {
            printPrefix(retp); printf("\n");
            evaluateTree(retp);
            assembly(retp); //func. to print ASSEMBLY code
            freeTree(retp);
            advance();
        } else {
            error(SYNTAXERR);
        }
    }
}

// assign_expr := ID ASSIGN assign_expr | or_expr
BTNode *assign_expr(void) {
    BTNode *retp = NULL, *node = NULL;
    node = or_expr();
    if(node -> data == ID) {
        if (match(ASSIGN)) {
                retp = makeNode(ASSIGN, getLexeme());
                for(int i = 0; i < TBLSIZE; i++) {
                    strcpy(curr_table[i].name, table[i].name);
                }
                advance();
                retp -> right = assign_expr(); //recursion process
                retp -> left = node;
                //
                if(find_new_var_right(retp -> right)) {
                    error(UNDEFINED);
                }
                //
        } else {
            retp = node;
        }
    } else {
        retp = node;
    }
    return retp;
}

// or_expr := xor_expr or_expr_tail
BTNode *or_expr(void) {
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}
// or_expr_tail := OR xor_expr or_expr_tail | NiL
BTNode *or_expr_tail(BTNode *left) {
    BTNode *node = NULL;
    if(match(OR)) {
        node = makeNode(OR, getLexeme());
        advance();
        node -> left = left;
        node -> right = xor_expr();
        return or_expr_tail(node);
    } else {
        return left; // NiL
    }
}

// xor_expr := and_expr xor_expr_tail
BTNode *xor_expr(void) {
    BTNode *node = and_expr();
    return xor_expr_tail(node);
    
}
// xor_expr_tail := XOR and_expr xor_expr_tail | NiL
BTNode *xor_expr_tail(BTNode *left) {
    BTNode *node = NULL;
    if(match(XOR)) {
        node = makeNode(XOR, getLexeme());
        advance();
        node -> left = left;
        node -> right = and_expr();
        return xor_expr_tail(node);
    } else {
        return left;
    }
}

// and_expr := addsub_expr and_expr_tail
BTNode *and_expr(void) {
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}
// and_expr_tail := AND addsub_expr and_expr_tail | NiL
BTNode *and_expr_tail(BTNode *left) {
    BTNode *node = NULL;
    if(match(AND)) {
        node = makeNode(AND, getLexeme());
        advance();
        node -> left = left;
        node -> right = addsub_expr();
        return and_expr_tail(node);
    } else {
        return left;
    }
}

// addsub_expr := muldiv_expr addsub_expr_tail
BTNode *addsub_expr(void) {
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}
// addsub_expr_tail := ADDSUB muldiv_expr addsub_expr_tail | NiL
BTNode *addsub_expr_tail(BTNode *left) {
    BTNode *node = NULL;
    if(match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node -> left = left;
        node -> right = muldiv_expr();
        return addsub_expr_tail(node);
    } else {
        return left;
    }
}

// muldiv_expr := unary_expr muldiv_expr_tail
BTNode *muldiv_expr(void) {
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}
// muldiv_expr_tail := MULDIV unary_expr muldiv_expr_tail | NiL
BTNode *muldiv_expr_tail(BTNode *left) {
    BTNode *node = NULL;
    if(match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node -> left = left;
        node -> right = unary_expr();
        return muldiv_expr_tail(node);
    } else {
        return left;
    }
}

// unary_expr := ADDSUB unary_expr | factor
BTNode *unary_expr(void) {
    BTNode *node = NULL, *left = NULL;
    if(match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node -> right = unary_expr();
        left = makeNode(INT, "0");
        left -> val = 0;
        node -> left = left;
        //return left;
        return node;
    } else {
        return factor();
    }
}

// factor := INT | ID | INCDEC ID | LPAREN assign_expr RPAREN
BTNode *factor(void) {
    BTNode *retp = NULL;
    
    if(match(INT)) {
        retp = makeNode(INT, getLexeme());
        retp -> val = atoi(getLexeme());
        advance();
    } else if(match(ID)) {
        retp = makeNode(ID, getLexeme());
        retp -> val = getval(getLexeme());
        advance();
    } else if (match(INCDEC)) {
        retp = makeNode(INCDEC, getLexeme());
        //retp -> right = makeNode(INT, "0");
        //retp -> right -> val = 0;
        advance();
        if(match(ID)) {
            retp -> left = makeNode(ID, getLexeme());
            advance();
        } else {
            error(SYNTAXERR);
        }
    } else if(match(LPAREN)) {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            error(MISPAREN);
    } else {
        error(NOTNUMID);
    }
    return retp;
}

void erro(ErrorType errorNum) {
    printf("EXIT 1\n");
    if (PRINTERR) {
        fprintf(stderr, "error: ");
        switch (errorNum) {
            case MISPAREN:
                fprintf(stderr, "mismatched parenthesis\n");
                break;
            case NOTNUMID:
                fprintf(stderr, "number or identifier expected\n");
                break;
            case NOTFOUND:
                fprintf(stderr, "variable not defined\n");
                break;
            case RUNOUT:
                fprintf(stderr, "out of memory\n");
                break;
            case NOTLVAL:
                fprintf(stderr, "lvalue required as an operand\n");
                break;
            case DIVZERO:
                fprintf(stderr, "divide by constant zero\n");
                break;
            case SYNTAXERR:
                fprintf(stderr, "syntax error\n");
                break;
            default:
                fprintf(stderr, "undefined error\n");
                break;
        }
    }
    exit(0);
}
