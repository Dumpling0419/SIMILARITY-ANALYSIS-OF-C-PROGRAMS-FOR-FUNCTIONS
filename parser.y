%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ast.h"
#include "similarity.h"
#include "y.tab.h"
//#define YYDEBUG 1
extern int yylineno;
extern char* yytext;
void yyerror(const char *s);
int yylex(void);
extern FILE *yyin;

ASTNode* g_localRoot = NULL;

/*
void parseFunctionDefinitions(ASTNode* root, FILE* file) {
    char functionName[100];
    while (fscanf(file, "%s", functionName) == 1) {
        ASTNode* funcNode = createFunctionNode(functionName, NULL, 0, NULL);
        if (funcNode) {
            addASTChild(root, funcNode);
        }
    }
}*/

ASTNode* parse(const char* filename) {
    yyin = fopen(filename, "r");
    if (!yyin) {
        perror("File opening error");
        return NULL;
    }
    char rootNodeName[256];
    snprintf(rootNodeName, sizeof(rootNodeName), "%s Root", filename);

    ASTNode* localRoot = createASTNode(NodeType_Root, rootNodeName);
    if (!localRoot) {
        fprintf(stderr, "Failed to create root node.\n");
        fclose(yyin);
        return NULL;
    }
    
    g_localRoot = localRoot;
    
    fprintf(stderr, "Log: Created local Root for parsing file: %s\n", filename);
    printf("Parsing file: %s\n", filename);
    fprintf(stderr, "Starting parsing process.\n");
    int parseResult = yyparse();
    fclose(yyin);
    
    if (parseResult != 0) {
        fprintf(stderr, "Parsing failed with error %d\n", parseResult);
        freeASTNode(localRoot);
        return NULL;
    }
    
    /*// Test amaçlı düğümleri doğrudan ekleyin
    ASTNode* functionsNode = createASTNode(NodeType_Functions, "Functions");
    ASTNode* mainNode = createMainFunctionNode("Main", NULL);  // Burada compoundStatementNode yerine NULL geçici olarak kullanıldı
    
    addASTChild(localRoot, functionsNode);
    addASTChild(localRoot, mainNode);*/

    fprintf(stderr, "Finished parsing file: %s\n", filename);
    return g_localRoot;
}

%}

%union {
    char* sval;
    ASTNode* ast;
}

%token <sval> IDENTIFIER NUMBER STRING_LITERAL
%token INT RETURN MAIN IF ELSE WHILE FOR
%token PLUS MINUS TIMES DIVIDE ASSIGN SEMICOLON LPAREN RPAREN COMMA LBRACE RBRACE
%token LT GT LE GE EQ NE AND OR NOT MOD BITAND BITOR XOR SHL SHR

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%left OR
%left AND
%left EQ NE
%left LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE MOD
%right NOT

%type <ast> program main_function function_definition function_call statements statement compound_statement expression parameter_list parameter

%%

program:
    function_definition {
        if (!g_localRoot->functions) {
            g_localRoot->functions = createASTNode(NodeType_Functions, "Functions");
            addASTChild(g_localRoot, g_localRoot->functions);
        }
        addASTChild(g_localRoot->functions, $1);
    }
    | program function_definition {
        if (!$1->functions) {
            g_localRoot->functions = createASTNode(NodeType_Functions, "Functions");
            addASTChild(g_localRoot, g_localRoot->functions);
        }
        addASTChild(g_localRoot->functions, $2);
        $$ = $1;
    }
    | main_function {
        addASTChild(g_localRoot, $1);
    }
    | program main_function {
        addASTChild(g_localRoot, $2);
        $$ = $1;
    }
;

main_function:
    INT MAIN LPAREN RPAREN compound_statement
    {
        $$ = createMainFunctionNode("Main", $5);
        fprintf(stderr, "Log: Created main function node with body\n");
    }
;

compound_statement:
    LBRACE statements RBRACE { $$ = $2; }
;

statements:
    statement
    | statements statement
    {
        addASTChild($1, $2);
        $$ = $1;
    }
;

