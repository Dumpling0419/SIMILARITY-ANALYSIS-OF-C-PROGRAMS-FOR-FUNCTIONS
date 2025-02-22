#ifndef SIMILARITY_H
#define SIMILARITY_H

#include "ast.h"
#include <cjson/cJSON.h>

int compareASTNodes(ASTNode* node1, ASTNode* node2, MatchTable* matchTable);

int compareAssignments(ASTNode* node1, ASTNode* node2, const char* functionName1, const char* functionName2, MatchTable* table, VarEntry* variableTable1, VarEntry* variableTable2, FunctionCallMatchTable* functionCallMatchTable);
int compareFunctionBodies(MatchTable* table, ASTNode* body1, ASTNode* body2, const char* funcName1, const char* funcName2, VarEntry* variableTable1, VarEntry* variableTable2, FunctionCallMatchTable* functionCallMatchTable);

#endif // SIMILARITY_H

