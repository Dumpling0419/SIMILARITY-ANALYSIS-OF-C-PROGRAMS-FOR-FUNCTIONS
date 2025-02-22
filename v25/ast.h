#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <cjson/cJSON.h>

typedef enum NodeType {
    // Program Structure
    NodeType_TranslationUnit,
    NodeType_ExternalDeclaration,
    NodeType_FunctionDefinition,
    NodeType_Declaration,
    NodeType_DeclarationList,
    NodeType_Declarator,
    NodeType_DirectDeclarator,
    NodeType_Pointer,
    NodeType_TypeQualifierList,
    NodeType_ParameterTypeList,
    NodeType_ParameterList,
    NodeType_ParameterDeclaration,
    NodeType_IdentifierList,
    NodeType_AbstractDeclarator,
    NodeType_DirectAbstractDeclarator,
    NodeType_InitDeclaratorList,
    NodeType_InitDeclarator,
    NodeType_DeclarationSpecifiers,
    NodeType_StorageClassSpecifier,
    NodeType_TypeSpecifier,
    NodeType_StructOrUnionSpecifier,
    NodeType_StructOrUnion,
    NodeType_StructDeclarationList,
    NodeType_StructDeclaration,
    NodeType_SpecifierQualifierList,
    NodeType_StructDeclaratorList,
    NodeType_StructDeclarator,
    NodeType_EnumSpecifier,
    NodeType_EnumeratorList,
    NodeType_Enumerator,
    NodeType_TypeQualifier,
    NodeType_FunctionDeclarator,
    NodeType_ArrayDeclarator,
    NodeType_InitializerList,
    NodeType_Initializer,
    NodeType_CompoundStatement,
    NodeType_ExpressionStatement,
    NodeType_LabeledStatement,
    NodeType_SelectionStatement,
    NodeType_IfStatement,
    NodeType_IfElseStatement,
    NodeType_SwitchStatement,
    NodeType_IterationStatement,
    NodeType_WhileStatement,
    NodeType_DoWhileStatement,
    NodeType_ForStatement,
    NodeType_JumpStatement,
    NodeType_GotoStatement,
    NodeType_ContinueStatement,
    NodeType_BreakStatement,
    NodeType_ReturnStatement,
    NodeType_Expression,
    NodeType_AssignmentExpression,
    NodeType_ConditionalExpression,
    NodeType_LogicalOrExpression,
    NodeType_LogicalAndExpression,
    NodeType_InclusiveOrExpression,
    NodeType_ExclusiveOrExpression,
    NodeType_AndExpression,
    NodeType_EqualityExpression,
    NodeType_RelationalExpression,
    NodeType_ShiftExpression,
    NodeType_AdditiveExpression,
    NodeType_MultiplicativeExpression,
    NodeType_CastExpression,
    NodeType_UnaryExpression,
    NodeType_UnaryOperator,
    NodeType_PostfixExpression,
    NodeType_PrimaryExpression,
    NodeType_ArgumentExpressionList,
    NodeType_FunctionCall,
    NodeType_ArrayAccess,
    NodeType_StructMember,
    NodeType_PointerMember,
    NodeType_Identifier,
    NodeType_Constant,
    NodeType_StringLiteral,
    NodeType_TypeName,
    NodeType_Ellipsis,
    NodeType_CaseStatement,
    NodeType_DefaultStatement,
    NodeType_StatementList,
} NodeType;

// MATCH ENTRY
typedef struct MatchEntry {
    char *varName1;
    char *funcName1;
    char *varName2;
    char *funcName2;
    char* comparisonTag;
    char *universalTag;
    struct MatchEntry* next; // entry for chainin in hash table
} MatchEntry;
typedef struct {
    MatchEntry** entries;
    int size;
} MatchTable;