statement:
    INT IDENTIFIER ASSIGN expression SEMICOLON {
        ASTNode* assign = createASTNode(NodeType_Assignment, "=");
        addASTChild(assign, createASTNode(NodeType_Identifier, $2));
        addASTChild(assign, $4);
        $$ = assign;
    }
    | INT IDENTIFIER ASSIGN function_call SEMICOLON {
        ASTNode* assign = createASTNode(NodeType_Assignment, "=");
        addASTChild(assign, createASTNode(NodeType_Identifier, $2));
        addASTChild(assign, $4);
        $$ = assign;
    }
    | IDENTIFIER ASSIGN expression SEMICOLON {
        ASTNode* assign = createASTNode(NodeType_Assignment, "=");
        addASTChild(assign, createASTNode(NodeType_Identifier, $1));
        addASTChild(assign, $3);
        $$ = assign;
    }
    | IDENTIFIER ASSIGN function_call SEMICOLON {
        ASTNode* assign = createASTNode(NodeType_Assignment, "=");
        addASTChild(assign, createASTNode(NodeType_Identifier, $1));
        addASTChild(assign, $3);
        $$ = assign;
    }
    | RETURN expression SEMICOLON {
        ASTNode* ret = createASTNode(NodeType_Return, "return");
        addASTChild(ret, $2);
        $$ = ret;
    }
    | expression SEMICOLON { $$ = $1; }
    | IF LPAREN expression RPAREN statement %prec LOWER_THAN_ELSE {
        ASTNode* ifCondNode = createASTNode(NodeType_Condition, "ifCond");
        addASTChild(ifCondNode, $3);
        ASTNode* ifBodyNode = createASTNode(NodeType_IfBody, "ifBody");
        addASTChild(ifBodyNode, $5);
        ASTNode* ifNode = createASTNode(NodeType_If, "ifNode");
        addASTChild(ifNode, ifCondNode);
        addASTChild(ifNode, ifBodyNode);
        $$ = ifNode;
    }
    | IF LPAREN expression RPAREN statement ELSE statement {
        ASTNode* ifCondNode = createASTNode(NodeType_Condition, "ifCond");
        addASTChild(ifCondNode, $3);
        ASTNode* ifBodyNode = createASTNode(NodeType_IfBody, "ifBody");
        addASTChild(ifBodyNode, $5);
        ASTNode* elseBodyNode = createASTNode(NodeType_ElseBody, "elseBody");
        addASTChild(elseBodyNode, $7);
        ASTNode* ifElseNode = createASTNode(NodeType_IfElse, "ifeLseNode");
        addASTChild(ifElseNode, ifCondNode);
        addASTChild(ifElseNode, ifBodyNode);
        addASTChild(ifElseNode, elseBodyNode);
        $$ = ifElseNode;
    }
    | WHILE LPAREN expression RPAREN statement {
        ASTNode* whileCondNode = createASTNode(NodeType_Condition, "whileCond");
        addASTChild(whileCondNode, $3);
        ASTNode* whileBodyNode = createASTNode(NodeType_WhileBody, "whileBody");
        addASTChild(whileBodyNode, $5);
        ASTNode* whileNode = createASTNode(NodeType_While, "whileNode");
        addASTChild(whileNode, whileCondNode);
        addASTChild(whileNode, whileBodyNode);
        $$ = whileNode;
    }
    | FOR LPAREN expression SEMICOLON expression SEMICOLON expression RPAREN statement {
        ASTNode* forInitNode = createASTNode(NodeType_ForInit, "forInit");
        addASTChild(forInitNode, $3);
        ASTNode* forCondNode = createASTNode(NodeType_Condition, "forCond");
        addASTChild(forCondNode, $5);
        ASTNode* forIncrNode = createASTNode(NodeType_ForIncrement, "forIncr");
        addASTChild(forIncrNode, $7);
        ASTNode* forBodyNode = createASTNode(NodeType_ForBody, "forBody");
        addASTChild(forBodyNode, $9);
        ASTNode* forNode = createASTNode(NodeType_For, "forNode");
        addASTChild(forNode, forInitNode);
        addASTChild(forNode, forCondNode);
        addASTChild(forNode, forIncrNode);
        addASTChild(forNode, forBodyNode);
        $$ = forNode;
    }
    | function_definition { $$ = $1; }
    | function_call { $$ = $1; }
    ;

function_definition:
    INT IDENTIFIER LPAREN parameter_list RPAREN compound_statement
    {
        $$ = createFunctionNode($2, $4, $6);
        fprintf(stderr, "Log: Created function node for %s with parameters and body\n", $2);
    }
;
    
