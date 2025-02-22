%{
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "ast.h"
#include "similarity.h"
//#include "y.tab.h"
#include "my_parser.h"
//#define YYDEBUG 1
extern int yylineno;
int yylex(void);
extern FILE *yyin;

void yyerror(const char *s);

ASTNode* g_localRoot = NULL;
extern ASTNode* globalASTRoot;
struct timeval begin, end;
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

    ASTNode* localRoot = createASTNode(NodeType_TranslationUnit, NULL);
    if (!localRoot) {
        fprintf(stderr, "Failed to create root node.\n");
        fclose(yyin);
        return NULL;
    }
    
    g_localRoot = localRoot;
    globalASTRoot = localRoot;
    
    fprintf(stderr, "Log: Created local Root for parsing file: %s\n", filename);
    printf("Parsing file: %s\n", filename);
    fprintf(stderr, "Starting parsing process.\n");
    
    gettimeofday(&begin, 0);
    int parseResult = yyparse();
    gettimeofday(&end, 0);
    
    long seconds = end.tv_sec - begin.tv_sec;
    long microseconds = end.tv_usec - begin.tv_usec;
    double elapsed = seconds + microseconds*1e-6;
    
    printf("Total time for parse: %.6f seconds.\n", elapsed);
    printf("Time per parse: %.6f seconds.\n", elapsed/100);
    
    fclose(yyin);
    
    if (parseResult != 0) {
        fprintf(stderr, "Parsing failed with error %d\n", parseResult);
        freeASTNode(localRoot);
        globalASTRoot = NULL;
        return NULL;
    }

    fprintf(stderr, "Finished parsing file: %s\n", filename);
    return g_localRoot;
}

void initFunctionCallTable(FunctionCallTable* table, int size) {
    table->entries = (FunctionCallEntry**)calloc(size, sizeof(FunctionCallEntry*));
    table->size = size;
}

%}

%union {
    ASTNode* ast;
    char* sval;
}

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%token <sval> IDENTIFIER CONSTANT STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN

%token <sval> TYPE_NAME
%token <sval> TYPEDEF EXTERN STATIC AUTO REGISTER
%token <sval> CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token <sval> STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%type <ast> primary_expression postfix_expression argument_expression_list
%type <ast> unary_expression unary_operator cast_expression multiplicative_expression
%type <ast> additive_expression shift_expression relational_expression
%type <ast> equality_expression and_expression exclusive_or_expression
%type <ast> inclusive_or_expression logical_and_expression logical_or_expression
%type <ast> conditional_expression assignment_expression assignment_operator
%type <ast> expression constant_expression declaration declaration_specifiers
%type <ast> init_declarator_list init_declarator storage_class_specifier
%type <ast> type_specifier struct_or_union_specifier struct_or_union
%type <ast> struct_declaration_list struct_declaration specifier_qualifier_list
%type <ast> struct_declarator_list struct_declarator enum_specifier
%type <ast> enumerator_list enumerator type_qualifier declarator direct_declarator
%type <ast> pointer type_qualifier_list parameter_type_list parameter_list
%type <ast> parameter_declaration identifier_list type_name abstract_declarator
%type <ast> direct_abstract_declarator initializer initializer_list statement
%type <ast> labeled_statement compound_statement declaration_list statement_list
%type <ast> expression_statement selection_statement iteration_statement
%type <ast> jump_statement translation_unit external_declaration function_definition

%start translation_unit

%token LOWER_THAN_ELSE

%%

primary_expression
    : IDENTIFIER { $$ = createASTNode(NodeType_Identifier, $1); }
    | CONSTANT { $$ = createASTNode(NodeType_Constant, $1); }
    | STRING_LITERAL { $$ = createASTNode(NodeType_StringLiteral, $1); }
    | '(' expression ')' { $$ = $2; }
    ;

