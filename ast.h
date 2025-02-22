#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <string.h>

typedef enum NodeType {
    NodeType_Program,
    NodeType_Root, // Root node type
    NodeType_FunctionDef, // Function definition
    NodeType_MainFunction, // Special node type for the main function
    NodeType_Functions, // Container for function nodes
    NodeType_Body, // Function body
    NodeType_FunctionCall, // Function call
    NodeType_Expression, // Expressions
    NodeType_Parameter, // Function parameters
    NodeType_VariableDecl, // Variable declaration
    NodeType_Assignment, // Variable assignment
    NodeType_Return, // Return statements
    NodeType_Condition, NodeType_IfBody, NodeType_ElseBody, NodeType_WhileBody, NodeType_ForInit, NodeType_ForIncrement, NodeType_ForBody, NodeType_If, NodeType_IfElse, NodeType_While, NodeType_For,
    NodeType_Statements, // Container for multiple statements
    NodeType_ParameterList, // List of parameters
    NodeType_Identifier, // Identifiers
    NodeType_Constant // Constants like numbers
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char* name;
    char* value;
    struct ASTNode* condition;
    struct ASTNode** children;
    int childCount;
    struct ASTNode* parent;
    struct ASTNode** params;
    int paramCount;
    struct ASTNode* body;
    char* dataType;
    struct ASTNode* functions; // Container for functions
    struct ASTNode* mainFunction; // container for main

} ASTNode;

ASTNode* createASTNode(NodeType type, char* value);
ASTNode* createFunctionNode(char* name, ASTNode* paramList, ASTNode* body);
ASTNode* createFunctionCallNode(char* name, ASTNode* params);
ASTNode* createMainFunctionNode(char* name, ASTNode* compoundStatement);
ASTNode* createParameterNode(char* type);
void printAST(ASTNode* node);
int countParams(ASTNode* paramList);
void freeASTNode(ASTNode* node);
void addASTChild(ASTNode* parent, ASTNode* child);
int isFunctionDefinition(ASTNode* node);
int compareASTNodes(ASTNode *node1, ASTNode *node2);
int compareSameTypeNodes(ASTNode *node1, ASTNode *node2);
int compareDifferentTypeNodes(ASTNode* node1, ASTNode* node2);
int compareAssignmentAndFunctionCall(ASTNode* assign, ASTNode* funcCall);
int compareAssignmentAndReturn(ASTNode* assign, ASTNode* ret);
int compareLoopAndConditional(ASTNode* loop, ASTNode* conditional);
int deepCompareASTNodes(ASTNode *node1, ASTNode *node2);
int calculateFunctionSimilarity(ASTNode *node1, ASTNode *node2);
int compareControlStructures(ASTNode* node1, ASTNode* node2);
int compareFunctionSignatures(ASTNode* node1, ASTNode* node2);
int compareReturnStatements(ASTNode *node1, ASTNode *node2);
int compareExpressions(ASTNode *expr1, ASTNode *expr2);
int compareExpressionsIgnoringNames(ASTNode* expr1, ASTNode* expr2);
int compareFunctionBodies(ASTNode *body1, ASTNode *body2);
int recursiveBodyComparison(ASTNode* body1, ASTNode* body2);
int compareFunctionCalls(ASTNode* call1, ASTNode* call2);
int compareMainFunctions(ASTNode *main1, ASTNode *main2);
int compareStatementBlocks(ASTNode* node1, ASTNode* node2);
int genericNodeComparison(ASTNode* node1, ASTNode* node2);
int compareASTs(ASTNode *root1, ASTNode *root2);
int compareParameters(ASTNode** params1, ASTNode** params2, int paramCount1, int paramCount2);
int compareBodies(ASTNode* body1, ASTNode* body2);
int compareBodyNodes(ASTNode* node1, ASTNode* node2);
int compareValues(ASTNode* value1, ASTNode* value2);
int compareAssignments(ASTNode* node1, ASTNode* node2);
int compareFunctionDefinition(ASTNode* node1, ASTNode* node2);
int evaluateFunctionCallContext(ASTNode* call, ASTNode* context);
int isCommutative(const char* opName);
int compareConstants(ASTNode* node1, ASTNode* node2);
int basicSemanticMatch(ASTNode* node1, ASTNode* node2);

#endif

