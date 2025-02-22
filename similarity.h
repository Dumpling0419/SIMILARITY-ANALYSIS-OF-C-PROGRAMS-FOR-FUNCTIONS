#ifndef SIMILARITY_H
#define SIMILARITY_H

#include "ast.h"

int compareASTNodes(ASTNode* node1, ASTNode* node2, MatchTable* matchTable);

int compareAssignments(ASTNode* node1, ASTNode* node2, const char* functionName1, const char* functionName2, MatchTable* table);
int compareFunctionBodies(ASTNode *body1, ASTNode *body2, const char *functionName1, const char *functionName2, MatchTable *matchTable);

#endif // SIMILARITY_H

