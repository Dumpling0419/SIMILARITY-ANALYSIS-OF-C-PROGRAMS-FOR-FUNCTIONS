
/*
void matchFunctionCalls(FunctionCallTable* callTable, FunctionCallMatchTable* matchTable, MatchTable* matchEntries) {
    // Iterate over the call table and check for matches
    for (int i = 0; i < callTable->size; i++) {
        FunctionCallEntry* entry1 = callTable->entries[i];
        while (entry1 != NULL) {
            // Compare with other entries (in another file's call table)
            for (int j = i + 1; j < callTable->size; j++) {
                FunctionCallEntry* entry2 = callTable->entries[j];
                while (entry2 != NULL) {
                    // Check if these calls match
                    if (findFunctionReturnMatch(matchEntries, entry1->functionName, entry2->functionName)) {
                        // Add to match table if parameters also match
                        int paramMatch = compareFunctionCallParameters(matchEntries, entry1, entry2);
                        if (paramMatch) {
                            addFunctionCallMatch(matchTable, entry1, entry2, 1);
                        }
                    }
                    entry2 = entry2->next;
                }
            }
            entry1 = entry1->next;
        }
    }
}

void addFunctionCallMatch(FunctionCallMatchTable* matchTable, FunctionCallEntry* call1, FunctionCallEntry* call2, int paramMatch) {
    // Create a new match entry
    FunctionCallMatchEntry* newMatch = (FunctionCallMatchEntry*)malloc(sizeof(FunctionCallMatchEntry));
    newMatch->functionCall1 = strdup(call1->functionName);
    newMatch->functionCall2 = strdup(call2->functionName);
    newMatch->paramMatch = paramMatch;
    newMatch->next = NULL;

    // Add the entry to the match table
    int index = hashFunction(newMatch->functionCall1, matchTable->size);
    if (matchTable->entries[index] == NULL) {
        matchTable->entries[index] = newMatch;
    } else {
        FunctionCallMatchEntry* current = matchTable->entries[index];
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newMatch;
    }
}

int findFunctionReturnMatch(MatchTable* matchTable, const char* func1, const char* func2) {
    MatchEntry* entry = findMatchEntry(matchTable, func1, func2);

    if (entry != NULL && strcmp(entry->funcName1, func1) == 0 && strcmp(entry->funcName2, func2) == 0) {
        fprintf(stderr, "Match found in MatchTable for %s and %s with ReturnMatch.\n", func1, func2);
        return 1;  // Functions Match with ReturnMatch
    }

    fprintf(stderr, "No Match found in MatchTable for %s and %s with ReturnMatch.\n", func1, func2);
    return 0;  // No match
}

MatchEntry* findMatchEntry(MatchTable* table, const char* call1, const char* call2) {
    int index = hashFunction(call1, table->size);
    MatchEntry* entry = table->entries[index];
    while (entry) {
        if ((strcmp(entry->varName1, call1) == 0 && strcmp(entry->varName2, call2) == 0) ||
            (strcmp(entry->varName1, call2) == 0 && strcmp(entry->varName2, call1) == 0)) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

int compareFunctionCallParameters(MatchTable* matchTable, FunctionCallEntry* func1, FunctionCallEntry* func2) {
    // Parametre sayılarının eşleşip eşleşmediğini kontrol et
    if (func1->paramCount != func2->paramCount) {
        fprintf(stderr, "Parameter count mismatch: %s has %d parameters, %s has %d parameters.\n",
                func1->functionName, func1->paramCount, func2->functionName, func2->paramCount);
        return 0;  // Parametre sayıları eşleşmiyor
    }

    // Her bir parametreyi karşılaştır
    for (int i = 0; i < func1->paramCount; i++) {
        fprintf(stderr, "Comparing parameter %d of %s and %s.\n", i + 1, func1->functionName, func2->functionName);

        // Parametrelerin sayısal değer olup olmadığını kontrol et
        char* param1 = func1->parameters[i];
        char* param2 = func2->parameters[i];

        if (param1 != NULL && param2 != NULL && isdigit(param1[0]) && isdigit(param2[0])) {
            int value1 = atoi(param1);
            int value2 = atoi(param2);
            fprintf(stderr, "Both parameters are numeric: %d and %d.\n", value1, value2);
            // Eğer her iki parametre de sayıysa, sayısal olarak karşılaştır
            if (value1 != value2) {
                fprintf(stderr, "Numeric mismatch: %d != %d.\n", value1, value2);
                return 0;  // Sayılar eşleşmiyor
            }
        } else {
            // Eğer değişkense, MatchTable'daki değişken denkliklerine bakılır
            fprintf(stderr, "Checking variable match for parameter %d of %s and %s.\n", i + 1, func1->functionName, func2->functionName);
            if (!compareASTNodes((ASTNode*)param1, (ASTNode*)param2, matchTable)) {
                fprintf(stderr, "Variable mismatch for parameter %d of %s and %s.\n", i + 1, func1->functionName, func2->functionName);
                return 0;  // Parametreler eşleşmiyor
            }
        }
    }

    fprintf(stderr, "All parameters matched between %s and %s.\n", func1->functionName, func2->functionName);
    return 1;  // Parametreler eşleşiyor
}

void processFunctionCallsInMain(ASTNode* root1, ASTNode* root2, MatchTable* matchTable, FunctionCallTable* callTable, FunctionCallMatchTable* callMatchTable) {
    // Find the main functions in both roots
    ASTNode* mainFunc1 = findFunctionNode(root1, "main");
    ASTNode* mainFunc2 = findFunctionNode(root2, "main");

    if (!mainFunc1 || !mainFunc2) {
        fprintf(stderr, "Error: One or both main functions not found.\n");
        return;
    }

    // Get the bodies of the main functions
    ASTNode* mainBody1 = NULL;
    ASTNode* mainBody2 = NULL;

    if (mainFunc1->childCount > 0) {
        mainBody1 = mainFunc1->children[mainFunc1->childCount - 1];
        if (mainBody1->type != NodeType_CompoundStatement) {
            fprintf(stderr, "Error: main function in first file does not have a CompoundStatement as its body.\n");
            return;
        }
    } else {
        fprintf(stderr, "Error: main function in first file has no children.\n");
        return;
    }

    if (mainFunc2->childCount > 0) {
        mainBody2 = mainFunc2->children[mainFunc2->childCount - 1];
        if (mainBody2->type != NodeType_CompoundStatement) {
            fprintf(stderr, "Error: main function in second file does not have a CompoundStatement as its body.\n");
            return;
        }
    } else {
        fprintf(stderr, "Error: main function in second file has no children.\n");
        return;
    }

    // Add function calls from mainBody1 to callTable
    for (int i = 0; i < mainBody1->childCount; i++) {
        ASTNode* child1 = mainBody1->children[i];
        if (child1 && child1->type == NodeType_ExpressionStatement) {
            if (child1->childCount > 0) {
                ASTNode* expr = child1->children[0];
                // Check for assignment expressions with function calls on the right side
                if (expr->type == NodeType_AssignmentExpression && expr->childCount > 1) {
                    ASTNode* rightSide = expr->children[1];
                    if (rightSide->type == NodeType_FunctionCall) {
                        const char* functionName = rightSide->name;
                        const char* location = "main";
                        char** parameters = extractParameters(rightSide);
                        int paramCount = getParamCount(rightSide);
                        // int lineNumber = rightSide->lineNumber; // Kaldırıldı

                        addFunctionCallEntry(callTable, functionName, location, parameters, paramCount);
                    }
                } else if (expr->type == NodeType_FunctionCall) { // Standalone function call
                    const char* functionName = expr->name;
                    const char* location = "main";
                    char** parameters = extractParameters(expr);
                    int paramCount = getParamCount(expr);
                    // int lineNumber = expr->lineNumber; // Kaldırıldı

                    addFunctionCallEntry(callTable, functionName, location, parameters, paramCount);
                }
            }
        }
    }

    // Add function calls from mainBody2 to callTable
    for (int j = 0; j < mainBody2->childCount; j++) {
        ASTNode* child2 = mainBody2->children[j];
        if (child2 && child2->type == NodeType_ExpressionStatement) {
            if (child2->childCount > 0) {
                ASTNode* expr = child2->children[0];
                // Check for assignment expressions with function calls on the right side
                if (expr->type == NodeType_AssignmentExpression && expr->childCount > 1) {
                    ASTNode* rightSide = expr->children[1];
                    if (rightSide->type == NodeType_FunctionCall) {
                        const char* functionName = rightSide->name;
                        const char* location = "main";
                        char** parameters = extractParameters(rightSide);
                        int paramCount = getParamCount(rightSide);
                        // int lineNumber = rightSide->lineNumber; // Kaldırıldı

                        addFunctionCallEntry(callTable, functionName, location, parameters, paramCount);
                    }
                } else if (expr->type == NodeType_FunctionCall) { // Standalone function call
                    const char* functionName = expr->name;
                    const char* location = "main";
                    char** parameters = extractParameters(expr);
                    int paramCount = getParamCount(expr);
                    // int lineNumber = expr->lineNumber; // Kaldırıldı

                    addFunctionCallEntry(callTable, functionName, location, parameters, paramCount);
                }
            }
        }
    }

    // Match the function calls
    if (callTable && callMatchTable) {
        matchFunctionCalls(callTable, callMatchTable, matchTable);
    } else {
        fprintf(stderr, "Error: callTable or callMatchTable is NULL.\n");
    }

    fprintf(stderr, "Completed processing function calls in both main functions.\n");
}

char** extractParameters(ASTNode* callNode) {
    if (callNode->childCount == 0) return NULL;
    ASTNode* argListNode = callNode->children[0];
    if (!argListNode) return NULL;

    char** params = malloc(argListNode->childCount * sizeof(char*));
    if (!params) {
        fprintf(stderr, "Memory allocation failed for parameters.\n");
        return NULL;
    }

    for (int i = 0; i < argListNode->childCount; i++) {
        ASTNode* arg = argListNode->children[i];
        params[i] = arg->name ? strdup(arg->name) : strdup("unknown");
    }
    return params;
}

int getParamCount(ASTNode* callNode) {
    if (callNode->childCount == 0) return 0;
    ASTNode* argListNode = callNode->children[0];
    return argListNode ? argListNode->childCount : 0;
}


*/






