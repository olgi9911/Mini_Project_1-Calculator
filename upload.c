#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define TBLSIZE 1024
#define MAXLEN 256
// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 0

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    if (PRINTERR) \
        fprintf(stderr, "error() called at %s:%d: \n", __FILE__, __LINE__); \
    erro(errorNum); \
}

// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR
} ErrorType;

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

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
} Symbol;

// Token types
typedef enum {
    UNKNOWN, END, ENDFILE, 
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN, 
    LPAREN, RPAREN,
    INCDEC,
    AND, OR, XOR
} TokenSet;

static TokenSet getToken(void);
static TokenSet curToken = UNKNOWN;
static char lexeme[MAXLEN]; //256

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left; 
    struct _Node *right;
} BTNode;

void statement(void);
BTNode *assign_expr(void);
BTNode *or_expr(void);
BTNode *or_expr_tail(BTNode *left);
BTNode *xor_expr(void);
BTNode *xor_expr_tail(BTNode *left);
BTNode *and_expr(void);
BTNode *and_expr_tail(BTNode *left);
BTNode *addsub_expr(void);
BTNode *addsub_expr_tail(BTNode *left);
BTNode *muldiv_expr(void);
BTNode *muldiv_expr_tail(BTNode *left);
BTNode *unary_expr(void);
BTNode *factor(void);
BTNode *term(void);
BTNode *term_tail(BTNode *left);
BTNode *expr(void);
BTNode *expr_tail(BTNode *left);

int evaluateTree(BTNode *root);
void assembly(BTNode *root);
/*
void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}*/

TokenSet getToken(void)
{
    int i = 0, j = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t');

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if (isalpha(c) || c == '_') {
        lexeme[0] = c;
        c = fgetc(stdin);
        j = 1;
        while((isalpha(c) || isdigit(c) || c == '_') && j < MAXLEN) {
            lexeme[j] = c;
            ++j;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[j] = '\0';
        return ID;
    } else if (c == '+' || c == '-') {
        lexeme[0] = c;
        if(c == '+') {
            c = fgetc(stdin);
            if(c == '+') {
                lexeme[1] = c;
                lexeme[2] = '\0';
                return INCDEC;
            }else {
                ungetc(c, stdin);
                lexeme[1] = '\0';
                return ADDSUB;
            }
        }else if(c == '-') {
            c = fgetc(stdin);
            if(c == '-') {
                lexeme[1] = c;
                lexeme[2] = '\0';
                return INCDEC;
            }else {
                ungetc(c, stdin);
                lexeme[1] = '\0';
                return ADDSUB;
            }
        }
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (c == '&') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return AND;
    } else if (c == '|') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return OR;
    } else if (c == '^') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return XOR;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == EOF) {
        return ENDFILE;
    } else {
        return UNKNOWN;
    }
    return UNKNOWN;
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void) {
    return lexeme;
}

int sbcount = 0;
Symbol table[TBLSIZE];
Symbol curr_table[TBLSIZE]; // store variables in the table before the "assign" operation

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



int reg[8] = {};

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
            //printPrefix(retp); printf("\n");
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
        //retp -> right = makeNode(INT, "1");
        //retp -> right -> val = 1;
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
    int used_reg, next_used_reg, idx = 0;
    if(root != NULL) {
        if(root -> data == ASSIGN) {
            assembly(root -> right); //right recursion
        }else {
            assembly(root -> left);
            assembly(root -> right);
        }
        switch (root -> data) {
            case ID:
                idx = find_empty_reg();
                reg[idx] = 1;
                printf("MOV r%d, [%d]\n", idx, find_id_in_memory(root -> lexeme));
                break;
            case INT:
                idx = find_empty_reg();
                reg[idx] = 1;
                printf("MOV r%d, %d\n", idx, root -> val);
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
                idx = find_used_reg();
                reg[idx] = 1;
                //printf("MOV r%d, [%d]\n", idx, find_id_in_memory(root -> left -> lexeme));
                int empty = find_empty_reg();
                reg[empty] = 1;
                printf("MOV r%d, 1\n", empty);
                printf("%s r%d, r%d\n", strcmp(root ->lexeme, "++") == 0 ? "ADD" : "SUB", idx, empty);
                reg[empty] = 0;
                printf("MOV [%d], r%d\n", find_id_in_memory(root -> left -> lexeme), idx);
                //printf("MOV r%d, [%d]\n", idx, find_id_in_memory(root -> left -> lexeme));
                //reg[idx] = 0;
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

int main() {
    //freopen("input.txt", "w", stdout);
    initTable();
    while (1) {
        statement();
    }
    return 0;
}