function_call:
    IDENTIFIER LPAREN parameter_list RPAREN {
        $$ = createFunctionCallNode($1, $3);
        fprintf(stderr, "Log: Created function call node: %s with %d args\n", $1, countParams($3));
        if (!$$) {
            fprintf(stderr, "Failed to create function call node for %s\n", $1);
        }
    }
    ;

parameter:
    INT IDENTIFIER {
        char fullType[1024];
        snprintf(fullType, sizeof(fullType), "int %s", $2);
        $$ = createParameterNode(fullType);
    }
    | IDENTIFIER { $$ = createParameterNode($1); }
    | NUMBER { $$ = createParameterNode($1); }
    ;

parameter_list:
    { 
        $$ = createASTNode(NodeType_ParameterList, "Empty Param List");
    }
    | parameter {
        ASTNode* pList = createASTNode(NodeType_ParameterList, "Param List");
        addASTChild(pList, $1);
        $$ = pList;
    }
    | parameter_list COMMA parameter {
        addASTChild($1, $3);
        $$ = $1;
    }
;

expression:
    IDENTIFIER { $$ = createASTNode(NodeType_Identifier, $1); }
    | NUMBER { $$ = createASTNode(NodeType_Constant, $1); }
    | LPAREN expression RPAREN { $$ = $2; }
    | expression PLUS expression {
        ASTNode* plusExpr = createASTNode(NodeType_Expression, "+");
        addASTChild(plusExpr, $1);
        addASTChild(plusExpr, $3);
        $$ = plusExpr;
    }
    | expression MINUS expression {
        ASTNode* minusExpr = createASTNode(NodeType_Expression, "-");
        addASTChild(minusExpr, $1);
        addASTChild(minusExpr, $3);
        $$ = minusExpr;
    }
    | expression TIMES expression {
        ASTNode* timesExpr = createASTNode(NodeType_Expression, "*");
        addASTChild(timesExpr, $1);
        addASTChild(timesExpr, $3);
        $$ = timesExpr;
    }
    | expression DIVIDE expression {
        ASTNode* divideExpr = createASTNode(NodeType_Expression, "/");
        addASTChild(divideExpr, $1);
        addASTChild(divideExpr, $3);
        $$ = divideExpr;
    }
    ;
%%

int main(int argc, char **argv) {
    //yydebug = 1;
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file1.c> <file2.c>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE* file1 = fopen(argv[1], "r");
    if (!file1) {
        perror("Failed to open file 1");
        return EXIT_FAILURE;
    }
    FILE* file2 = fopen(argv[2], "r");
    if (!file2) {
        perror("Failed to open file 2");
        fclose(file1);
        return EXIT_FAILURE;
    }
    
    ASTNode* root1 = parse(argv[1]);
    if (!root1) {
        fprintf(stderr, "Error: Parsing failed for %s.\n", argv[1]);
        return EXIT_FAILURE;
    }

    ASTNode* root2 = parse(argv[2]);
    if (!root2) {
        fprintf(stderr, "Error: Parsing failed for %s.\n", argv[2]);
        freeASTNode(root1);  // İlk ağacı serbest bırakın
        return EXIT_FAILURE;
    }
    
    fclose(file1);
    fclose(file2);

    if (!root1 || !root2) {
        fprintf(stderr, "Error: Parsing failed for one or both files.\n");
        return EXIT_FAILURE;
    }

    printf("AST for %s:\n", argv[1]);
    printAST(root1);
    printf("AST for %s:\n", argv[2]);
    printAST(root2);

    if (!root1 || !root2) {
        fprintf(stderr, "Error: One or both ASTs have no children or failed to parse.\n");
        return EXIT_FAILURE;
    }

    if (root1 && root1->children) {
        for (int i = 0; i < root1->childCount; i++) {
            fprintf(stderr, "Child %d of root1 is of type %d\n", i, root1->children[i]->type);
        }
    }
    if (root2 && root2->children) {
        for (int i = 0; i < root2->childCount; i++) {
            fprintf(stderr, "Child %d of root2 is of type %d\n", i, root2->children[i]->type);
        }
    }

    int similarityScore = compareASTs(root1, root2);
    printf("Total similarity score between %s and %s is: %d\n", argv[1], argv[2], similarityScore);

    freeASTNode(root1);
    freeASTNode(root2);

    return EXIT_SUCCESS;
}

void yyerror(const char *s) {
    fprintf(stderr, "%s at line %d before '%s'\n", s, yylineno, yytext);
}