/*

ASTNode* createFunctionNode(char* name, ASTNode* paramList, ASTNode* body);
ASTNode* createFunctionCallNode(char* name, ASTNode* params);
ASTNode* createMainFunctionNode(char* name, ASTNode* compoundStatement);
ASTNode* createParameterNode(char* type);

ASTNode* findReturnNode(ASTNode* node);
ASTNode* findAssignment(ASTNode* node);

int countParams(ASTNode* paramList);

void handleFunctionCall(char* functionName, ASTNode** args, int argCount);

int isFunctionDefinition(ASTNode* node);
int compareASTNodes(ASTNode* node1, ASTNode* node2, MatchTable* matchTable);
int compareSameTypeNodes(ASTNode* node1, ASTNode* node2, MatchTable* MatchTable);
int compareDifferentTypeNodes(ASTNode* node1, ASTNode* node2);
int compareAssignmentAndFunctionCall(ASTNode* assign, ASTNode* funcCall);
int compareAssignmentAndReturn(ASTNode* assign, ASTNode* ret);
int compareLoopAndConditional(ASTNode* loop, ASTNode* conditional);
int deepCompareASTNodes(ASTNode *node1, ASTNode *node2);
int calculateFunctionSimilarity(ASTNode *node1, ASTNode *node2, MatchTable *matchTable);
int compareControlStructures(ASTNode *node1, ASTNode *node2, MatchTable *matchTable);
int compareFunctionSignatures(MatchTable* matchTable, ASTNode* node1, ASTNode* node2);

int compareExpressions(ASTNode *expr1, ASTNode *expr2);
int compareExpressionsIgnoringNames(ASTNode* expr1, ASTNode* expr2);
int compareFunctionBodies(MatchTable* table, ASTNode* body1, ASTNode* body2, const char* funcName1, const char* funcName2);
int recursiveBodyComparison(ASTNode* body1, ASTNode* body2);
int compareASTFunctionCalls(ASTNode* call1, ASTNode* call2);
int compareMainFunctions(ASTNode *main1, ASTNode *main2, MatchTable *matchTable);
int compareStatementBlocks(ASTNode* node1, ASTNode* node2);
int genericNodeComparison(ASTNode* node1, ASTNode* node2);
int compareASTs(ASTNode* root1, ASTNode* root2, MatchTable* matchTable);
int compareParameters(ASTNode** params1, ASTNode** params2, int paramCount1, int paramCount2, MatchTable *matchTable);
int compareBodies(ASTNode* body1, ASTNode* body2, MatchTable *matchTable);
int compareBodyNodes(ASTNode* node1, ASTNode* node2, MatchTable *matchTable);
int compareValues(ASTNode* value1, ASTNode* value2);

int compareFunctionDefinition(MatchTable* matchTable, ASTNode* node1, ASTNode* node2);
int evaluateFunctionCallContext(ASTNode* call, ASTNode* context);

int compareConstants(ASTNode* node1, ASTNode* node2);
int basicSemanticMatch(ASTNode* node1, ASTNode* node2);

int countArgs(ASTNode* args);

const char* getCurrentFunctionNameForNode(ASTNode* node);
void storeReturnValue(const char* functionName, int callCount, int returnValue);

void handleAssignment(ASTNode* node, VarEntry** variableTable, const char* functionName);

void processAllFunctionsForAssignments(ASTNode* root);
void processAssignmentsInAST(ASTNode* node, const char* functionName);
void processFunctionCallsInMain(ASTNode* root1, ASTNode* root2, MatchTable* matchTable, FunctionCallTable* callTable, FunctionCallMatchTable* callMatchTable);
void addFunctionCallMatchEntry(FunctionCallMatchTable* table, const char* funcCall1, const char* funcCall2, const char* matchedParams);
int compareFunctionCallsInFunctions(const char* funcCall1, const char* funcCall2, MatchTable* matchTable);
void printFunctionCallMatchTable(FunctionCallMatchTable* table);

void processFunctionCalls(ASTNode* node, FunctionCallTable* callTable, const char* currentFunction);

char** extractParameters(ASTNode* callNode);
int getParamCount(ASTNode* callNode);
ASTNode* getMainBody(ASTNode* root);
void traverseASTForFunctionCalls(ASTNode* node, FunctionCallTable* callTable, const char* currentFunction);
void processFunctionParameters(ASTNode* functionNode, VarEntry** variableTable, const char* functionName);
bool isAssignment(ASTNode* node);

*/