postfix_expression
    : primary_expression { $$ = $1; }
    | postfix_expression '[' expression ']' {
        $$ = createASTNode(NodeType_ArrayAccess, "ArrayAccess");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | postfix_expression '(' ')' {
        $$ = createASTNode(NodeType_FunctionCall, $1->name ? $1->name : "UnnamedFunctionCall");
    }
    | postfix_expression '(' argument_expression_list ')' {
        $$ = createASTNode(NodeType_FunctionCall, $1->name ? $1->name : "UnnamedFunctionCall");
        addASTChild($$, $3);
    }
    | postfix_expression '.' IDENTIFIER {
        $$ = createASTNode(NodeType_StructMember, $3);
        addASTChild($$, $1);
    }
    | postfix_expression PTR_OP IDENTIFIER {
        $$ = createASTNode(NodeType_PointerMember, $3);
        addASTChild($$, $1);
    }
    | postfix_expression INC_OP {
        $$ = createASTNode(NodeType_PostfixExpression, "++");
        addASTChild($$, $1);
    }
    | postfix_expression DEC_OP {
        $$ = createASTNode(NodeType_PostfixExpression, "--");
        addASTChild($$, $1);
    }
    ;

argument_expression_list
    : assignment_expression {
        $$ = createASTNode(NodeType_ArgumentExpressionList, "ArgumentList");
        addASTChild($$, $1);
    }
    | argument_expression_list ',' assignment_expression {
        $$ = $1;
        addASTChild($$, $3);
    }
    ;

unary_expression
    : postfix_expression { $$ = $1; }
    | INC_OP unary_expression {
        $$ = createASTNode(NodeType_UnaryExpression, "++");
        addASTChild($$, $2);
    }
    | DEC_OP unary_expression {
        $$ = createASTNode(NodeType_UnaryExpression, "--");
        addASTChild($$, $2);
    }
    | unary_operator cast_expression {
        $$ = createASTNode(NodeType_UnaryExpression, $1->name ? $1->name : "UnnamedUnaryOperator");
        addASTChild($$, $2);
    }
    | SIZEOF unary_expression {
        $$ = createASTNode(NodeType_UnaryExpression, "sizeof");
        addASTChild($$, $2);
    }
    | SIZEOF '(' type_name ')' {
        $$ = createASTNode(NodeType_UnaryExpression, "sizeof");
        addASTChild($$, $3);
    }
    ;

unary_operator
    : '&' { $$ = createASTNode(NodeType_UnaryOperator, "&"); }
    | '*' { $$ = createASTNode(NodeType_UnaryOperator, "*"); }
    | '+' { $$ = createASTNode(NodeType_UnaryOperator, "+"); }
    | '-' { $$ = createASTNode(NodeType_UnaryOperator, "-"); }
    | '~' { $$ = createASTNode(NodeType_UnaryOperator, "~"); }
    | '!' { $$ = createASTNode(NodeType_UnaryOperator, "!"); }
    ;

cast_expression
    : unary_expression { $$ = $1; }
    | '(' type_name ')' cast_expression {
        $$ = createASTNode(NodeType_CastExpression, "Cast");
        addASTChild($$, $2);
        addASTChild($$, $4);
    }
    ;

multiplicative_expression
    : cast_expression { $$ = $1; }
    | multiplicative_expression '*' cast_expression {
        $$ = createASTNode(NodeType_MultiplicativeExpression, "*");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | multiplicative_expression '/' cast_expression {
        $$ = createASTNode(NodeType_MultiplicativeExpression, "/");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | multiplicative_expression '%' cast_expression {
        $$ = createASTNode(NodeType_MultiplicativeExpression, "%");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression '+' multiplicative_expression {
        $$ = createASTNode(NodeType_AdditiveExpression, "+");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | additive_expression '-' multiplicative_expression {
        $$ = createASTNode(NodeType_AdditiveExpression, "-");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

shift_expression
    : additive_expression { $$ = $1; }
    | shift_expression LEFT_OP additive_expression {
        $$ = createASTNode(NodeType_ShiftExpression, "<<");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | shift_expression RIGHT_OP additive_expression {
        $$ = createASTNode(NodeType_ShiftExpression, ">>");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

relational_expression
    : shift_expression { $$ = $1; }
    | relational_expression '<' shift_expression {
        $$ = createASTNode(NodeType_RelationalExpression, "<");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | relational_expression '>' shift_expression {
        $$ = createASTNode(NodeType_RelationalExpression, ">");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | relational_expression LE_OP shift_expression {
        $$ = createASTNode(NodeType_RelationalExpression, "<=");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | relational_expression GE_OP shift_expression {
        $$ = createASTNode(NodeType_RelationalExpression, ">=");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression EQ_OP relational_expression {
        $$ = createASTNode(NodeType_EqualityExpression, "==");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | equality_expression NE_OP relational_expression {
        $$ = createASTNode(NodeType_EqualityExpression, "!=");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

and_expression
    : equality_expression { $$ = $1; }
    | and_expression '&' equality_expression {
        $$ = createASTNode(NodeType_AndExpression, "&");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

exclusive_or_expression
    : and_expression { $$ = $1; }
    | exclusive_or_expression '^' and_expression {
        $$ = createASTNode(NodeType_ExclusiveOrExpression, "^");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

inclusive_or_expression
    : exclusive_or_expression { $$ = $1; }
    | inclusive_or_expression '|' exclusive_or_expression {
        $$ = createASTNode(NodeType_InclusiveOrExpression, "|");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

logical_and_expression
    : inclusive_or_expression { $$ = $1; }
    | logical_and_expression AND_OP inclusive_or_expression {
        $$ = createASTNode(NodeType_LogicalAndExpression, "&&");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

logical_or_expression
    : logical_and_expression { $$ = $1; }
    | logical_or_expression OR_OP logical_and_expression {
        $$ = createASTNode(NodeType_LogicalOrExpression, "||");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

conditional_expression
    : logical_or_expression { $$ = $1; }
    | logical_or_expression '?' expression ':' conditional_expression {
        $$ = createASTNode(NodeType_ConditionalExpression, "?");
        addASTChild($$, $1);
        addASTChild($$, $3);
        addASTChild($$, $5);
    }
    ;

assignment_expression
    : conditional_expression { $$ = $1; }
    | unary_expression assignment_operator assignment_expression {
        $$ = createASTNode(NodeType_AssignmentExpression, $2->name ? $2->name : "Assignment");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

assignment_operator
    : '=' { $$ = createASTNode(NodeType_AssignmentExpression, "="); }
    | MUL_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, "*="); }
    | DIV_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, "/="); }
    | MOD_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, "%="); }
    | ADD_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, "+="); }
    | SUB_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, "-="); }
    | LEFT_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, "<<="); }
    | RIGHT_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, ">>="); }
    | AND_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, "&="); }
    | XOR_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, "^="); }
    | OR_ASSIGN { $$ = createASTNode(NodeType_AssignmentExpression, "|="); }
    ;

expression
    : assignment_expression { $$ = $1; }
    | expression ',' assignment_expression {
        $$ = createASTNode(NodeType_Expression, $1->name);
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

constant_expression
    : conditional_expression { $$ = $1; }
    ;

declaration
    : declaration_specifiers ';' {
        $$ = createASTNode(NodeType_Declaration, $1->name);
        addASTChild($$, $1);
    }
    | declaration_specifiers init_declarator_list ';' {
        $$ = createASTNode(NodeType_Declaration, $2->name);
        addASTChild($$, $1);
        addASTChild($$, $2);
    }
    ;

declaration_specifiers
    : storage_class_specifier {
        $$ = createASTNode(NodeType_DeclarationSpecifiers, $1->name);
        addASTChild($$, $1);
    }
    | storage_class_specifier declaration_specifiers {
        $$ = $2;
        addASTChild($$, $1);
    }
    | type_specifier {
        $$ = createASTNode(NodeType_DeclarationSpecifiers, $1->name);
        $$->dataType = strdup($1->name);
        addASTChild($$, $1);
    }
    | type_specifier declaration_specifiers {
        $$ = $2;
        if (!$$->dataType) {
        	$$->dataType = strdup($1->name);
         }
        addASTChild($$, $1);
    }
    | type_qualifier {
        $$ = createASTNode(NodeType_DeclarationSpecifiers, $1->name);
        addASTChild($$, $1);
    }
    | type_qualifier declaration_specifiers {
        $$ = $2;
        addASTChild($$, $1);
    }
    ;

init_declarator_list
    : init_declarator {
        $$ = createASTNode(NodeType_InitDeclaratorList, $1->name);
        addASTChild($$, $1);
    }
    | init_declarator_list ',' init_declarator {
        $$ = $1;
        addASTChild($$, $3);
    }
    ;

init_declarator
    : declarator { $$ = $1; }
    | declarator '=' initializer {
        $$ = createASTNode(NodeType_InitDeclarator, $1->name ? $1->name : "InitDeclarator");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

storage_class_specifier
    : TYPEDEF { $$ = createASTNode(NodeType_StorageClassSpecifier, "typedef"); }
    | EXTERN { $$ = createASTNode(NodeType_StorageClassSpecifier, "extern"); }
    | STATIC { $$ = createASTNode(NodeType_StorageClassSpecifier, "static"); }
    | AUTO { $$ = createASTNode(NodeType_StorageClassSpecifier, "auto"); }
    | REGISTER { $$ = createASTNode(NodeType_StorageClassSpecifier, "register"); }
    ;

type_specifier
    : VOID { $$ = createASTNode(NodeType_TypeSpecifier, "void"); $$->name = strdup("void"); }
    | CHAR { $$ = createASTNode(NodeType_TypeSpecifier, "char"); $$->name = strdup("char"); }
    | SHORT { $$ = createASTNode(NodeType_TypeSpecifier, "short"); $$->name = strdup("short"); }
    | INT { $$ = createASTNode(NodeType_TypeSpecifier, "int"); $$->name = strdup("int"); }
    | LONG { $$ = createASTNode(NodeType_TypeSpecifier, "long"); $$->name = strdup("long"); }
    | FLOAT { $$ = createASTNode(NodeType_TypeSpecifier, "float"); $$->name = strdup("float"); }
    | DOUBLE { $$ = createASTNode(NodeType_TypeSpecifier, "double"); $$->name = strdup("double"); }
    | SIGNED { $$ = createASTNode(NodeType_TypeSpecifier, "signed"); $$->name = strdup("signed"); }
    | UNSIGNED { $$ = createASTNode(NodeType_TypeSpecifier, "unsigned"); $$->name = strdup("unsigned"); }
    | struct_or_union_specifier { $$ = $1; }
    | enum_specifier { $$ = $1; }
    | TYPE_NAME { $$ = createASTNode(NodeType_TypeSpecifier, $1); }
    ;

struct_or_union_specifier
    : struct_or_union IDENTIFIER '{' struct_declaration_list '}' {
        $$ = createASTNode(NodeType_StructOrUnionSpecifier, $2);
        addASTChild($$, $1);
        addASTChild($$, $4);
    }
    | struct_or_union '{' struct_declaration_list '}' {
        $$ = createASTNode(NodeType_StructOrUnionSpecifier, "UnnamedStructOrUnion");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | struct_or_union IDENTIFIER {
        $$ = createASTNode(NodeType_StructOrUnionSpecifier, $2);
        addASTChild($$, $1);
    }
    ;

struct_or_union
    : STRUCT { $$ = createASTNode(NodeType_StructOrUnion, "struct"); }
    | UNION { $$ = createASTNode(NodeType_StructOrUnion, "union"); }
    ;

struct_declaration_list
    : struct_declaration {
        $$ = createASTNode(NodeType_StructDeclarationList, "StructDeclarations");
        addASTChild($$, $1);
    }
    | struct_declaration_list struct_declaration {
        $$ = $1;
        addASTChild($$, $2);
    }
    ;

struct_declaration
    : specifier_qualifier_list struct_declarator_list ';' {
        $$ = createASTNode(NodeType_StructDeclaration, "StructDeclaration");
        addASTChild($$, $1);
        addASTChild($$, $2);
    }
    ;

specifier_qualifier_list
    : type_specifier specifier_qualifier_list {
        $$ = $2;
        addASTChild($$, $1);
    }
    | type_specifier {
        $$ = createASTNode(NodeType_SpecifierQualifierList, "TypeSpecifier");
        addASTChild($$, $1);
    }
    | type_qualifier specifier_qualifier_list {
        $$ = $2;
        addASTChild($$, $1);
    }
    | type_qualifier {
        $$ = createASTNode(NodeType_SpecifierQualifierList, "TypeQualifier");
        addASTChild($$, $1);
    }
    ;

struct_declarator_list
    : struct_declarator {
        $$ = createASTNode(NodeType_StructDeclaratorList, "StructDeclaratorList");
        addASTChild($$, $1);
    }
    | struct_declarator_list ',' struct_declarator {
        $$ = $1;
        addASTChild($$, $3);
    }
    ;

struct_declarator
    : declarator { $$ = $1; }
    | ':' constant_expression {
        $$ = createASTNode(NodeType_StructDeclarator, "StructDeclarator");
        addASTChild($$, $2);
    }
    | declarator ':' constant_expression {
        $$ = createASTNode(NodeType_StructDeclarator, $1->name ? $1->name : "UnnamedDeclarator");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    ;

enum_specifier
    : ENUM '{' enumerator_list '}' {
        $$ = createASTNode(NodeType_EnumSpecifier, "UnnamedEnum");
        addASTChild($$, $3);
    }
    | ENUM IDENTIFIER '{' enumerator_list '}' {
        $$ = createASTNode(NodeType_EnumSpecifier, $2);
        addASTChild($$, $4);
    }
    | ENUM IDENTIFIER {
        $$ = createASTNode(NodeType_EnumSpecifier, $2);
    }
    ;

enumerator_list
    : enumerator {
        $$ = createASTNode(NodeType_EnumeratorList, "EnumeratorList");
        addASTChild($$, $1);
    }
    | enumerator_list ',' enumerator {
        $$ = $1;
        addASTChild($$, $3);
    }
    ;

enumerator
    : IDENTIFIER {
        $$ = createASTNode(NodeType_Enumerator, $1);
    }
    | IDENTIFIER '=' constant_expression {
        $$ = createASTNode(NodeType_Enumerator, $1);
        addASTChild($$, $3);
    }
    ;

type_qualifier
    : CONST { $$ = createASTNode(NodeType_TypeQualifier, "const"); }
    | VOLATILE { $$ = createASTNode(NodeType_TypeQualifier, "volatile"); }
    ;

declarator
    : pointer direct_declarator {
        $$ = createASTNode(NodeType_Declarator, $2->name ? $2->name : "PointerDeclarator");
        addASTChild($$, $1);
        addASTChild($$, $2);
    }
    | direct_declarator { $$ = $1; }
    ;

direct_declarator
    : IDENTIFIER 
        {
            $$ = createASTNode(NodeType_DirectDeclarator, $1);
            $$->name = strdup($1);
        }
    | '(' declarator ')' {
        $$ = $2;
    }
    | direct_declarator '[' constant_expression ']' {
        $$ = createASTNode(NodeType_ArrayDeclarator, $1->name ? $1->name : "UnnamedArray");
        addASTChild($$, $1);
        addASTChild($$, $3);
    }
    | direct_declarator '[' ']' {
        $$ = createASTNode(NodeType_ArrayDeclarator, $1->name ? $1->name : "UnnamedArray");
        addASTChild($$, $1);
    }
    | direct_declarator '(' parameter_type_list ')' {
        $$ = createASTNode(NodeType_FunctionDeclarator, $1->name ? $1->name : "UnnamedFunction");
        addASTChild($$, $1);
        addASTChild($$, $3);
        
        if ($3->paramCount > 0) {
            $$->paramCount = $3->paramCount;
            $$->params = $3->params;
        } else {
            $$->paramCount = 0;
            $$->params = NULL;
        }        
        
    }
    | direct_declarator '(' identifier_list ')' {
        $$ = createASTNode(NodeType_FunctionDeclarator, $1->name ? $1->name : "UnnamedFunction");
        addASTChild($$, $1);
        addASTChild($$, $3);
                
        if ($3->paramCount > 0) {
            $$->paramCount = $3->paramCount;
            $$->params = $3->params;
        } else {
            $$->paramCount = 0;
            $$->params = NULL;
        }        
        
    }
    | direct_declarator '(' ')' {
        $$ = createASTNode(NodeType_FunctionDeclarator, $1->name ? $1->name : "UnnamedFunction");
        addASTChild($$, $1);
    }
    ;

pointer
    : '*'
        {
            $$ = createASTNode(NodeType_Pointer, "*");
        }
    | '*' type_qualifier_list
        {
            $$ = createASTNode(NodeType_Pointer, "*");
            addASTChild($$, $2);
        }
    | '*' pointer
        {
            $$ = createASTNode(NodeType_Pointer, "*");
            addASTChild($$, $2);
        }
    | '*' type_qualifier_list pointer
        {
            $$ = createASTNode(NodeType_Pointer, "*");
            addASTChild($$, $2);
            addASTChild($$, $3);
        }
    ;

type_qualifier_list
    : type_qualifier
        {
            $$ = createASTNode(NodeType_TypeQualifierList, "TypeQualifierList");
            addASTChild($$, $1);
        }
    | type_qualifier_list type_qualifier
        {
            $$ = $1;
            addASTChild($$, $2);
        }
    ;

parameter_type_list
    : parameter_list
        { $$ = $1; }
    | parameter_list ',' ELLIPSIS
        {
            $$ = $1;
            ASTNode* ellipsisNode = createASTNode(NodeType_Ellipsis, "...");
            addASTChild($$, ellipsisNode);
        }
    ;

parameter_list
    : parameter_declaration
        {
            $$ = createASTNode(NodeType_ParameterList, "ParameterList");
            $$->paramCount = 1;
            $$->params = (ASTNode**)malloc(sizeof(ASTNode*));
            $$->params[0] = $1;
            addASTChild($$, $1);
        }
    | parameter_list ',' parameter_declaration
        {
            $$ = $1;
            $$->paramCount += 1;
            $$->params = (ASTNode**)realloc($$->params, $$->paramCount * sizeof(ASTNode*));
            $$->params[$$->paramCount - 1] = $3;
            addASTChild($$, $3);
        }
    ;

parameter_declaration
    : declaration_specifiers declarator
        {
            $$ = createASTNode(NodeType_ParameterDeclaration, $2->name ? $2->name : "UnnamedParameter");
            $$->dataType = $1->dataType ? strdup($1->dataType) : strdup("unknown");
            addASTChild($$, $1);
            addASTChild($$, $2);
        }
    | declaration_specifiers abstract_declarator
        {
            $$ = createASTNode(NodeType_ParameterDeclaration, "AbstractParameter");
            addASTChild($$, $1);
            addASTChild($$, $2);
        }
    | declaration_specifiers
        {
            $$ = createASTNode(NodeType_ParameterDeclaration, "UnnamedParameter");
            $$->dataType = $1->dataType ? strdup($1->dataType) : strdup("unknown");
            addASTChild($$, $1);
        }
    ;

identifier_list
    : IDENTIFIER
        {
            $$ = createASTNode(NodeType_IdentifierList, "IdentifierList");
            ASTNode* idNode = createASTNode(NodeType_Identifier, $1);
            addASTChild($$, idNode);
        }
    | identifier_list ',' IDENTIFIER
        {
            $$ = $1;
            ASTNode* idNode = createASTNode(NodeType_Identifier, $3);
            addASTChild($$, idNode);
        }
    ;

type_name
    : specifier_qualifier_list
        { $$ = $1; }
    | specifier_qualifier_list abstract_declarator
        {
            $$ = createASTNode(NodeType_TypeName, "TypeName");
            addASTChild($$, $1);
            addASTChild($$, $2);
        }
    ;

abstract_declarator
    : pointer
        { $$ = $1; }
    | direct_abstract_declarator
        { $$ = $1; }
    | pointer direct_abstract_declarator
        {
            $$ = createASTNode(NodeType_AbstractDeclarator, "AbstractDeclarator");
            addASTChild($$, $1);
            addASTChild($$, $2);
        }
    ;

direct_abstract_declarator
    : '(' abstract_declarator ')'
        { $$ = $2; }
    | '[' ']'
        {
            $$ = createASTNode(NodeType_ArrayDeclarator, "UnnamedArray");
        }
    | '[' constant_expression ']'
        {
            $$ = createASTNode(NodeType_ArrayDeclarator, "UnnamedArray");
            addASTChild($$, $2);
        }
    | direct_abstract_declarator '[' ']'
        {
            $$ = createASTNode(NodeType_ArrayDeclarator, "UnnamedArray");
            addASTChild($$, $1);
        }
    | direct_abstract_declarator '[' constant_expression ']'
        {
            $$ = createASTNode(NodeType_ArrayDeclarator, "UnnamedArray");
            addASTChild($$, $1);
            addASTChild($$, $3);
        }
    | '(' ')'
        {
            $$ = createASTNode(NodeType_FunctionDeclarator, "UnnamedFunction");
        }
    | '(' parameter_type_list ')'
        {
            $$ = createASTNode(NodeType_FunctionDeclarator, "UnnamedFunction");
            addASTChild($$, $2);
        }
    | direct_abstract_declarator '(' ')'
        {
            $$ = createASTNode(NodeType_FunctionDeclarator, "UnnamedFunction");
            addASTChild($$, $1);
        }
    | direct_abstract_declarator '(' parameter_type_list ')'
        {
            $$ = createASTNode(NodeType_FunctionDeclarator, "UnnamedFunction");
            addASTChild($$, $1);
            addASTChild($$, $3);
        }
    ;

initializer
    : assignment_expression
        { $$ = $1; }
    | '{' initializer_list '}'
        {
            $$ = createASTNode(NodeType_InitializerList, "InitializerList");
            addASTChild($$, $2);
        }
    | '{' initializer_list ',' '}'
        {
            $$ = createASTNode(NodeType_InitializerList, "InitializerList");
            addASTChild($$, $2);
        }
    ;

initializer_list
    : initializer
        {
            $$ = createASTNode(NodeType_Initializer, "Initializer");
            addASTChild($$, $1);
        }
    | initializer_list ',' initializer
        {
            $$ = $1;
            addASTChild($$, $3);
        }
    ;

statement
    : labeled_statement
        { $$ = $1; }
    | compound_statement
        { $$ = $1; }
    | expression_statement
        { $$ = $1; }
    | selection_statement
        { $$ = $1; }
    | iteration_statement
        { $$ = $1; }
    | jump_statement
        { $$ = $1; }
    ;

labeled_statement
    : IDENTIFIER ':' statement
        {
            $$ = createASTNode(NodeType_LabeledStatement, $1);
            addASTChild($$, $3);
        }
    | CASE constant_expression ':' statement
        {
            $$ = createASTNode(NodeType_CaseStatement, "CaseStatement");
            addASTChild($$, $2);
            addASTChild($$, $4);
        }
    | DEFAULT ':' statement
        {
            $$ = createASTNode(NodeType_DefaultStatement, "DefaultStatement");
            addASTChild($$, $3);
        }
    ;

compound_statement
    : '{' '}'
        {
            $$ = createASTNode(NodeType_CompoundStatement, "EmptyCompound");
        }
    | '{' statement_list '}'
        {
            $$ = createASTNode(NodeType_CompoundStatement, "CompoundStatement");
            addASTChild($$, $2);
        }
    | '{' declaration_list '}'
        {
            $$ = createASTNode(NodeType_CompoundStatement, "CompoundStatement");
            addASTChild($$, $2);
        }
    | '{' declaration_list statement_list '}'
        {
            $$ = createASTNode(NodeType_CompoundStatement, "CompoundStatement");
            addASTChild($$, $2);
            addASTChild($$, $3);
        }
    ;

declaration_list
    : declaration
        {
            $$ = createASTNode(NodeType_DeclarationList, "DeclarationList");
            addASTChild($$, $1);
        }
    | declaration_list declaration
        {
            $$ = $1;
            addASTChild($$, $2);
        }
    ;

statement_list
    : statement
        {
            $$ = createASTNode(NodeType_StatementList, "StatementList");
            addASTChild($$, $1);
        }
    | statement_list statement
        {
            $$ = $1;
            addASTChild($$, $2);
        }
    ;

expression_statement
    : ';'
        {
            $$ = createASTNode(NodeType_ExpressionStatement, "EmptyExpression");
        }
    | expression ';'
        {
            $$ = createASTNode(NodeType_ExpressionStatement, "ExpressionStatement");
            addASTChild($$, $1);
        }
    ;

selection_statement
    : IF '(' expression ')' statement %prec LOWER_THAN_ELSE
        {
            $$ = createASTNode(NodeType_IfStatement, "IfStatement");
            addASTChild($$, $3); // expression
            addASTChild($$, $5); // statement
        }
    | IF '(' expression ')' statement ELSE statement
        {
            $$ = createASTNode(NodeType_IfElseStatement, "IfElseStatement");
            addASTChild($$, $3); // expression
            addASTChild($$, $5); // if-statement
            addASTChild($$, $7); // else-statement
        }
    | SWITCH '(' expression ')' statement
        {
            $$ = createASTNode(NodeType_SwitchStatement, "SwitchStatement");
            addASTChild($$, $3); // expression
            addASTChild($$, $5); // statement
        }
    ;

iteration_statement
    : WHILE '(' expression ')' statement
        {
            $$ = createASTNode(NodeType_WhileStatement, "WhileStatement");
            addASTChild($$, $3);
            addASTChild($$, $5);
        }
    | DO statement WHILE '(' expression ')' ';'
        {
            $$ = createASTNode(NodeType_DoWhileStatement, "DoWhileStatement");
            addASTChild($$, $2);
            addASTChild($$, $5);
        }
    | FOR '(' expression_statement expression_statement ')' statement
        {
            $$ = createASTNode(NodeType_ForStatement, "ForStatement");
            addASTChild($$, $3); // init
            addASTChild($$, $4); // condition
            addASTChild($$, NULL); // increment (default NULL)
            addASTChild($$, $6); // body
        }
    | FOR '(' expression_statement expression_statement expression ')' statement
        {
            $$ = createASTNode(NodeType_ForStatement, "ForStatement");
            addASTChild($$, $3); // init
            addASTChild($$, $4); // condition
            addASTChild($$, $5); // increment
            addASTChild($$, $7); // body
        }
    ;
    
jump_statement
    : GOTO IDENTIFIER ';'
        {
            $$ = createASTNode(NodeType_GotoStatement, $2);
        }
    | CONTINUE ';'
        {
            $$ = createASTNode(NodeType_ContinueStatement, "ContinueStatement");
        }
    | BREAK ';'
        {
            $$ = createASTNode(NodeType_BreakStatement, "BreakStatement");
        }
    | RETURN ';'
        {
            $$ = createASTNode(NodeType_ReturnStatement, "ReturnStatement");
        }
    | RETURN expression ';'
        {
            $$ = createASTNode(NodeType_ReturnStatement, "ReturnStatement");
            addASTChild($$, $2);
        }
    ;

translation_unit
    : external_declaration
        {
            $$ = createASTNode(NodeType_TranslationUnit, "TranslationUnit");
            addASTChild($$, $1);
            g_localRoot = $$;
        }
    | translation_unit external_declaration
        {
            $$ = $1;
            addASTChild($$, $2);
        }
    ;

external_declaration
    : function_definition
        { $$ = $1; }
    | declaration
        { $$ = $1; }
    ;

function_definition
    : declaration_specifiers declarator declaration_list compound_statement
        {
            $$ = createASTNode(NodeType_FunctionDefinition, $2->name);
            addASTChild($$, $1);
            addASTChild($$, $2);
            addASTChild($$, $3);
            addASTChild($$, $4);
            
            setCurrentFunctionName($2->name);

            // Parametre sayısını ve parametreleri ayarlayın
            if ($2->paramCount > 0) {
                $$->paramCount = $2->paramCount;
                $$->params = $2->params;
            } else {
                $$->paramCount = 0;
                $$->params = NULL;
            }
        }
    | declaration_specifiers declarator compound_statement
        {
            $$ = createASTNode(NodeType_FunctionDefinition, $2->name);
            addASTChild($$, $1);
            addASTChild($$, $2);
            addASTChild($$, $3);
            
            setCurrentFunctionName($2->name);

            // Parametre sayısını ve parametreleri ayarlayın
            if ($2->paramCount > 0) {
                $$->paramCount = $2->paramCount;
                $$->params = $2->params;
            } else {
                $$->paramCount = 0;
                $$->params = NULL;
            }
        }
    | declarator declaration_list compound_statement
        {
            $$ = createASTNode(NodeType_FunctionDefinition, $1->name);
            addASTChild($$, $1);
            addASTChild($$, $2);
            addASTChild($$, $3);
            
            setCurrentFunctionName($2->name);
        }
    | declarator compound_statement
        {
            $$ = createASTNode(NodeType_FunctionDefinition, $1->name);
            addASTChild($$, $1);
            addASTChild($$, $2);
            
            setCurrentFunctionName($2->name);
        }
    ;

%%

int main(int argc, char **argv) {

    MatchTable* matchTable = createMatchTable(256);
    if (!matchTable) {
        fprintf(stderr, "Failed to create match table.\n");
        return 1;
    }
    
    FunctionCallTable callTable;
    initFunctionCallTable(&callTable, 100);
    
    FunctionCallMatchTable* callMatchTable = createFunctionCallMatchTable(100);
    if (!callMatchTable) {
        fprintf(stderr, "Failed to create function call match table.\n");
        return 1;
    }
    
    setCurrentFunctionName("main");
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
        freeASTNode(root1);
        return EXIT_FAILURE;
    }
    
    fclose(file1);
    fclose(file2);

    if (!root1 || !root2) {
        fprintf(stderr, "Error: Parsing failed for one or both files.\n");
        return EXIT_FAILURE;
    }

    printf("AST for %s:\n", argv[1]);
    printAST(root1, 0);
    printf("AST for %s:\n", argv[2]);
    printAST(root2, 0);

    if (!root1 || !root2) {
        fprintf(stderr, "Error: One or both ASTs have no children or failed to parse.\n");
        return EXIT_FAILURE;
    }

    if (root1 && root1->children) {
        for (int i = 0; i < root1->childCount; i++) {
          //  fprintf(stderr, "Child %d of root1 is of type %d\n", i, root1->children[i]->type);
        }
    }
    if (root2 && root2->children) {
        for (int i = 0; i < root2->childCount; i++) {
          //  fprintf(stderr, "Child %d of root2 is of type %d\n", i, root2->children[i]->type);
        }
    }
    
    VarEntry* variableTable1 = NULL;
    VarEntry* variableTable2 = NULL;
    FunctionCallTable* callTable1 = createFunctionCallTable(100);
    FunctionCallTable* callTable2 = createFunctionCallTable(100);

    traverseAST(root1, &variableTable1, callTable1);
    traverseAST(root2, &variableTable2, callTable2);
    
	compareAllFunctions(matchTable, variableTable1, variableTable2, root1, root2, callMatchTable);
	compareFunctionCalls(matchTable, callTable1, callTable2, callMatchTable, variableTable1, variableTable2);
	compareAllFunctions(matchTable, variableTable1, variableTable2, root1, root2, callMatchTable);
	compareAllVariablesInFunctions(matchTable, variableTable1, variableTable2, root1, root2, callMatchTable);

    // printFunctionTable(g_localRoot);
    
    // processFunctionCallsInMain(root1, root2, matchTable, &callTable, callMatchTable);
    
	// traverseASTForFunctionCalls(mainBody1, &callTable, "main");
	// traverseASTForFunctionCalls(mainBody2, &callTable, "main");
    
    // int similarityScore = compareASTs(root1, root2, matchTable);
    
    printf("\nVariable Table for %s:\n", argv[1]);
    printVariableTable(variableTable1);
    printf("\nVariable Table for %s:\n", argv[2]);
    printVariableTable(variableTable2);
    
    printMatchTable(matchTable);
    printFunctionCallTable(callTable1);
    printFunctionCallTable(callTable2);
    printFunctionCallMatchTable(callMatchTable);
    // printf("Total similarity score between %s and %s is: %d\n", argv[1], argv[2], similarityScore);

	ScoreConfig config = loadScoreConfig("score_config.json");
	int totalScore = calculateScore(config, matchTable, callMatchTable, variableTable1, variableTable2);
    printf("\nTotal Score: %d\n", totalScore);

    freeASTNode(root1);
    freeASTNode(root2);
	freeVarTable(variableTable1);
    freeVarTable(variableTable2);
    freeMatchTable(matchTable);
    freeFunctionCallMatchTable(callMatchTable);
    freeFunctionCallTable(callTable1);
    freeFunctionCallTable(callTable2);
    return EXIT_SUCCESS;
}

extern char yytext[];
extern int column;

void yyerror(const char *s) {
    fprintf(stderr, "%s at line %d before '%s'\n", s, yylineno, yytext);
}