// FUNCTION CALL ENTRY
typedef struct FunctionCallEntry {
    char *functionName;    // Name of the function being called
    char *location;        // Location, "main" or "functionName"
    char **parameters;     // Array of parameter values or names
    int paramCount;        // Number of parameters
    int lineNumber;        // Line number or position in the function body
    struct FunctionCallEntry* next;  // Pointer to the next entry
} FunctionCallEntry;
typedef struct FunctionCallTable {
    FunctionCallEntry** entries;
    int size;
} FunctionCallTable;

// FUNCTION CALL MATCH ENTRY
typedef struct FunctionCallMatchEntry {
    char *functionCall1;  // First function call
    char *functionCall2;  // Second function call
    int paramMatch;       // Whether parameters are equivalent (1 for yes, 0 for no)
    struct FunctionCallMatchEntry* next;
} FunctionCallMatchEntry;
typedef struct FunctionCallMatchTable {
    FunctionCallMatchEntry** entries;
    int size;
} FunctionCallMatchTable;

typedef struct FunctionCallCounter {
    char* functionName;
    int callCount;
    struct FunctionCallCounter* next;
} FunctionCallCounter;

typedef struct VarEntry {
    char* varName;
    char* functionName;
    char* value;
    int isParameter;
    int isReturn;
    int isInMain;
    int isFunctionCall;
    int callCount;
    struct VarEntry* next;
} VarEntry;

/////////////////////////
//  ***GRADE***
typedef struct TeacherCallParam {
    char type[50]; // "function_call", "constant", "variable_expression"
    int value; // "constant" için kullanılacak
    char expr[256]; // "variable_expression" için
    char fcall_function_name[50]; // "function_call" için fonksiyon adı
    int fcall_param_count; // "function_call" için parametre sayısı
    struct TeacherCallParam* fcall_params; // "function_call" için iç içe parametreler
} TeacherCallParam;

typedef struct {
    char teacher_name[50];    // Öğretmen fonksiyon adı
    int points;               // Puan
    char parameters[10][50];  // Parametre isimleri (maks 10)
    int param_count;          // Parametre sayısı
    char return_var[256];     // Return ifadesinin değişken adı
} FunctionScore;

typedef struct {
    char description[256];  // Açıklama
    char location[50];      // Fonksiyon çağrısının yapıldığı konum (main veya başka fonk)
    int points;             // Bu çağrı için verilecek puan
    struct {
        char function_name[50];  // Öğretmen fonksiyon çağrısı fonksiyon adı
        int param_count;         // Parametre sayısı
        TeacherCallParam params[10]; // Parametreler (max 10). Her biri TeacherCallParam
    } teacher_call;
} FunctionCallScore;

typedef struct {
    char teacher_function[50]; // Öğretmen fonksiyon adı
    char teacher_var[50];      // Öğretmen değişken adı
    int points;                // Bu değişken için verilecek puan
} VariableScore;

typedef struct {
    FunctionScore* functions;
    int func_count;

    FunctionCallScore* function_calls;
    int call_count;

    VariableScore* variables;
    int var_count;
} ScoreConfig;



/////////////////////////////////////
//  ***AST STRUCTURE***
typedef struct ASTNode {
    NodeType type;
    char* name;
    char* value;
    struct ASTNode** children;
    int childCount;
    struct ASTNode* parent;
    struct ASTNode** params;
    struct ASTNode** args;
    int argCount;
    struct ASTNode* next; 
    int paramCount;
    struct ASTNode* body;
    char* dataType;

} ASTNode;
typedef struct {
    ASTNode** nodes;
    int count;
    int capacity;
} ASTNodeList;

/////////////////////////////
// FLATTEN
typedef struct AddTerm {
    int sign;         // +1 ya da -1
    ASTNode* operand; // Bu terimin AST düğümü
} AddTerm;

typedef struct AddTermList {
    AddTerm* terms;
    int count;
    int capacity;
} AddTermList;

// Basit bir dinamik dizi işlevi
static void initAddTermList(AddTermList* list) {
    list->count = 0;
    list->capacity = 4;
    list->terms = (AddTerm*)malloc(sizeof(AddTerm) * list->capacity);
}

static void freeAddTermList(AddTermList* list) {
    free(list->terms);
    list->terms = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void pushAddTerm(AddTermList* list, int sign, ASTNode* operand) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->terms = (AddTerm*)realloc(list->terms, sizeof(AddTerm) * list->capacity);
    }
    list->terms[list->count].sign = sign;
    list->terms[list->count].operand = operand;
    list->count++;
}

extern VarEntry* variableTable;
extern ASTNode* g_localRoot;
extern ASTNode* globalASTRoot;
extern char currentFuncName[256];
extern const char* currentFunctionName(void);

unsigned int hashFunction(const char* str, int tableSize);
ASTNode* findFunctionNode(ASTNode* root, const char* functionName);
ASTNode* createASTNode(NodeType type, const char* name);
void addASTChild(ASTNode* parent, ASTNode* child);
MatchTable* createMatchTable(int size);
FunctionCallTable* createFunctionCallTable(int size);
FunctionCallMatchTable* createFunctionCallMatchTable(int size);
VarEntry* findVariableInFunction(const char* varName, const char* functionName, VarEntry* variableTable);
MatchEntry* findMatchEntry(MatchTable* table, const char* var1, const char* func1, const char* var2, const char* func2);
VarEntry* findReturnExpression(const char* functionName, VarEntry* variableTable);
void printVariableTable(VarEntry* variableTable);
void printMatchTable(MatchTable* table);
void printFunctionCallTable(FunctionCallTable* table);
void printFunctionCallMatchTable(FunctionCallMatchTable* table);
void setCurrentFunctionName(const char* name);
const char* getCurrentFunctionName();
void addVariableToTable(VarEntry** variableTable, const char* varName, const char* functionName, const char* value, int isParameter, int isReturn, int isInMain, int isFunctionCall, int callCount);
void addMatchEntry(MatchTable *table, const char *var1, const char *func1, const char *var2, const char *func2, const char *comparisonTag);
void addFunctionCallEntry(FunctionCallTable* table, const char* functionName, const char* location, char** parameters, int paramCount);
void addFunctionCallMatchEntry(FunctionCallMatchTable* table, const char* funcCall1, const char* funcCall2);
VarEntry* getVariablesByFunction(VarEntry* variableTable, const char* functionName);
char* getVarValue(const char* varName, const char* functionName, VarEntry* variableTable);
void compareFunctionParameters(MatchTable* table, ASTNode* func1, ASTNode* func2);
void compareAllFunctions(MatchTable* table, VarEntry* variableTable1, VarEntry* variableTable2, ASTNode* root1, ASTNode* root2, FunctionCallMatchTable* functionCallMatchTable);
void compareVariablesInFunctions(MatchTable* table, VarEntry* vars1, VarEntry* vars2, const char* funcName1, const char* funcName2, VarEntry* variableTable1, VarEntry* variableTable2, FunctionCallMatchTable* functionCallMatchTable);
void compareAllVariablesInFunctions(MatchTable* matchTable, VarEntry* variableTable1, VarEntry* variableTable2, ASTNode* root1, ASTNode* root2, FunctionCallMatchTable* functionCallMatchTable);
void freeVariableEntries(VarEntry* vars);
int isConstantValue(const char* value);
bool isNumeric(ASTNode* node);
const char* getNodeTypeName(NodeType type);
void printAST(ASTNode* node, int indent);
void freeASTNode(ASTNode* node);
void freeVarTable(VarEntry* variableTable);
void freeMatchTable(MatchTable* table);
void freeFunctionCallTable(FunctionCallTable* table);
void freeFunctionCallMatchTable(FunctionCallMatchTable* table);
char* buildExpressionString(ASTNode* expr);
int compareReturnStatements(VarEntry* return1, VarEntry* return2, const char* funcName1, const char* funcName2, MatchTable* table, VarEntry* variableTable1, VarEntry* variableTable2, FunctionCallMatchTable* functionCallMatchTable);
int compareAssignments(ASTNode* node1, ASTNode* node2, const char* functionName1, const char* functionName2, MatchTable* table, VarEntry* variableTable1, VarEntry* variableTable2, FunctionCallMatchTable* functionCallMatchTable);
int getFunctionCallCount(const char* functionName);
int compareExpressionsInDetail(ASTNode *expr1, ASTNode *expr2, const char *functionName1, const char *functionName2, 
                               MatchTable *table, VarEntry* variableTable1, VarEntry* variableTable2, 
                               int isReturnComparison, int isParameterComparison, FunctionCallMatchTable* functionCallMatchTable, int depth);
int isCommutative(const char* op);

int isBinaryOperator(const char* op);
int isUnaryOperator(const char* op);

void traverseAST(ASTNode* node, VarEntry** variableTable, FunctionCallTable* callTable);





ASTNode* resolveVariableExpression(const char* varName, const char* functionName, VarEntry* variableTable, int depth);
int areParametersMatched(const char* varName1, const char* functionName1, const char* varName2, const char* functionName2,
                         MatchTable* table, VarEntry* variableTable1, VarEntry* variableTable2);
ASTNode* resolveExpressionIdentifiers(ASTNode* expr, const char* functionName, VarEntry* variableTable, int depth);

ASTNode* findReturnStatement(ASTNode* node);
int checkMatchTableForVariables(MatchTable* table, const char* var1, const char* func1, const char* var2, const char* func2, VarEntry* variableTable1, VarEntry* variableTable2);
ASTNode* buildASTNodeFromValue(const char* value);
const char* skipWhitespace(const char* str);
ASTNode* parseExpression(const char** expr);
ASTNode* parseTerm(const char** expr);
ASTNode* parseFactor(const char** expr);
ASTNode* parseFunctionCall(const char** expr);
int checkFunctionCallMatchTable(const char* funcCall1, const char* funcCall2, FunctionCallMatchTable* functionCallMatchTable);
char* buildFunctionCallString(const char* funcName, char** parameters, int paramCount);
int matchReturnExpressions(MatchTable* table, const char* funcName1, const char* funcName2);

void compareFunctionCalls(MatchTable* matchTable, FunctionCallTable* table1, FunctionCallTable* table2, 
                         FunctionCallMatchTable* matchTableCalls, VarEntry* variableTable1, VarEntry* variableTable2);
                         
static void flattenAdditiveExpression(ASTNode* expr, int parentSign, AddTermList* list, 
                                      const char* functionName, VarEntry* variableTable, int depth);
static int compareFlattenedAdditive(AddTermList* list1, AddTermList* list2, 
                                    const char* functionName1, const char* functionName2,
                                    MatchTable* table, VarEntry* varTable1, VarEntry* varTable2,
                                    FunctionCallMatchTable* fcMatch, int depth,
                                    int isReturnComparison, int isParameterComparison);




ScoreConfig loadScoreConfig(const char* filename);
int calculateScore(ScoreConfig config, MatchTable* table, FunctionCallMatchTable* fcmt,
                   VarEntry* variableTable1, VarEntry* variableTable2);

int checkFunctionCallMatch(MatchTable* table, FunctionCallMatchTable* fcmt, VarEntry* variableTable1, VarEntry* variableTable2,
                           ScoreConfig* config, FunctionCallScore* fcScore);
int areVariablesMatched(MatchTable* table, const char* teacherFunction, const char* teacherVar);
int getMatchedStudentFunctionName(MatchTable* table, const char* teacherName, char* studentName, int size);
int getStudentFunctionParams(const char* studentFuncName, VarEntry* variableTable, char paramNames[][50], int maxParams);
char* buildTeacherCallString(const char* funcName, int paramCount, TeacherCallParam* params);
static void safeStringCopy(char* dest, size_t destSize, const char* src);



#endif

