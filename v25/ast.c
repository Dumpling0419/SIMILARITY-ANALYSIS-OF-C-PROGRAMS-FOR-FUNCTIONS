#include <stdio.h>
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

ASTNode* globalASTRoot = NULL;

const int initial_capacity = 10;

int max(int a, int b) {
    return (a > b) ? a : b;
}
int min(int a, int b) {
    return (a < b) ? a : b;
}
static char* currentFunctionNameValue = NULL;
char currentFuncName[256] = "unknown";
VarEntry* variableTable = NULL;

VarEntry varTable[100];
int varTableSize = 0;

FunctionCallCounter* functionCallCounters = NULL;

unsigned int hashFunction(const char* str, int size) {
    unsigned int hash = 0;
    while (*str)
        hash = (hash * 31) + *str++;
    return hash % size;
}

ASTNode* findFunctionNode(ASTNode* root, const char* functionName) {
    if (root == NULL) {
        fprintf(stderr, "findFunctionNode: root is NULL\n");
        return NULL;
    }
    if (functionName == NULL) {
        fprintf(stderr, "findFunctionNode: functionName is NULL\n");
        return NULL;
    }
    
    if ((root->type == NodeType_FunctionDefinition) && strcmp(root->name, functionName) == 0) {
        return root;
    }
    
    for (int i = 0; i < root->childCount; i++) {
        ASTNode* found = findFunctionNode(root->children[i], functionName);
        if (found != NULL) {
            return found;
        }
    }
    return NULL;
}

ASTNode* createASTNode(NodeType type, const char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for ASTNode.\n");
        return NULL;
    }
    node->type = type;
    node->name = name ? strdup(name) : NULL;
    node->childCount = 0;
    node->children = NULL;
    return node;
}

void addASTChild(ASTNode* parent, ASTNode* child) {
    if (!parent || !child) return;
    parent->childCount++;
    parent->children = (ASTNode**)realloc(parent->children, parent->childCount * sizeof(ASTNode*));
    parent->children[parent->childCount - 1] = child;
}

MatchTable* createMatchTable(int size) {
    MatchTable* table = (MatchTable*)malloc(sizeof(MatchTable));
    if (!table) {
        fprintf(stderr, "Memory allocation failed for MatchTable.\n");
        return NULL;
    }
    table->size = size;
    table->entries = (MatchEntry**)calloc(size, sizeof(MatchEntry*));
    if (!table->entries) {
        fprintf(stderr, "Memory allocation failed for MatchTable entries.\n");
        free(table);
        return NULL;
    }
    return table;
}

FunctionCallTable* createFunctionCallTable(int size) {
    FunctionCallTable* table = (FunctionCallTable*)malloc(sizeof(FunctionCallTable));
    table->entries = (FunctionCallEntry**)malloc(sizeof(FunctionCallEntry*) * size);
    for (int i = 0; i < size; i++) {
        table->entries[i] = NULL;
    }
    table->size = size;
    return table;
}

FunctionCallMatchTable* createFunctionCallMatchTable(int size) {
    FunctionCallMatchTable* table = malloc(sizeof(FunctionCallMatchTable));
    if (!table) {
        fprintf(stderr, "Memory allocation failed for FunctionCallMatchTable.\n");
        return NULL;
    }
    table->size = size;
    table->entries = calloc(size, sizeof(FunctionCallMatchEntry*));
    if (!table->entries) {
        fprintf(stderr, "Memory allocation failed for FunctionCallMatchTable entries.\n");
        free(table);
        return NULL;
    }
    return table;
}

VarEntry* findVariableInFunction(const char* varName, const char* functionName, VarEntry* variableTable) {
    VarEntry* current = variableTable;
    while (current) {
        if (strcmp(current->varName, varName) == 0 && strcmp(current->functionName, functionName) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

VarEntry* findReturnExpression(const char* functionName, VarEntry* variableTable) {
    char returnExprName[256];
    snprintf(returnExprName, sizeof(returnExprName), "Return Expression in %s", functionName);
    
    VarEntry* current = variableTable;
    while (current != NULL) {
        if (strcmp(current->varName, returnExprName) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

MatchEntry* findMatchEntry(MatchTable* table, const char* var1, const char* func1, const char* var2, const char* func2) {
    if (!table || !var1 || !func1 || !var2 || !func2) {
        fprintf(stderr, "[findMatchEntry] Invalid arguments.\n");
        return NULL;
    }

    if (table->size <= 0 || !table->entries) {
        fprintf(stderr, "[findMatchEntry] Empty MatchTable.\n");
        return NULL;
    }
    
    unsigned int index = hashFunction(var1, table->size);
    MatchEntry* current = table->entries[index];
    while (current) {
        if ((strcmp(current->varName1, var1) == 0 && strcmp(current->funcName1, func1) == 0 &&
             strcmp(current->varName2, var2) == 0 && strcmp(current->funcName2, func2) == 0) ||
            (strcmp(current->varName1, var2) == 0 && strcmp(current->funcName1, func2) == 0 &&
             strcmp(current->varName2, var1) == 0 && strcmp(current->funcName2, func1) == 0)) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void printVariableTable(VarEntry* variableTable) {
    VarEntry* current = variableTable;
    printf("\nPrinting Variable Table:\n");
    while (current) {
        if (current->isReturn) {
            printf("%s has value %s [Return]",
                   current->varName, current->value ? current->value : "NULL");
        } else {
            printf("Variable %s in %s has value %s", 
                   current->varName, current->functionName, current->value ? current->value : "NULL");

            if (current->isParameter) {
                printf(" [Parameter]");
            }
            if (current->isInMain) {
                printf(" [In Main]");
            }
            if (current->isFunctionCall) {
                printf(" [Function Call]");
            }
        }
        printf("\n");
        current = current->next;
    }
}

void printMatchTable(MatchTable* table) {
    printf("\nPrinting Match Table:\n");
    int hasEntries = 0;
    for (int i = 0; i < table->size; i++) {
        MatchEntry* entry = table->entries[i];
        while (entry) {
            printf("Entry at index %d: %s(%s) matches %s(%s) [%s]\n",
                   i,
                   entry->funcName1,
                   entry->varName1,
                   entry->funcName2,
                   entry->varName2,
                   entry->comparisonTag);
            entry = entry->next;
            hasEntries = 1;
        }
    }
    if (!hasEntries) {
        printf("No entries in the Match Table.\n");
    }
}

void printFunctionCallTable(FunctionCallTable* table) {
    printf("\nPrinting Function Call Table:\n");
    for (int i = 0; i < table->size; i++) {
        FunctionCallEntry* entry = table->entries[i];
        while (entry) {
            printf("Function Call in '%s', function '%s' with '%d' parameter(s), respectively parameters are ", entry->location, entry->functionName, entry->paramCount);
            for (int j = 0; j < entry->paramCount; j++) {
                printf("'%s'", entry->parameters[j]);
                if (j < entry->paramCount - 1) printf(", ");
            }
            printf(".\n");
            entry = entry->next;
        }
    }
}

void printFunctionCallMatchTable(FunctionCallMatchTable* table) {
    if (!table) {
        printf("Function Call Match Table is NULL.\n");
        return;
    }
    
    printf("\nPrinting Function Call Match Table:\n");
    int hasEntries = 0;
    for (int i = 0; i < table->size; i++) {
        FunctionCallMatchEntry* entry = table->entries[i];
        while (entry) {
            printf("Match Entry: %s <-> %s\n", entry->functionCall1, entry->functionCall2);
            entry = entry->next;
            hasEntries = 1;
        }
    }
    if (!hasEntries) {
        printf("No matches found in Function Call Match Table.\n");
    }
}

void setCurrentFunctionName(const char* name) {
    if (name && strlen(name) > 0) {
        strncpy(currentFuncName, name, sizeof(currentFuncName) - 1);
        currentFuncName[sizeof(currentFuncName) - 1] = '\0';
    } else {
        fprintf(stderr, "setCurrentFunctionName: Function name is NULL or empty, setting to 'unknown'.\n");
        strncpy(currentFuncName, "unknown", sizeof(currentFuncName));
    }
}

const char* getCurrentFunctionName() {
    return currentFuncName;
}

void addVariableToTable(VarEntry** variableTable, const char* varName, const char* value, const char* functionName, int isParameter, int isReturn, int isInMain, int isFunctionCall, int callCount) {

    if (functionName == NULL || strcmp(functionName, "unknown") == 0) {
        fprintf(stderr, "Function name missing or 'unknown'. Attempting to retrieve from context.\n");
        functionName = getCurrentFunctionName();

        if (strcmp(functionName, "unknown") == 0) {
            fprintf(stderr, "Warning: Could not retrieve a valid function name. Variable %s may be misassigned.\n", varName);
        } else {
            fprintf(stderr, "Retrieved function name: %s for variable %s\n", functionName, varName);
        }
    }

    if (varName == NULL || functionName == NULL) {
        fprintf(stderr, "Error: Null pointer supplied for varName or functionName in addVariableToTable. varName: %s, functionName: %s\n", 
                varName ? varName : "NULL", functionName ? functionName : "NULL");
        return;
    }

    // Check if the variable already exists in the function's scope
    VarEntry* existingEntry = findVariableInFunction(varName, functionName, *variableTable);
    if (existingEntry) {
    
        if (existingEntry->value) {
            free(existingEntry->value);
        }
        existingEntry->value = value ? strdup(value) : NULL;

        return;
    }

    VarEntry* newEntry = malloc(sizeof(VarEntry));
    if (!newEntry) {
        fprintf(stderr, "Memory allocation failed for variable entry %s in function %s.\n", varName, functionName);
        return;
    }

    newEntry->varName = strdup(varName);
    newEntry->functionName = strdup(functionName);
    newEntry->value = value ? strdup(value) : NULL;
    newEntry->isParameter = isParameter;
    newEntry->isReturn = isReturn;
    newEntry->isInMain = isInMain;
    newEntry->isFunctionCall = isFunctionCall;
    newEntry->callCount = callCount;
    newEntry->next = *variableTable;
    *variableTable = newEntry;

}

void addMatchEntry(MatchTable* table, const char* var1, const char* func1, const char* var2, const char* func2, const char* comparisonTag) {
    MatchEntry* existingEntry = findMatchEntry(table, var1, func1, var2, func2);
    if (existingEntry) {
        // comparisonTag güncelle
        size_t newTagLen = strlen(existingEntry->comparisonTag) + strlen(comparisonTag) + 3;
        char* updatedTag = (char*)malloc(newTagLen);
        if (!updatedTag) {
            fprintf(stderr, "Memory allocation failed for updatedTag.\n");
            exit(EXIT_FAILURE);
        }
        snprintf(updatedTag, newTagLen, "%s, %s", existingEntry->comparisonTag, comparisonTag);
        free(existingEntry->comparisonTag);
        existingEntry->comparisonTag = updatedTag;

    } else {
        // Yeni MatchEntry
        unsigned int index = hashFunction(var1, table->size);
        MatchEntry* newEntry = (MatchEntry*)malloc(sizeof(MatchEntry));
        if (!newEntry) {
            fprintf(stderr, "Memory allocation failed for MatchEntry.\n");
            exit(EXIT_FAILURE);
        }
        newEntry->varName1 = strdup(var1);
        newEntry->funcName1 = strdup(func1);
        newEntry->varName2 = strdup(var2);
        newEntry->funcName2 = strdup(func2);
        newEntry->comparisonTag = strdup(comparisonTag);
        newEntry->next = table->entries[index];
        table->entries[index] = newEntry;

    }
}

void addFunctionCallEntry(FunctionCallTable* table, const char* functionName, const char* location, char** parameters, int paramCount) {
    if (strcmp(location, "global") == 0) {
        return;
    }

    int index = hashFunction(functionName, table->size);

    // fonksiyon çağrısını tabloya eklemeden önce kontrol et
    FunctionCallEntry* current = table->entries[index];
    while (current) {
        if (strcmp(current->functionName, functionName) == 0 &&
            strcmp(current->location, location) == 0 &&
            current->paramCount == paramCount) {
            int paramsMatch = 1;
            for (int i = 0; i < paramCount; i++) {
                if (strcmp(current->parameters[i], parameters[i]) != 0) {
                    paramsMatch = 0;
                    break;
                }
            }
            if (paramsMatch) {
                return;
            }
        }
        current = current->next;
    }

    // Yeni FunctionCallEntry
    FunctionCallEntry* newEntry = (FunctionCallEntry*)malloc(sizeof(FunctionCallEntry));
    if (!newEntry) {
        fprintf(stderr, "Memory allocation failed for FunctionCallEntry.\n");
        return;
    }
    newEntry->functionName = strdup(functionName);
    newEntry->location = strdup(location);
    newEntry->parameters = (char**)malloc(paramCount * sizeof(char*));
    for (int i = 0; i < paramCount; ++i) {
        newEntry->parameters[i] = strdup(parameters[i]);
    }
    newEntry->paramCount = paramCount;
    newEntry->lineNumber = 0;
    newEntry->next = table->entries[index];

    table->entries[index] = newEntry;
}

void addFunctionCallMatchEntry(FunctionCallMatchTable* table, const char* funcCall1, const char* funcCall2) {
    if (!table || !funcCall1 || !funcCall2) {
        fprintf(stderr, "addFunctionCallMatchEntry: Invalid arguments.\n");
        return;
    }
    
    if (checkFunctionCallMatchTable(funcCall1, funcCall2, table)) {
        fprintf(stderr, "addFunctionCallMatchEntry: Match between '%s' and '%s' already exists in FunctionCallMatchTable.\n", funcCall1, funcCall2);
        return; // Eşleşme zaten varsa, ekleme yapma
    }
    
    unsigned int index = hashFunction(funcCall1, table->size);
    FunctionCallMatchEntry* newEntry = malloc(sizeof(FunctionCallMatchEntry));
    if (!newEntry) {
        fprintf(stderr, "Memory allocation failed for FunctionCallMatchEntry.\n");
        return;
    }
    newEntry->functionCall1 = strdup(funcCall1);
    newEntry->functionCall2 = strdup(funcCall2);
    newEntry->next = table->entries[index];
    table->entries[index] = newEntry;
    
}

VarEntry* getVariablesByFunction(VarEntry* variableTable, const char* functionName) {
    VarEntry* result = NULL;
    VarEntry* tail = NULL;
    VarEntry* entry = variableTable;
    while (entry) {
        if (strcmp(entry->functionName, functionName) == 0) {
            VarEntry* newEntry = (VarEntry*)malloc(sizeof(VarEntry));
            memcpy(newEntry, entry, sizeof(VarEntry));
            newEntry->varName = strdup(entry->varName);
            newEntry->functionName = strdup(entry->functionName);
            newEntry->value = entry->value ? strdup(entry->value) : NULL;
            newEntry->next = NULL;
            if (!result) {
                result = newEntry;
                tail = newEntry;
            } else {
                tail->next = newEntry;
                tail = newEntry;
            }
        }
        entry = entry->next;
    }
    return result;
}

char* getVarValue(const char* varName, const char* functionName, VarEntry* variableTable) {
    VarEntry* current = variableTable;
    while (current) {
        if (strcmp(current->varName, varName) == 0 && strcmp(current->functionName, functionName) == 0) {
            if (current->value) {
                fprintf(stderr, "getVarValue: Found value '%s' for variable '%s' in function '%s'.\n",
                        current->value, varName, functionName);
                return current->value;
            } else {
                fprintf(stderr, "getVarValue: Variable '%s' in function '%s' has no value set.\n", varName, functionName);
                return NULL;
            }
        }
        current = current->next;
    }
    fprintf(stderr, "getVarValue: Variable '%s' not found in function '%s'.\n", varName, functionName);
    return NULL;
}

void compareFunctionParameters(MatchTable* table, ASTNode* func1, ASTNode* func2) {
    if (func1->paramCount != func2->paramCount) return;

    int matched = 1;
    int* checked = (int*)calloc(func2->paramCount, sizeof(int)); // Karşılaştırılan parametreleri işaretle

    for (int i = 0; i < func1->paramCount; i++) {
        int foundMatch = 0;

        for (int j = 0; j < func2->paramCount; j++) {
            //fprintf(stderr, "Comparing with Function %s Parameter %d: name = %s, dataType = %s\n",
            //        func2->name, j, func2->params[j]->name, func2->params[j]->dataType);

            if (!checked[j] && strcmp(func1->params[i]->dataType, func2->params[j]->dataType) == 0) {
                addMatchEntry(table, func1->params[i]->name, func1->name, func2->params[j]->name, func2->name, "FuncParamMatch");

                foundMatch = 1;
                checked[j] = 1;
                break;
            }
        }

        if (!foundMatch) {
            matched = 0;
            break;
        }
    }

    free(checked);
}


void compareAllFunctions(MatchTable* table, VarEntry* variableTable1, VarEntry* variableTable2, ASTNode* root1, ASTNode* root2, FunctionCallMatchTable* functionCallMatchTable) {
    if (!root1 || !root2) return;

    for (int i = 0; i < root1->childCount; i++) {
        ASTNode* func1 = root1->children[i];
        if (func1->type == NodeType_FunctionDefinition && strcmp(func1->name, "main") != 0) {
            for (int j = 0; j < root2->childCount; j++) {
                ASTNode* func2 = root2->children[j];
                if (func2->type == NodeType_FunctionDefinition && strcmp(func2->name, "main") != 0) {

                    compareFunctionParameters(table, func1, func2);
                    compareAllVariablesInFunctions(table, variableTable1, variableTable2, func1, func2, functionCallMatchTable);
                }
            }
        }
    }
    for (int i = 0; i < root1->childCount; i++) {
        ASTNode* func1 = root1->children[i];
        if (func1->type == NodeType_FunctionDefinition && strcmp(func1->name, "main") == 0) {
            for (int j = 0; j < root2->childCount; j++) {
                ASTNode* func2 = root2->children[j];
                if (func2->type == NodeType_FunctionDefinition && strcmp(func2->name, "main") == 0) {

                    compareFunctionParameters(table, func1, func2);
                    compareAllVariablesInFunctions(table, variableTable1, variableTable2, func1, func2, functionCallMatchTable);
                }
            }
        }
    }
}

void compareVariablesInFunctions(MatchTable* table, VarEntry* vars1, VarEntry* vars2, 
                                 const char* funcName1, const char* funcName2, 
                                 VarEntry* variableTable1, VarEntry* variableTable2, FunctionCallMatchTable* functionCallMatchTable) {
    VarEntry* v1 = vars1;
    VarEntry* return1 = NULL;
    VarEntry* return2 = NULL;

    // İlk önce return ifadelerini ayır
    while (v1) {
        if (v1->isReturn) {
            return1 = v1;
            break;
        }
        v1 = v1->next;
    }

    VarEntry* v2 = vars2;
    while (v2) {
        if (v2->isReturn) {
            return2 = v2;
            break;
        }
        v2 = v2->next;
    }

    // diğer değişkenleri karşılaştır
    v1 = vars1;
    while (v1) {
        if (v1->isReturn) {
            v1 = v1->next;
            continue;
        }
        VarEntry* v2_iter = vars2;
        while (v2_iter) {
            if (v2_iter->isReturn) {
                v2_iter = v2_iter->next;
                continue; // Returnleri atla
            }

            if (v1->value && v2_iter->value) {
                // Sabit değerler için kıyaslama
                if (isConstantValue(v1->value) && isConstantValue(v2_iter->value)) {
                    if (strcmp(v1->value, v2_iter->value) == 0) {
                        char comparisonTag[256];
                        snprintf(comparisonTag, sizeof(comparisonTag), "VariableValueMatch (%s)", v1->value);
                        addMatchEntry(table, v1->varName, funcName1, v2_iter->varName, funcName2, 
                                     comparisonTag);
                    }
                } else {
                    // İfadeler için kıyaslama
                    ASTNode* exprAST1 = buildASTNodeFromValue(v1->value);
                    ASTNode* exprAST2 = buildASTNodeFromValue(v2_iter->value);

                    if (exprAST1 && exprAST2) {
                        int eval1 = compareExpressionsInDetail(exprAST1, exprAST2, 
                                                               funcName1, funcName2, table, 
                                                               variableTable1, variableTable2, 0, 0, functionCallMatchTable, 0);
                        if (eval1 > 0) {
                            char comparisonTag[256];
                            snprintf(comparisonTag, sizeof(comparisonTag), "ExpressionValueMatch");
                            addMatchEntry(table, v1->varName, funcName1, v2_iter->varName, funcName2, 
                                         comparisonTag);
                        }
                        freeASTNode(exprAST1);
                        freeASTNode(exprAST2);
                    }
                }
            }
            v2_iter = v2_iter->next;
        }
        v1 = v1->next;
    }

    if (return1 && return2) {
        compareReturnStatements(return1, return2, funcName1, funcName2, table, variableTable1, variableTable2, functionCallMatchTable);
    }
}

void compareAllVariablesInFunctions(MatchTable* matchTable, VarEntry* variableTable1, VarEntry* variableTable2, ASTNode* func1, ASTNode* func2, FunctionCallMatchTable* functionCallMatchTable) {
    if (!func1 || !func2) return;

    VarEntry* vars1 = getVariablesByFunction(variableTable1, func1->name);
    VarEntry* vars2 = getVariablesByFunction(variableTable2, func2->name);

    compareVariablesInFunctions(matchTable, vars1, vars2, func1->name, func2->name, variableTable1, variableTable2, functionCallMatchTable);

    freeVariableEntries(vars1);
    freeVariableEntries(vars2);
}

void compareFunctionCalls(MatchTable* matchTable, FunctionCallTable* table1, FunctionCallTable* table2, 
                         FunctionCallMatchTable* matchTableCalls, VarEntry* variableTable1, VarEntry* variableTable2) {
    if (!matchTable || !table1 || !table2 || !matchTableCalls) {
        fprintf(stderr, "compareFunctionCalls: One or more tables are NULL.\n");
        return;
    }
    
    for (int i = 0; i < table1->size; i++) {
        FunctionCallEntry* call1 = table1->entries[i];
        while (call1) {
            const char* calledFunc1 = call1->functionName;
            
            for (int j = 0; j < table2->size; j++) {
                FunctionCallEntry* call2 = table2->entries[j];
                while (call2) {
                    const char* calledFunc2 = call2->functionName;
                    bool returnMatch = false;
                    
                    char returnVar1[256];
                    snprintf(returnVar1, sizeof(returnVar1), "Return Expression in %s", calledFunc1);
                    
                    char returnVar2[256];
                    snprintf(returnVar2, sizeof(returnVar2), "Return Expression in %s", calledFunc2);
                    
                    if (checkMatchTableForVariables(matchTable, returnVar1, calledFunc1, returnVar2, calledFunc2, 
                                                   variableTable1, variableTable2)) {
                        returnMatch = true;
                    }
                    
                    if (!returnMatch) {
                        call2 = call2->next;
                        continue;
                    }
                    
                    bool sameLocation = false;
                    if (strcmp(call1->location, call2->location) == 0 || 
                        (strcmp(call1->location, "main") == 0 && strcmp(call2->location, "main") == 0)) {
                        sameLocation = true;
                    }
                    if (!sameLocation) {
                        call2 = call2->next;
                        continue;
                    }
                    
                    if (call1->paramCount != call2->paramCount) {
                        call2 = call2->next;
                        continue;
                    }
                    
                    bool paramsMatch = true;
                    for (int p = 0; p < call1->paramCount; p++) {
                        const char* param1 = call1->parameters[p];
                        const char* param2 = call2->parameters[p];

                        if (isdigit(param1[0]) && isdigit(param2[0])) {
                            if (strcmp(param1, param2) != 0) {
                                paramsMatch = false;
                                break;
                            }
                        } else if (!checkMatchTableForVariables(matchTable, param1, call1->location, param2, call2->location, variableTable1, variableTable2)) {
                            paramsMatch = false;
                            break;
                        }
                    }

                    if (paramsMatch) {
                        char* funcCall1Str = buildFunctionCallString(calledFunc1, call1->parameters, call1->paramCount);
                        char* funcCall2Str = buildFunctionCallString(calledFunc2, call2->parameters, call2->paramCount);

                        addFunctionCallMatchEntry(matchTableCalls, funcCall1Str, funcCall2Str);

                        free(funcCall1Str);
                        free(funcCall2Str);
                    }
                    
                    call2 = call2->next;
                }
            }
            
            call1 = call1->next;
        }
    }
}

void freeVariableEntries(VarEntry* vars) {
    VarEntry* current = vars;
    while (current) {
        VarEntry* next = current->next;
        if (current->varName) free(current->varName);
        if (current->functionName) free(current->functionName);
        if (current->value) free(current->value);
        free(current);
        current = next;
    }
}

int isConstantValue(const char* value) {
    if (!value) return 0;
    char* endptr;
    strtod(value, &endptr);
    return (*endptr == '\0');
}

bool isNumeric(ASTNode* node) {
    if (node->type == NodeType_Constant) {
        char* end;
        strtol(node->value, &end, 10);
        return *end == '\0';
    }
    return false;
}
int getNumericValue(ASTNode* node) {
    return atoi(node->value);
}

const char* getNodeTypeName(NodeType type) {
    switch (type) {
        // Program Structure
        case NodeType_TranslationUnit: return "TranslationUnit";
        case NodeType_ExternalDeclaration: return "ExternalDeclaration";
        case NodeType_FunctionDefinition: return "FunctionDefinition";
        case NodeType_Declaration: return "Declaration";
        case NodeType_DeclarationList: return "DeclarationList";
        case NodeType_DeclarationSpecifiers: return "DeclarationSpecifiers";
        case NodeType_InitDeclaratorList: return "InitDeclaratorList";
        case NodeType_InitDeclarator: return "InitDeclarator";
        case NodeType_Declarator: return "Declarator";
        case NodeType_DirectDeclarator: return "DirectDeclarator";
        case NodeType_Pointer: return "Pointer";
        case NodeType_TypeQualifierList: return "TypeQualifierList";
        case NodeType_ParameterTypeList: return "ParameterTypeList";
        case NodeType_ParameterList: return "ParameterList";
        case NodeType_ParameterDeclaration: return "ParameterDeclaration";
        case NodeType_IdentifierList: return "IdentifierList";
        case NodeType_AbstractDeclarator: return "AbstractDeclarator";
        case NodeType_DirectAbstractDeclarator: return "DirectAbstractDeclarator";
        case NodeType_TypeName: return "TypeName";
        case NodeType_StructOrUnionSpecifier: return "StructOrUnionSpecifier";
        case NodeType_StructOrUnion: return "StructOrUnion";
        case NodeType_StructDeclarationList: return "StructDeclarationList";
        case NodeType_StructDeclaration: return "StructDeclaration";
        case NodeType_SpecifierQualifierList: return "SpecifierQualifierList";
        case NodeType_StructDeclaratorList: return "StructDeclaratorList";
        case NodeType_StructDeclarator: return "StructDeclarator";
        case NodeType_EnumSpecifier: return "EnumSpecifier";
        case NodeType_EnumeratorList: return "EnumeratorList";
        case NodeType_Enumerator: return "Enumerator";
        case NodeType_TypeSpecifier: return "TypeSpecifier";
        case NodeType_TypeQualifier: return "TypeQualifier";
        case NodeType_StorageClassSpecifier: return "StorageClassSpecifier";
        case NodeType_FunctionDeclarator: return "FunctionDeclarator";
        case NodeType_ArrayDeclarator: return "ArrayDeclarator";
        case NodeType_InitializerList: return "InitializerList";
        case NodeType_Initializer: return "Initializer";
        case NodeType_CompoundStatement: return "CompoundStatement";
        
        // Statements
        case NodeType_LabeledStatement: return "LabeledStatement";
        case NodeType_CaseStatement: return "CaseStatement";
        case NodeType_DefaultStatement: return "DefaultStatement";
        case NodeType_ExpressionStatement: return "ExpressionStatement";
        case NodeType_SelectionStatement: return "SelectionStatement";
        case NodeType_IfStatement: return "IfStatement";
        case NodeType_IfElseStatement: return "IfElseStatement";
        case NodeType_SwitchStatement: return "SwitchStatement";
        case NodeType_IterationStatement: return "IterationStatement";
        case NodeType_WhileStatement: return "WhileStatement";
        case NodeType_DoWhileStatement: return "DoWhileStatement";
        case NodeType_ForStatement: return "ForStatement";
        case NodeType_JumpStatement: return "JumpStatement";
        case NodeType_GotoStatement: return "GotoStatement";
        case NodeType_ContinueStatement: return "ContinueStatement";
        case NodeType_BreakStatement: return "BreakStatement";
        case NodeType_ReturnStatement: return "ReturnStatement";
        case NodeType_StatementList: return "StatementList";

        // Expressions
        case NodeType_Expression: return "Expression";
        case NodeType_AssignmentExpression: return "AssignmentExpression";
        case NodeType_ConditionalExpression: return "ConditionalExpression";
        case NodeType_LogicalOrExpression: return "LogicalOrExpression";
        case NodeType_LogicalAndExpression: return "LogicalAndExpression";
        case NodeType_InclusiveOrExpression: return "InclusiveOrExpression";
        case NodeType_ExclusiveOrExpression: return "ExclusiveOrExpression";
        case NodeType_AndExpression: return "AndExpression";
        case NodeType_EqualityExpression: return "EqualityExpression";
        case NodeType_RelationalExpression: return "RelationalExpression";
        case NodeType_ShiftExpression: return "ShiftExpression";
        case NodeType_AdditiveExpression: return "AdditiveExpression";
        case NodeType_MultiplicativeExpression: return "MultiplicativeExpression";
        case NodeType_CastExpression: return "CastExpression";
        case NodeType_UnaryExpression: return "UnaryExpression";
        case NodeType_UnaryOperator: return "UnaryOperator";
        case NodeType_PostfixExpression: return "PostfixExpression";
        case NodeType_PrimaryExpression: return "PrimaryExpression";
        case NodeType_ArgumentExpressionList: return "ArgumentExpressionList";

        // Special Nodes
        case NodeType_FunctionCall: return "FunctionCall";
        case NodeType_ArrayAccess: return "ArrayAccess";
        case NodeType_StructMember: return "StructMember";
        case NodeType_PointerMember: return "PointerMember";
        case NodeType_Ellipsis: return "Ellipsis";

        // Constants and Identifiers
        case NodeType_Identifier: return "Identifier";
        case NodeType_Constant: return "Constant";
        case NodeType_StringLiteral: return "StringLiteral";

        default: return "Unknown";
    }
}

void printAST(ASTNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; ++i) printf("  ");
    printf("%s", getNodeTypeName(node->type));
    if (node->name) printf(" (%s)", node->name);
    printf("\n");
    for (int i = 0; i < node->childCount; ++i) {
        printAST(node->children[i], indent + 1);
    }
}

void freeASTNode(ASTNode* node) {
    if (!node) return;
    for (int i = 0; i < node->childCount; i++) {
        freeASTNode(node->children[i]);
    }
    free(node->name);
    free(node->children);
    free(node);
}

void freeVarTable(VarEntry* variableTable) {
    VarEntry* current = variableTable;
    while (current) {
        VarEntry* next = current->next;
        if (current->varName) free(current->varName);
        if (current->functionName) free(current->functionName);
        if (current->value) free(current->value);
        free(current);
        current = next;
    }
}

void freeMatchTable(MatchTable *table) {
    for (int i = 0; i < table->size; i++) {
        MatchEntry *entry = table->entries[i];
        while (entry) {
            MatchEntry *temp = entry;
            entry = entry->next;
            free(temp->varName1);
            free(temp->funcName1);
            free(temp->varName2);
            free(temp->funcName2);
            free(temp);
        }
    }
    free(table->entries);
    free(table);
}

void freeFunctionCallMatchTable(FunctionCallMatchTable* table) {
    if (!table) return;
    
    for (int i = 0; i < table->size; i++) {
        FunctionCallMatchEntry* entry = table->entries[i];
        while (entry) {
            FunctionCallMatchEntry* temp = entry;
            entry = entry->next;
            free(temp->functionCall1);
            free(temp->functionCall2);
            free(temp);
        }
    }
    free(table->entries);
    free(table);
}

void freeFunctionCallTable(FunctionCallTable* table) {
    for (int i = 0; i < table->size; i++) {
        FunctionCallEntry* entry = table->entries[i];
        while (entry) {
            FunctionCallEntry* temp = entry;
            entry = entry->next;
            free(temp->functionName);
            free(temp->location);
            for (int j = 0; j < temp->paramCount; j++) {
                free(temp->parameters[j]);
            }
            free(temp->parameters);
            free(temp);
        }
    }
    free(table->entries);
}

char* buildExpressionString(ASTNode* expr) {
    if (!expr) {
        fprintf(stderr, "buildExpressionString: Received NULL expression node. Returning 'unknown'.\n");
        return strdup("unknown");
    }

    if (expr->type == NodeType_FunctionCall) {
       // fprintf(stderr, "buildExpressionString: Processing Function Call '%s'.\n", expr->name ? expr->name : "NULL");
      
        if (!expr->name) {
            fprintf(stderr, "buildExpressionString: Function call has NULL name. Returning 'unknown'.\n");
            return strdup("unknown");
        }
        
        char* funcName = expr->name;
        char** argsArray = NULL;
        int argCount = 0;

        if (expr->childCount > 0 && expr->children[0]) {
            ASTNode* argList = expr->children[0];
            if (argList->type == NodeType_ArgumentExpressionList) {
                argCount = argList->childCount;
                argsArray = (char**)malloc(sizeof(char*) * argCount);
                if (!argsArray) {
                    fprintf(stderr, "buildExpressionString: Memory allocation failed for arguments.\n");
                    return strdup("unknown");
                }
                for (int i = 0; i < argCount; i++) {
                    if (!argList->children[i]) {
                        fprintf(stderr, "buildExpressionString: Argument %d is NULL. Using 'unknown'.\n", i);
                        argsArray[i] = strdup("unknown");
                    } else {
                        argsArray[i] = buildExpressionString(argList->children[i]);
                    }
                }
            } else {
                fprintf(stderr, "buildExpressionString: Expected ArgumentExpressionList but got type %d.\n", argList->type);
            }
        } else {
            fprintf(stderr, "buildExpressionString: Function call has no arguments.\n");
        }

        char* result = buildFunctionCallString(funcName, argsArray, argCount);

        for (int i = 0; i < argCount; i++) {
            free(argsArray[i]);
        }
        free(argsArray);

       // fprintf(stderr, "buildExpressionString: Built function call expression string: '%s'.\n", result);
        return result;
    }
    else if (expr->type == NodeType_Constant) {
      //  fprintf(stderr, "buildExpressionString: Processing Constant '%s'.\n", expr->name ? expr->name : "NULL");
        if (!expr->name) {
            fprintf(stderr, "buildExpressionString: Constant has NULL name. Returning 'unknown'.\n");
            return strdup("unknown");
        }
        return strdup(expr->name);
    }
    else if (expr->type == NodeType_Identifier) {
      //  fprintf(stderr, "buildExpressionString: Processing Identifier '%s'.\n", expr->name ? expr->name : "NULL");
        if (!expr->name) {
            fprintf(stderr, "buildExpressionString: Identifier has NULL name. Returning 'unknown'.\n");
            return strdup("unknown");
        }
        return strdup(expr->name);
    }
    else if ((expr->type == NodeType_AdditiveExpression ||
              expr->type == NodeType_MultiplicativeExpression ||
              expr->type == NodeType_RelationalExpression ||
              expr->type == NodeType_EqualityExpression ||
              expr->type == NodeType_AssignmentExpression) && expr->childCount >= 2) {
      //  fprintf(stderr, "buildExpressionString: Processing Binary Operator '%s'.\n", expr->name ? expr->name : "NULL");
        
        if (!expr->name) {
            fprintf(stderr, "buildExpressionString: Binary operator has NULL name. Returning 'unknown'.\n");
            return strdup("unknown");
        }

        char* left = buildExpressionString(expr->children[0]);
        char* right = buildExpressionString(expr->children[1]);

        size_t resultSize = strlen(left) + strlen(expr->name) + strlen(right) + 4; // For spaces and null terminator
        char* result = (char*)malloc(resultSize);
        if (!result) {
            fprintf(stderr, "buildExpressionString: Memory allocation failed for binary expression.\n");
            free(left);
            free(right);
            return strdup("unknown");
        }
        snprintf(result, resultSize, "%s %s %s", left, expr->name, right);

     //   fprintf(stderr, "buildExpressionString: Built binary expression string: '%s'.\n", result);

        free(left);
        free(right);

        return result;
    }
    else if (expr->type == NodeType_ArgumentExpressionList) {
     //   fprintf(stderr, "buildExpressionString: Processing Argument Expression List.\n");

        int totalLength = 0;
        char** argStrings = (char**)malloc(sizeof(char*) * expr->childCount);
        if (!argStrings) {
            fprintf(stderr, "buildExpressionString: Memory allocation failed for argument strings.\n");
            return strdup("unknown");
        }

        for (int i = 0; i < expr->childCount; i++) {
            if (!expr->children[i]) {
                fprintf(stderr, "buildExpressionString: Argument %d is NULL. Using 'unknown'.\n", i);
                argStrings[i] = strdup("unknown");
            } else {
                argStrings[i] = buildExpressionString(expr->children[i]);
            }
            totalLength += strlen(argStrings[i]) + 1; // For ',' separator
        }

        char* result = (char*)malloc(totalLength + 1); // +1 for null terminator
        if (!result) {
            fprintf(stderr, "buildExpressionString: Memory allocation failed for argument list string.\n");
            for (int i = 0; i < expr->childCount; i++) {
                free(argStrings[i]);
            }
            free(argStrings);
            return strdup("unknown");
        }
        result[0] = '\0';

        for (int i = 0; i < expr->childCount; i++) {
            strcat(result, argStrings[i]);
            if (i < expr->childCount - 1)
                strcat(result, ","); // No space after comma
            free(argStrings[i]);
        }
        free(argStrings);

     //   fprintf(stderr, "buildExpressionString: Built argument list string: '%s'.\n", result);
        return result;
    }
    else if (expr->childCount > 0 && expr->children[0]) {
     //   fprintf(stderr, "buildExpressionString: Processing nested expression with %d children.\n", expr->childCount);
        char* result = buildExpressionString(expr->children[0]);
     //   fprintf(stderr, "buildExpressionString: Built nested expression string: '%s'.\n", result);
        return result;
    }

    fprintf(stderr, "buildExpressionString: Unhandled expression type or no children. Returning 'unknown'.\n");
    return strdup("unknown");
}

int compareReturnStatements(VarEntry* return1, VarEntry* return2, const char* functionName1, const char* functionName2, 
                            MatchTable* table, VarEntry* variableTable1, VarEntry* variableTable2, FunctionCallMatchTable* functionCallMatchTable) {
    if (!return1 || !return2) {
        fprintf(stderr, "compareReturnStatements: One or both return entries are NULL.\n");
        return 0;
    }

    if (!return1->isReturn || !return2->isReturn) {
        fprintf(stderr, "compareReturnStatements: One or both variables are not marked as return statements.\n");
        return 0;
    }

    //fprintf(stderr, "compareReturnStatements: Comparing return statements: %s (%s) vs %s (%s)\n",
     //       return1->varName, functionName1, return2->varName, functionName2);

    if (checkMatchTableForVariables(table, return1->varName, functionName1, return2->varName, functionName2, 
                                   variableTable1, variableTable2)) {
        fprintf(stderr, "compareReturnStatements: Match already exists for return statements: %s and %s.\n", 
                return1->varName, return2->varName);
        return 10;
    }
    
    // Build AST from the return expressions
    ASTNode* exprAST1 = NULL;
    ASTNode* exprAST2 = NULL;

    exprAST1 = buildASTNodeFromValue(return1->value);
    if (exprAST1) {
        exprAST1 = resolveExpressionIdentifiers(exprAST1, functionName1, variableTable1, 0);
       // fprintf(stderr, "compareReturnStatements: AST built and resolved for return1 value: %s\n", return1->value);
    } else {
        exprAST1 = resolveVariableExpression(return1->varName, functionName1, variableTable1, 0);
        if (exprAST1) {
        } else {
            fprintf(stderr, "compareReturnStatements: Failed to resolve AST for return1 variable: %s\n", return1->varName);
        }
    }

    // Build AST for return2
    exprAST2 = buildASTNodeFromValue(return2->value);
    if (exprAST2) {
        exprAST2 = resolveExpressionIdentifiers(exprAST2, functionName2, variableTable2, 0);
       // fprintf(stderr, "compareReturnStatements: AST built and resolved for return2 value: %s\n", return2->value);
    } else {
        exprAST2 = resolveVariableExpression(return2->varName, functionName2, variableTable2, 0);
        if (exprAST2) {
            fprintf(stderr, "compareReturnStatements: Resolved AST for return2 variable: %s\n", return2->varName);
        } else {
            fprintf(stderr, "compareReturnStatements: Failed to resolve AST for return2 variable: %s\n", return2->varName);
        }
    }

    if (!exprAST1 || !exprAST2) {
        fprintf(stderr, "compareReturnStatements: Failed to build AST from return expressions.\n");
        if (exprAST1) freeASTNode(exprAST1);
        if (exprAST2) freeASTNode(exprAST2);
        return 0;
    }
    
    int exprScore = compareExpressionsInDetail(exprAST1, exprAST2, 
                                               functionName1, functionName2, table, 
                                               variableTable1, variableTable2, 1, 0, functionCallMatchTable, 0);
    freeASTNode(exprAST1);
    freeASTNode(exprAST2);

    if (exprScore > 0) {
        char comparisonTag[256];
        snprintf(comparisonTag, sizeof(comparisonTag), "ReturnValueMatch");
        addMatchEntry(table, return1->varName, functionName1, return2->varName, functionName2, 
                      comparisonTag);
        fprintf(stderr, "compareReturnStatements: Return statements match with score %d.\n", exprScore);
        return exprScore;
    }

    fprintf(stderr, "compareReturnStatements: Return statements do not match.\n");
    return 0;
}

int compareAssignments(ASTNode* node1, ASTNode* node2, const char* functionName1, const char* functionName2, MatchTable* table, VarEntry* variableTable1, VarEntry* variableTable2, FunctionCallMatchTable* functionCallMatchTable) {
    if (!node1 || !node2) {
        fprintf(stderr, "compareAssignments: One or both assignment nodes are NULL.\n");
        return 0;
    }
    
    if (node1->type == NodeType_ReturnStatement || node2->type == NodeType_ReturnStatement) {
        fprintf(stderr, "compareAssignments: One or both nodes are ReturnStatements. Skipping comparison here.\n");
        return 0;
    }

    if (node1->type != NodeType_AssignmentExpression || node2->type != NodeType_AssignmentExpression) {
        fprintf(stderr, "compareAssignments: One or both nodes are not AssignmentExpression types.\n");
        return 0;
    }

    const char* varName1 = node1->children[0]->name;
    const char* varName2 = node2->children[0]->name;

    VarEntry* entry1 = findVariableInFunction(varName1, functionName1, variableTable1);
    VarEntry* entry2 = findVariableInFunction(varName2, functionName2, variableTable2);

    if ((entry1 && entry1->isReturn) || (entry2 && entry2->isReturn)) {
        fprintf(stderr, "compareAssignments: One or both variables are Return expressions. Skipping comparison.\n");
        return 0;
    }

    ASTNode* expr1 = node1->children[1];
    ASTNode* expr2 = node2->children[1];

    if (compareExpressionsInDetail(expr1, expr2, functionName1, functionName2, table, variableTable1, variableTable2, 0, 0, functionCallMatchTable, 0)) {
        char comparisonTag[256];
        snprintf(comparisonTag, sizeof(comparisonTag), "ExpressionMatch");
        addMatchEntry(table, varName1, functionName1, varName2, functionName2, comparisonTag);
        return 1;
    }

    //fprintf(stderr, "compareAssignments: Assignments do not match: %s and %s.\n", varName1, varName2);
    return 0;
}

int getFunctionCallCount(const char* functionName) {
    FunctionCallCounter* current = functionCallCounters;
    while (current) {
        if (strcmp(current->functionName, functionName) == 0) {
            current->callCount++;
            return current->callCount;
        }
        current = current->next;
    }

    FunctionCallCounter* newCounter = malloc(sizeof(FunctionCallCounter));
    newCounter->functionName = strdup(functionName);
    newCounter->callCount = 1;
    newCounter->next = functionCallCounters;
    functionCallCounters = newCounter;
    return newCounter->callCount;
}

int compareExpressionsInDetail(ASTNode *expr1, ASTNode *expr2, const char *functionName1, const char *functionName2, 
                               MatchTable *table, VarEntry* variableTable1, VarEntry* variableTable2, 
                               int isReturnComparison, int isParameterComparison, FunctionCallMatchTable* functionCallMatchTable, int depth)
{
    const int MAX_RECURSION_DEPTH = 20;
    
    if (!expr1 || !expr2) {
        fprintf(stderr, "compareExpressionsInDetail: One or both expression nodes are NULL.\n");
        return 0;
    }

    if (depth > MAX_RECURSION_DEPTH) {
        fprintf(stderr, "compareExpressionsInDetail: Maximum recursion depth exceeded.\n");
        return 0;
    }

    if (!expr1->name || !expr2->name) {
        fprintf(stderr, "compareExpressionsInDetail: One or both expressions have NULL names.\n");
        return 0;
    }
    
   // fprintf(stderr, "compareExpressionsInDetail: Comparing '%s' (%s) with '%s' (%s)\n", 
     //       expr1->name, functionName1, expr2->name, functionName2);

    VarEntry* entry1 = findVariableInFunction(expr1->name, functionName1, variableTable1);
    VarEntry* entry2 = findVariableInFunction(expr2->name, functionName2, variableTable2);

    if ((entry1 && entry1->isReturn) != (entry2 && entry2->isReturn)) {
        fprintf(stderr, "compareExpressionsInDetail: Skipping comparison between '%s' and '%s' due to mismatched return status.\n", 
                expr1->name, expr2->name);
        return 0;
    }


    if (expr1->type == NodeType_AdditiveExpression && expr2->type == NodeType_AdditiveExpression) {
        // İfadeleri terimlerine ayır
        AddTermList listA, listB;
        initAddTermList(&listA);
        initAddTermList(&listB);

        flattenAdditiveExpression(expr1, +1, &listA, functionName1, variableTable1, depth+1);
        flattenAdditiveExpression(expr2, +1, &listB, functionName2, variableTable2, depth+1);

        int result = compareFlattenedAdditive(&listA, &listB,
                                              functionName1, functionName2,
                                              table, variableTable1, variableTable2,
                                              functionCallMatchTable, depth,
                                              isReturnComparison, isParameterComparison);

        freeAddTermList(&listA);
        freeAddTermList(&listB);

        return result;
    }

    if (expr1->type == expr2->type && strcmp(expr1->name, expr2->name) == 0) {
        // Multiplicative
        if (expr1->type == NodeType_MultiplicativeExpression) {
            if (expr1->childCount < 2 || expr2->childCount < 2) {
                fprintf(stderr, "compareExpressionsInDetail: Binary operator nodes do not have enough children.\n");
                return 0;
            }
            
            int leftScoreNormal = compareExpressionsInDetail(expr1->children[0], expr2->children[0], 
                                                             functionName1, functionName2, table, 
                                                             variableTable1, variableTable2, isReturnComparison, isParameterComparison, functionCallMatchTable, depth + 1);
            int rightScoreNormal = compareExpressionsInDetail(expr1->children[1], expr2->children[1], 
                                                              functionName1, functionName2, table, 
                                                              variableTable1, variableTable2, isReturnComparison, isParameterComparison, functionCallMatchTable, depth + 1);
            int totalScoreNormal = leftScoreNormal + rightScoreNormal;

            int leftScoreReverse = compareExpressionsInDetail(expr1->children[0], expr2->children[1], 
                                                              functionName1, functionName2, table, 
                                                              variableTable1, variableTable2, isReturnComparison, isParameterComparison, functionCallMatchTable, depth + 1);
            int rightScoreReverse = compareExpressionsInDetail(expr1->children[1], expr2->children[0], 
                                                               functionName1, functionName2, table, 
                                                               variableTable1, variableTable2, isReturnComparison, isParameterComparison, functionCallMatchTable, depth + 1);
            int totalScoreReverse = leftScoreReverse + rightScoreReverse;

            if (totalScoreNormal >= totalScoreReverse && leftScoreNormal > 0 && rightScoreNormal > 0) {
                return totalScoreNormal;
            } else if (totalScoreReverse > totalScoreNormal && leftScoreReverse > 0 && rightScoreReverse > 0) {
                return totalScoreReverse;
            }
            return 0;
        }
        // Unary operatörler
        else if (expr1->type == NodeType_UnaryOperator) {
            if (expr1->childCount < 1 || expr2->childCount < 1) {
                fprintf(stderr, "compareExpressionsInDetail: Unary operator nodes do not have enough children.\n");
                return 0;
            }
            int operandScore = compareExpressionsInDetail(expr1->children[0], expr2->children[0], 
                                                         functionName1, functionName2, table, 
                                                         variableTable1, variableTable2, isReturnComparison, isParameterComparison, functionCallMatchTable, depth + 1);
            if (operandScore > 0) {
                return operandScore;
            } else {
                fprintf(stderr, "compareExpressionsInDetail: Unary expressions do not match.\n");
                return 0;
            }
        }
    }

    // Sabit ifadeleri karşılaştırma
    if (expr1->type == NodeType_Constant && expr2->type == NodeType_Constant) {
        if (isdigit(expr1->name[0]) && isdigit(expr2->name[0])) {
            if (strcmp(expr1->name, expr2->name) == 0) {
                return 10;
            } else {
                fprintf(stderr, "compareExpressionsInDetail: Numeric constants '%s' and '%s' do not match.\n",
                        expr1->name, expr2->name);
                return 0;
            }
        } else {
            // If constants are non-numeric, compare them directly
            if (strcmp(expr1->name, expr2->name) == 0) {
                if (!isReturnComparison && !isParameterComparison) {
                    addMatchEntry(table, expr1->name, functionName1, expr2->name, functionName2, "ConstantMatch");
                }
                return 10;
            } else {
                fprintf(stderr, "compareExpressionsInDetail: Constants '%s' and '%s' do not match.\n", expr1->name, expr2->name);
                return 0;
            }
        }
    }

    if (expr1->type == NodeType_Identifier && expr2->type == NodeType_Identifier) {
    //    fprintf(stderr, "compareExpressionsInDetail: Comparing identifiers '%s' and '%s'.\n", expr1->name, expr2->name);

        if (areParametersMatched(expr1->name, functionName1, expr2->name, functionName2, table, variableTable1, variableTable2)) {
            // fprintf(stderr, "compareExpressionsInDetail: Parameters '%s' and '%s' match via areParametersMatched.\n", expr1->name, expr2->name);
            if (!isReturnComparison && !isParameterComparison) {
                addMatchEntry(table, expr1->name, functionName1, expr2->name, functionName2, "ParamMatch");
            }
            return 10;
        }

        if (checkMatchTableForVariables(table, expr1->name, functionName1, expr2->name, functionName2, variableTable1, variableTable2) ||
            checkMatchTableForVariables(table, expr2->name, functionName2, expr1->name, functionName1, variableTable2, variableTable1)) {
            return 10;
        } else {
            ASTNode* resolvedExpr1 = resolveVariableExpression(expr1->name, functionName1, variableTable1, depth + 1);
            ASTNode* resolvedExpr2 = resolveVariableExpression(expr2->name, functionName2, variableTable2, depth + 1);

            if (resolvedExpr1 && resolvedExpr2) {
                int score = compareExpressionsInDetail(resolvedExpr1, resolvedExpr2, functionName1, functionName2, table, variableTable1, variableTable2, isReturnComparison, isParameterComparison, functionCallMatchTable, depth + 1);
                freeASTNode(resolvedExpr1);
                freeASTNode(resolvedExpr2);
                if (score > 0) {
                    if (!isReturnComparison && !isParameterComparison) {
                        addMatchEntry(table, expr1->name, functionName1, expr2->name, functionName2, "ExpressionMatch");
                    }
                    return score;
                } else {
                }
            } else {
                fprintf(stderr, "compareExpressionsInDetail: Could not resolve expressions for '%s' and/or '%s'.\n", expr1->name, expr2->name);
            }
        }

        fprintf(stderr, "compareExpressionsInDetail: Identifiers '%s' and '%s' do not match.\n", expr1->name, expr2->name);
        return 0;
    }
    
    // Function Call
    if (expr1->type == NodeType_FunctionCall && expr2->type == NodeType_FunctionCall) {
        // Build function call strings
        char* funcCallStr1 = buildExpressionString(expr1);
        char* funcCallStr2 = buildExpressionString(expr2);
        if (!funcCallStr1 || !funcCallStr2) {
            fprintf(stderr, "compareExpressionsInDetail: Failed to build function call strings.\n");
            free(funcCallStr1);
            free(funcCallStr2);
            return 0;
        }

        if (checkFunctionCallMatchTable(funcCallStr1, funcCallStr2, functionCallMatchTable)) {
          //  fprintf(stderr, "compareExpressionsInDetail: Function calls '%s' and '%s' match via FunctionCallMatchTable.\n", funcCallStr1, funcCallStr2);
            free(funcCallStr1);
            free(funcCallStr2);
            return 10;
        } else {
            fprintf(stderr, "compareExpressionsInDetail: Function calls '%s' and '%s' not in FunctionCallMatchTable, comparing parameters.\n", funcCallStr1, funcCallStr2);

            // Check if the return expressions of the functions match
            if (!matchReturnExpressions(table, expr1->name, expr2->name)) {
                fprintf(stderr, "compareExpressionsInDetail: Return expressions of functions '%s' and '%s' do not match. Function calls cannot be matched.\n", expr1->name, expr2->name);
                free(funcCallStr1);
                free(funcCallStr2);
                return 0;
            }

            fprintf(stderr, "compareExpressionsInDetail: Return expressions of functions '%s' and '%s' match. Proceeding to compare parameters.\n", expr1->name, expr2->name);

            ASTNode* args1 = expr1->childCount > 0 ? expr1->children[0] : NULL;
            ASTNode* args2 = expr2->childCount > 0 ? expr2->children[0] : NULL;

            int paramCount1 = args1 ? args1->childCount : 0;
            int paramCount2 = args2 ? args2->childCount : 0;

            if (paramCount1 != paramCount2) {
                fprintf(stderr, "compareExpressionsInDetail: Function '%s' parameter counts do not match: %d vs %d.\n", expr1->name, paramCount1, paramCount2);
                free(funcCallStr1);
                free(funcCallStr2);
                return 0;
            }

            // Compare parameters
            int totalScore = 0;
            for (int i = 0; i < paramCount1; i++) {
                ASTNode* param1 = args1->children[i];
                ASTNode* param2 = args2->children[i];

                ASTNode* resolvedParam1 = resolveExpressionIdentifiers(param1, functionName1, variableTable1, depth + 1);
                ASTNode* resolvedParam2 = resolveExpressionIdentifiers(param2, functionName2, variableTable2, depth + 1);

                if (!resolvedParam1 || !resolvedParam2) {
                    fprintf(stderr, "compareExpressionsInDetail: Failed to resolve parameters at position %d.\n", i + 1);
                    if (resolvedParam1) freeASTNode(resolvedParam1);
                    if (resolvedParam2) freeASTNode(resolvedParam2);
                    free(funcCallStr1);
                    free(funcCallStr2);
                    return 0;
                }

                int paramScore = compareExpressionsInDetail(resolvedParam1, resolvedParam2,
                                                            functionName1, functionName2, table,
                                                            variableTable1, variableTable2, isReturnComparison, 1, functionCallMatchTable, depth + 1);

                freeASTNode(resolvedParam1);
                freeASTNode(resolvedParam2);

                if (paramScore == 0) {
                    fprintf(stderr, "compareExpressionsInDetail: Parameters at position %d do not match.\n", i + 1);
                    free(funcCallStr1);
                    free(funcCallStr2);
                    return 0;
                }

                totalScore += paramScore;
            }

            if (!isReturnComparison) {
                addFunctionCallMatchEntry(functionCallMatchTable, funcCallStr1, funcCallStr2);
            }

            free(funcCallStr1);
            free(funcCallStr2);

            return 10 + totalScore;
        }
    }
    
    //fprintf(stderr, "compareExpressionsInDetail: Expressions '%s' and '%s' do not match in detail.\n", 
    //        expr1->name, expr2->name);
    return 0;
}

int isCommutative(const char* op) {
    const char* commutativeOps[] = { "+", "*" };
    int numCommutativeOps = sizeof(commutativeOps) / sizeof(commutativeOps[0]);
    for (int i = 0; i < numCommutativeOps; i++) {
        if (strcmp(op, commutativeOps[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int isBinaryOperator(const char* op) {
    const char* binaryOps[] = { "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=", "&&", "||" };
    int numBinaryOps = sizeof(binaryOps) / sizeof(binaryOps[0]);
    for (int i = 0; i < numBinaryOps; i++) {
        if (strcmp(op, binaryOps[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int isUnaryOperator(const char* op) {
    const char* unaryOps[] = { "!", "~", "++", "--", "+", "-" };
    int numUnaryOps = sizeof(unaryOps) / sizeof(unaryOps[0]);
    for (int i = 0; i < numUnaryOps; i++) {
        if (strcmp(op, unaryOps[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void traverseAST(ASTNode* node, VarEntry** variableTable, FunctionCallTable* callTable) {
    if (!node) return;
    
    const char* previousFunction = getCurrentFunctionName();
    
    if (node->type == NodeType_FunctionDefinition) {
        setCurrentFunctionName(node->name);
        // fprintf(stderr, "Current function set to: %s\n", getCurrentFunctionName());
        
        // Parametreleri tabloya ekleme
        if (node->paramCount > 0 && node->params) {
            for (int i = 0; i < node->paramCount; i++) {
                ASTNode* paramNode = node->params[i];
                
                const char* paramName = NULL;
                if (paramNode->type == NodeType_ParameterDeclaration) {
                    if (paramNode->childCount > 1) {
                        ASTNode* declarator = paramNode->children[1];
                        if (declarator && declarator->name) {
                            paramName = declarator->name;
                        }
                    }
                }
                if (paramName) {
                    addVariableToTable(variableTable, paramName, NULL, getCurrentFunctionName(), 1, 0, 0, 0, 0);
                } else {
                    fprintf(stderr, "Parameter name is NULL in function '%s'.\n", getCurrentFunctionName());
                }
            }
        }
    }
    
    if (node->type == NodeType_Declaration) {
        ASTNode* declarationSpecifiers = node->children[0];
        ASTNode* initDeclaratorList = node->children[1];
        
        char* typeName = NULL;
        if (declarationSpecifiers->childCount > 0) {
            ASTNode* typeSpecifier = declarationSpecifiers->children[0];
            if (typeSpecifier->type == NodeType_TypeSpecifier) {
                typeName = typeSpecifier->name;
            }
        }
        
        for (int i = 0; i < initDeclaratorList->childCount; i++) {
            ASTNode* initDeclarator = initDeclaratorList->children[i];
            ASTNode* directDeclarator = initDeclarator->children[0];
            char* varName = directDeclarator->name;
            
            char* valueStr = NULL;
            if (initDeclarator->childCount > 1) {
                ASTNode* initializer = initDeclarator->children[1];
                // Sağ tarafı çözümle
                ASTNode* resolvedExprAST = resolveExpressionIdentifiers(initializer, getCurrentFunctionName(), *variableTable, 0);
                valueStr = buildExpressionString(resolvedExprAST);
                freeASTNode(resolvedExprAST);
            } else {
                valueStr = strdup("0");
                fprintf(stderr, "No initializer found for variable: %s. Defaulting to 0.\n", varName);
            }
            
            VarEntry* existingVar = findVariableInFunction(varName, getCurrentFunctionName(), *variableTable);
            if (existingVar) {

                fprintf(stderr, "Variable '%s' already exists in function '%s'. Treating 'int %s = ...' as update.\n", 
                        varName, getCurrentFunctionName(), varName);

                addVariableToTable(variableTable, varName, valueStr, getCurrentFunctionName(), 
                                   existingVar->isParameter, existingVar->isReturn, existingVar->isInMain, 
                                   existingVar->isFunctionCall, existingVar->callCount);

                fprintf(stderr, "Updated variable: %s in function %s with value %s (Declaration-as-Update)\n", 
                        varName, getCurrentFunctionName(), valueStr ? valueStr : "NULL");
            } else {
                // Değişken yoksa gerçekten yeni bir değişken olarak ekle
                addVariableToTable(variableTable, varName, valueStr, getCurrentFunctionName(), 0, 0, 
                                   strcmp(getCurrentFunctionName(), "main") == 0, 0, 0);

                //fprintf(stderr, "Added new variable: %s in function %s with value %s (New Declaration)\n", 
                //        varName, getCurrentFunctionName(), valueStr ? valueStr : "NULL");
            }

            free(valueStr);
        }
    }
    // Assignment
    if (node->type == NodeType_AssignmentExpression) {
        char* varName = node->children[0]->name;
        
        // Resolve right side
        ASTNode* resolvedExprAST = resolveExpressionIdentifiers(node->children[1], getCurrentFunctionName(), *variableTable, 0);
        char* rhsValue = buildExpressionString(resolvedExprAST);
        freeASTNode(resolvedExprAST);

        fprintf(stderr, "Processing assignment for variable '%s' in function '%s' with resolved value: %s\n",
                varName, getCurrentFunctionName(), rhsValue ? rhsValue : "NULL");
        
        addVariableToTable(variableTable, varName, rhsValue, getCurrentFunctionName(), 0, 0,
                           strcmp(getCurrentFunctionName(),"main")==0, 0, 0);
        
        if (rhsValue) free(rhsValue);
    }
    
    // Return
    if (node->type == NodeType_ReturnStatement) {
        if (node->childCount > 0 && node->children[0]) {
            ASTNode* resolvedExprAST = resolveExpressionIdentifiers(node->children[0], getCurrentFunctionName(), *variableTable, 0);
            char* returnValue = buildExpressionString(resolvedExprAST);
            freeASTNode(resolvedExprAST);
            
            char returnEntryName[256];
            snprintf(returnEntryName, sizeof(returnEntryName), "Return Expression in %s", getCurrentFunctionName());


            addVariableToTable(variableTable, returnEntryName, returnValue, getCurrentFunctionName(), 0, 1,
                               strcmp(getCurrentFunctionName(), "main") == 0, 0, 0);

            if (returnValue) free(returnValue);
        }
    }
    
    // Fonksiyon çağrıları
    if (node->type == NodeType_FunctionCall) {
        char* functionName = node->name;
        
        // Parametreleri al
        char** parameters = NULL;
        int paramCount = 0;
        
        // İlk child argüman listesi ise
        if (node->childCount > 0 && node->children[0]) {
            ASTNode* argList = node->children[0];
            
            if (argList->type == NodeType_ArgumentExpressionList) {
                paramCount = argList->childCount;
                parameters = (char**)malloc(sizeof(char*) * paramCount);
                if (!parameters) {
                    fprintf(stderr, "traverseAST: Memory allocation failed for parameters.\n");
                    exit(EXIT_FAILURE);
                }
                
                for (int p = 0; p < paramCount; p++) {
                    parameters[p] = buildExpressionString(argList->children[p]);
                  //  fprintf(stderr, "Function call parameter: %s\n", parameters[p]);
                }
            } else {
                // Tek bir argüman
                paramCount = 1;
                parameters = (char**)malloc(sizeof(char*));
                if (!parameters) {
                    fprintf(stderr, "traverseAST: Memory allocation failed for single parameter.\n");
                    exit(EXIT_FAILURE);
                }
                parameters[0] = buildExpressionString(argList);
                fprintf(stderr, "Function call single parameter: %s\n", parameters[0]);
            }
        }
        
        addFunctionCallEntry(callTable, functionName, getCurrentFunctionName(), parameters, paramCount);
        
        for (int p = 0; p < paramCount; p++) {
            free(parameters[p]);
        }
        free(parameters);
    }
    
    for (int i = 0; i < node->childCount; i++) {
        traverseAST(node->children[i], variableTable, callTable);
    }
    
    if (node->type == NodeType_FunctionDefinition) {
        setCurrentFunctionName(previousFunction);
       // fprintf(stderr, "Current function reverted to: %s\n", getCurrentFunctionName());
    }
}



// HELPERS ==========================

ASTNode* resolveVariableExpression(const char* varName, const char* functionName, VarEntry* variableTable, int depth) {
    const int MAX_RECURSION_DEPTH = 20;

    if (depth > MAX_RECURSION_DEPTH) {
        return NULL;
    }

    VarEntry* varEntry = findVariableInFunction(varName, functionName, variableTable);
    if (!varEntry) {
        return createASTNode(NodeType_Identifier, varName);
    }

    if (varEntry->isParameter && (!varEntry->value || strlen(varEntry->value) == 0)) {
        return createASTNode(NodeType_Identifier, varName);
    }

    if (varEntry->value && isConstantValue(varEntry->value)) {
        return createASTNode(NodeType_Constant, varEntry->value);
    } else if (varEntry->value) {
        ASTNode* exprAST = buildASTNodeFromValue(varEntry->value);
        if (!exprAST) {
            fprintf(stderr, "resolveVariableExpression: Failed to parse expression '%s' for variable '%s' in function '%s'.\n", varEntry->value, varName, functionName);
            return createASTNode(NodeType_Identifier, varName);
        }

        // İfadeyi çözümle
        ASTNode* resolvedExpr = resolveExpressionIdentifiers(exprAST, functionName, variableTable, depth + 1);
        freeASTNode(exprAST);
        return resolvedExpr;
    }

    return createASTNode(NodeType_Identifier, varName);
}

int areParametersMatched(const char* varName1, const char* functionName1, const char* varName2, const char* functionName2,
                         MatchTable* table, VarEntry* variableTable1, VarEntry* variableTable2) {
    VarEntry* varEntry1 = findVariableInFunction(varName1, functionName1, variableTable1);
    VarEntry* varEntry2 = findVariableInFunction(varName2, functionName2, variableTable2);

    if (varEntry1 && varEntry2 && varEntry1->isParameter && varEntry2->isParameter) {
        if (checkMatchTableForVariables(table, varName1, functionName1, varName2, functionName2, variableTable1, variableTable2)) {
            return 1;
        }
    }
    return 0;
}

ASTNode* resolveExpressionIdentifiers(ASTNode* expr, const char* functionName, VarEntry* variableTable, int depth) {
    if (!expr) return NULL;

    if (expr->type == NodeType_Identifier) {
        ASTNode* resolved = resolveVariableExpression(expr->name, functionName, variableTable, depth + 1);
        if (resolved) {
            return resolved;
        } else {
            return createASTNode(NodeType_Identifier, expr->name);
        }
    }

    ASTNode* newExpr = createASTNode(expr->type, expr->name);
    newExpr->childCount = expr->childCount;
    newExpr->children = (ASTNode**)malloc(sizeof(ASTNode*) * expr->childCount);
    for (int i = 0; i < expr->childCount; i++) {
        newExpr->children[i] = resolveExpressionIdentifiers(expr->children[i], functionName, variableTable, depth + 1);
        newExpr->children[i]->parent = newExpr;
    }
    return newExpr;
}

ASTNode* parseFactor(const char** expr) {
    *expr = skipWhitespace(*expr);

    if (**expr == '(') {
        (*expr)++; // Skip `(`
        ASTNode* node = parseExpression(expr);
        if (**expr == ')') {
            (*expr)++; // Skip )
        } else {
            fprintf(stderr, "parseFactor: Missing closing parenthesis.\n");
        }
        return node;
    } else if (isalpha(**expr) || **expr == '_') {
        // Identifier or Function Call
        const char* start = *expr;
        while (isalnum(**expr) || **expr == '_') (*expr)++;
        size_t len = *expr - start;
        char* name = (char*)malloc(len + 1);
        strncpy(name, start, len);
        name[len] = '\0';

        *expr = skipWhitespace(*expr);
        if (**expr == '(') {
            (*expr)++;
            ASTNode* argList = createASTNode(NodeType_ArgumentExpressionList, "ArgumentList");
            argList->children = NULL;
            argList->childCount = 0;
            while (1) {
                *expr = skipWhitespace(*expr);
                if (**expr == ')') {
                    (*expr)++;
                    break;
                }

                ASTNode* arg = parseExpression(expr);
                if (!arg) {
                    fprintf(stderr, "parseFactor: Failed to parse argument in function call '%s'\n", name);
                    free(name);
                    freeASTNode(argList);
                    return NULL;
                }

                argList->children = (ASTNode**)realloc(argList->children, sizeof(ASTNode*) * (argList->childCount + 1));
                argList->children[argList->childCount++] = arg;
                arg->parent = argList;

                *expr = skipWhitespace(*expr);
                if (**expr == ',') {
                    (*expr)++;
                    continue;
                } else if (**expr == ')') {
                    (*expr)++;
                    break;
                } else {
                    fprintf(stderr, "parseFactor: Unexpected character '%c' in function call '%s'\n", **expr, name);
                    free(name);
                    freeASTNode(argList);
                    return NULL;
                }
            }

            // Create FunctionCall Node
            ASTNode* node = createASTNode(NodeType_FunctionCall, name);
            node->children = (ASTNode**)malloc(sizeof(ASTNode*));
            node->children[0] = argList;
            node->childCount = 1;
            argList->parent = node;

            free(name);
            return node;
        } else {
            // J Identifier
            ASTNode* node = createASTNode(NodeType_Identifier, name);
            free(name);
            return node;
        }
    } else if (isdigit(**expr)) {
        // Constant
        const char* start = *expr;
        while (isdigit(**expr)) (*expr)++;
        size_t len = *expr - start;
        char* value = (char*)malloc(len + 1);
        strncpy(value, start, len);
        value[len] = '\0';
        ASTNode* node = createASTNode(NodeType_Constant, value);
        free(value);
        return node;
    } else {
        fprintf(stderr, "parseFactor: Unexpected character '%c'\n", **expr);
        return NULL;
    }
}

ASTNode* parseTerm(const char** expr) {
    ASTNode* node = parseFactor(expr);

    while (1) {
        *expr = skipWhitespace(*expr);
        if (**expr == '*' || **expr == '/') {
            char op = **expr;
            (*expr)++;
            ASTNode* right = parseFactor(expr);
            if (!right) {
                fprintf(stderr, "parseTerm: Missing operand after '%c'\n", op);
                freeASTNode(node);
                return NULL;
            }
            ASTNode* newNode = createASTNode(NodeType_MultiplicativeExpression, op == '*' ? "*" : "/");
            newNode->children = (ASTNode**)malloc(sizeof(ASTNode*) * 2);
            newNode->children[0] = node;
            newNode->children[1] = right;
            newNode->childCount = 2;
            node->parent = newNode;
            right->parent = newNode;
            node = newNode;
        } else {
            break;
        }
    }

    return node;
}

ASTNode* parseExpression(const char** expr) {
    ASTNode* node = parseTerm(expr);

    while (1) {
        *expr = skipWhitespace(*expr);
        if (**expr == '+' || **expr == '-') {
            char op = **expr;
            (*expr)++;
            ASTNode* right = parseTerm(expr);
            if (!right) {
                fprintf(stderr, "parseExpression: Missing operand after '%c'\n", op);
                freeASTNode(node);
                return NULL;
            }
            ASTNode* newNode = createASTNode(NodeType_AdditiveExpression, op == '+' ? "+" : "-");
            newNode->children = (ASTNode**)malloc(sizeof(ASTNode*) * 2);
            newNode->children[0] = node;
            newNode->children[1] = right;
            newNode->childCount = 2;
            node->parent = newNode;
            right->parent = newNode;
            node = newNode;
        } else {
            break;
        }
    }

    return node;
}

ASTNode* parseFunctionCall(const char** expr) {
    *expr = skipWhitespace(*expr);

    if (!isalpha(**expr) && **expr != '_') {
        return NULL;
    }
    const char* start = *expr;
    while (isalnum(**expr) || **expr == '_') (*expr)++;
    size_t len = *expr - start;
    char* funcName = (char*)malloc(len + 1);
    strncpy(funcName, start, len);
    funcName[len] = '\0';

    *expr = skipWhitespace(*expr);
    if (**expr != '(') {
        free(funcName);
        return NULL;
    }
    (*expr)++;

    // Parse arguments
    ASTNode* argList = createASTNode(NodeType_ArgumentExpressionList, "ArgumentList");
    argList->children = NULL;
    argList->childCount = 0;
    while (1) {
        *expr = skipWhitespace(*expr);
        if (**expr == ')') {
            (*expr)++;
            break;
        }

        ASTNode* arg = parseExpression(expr);
        if (!arg) {
            fprintf(stderr, "parseFunctionCall: Failed to parse argument in function call '%s'\n", funcName);
            free(funcName);
            freeASTNode(argList);
            return NULL;
        }

        argList->children = (ASTNode**)realloc(argList->children, sizeof(ASTNode*) * (argList->childCount + 1));
        argList->children[argList->childCount++] = arg;
        arg->parent = argList;

        *expr = skipWhitespace(*expr);
        if (**expr == ',') {
            (*expr)++;
            continue;
        } else if (**expr == ')') {
            (*expr)++;
            break;
        } else {
            fprintf(stderr, "parseFunctionCall: Unexpected character '%c' in function call '%s'\n", **expr, funcName);
            free(funcName);
            freeASTNode(argList);
            return NULL;
        }
    }

    // Create the FunctionCall node
    ASTNode* node = createASTNode(NodeType_FunctionCall, funcName);
    node->children = (ASTNode**)malloc(sizeof(ASTNode*));
    node->children[0] = argList;
    node->childCount = 1;
    argList->parent = node;

    free(funcName);
    return node;
}

char* buildFunctionCallString(const char* funcName, char** parameters, int paramCount) {
    int totalLength = strlen(funcName) + 2; // For '(' and ')'
    for (int i = 0; i < paramCount; i++) {
        totalLength += strlen(parameters[i]);
        if (i < paramCount - 1) {
            totalLength += 1; // For ','
        }
    }
    char* funcCallStr = (char*)malloc(totalLength + 1);
    if (!funcCallStr) {
        fprintf(stderr, "Memory allocation failed for funcCallStr.\n");
        exit(EXIT_FAILURE);
    }

    strcpy(funcCallStr, funcName);
    strcat(funcCallStr, "(");
    for (int i = 0; i < paramCount; i++) {
        strcat(funcCallStr, parameters[i]);
        if (i < paramCount - 1) {
            strcat(funcCallStr, ",");
        }
    }
    strcat(funcCallStr, ")");

    return funcCallStr;
}

ASTNode* findReturnStatement(ASTNode* node) {
    if (!node) return NULL;

    if (node->type == NodeType_ReturnStatement) {
        return node;
    }

    for (int i = 0; i < node->childCount; i++) {
        ASTNode* result = findReturnStatement(node->children[i]);
        if (result) {
            return result;
        }
    }

    return NULL;
}

int checkMatchTableForVariables(MatchTable* table, const char* var1, const char* func1, 
                                 const char* var2, const char* func2, 
                                 VarEntry* variableTable1, VarEntry* variableTable2) {
    
    for (int i = 0; i < table->size; i++) {
        MatchEntry* entry = table->entries[i];
        while (entry) {
            if ((strcmp(entry->varName1, var1) == 0 && strcmp(entry->funcName1, func1) == 0 &&
                 strcmp(entry->varName2, var2) == 0 && strcmp(entry->funcName2, func2) == 0) ||
                (strcmp(entry->varName1, var2) == 0 && strcmp(entry->funcName1, func2) == 0 &&
                 strcmp(entry->varName2, var1) == 0 && strcmp(entry->funcName2, func1) == 0)) {
                
                VarEntry* entry1 = findVariableInFunction(var1, func1, variableTable1);
                VarEntry* entry2 = findVariableInFunction(var2, func2, variableTable2);
                
                if ((entry1 && entry1->isReturn) != (entry2 && entry2->isReturn)) {
                    return 0;
                }

                return 1;
            }
            entry = entry->next;
        }
    }

   // fprintf(stderr, "checkMatchTableForVariables: No match found for '%s' (%s) <-> '%s' (%s)\n",
     //       var1, func1, var2, func2);
    return 0;
}

ASTNode* buildASTNodeFromValue(const char* value) {
    if (!value) return NULL;

    const char* expr = value;
    ASTNode* node = parseExpression(&expr);

    if (node == NULL) {
        // Try parsing as a function call
        expr = value;
        node = parseFunctionCall(&expr);
    }

    if (node == NULL) {
        fprintf(stderr, "buildASTNodeFromValue: Failed to parse expression '%s'\n", value);
    }

    return node;
}

const char* skipWhitespace(const char* str) {
    while (isspace(*str)) str++;
    return str;
}


int checkFunctionCallMatchTable(const char* funcCall1, const char* funcCall2, FunctionCallMatchTable* functionCallMatchTable) {
    if (!functionCallMatchTable || !funcCall1 || !funcCall2) {
        fprintf(stderr, "checkFunctionCallMatchTable: Invalid arguments.\n");
        return 0;
    }

    for (int i = 0; i < functionCallMatchTable->size; i++) {
        FunctionCallMatchEntry* entry = functionCallMatchTable->entries[i];
        while (entry) {
            if ((strcmp(entry->functionCall1, funcCall1) == 0 && strcmp(entry->functionCall2, funcCall2) == 0) ||
                (strcmp(entry->functionCall1, funcCall2) == 0 && strcmp(entry->functionCall2, funcCall1) == 0)) {
                return 1;
            }
            entry = entry->next;
        }
    }

    return 0;
}

int matchReturnExpressions(MatchTable* table, const char* funcName1, const char* funcName2) {

    char returnVar1[256];
    char returnVar2[256];
    snprintf(returnVar1, sizeof(returnVar1), "Return Expression in %s", funcName1);
    snprintf(returnVar2, sizeof(returnVar2), "Return Expression in %s", funcName2);

    for (int i = 0; i < table->size; i++) {
        MatchEntry* entry = table->entries[i];
        while (entry) {
            if (strcmp(entry->varName1, returnVar1) == 0 &&
                strcmp(entry->funcName1, funcName1) == 0 &&
                strcmp(entry->varName2, returnVar2) == 0 &&
                strcmp(entry->funcName2, funcName2) == 0) {
                return 1;
            }

            if (strcmp(entry->varName1, returnVar2) == 0 &&
                strcmp(entry->funcName1, funcName2) == 0 &&
                strcmp(entry->varName2, returnVar1) == 0 &&
                strcmp(entry->funcName2, funcName1) == 0) {
                return 1;
            }

            entry = entry->next;
        }
    }

    fprintf(stderr, "matchReturnExpressions: Return expressions of '%s' and '%s' are NOT matched.\n", funcName1, funcName2);
    return 0;
}

//////////////////////////////////////////////////////////
//    GRADING WITH JSON
ScoreConfig loadScoreConfig(const char* filename) {
    ScoreConfig config;
    config.functions = NULL;
    config.func_count = 0;
    config.function_calls = NULL;
    config.call_count = 0;
    config.variables = NULL;
    config.var_count = 0;

    if(!filename) {
        fprintf(stderr, "[loadScoreConfig] Filename is NULL.\n");
        return config;
    }

    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return config;
    }

    fseek(f,0,SEEK_END);
    long len = ftell(f);
    rewind(f);
    if(len<=0) {
        fprintf(stderr, "[loadScoreConfig] Empty or invalid file.\n");
        fclose(f);
        return config;
    }

    char* data = (char*)malloc(len+1);
    if (!data) {
        fclose(f);
        fprintf(stderr, "Memory allocation failed while reading config.\n");
        return config;
    }
    size_t readCount = fread(data,1,len,f);
    data[len]='\0';
    fclose(f);

    if (readCount != (size_t)len) {
        fprintf(stderr,"[loadScoreConfig] File read mismatch.\n");
        free(data);
        return config;
    }

    cJSON* json = cJSON_Parse(data);
    free(data);
    if (!json) {
        fprintf(stderr,"Failed to parse JSON\n");
        return config;
    }

    // FUNCTIONS
    cJSON* funcs = cJSON_GetObjectItem(json,"functions");
    if (cJSON_IsArray(funcs)) {
        config.func_count = cJSON_GetArraySize(funcs);
        if(config.func_count>0) {
            config.functions = (FunctionScore*)malloc(sizeof(FunctionScore)*config.func_count);
            if(!config.functions) {
                fprintf(stderr,"Memory allocation failed for functions.\n");
                cJSON_Delete(json);
                return config;
            }
            for (int i=0; i<config.func_count; i++) {
                cJSON* fobj = cJSON_GetArrayItem(funcs,i);
                FunctionScore fs;
                memset(&fs, 0, sizeof(fs));
                if(!fobj) {
                    fs.teacher_name[0]='\0';
                    fs.points=0;
                    fs.param_count=0;
                    config.functions[i]=fs;
                    continue;
                }

                cJSON* tname = cJSON_GetObjectItem(fobj,"teacher_name");
                cJSON* points = cJSON_GetObjectItem(fobj,"points");
                cJSON* tparams = cJSON_GetObjectItem(fobj,"parameters");
                cJSON* retvar = cJSON_GetObjectItem(fobj,"return_var");

                if(tname && tname->valuestring && strlen(tname->valuestring)>0) {
                    safeStringCopy(fs.teacher_name, sizeof(fs.teacher_name), tname->valuestring);
                } else {
                    fprintf(stderr,"[loadScoreConfig] 'teacher_name' missing or empty in functions[%d].\n", i);
                    fs.teacher_name[0] = '\0';
                }

                fs.points = points ? points->valueint : 0;

                int paramCount = 0;
                if(cJSON_IsArray(tparams)) {
                    paramCount = cJSON_GetArraySize(tparams);
                    if(paramCount > 10) paramCount = 10;
                    fs.param_count = paramCount;
                    for (int p=0; p<paramCount; p++) {
                        cJSON* par = cJSON_GetArrayItem(tparams,p);
                        if(par && par->valuestring)
                            safeStringCopy(fs.parameters[p], sizeof(fs.parameters[p]), par->valuestring);
                        else
                            fs.parameters[p][0] = '\0';
                    }
                    for(int p=paramCount; p<10; p++) {
                        fs.parameters[p][0] = '\0';
                    }
                }
                if(retvar && retvar->valuestring)
                    safeStringCopy(fs.return_var, sizeof(fs.return_var), retvar->valuestring);

                config.functions[i] = fs;
            }
        }
    }

    // FUNCTION_CALLS
    cJSON* calls = cJSON_GetObjectItem(json,"function_calls");
    if (cJSON_IsArray(calls)) {
        config.call_count = cJSON_GetArraySize(calls);
        if(config.call_count>0) {
            config.function_calls = (FunctionCallScore*)malloc(sizeof(FunctionCallScore)*config.call_count);
            if(!config.function_calls) {
                fprintf(stderr,"Memory allocation failed for function_calls.\n");
                cJSON_Delete(json);
                return config;
            }
            for (int i=0; i<config.call_count; i++) {
                cJSON* cobj = cJSON_GetArrayItem(calls,i);
                FunctionCallScore fcs;
                memset(&fcs, 0, sizeof(fcs));

                cJSON* desc = cJSON_GetObjectItem(cobj,"description");
                cJSON* loc = cJSON_GetObjectItem(cobj,"location");
                cJSON* cpoints = cJSON_GetObjectItem(cobj,"points");
                cJSON* tcall = cJSON_GetObjectItem(cobj,"teacher_call");

                if(desc && desc->valuestring)
                    safeStringCopy(fcs.description, sizeof(fcs.description), desc->valuestring);
                else
                    fcs.description[0]='\0';

                if(loc && loc->valuestring)
                    safeStringCopy(fcs.location, sizeof(fcs.location), loc->valuestring);
                else
                    fcs.location[0]='\0';

                fcs.points = cpoints ? cpoints->valueint : 0;

                if(tcall) {
                    cJSON* tc_fname = cJSON_GetObjectItem(tcall,"function_name");
                    cJSON* tc_pcount = cJSON_GetObjectItem(tcall,"param_count");
                    cJSON* tc_params = cJSON_GetObjectItem(tcall,"params");

                    if(tc_fname && tc_fname->valuestring)
                        safeStringCopy(fcs.teacher_call.function_name,sizeof(fcs.teacher_call.function_name),tc_fname->valuestring);
                    else
                        fcs.teacher_call.function_name[0]='\0';

                    int paramCount = (tc_pcount ? tc_pcount->valueint : 0);
                    if(paramCount<0) paramCount=0;
                    if(paramCount>10) paramCount=10; 
                    fcs.teacher_call.param_count = paramCount;

                    if(cJSON_IsArray(tc_params)) {
                        for (int p=0; p<paramCount; p++) {
                            cJSON* param = cJSON_GetArrayItem(tc_params,p);
                            if(!param) continue;
                            cJSON* ptype = cJSON_GetObjectItem(param,"type");
                            if(!ptype || !ptype->valuestring) {
                                fprintf(stderr,"[loadScoreConfig] Missing param type at %d in call[%d].\n", p,i);
                                continue;
                            }

                            TeacherCallParam* cp = &fcs.teacher_call.params[p];
                            memset(cp,0,sizeof(*cp));
                            safeStringCopy(cp->type,sizeof(cp->type), ptype->valuestring);

                            if(strcmp(cp->type,"constant")==0) {
                                cJSON* val = cJSON_GetObjectItem(param,"value");
                                cp->value = val ? val->valueint : 0;
                                cp->expr[0]='\0';
                                cp->fcall_function_name[0]='\0';
                                cp->fcall_param_count=0;
                                cp->fcall_params=NULL;
                            } else if(strcmp(cp->type,"variable_expression")==0) {
                                cJSON* exprVal = cJSON_GetObjectItem(param,"expr");
                                if(exprVal && exprVal->valuestring)
                                    safeStringCopy(cp->expr,sizeof(cp->expr), exprVal->valuestring);
                                else
                                    cp->expr[0]='\0';
                                cp->value=0;
                                cp->fcall_function_name[0]='\0';
                                cp->fcall_param_count=0;
                                cp->fcall_params=NULL;
                            } else if(strcmp(cp->type,"function_call")==0) {
                                // Nested function call
                                cJSON* fname = cJSON_GetObjectItem(param,"function_name");
                                cJSON* fpc = cJSON_GetObjectItem(param,"param_count");
                                cJSON* fparams = cJSON_GetObjectItem(param,"params");

                                cp->value=0;
                                cp->expr[0]='\0';

                                if(fname && fname->valuestring)
                                    safeStringCopy(cp->fcall_function_name,sizeof(cp->fcall_function_name),fname->valuestring);
                                else
                                    cp->fcall_function_name[0]='\0';

                                int fpCount = fpc ? fpc->valueint : 0;
                                if(fpCount<0) fpCount=0;
                                cp->fcall_param_count = fpCount;
                                if(fpCount>0 && cJSON_IsArray(fparams)) {
                                    cp->fcall_params = (TeacherCallParam*)calloc(fpCount,sizeof(TeacherCallParam));
                                    for(int fp=0; fp<fpCount; fp++) {
                                        cJSON* fparam = cJSON_GetArrayItem(fparams,fp);
                                        if(!fparam) continue;
                                        cJSON* fptype = cJSON_GetObjectItem(fparam,"type");
                                        if(!fptype || !fptype->valuestring) continue;
                                        TeacherCallParam* fpParam = &cp->fcall_params[fp];
                                        memset(fpParam,0,sizeof(*fpParam));
                                        safeStringCopy(fpParam->type,sizeof(fpParam->type),fptype->valuestring);

                                        if(strcmp(fpParam->type,"constant")==0) {
                                            cJSON* val = cJSON_GetObjectItem(fparam,"value");
                                            fpParam->value = val ? val->valueint : 0;
                                            fpParam->expr[0]='\0';
                                            fpParam->fcall_function_name[0]='\0';
                                            fpParam->fcall_param_count=0;
                                            fpParam->fcall_params=NULL;
                                        } else if(strcmp(fpParam->type,"variable_expression")==0) {
                                            cJSON* exprVal = cJSON_GetObjectItem(fparam,"expr");
                                            if(exprVal && exprVal->valuestring)
                                                safeStringCopy(fpParam->expr,sizeof(fpParam->expr),exprVal->valuestring);
                                            else
                                                fpParam->expr[0]='\0';
                                            fpParam->value=0;
                                            fpParam->fcall_function_name[0]='\0';
                                            fpParam->fcall_param_count=0;
                                            fpParam->fcall_params=NULL;
                                        } else {
                                            fpParam->value=0;
                                            fpParam->expr[0]='\0';
                                            fpParam->fcall_function_name[0]='\0';
                                            fpParam->fcall_param_count=0;
                                            fpParam->fcall_params=NULL;
                                        }
                                    }
                                } else {
                                    cp->fcall_params=NULL;
                                }

                            } else {
                                fprintf(stderr,"Unknown param type '%s' at param %d in call[%d].\n", cp->type,p,i);
                                cp->type[0]='\0';
                            }
                        }
                    }
                }

                config.function_calls[i] = fcs;
            }
        }
    }

    // VARIABLES
    cJSON* vars = cJSON_GetObjectItem(json,"variables");
    if (cJSON_IsArray(vars)) {
        config.var_count = cJSON_GetArraySize(vars);
        if(config.var_count>0) {
            config.variables = (VariableScore*)malloc(sizeof(VariableScore)*config.var_count);
            if(!config.variables) {
                fprintf(stderr,"Memory allocation failed for variables.\n");
                cJSON_Delete(json);
                return config;
            }
            for (int i=0; i<config.var_count; i++) {
                cJSON* vobj = cJSON_GetArrayItem(vars,i);
                VariableScore vs;
                memset(&vs,0,sizeof(vs));
                if(!vobj) {
                    vs.teacher_function[0]='\0';
                    vs.teacher_var[0]='\0';
                    vs.points=0;
                    config.variables[i]=vs;
                    continue;
                }

                cJSON* tf = cJSON_GetObjectItem(vobj,"teacher_function");
                cJSON* tv = cJSON_GetObjectItem(vobj,"teacher_var");
                cJSON* vpts = cJSON_GetObjectItem(vobj,"points");

                if(tf && tf->valuestring && strlen(tf->valuestring)>0)
                    safeStringCopy(vs.teacher_function, sizeof(vs.teacher_function), tf->valuestring);
                else {
                    fprintf(stderr,"[loadScoreConfig] 'teacher_function' missing or empty in variables[%d].\n", i);
                    vs.teacher_function[0] = '\0';
                }

                if(tv && tv->valuestring && strlen(tv->valuestring)>0)
                    safeStringCopy(vs.teacher_var, sizeof(vs.teacher_var), tv->valuestring);
                else {
                    fprintf(stderr,"[loadScoreConfig] 'teacher_var' missing or empty in variables[%d].\n", i);
                    vs.teacher_var[0] = '\0';
                }

                vs.points = vpts ? vpts->valueint : 0;
                config.variables[i] = vs;
            }
        }
    }

    cJSON_Delete(json);
    return config;
}

int calculateScore(ScoreConfig config, MatchTable* table, FunctionCallMatchTable* fcmt,
                   VarEntry* variableTable1, VarEntry* variableTable2) {
    fprintf(stderr, "\n[calculateScore] Starting score calculation.");

    if(!table || !fcmt) {
       // fprintf(stderr, "[calculateScore] Invalid arguments: table or fcmt is NULL.\n");
        return 0;
    }

    int totalScore = 0;

    // 1) Fonksiyonları puanla
    for (int i = 0; i < config.func_count && config.functions; i++) {
        FunctionScore fs = config.functions[i];
        if (strlen(fs.teacher_name) == 0) {
            continue;
        }

        char studentName[50];
        memset(studentName, 0, sizeof(studentName));

        if (!getMatchedStudentFunctionName(table, fs.teacher_name, studentName, sizeof(studentName))) {
            continue;
        }

        char teacherReturnVar[256];
        snprintf(teacherReturnVar, sizeof(teacherReturnVar),
                 "Return Expression in %s", fs.teacher_name);

        char studentReturnVar[256];
        snprintf(studentReturnVar, sizeof(studentReturnVar),
                 "Return Expression in %s", studentName);

        int isReturnMatched = checkMatchTableForVariables(
            table,
            teacherReturnVar, fs.teacher_name,
            studentReturnVar, studentName,
            variableTable1, variableTable2
        );

        if (!isReturnMatched) {
            continue;
        }

        totalScore += fs.points;
    }

    // Fonksiyon çağrılarını puanla
    for (int i=0; i<config.call_count && config.function_calls; i++) {
        FunctionCallScore fcs = config.function_calls[i];

        char* teacherCallStr = buildTeacherCallString(fcs.teacher_call.function_name,
                                                      fcs.teacher_call.param_count,
                                                      fcs.teacher_call.params);
        if(!teacherCallStr) {
         //   fprintf(stderr,"[calculateScore] Failed to build teacherCallStr for '%s'.\n", fcs.description);
            continue;
        }

        if (checkFunctionCallMatch(table, fcmt, variableTable1, variableTable2, &config, &fcs)) {
            totalScore += fcs.points;
        } else {
          //  fprintf(stderr, "[calculateScore] Function call '%s' not matched. No points.\n", fcs.description);
        }

        free(teacherCallStr); 
    }

    // Değişkenleri puanla
    for (int i=0; i<config.var_count && config.variables; i++) {
        VariableScore vs = config.variables[i];
        if(strlen(vs.teacher_function)==0 || strlen(vs.teacher_var)==0) {
         //   fprintf(stderr,"[calculateScore] teacher_function or teacher_var empty for variable score entry %d.\n", i);
            continue;
        }

        if (areVariablesMatched(table, vs.teacher_function, vs.teacher_var)) {
            totalScore += vs.points;
        } else {
         //   fprintf(stderr, "[calculateScore] Variable '%s' (in '%s') not matched. No points.\n",
           //         vs.teacher_var, vs.teacher_function);
        }
    }

    int maxPossible = 0;
    for (int i = 0; i < config.func_count; i++) {
        maxPossible += config.functions[i].points;
    }
    for (int i = 0; i < config.call_count; i++) {
        maxPossible += config.function_calls[i].points;
    }
    for (int i = 0; i < config.var_count; i++) {
        maxPossible += config.variables[i].points;
    }

    if (maxPossible > 0) {
        double normalized = (100.0 * (double)totalScore) / (double)maxPossible;
        fprintf(stderr, "Normalized Score: %.2f%%\n", normalized);
    } else {
        fprintf(stderr, "No points in config (maxPossible=0). Normalized Score: N/A\n");
    }

  //  fprintf(stderr, "[calculateScore] Total Score Computed: %d\n", totalScore);
    return totalScore;
}

int getMatchedStudentFunctionName(MatchTable* table, const char* teacherName, char* studentName, int size) {
    if (!table || !teacherName || !studentName || size <= 0) {
     //   fprintf(stderr, "[getMatchedStudentFunctionName] Invalid arguments.\n");
        return 0;
    }

    if(strlen(teacherName)==0) {
     //   fprintf(stderr, "[getMatchedStudentFunctionName] teacherName is empty.\n");
        return 0;
    }

    if(table->size<=0 || !table->entries) {
      //  fprintf(stderr, "[getMatchedStudentFunctionName] Empty MatchTable.\n");
        return 0;
    }

    char teacherReturnVar[256];
    snprintf(teacherReturnVar, sizeof(teacherReturnVar), "Return Expression in %s", teacherName);

    for (int i=0; i<table->size; i++) {
        MatchEntry* e = table->entries[i];
        while (e) {
                if (strcmp(e->varName1, teacherReturnVar)==0 && strcmp(e->funcName1, teacherName)==0) {
                    if(e->funcName2 && strlen(e->funcName2)>0) {
                        safeStringCopy(studentName, size, e->funcName2);
                        return 1;
                    }
                } else if (strcmp(e->varName2, teacherReturnVar)==0 && strcmp(e->funcName2, teacherName)==0) {
                    if(e->funcName1 && strlen(e->funcName1)>0) {
                        safeStringCopy(studentName, size, e->funcName1);
                        return 1;
                    }
                }
            e = e->next;
        }
    }

  //  fprintf(stderr, "[getMatchedStudentFunctionName] No matched student function for teacher '%s'.\n", teacherName);
    return 0;
}

int areVariablesMatched(MatchTable* table, const char* teacherFunction, const char* teacherVar) {

    if(!table || !teacherFunction || !teacherVar) {
      //  fprintf(stderr, "[areVariablesMatched] Invalid arguments.\n");
        return 0;
    }

    if(strlen(teacherFunction)==0 || strlen(teacherVar)==0) {
      //  fprintf(stderr,"[areVariablesMatched] teacherFunction or teacherVar empty.\n");
        return 0;
    }

    if(table->size<=0 || !table->entries) {
      //  fprintf(stderr, "[areVariablesMatched] Empty MatchTable.\n");
        return 0;
    }

    for (int i = 0; i < table->size; i++) {
        MatchEntry* entry = table->entries[i];
        while (entry) {
            if (((strcmp(entry->varName1, teacherVar)==0 && strcmp(entry->funcName1, teacherFunction)==0) ||
                 (strcmp(entry->varName2, teacherVar)==0 && strcmp(entry->funcName2, teacherFunction)==0))) {
            //    fprintf(stderr, "[areVariablesMatched] Variable '%s' in '%s' matched.\n", teacherVar, teacherFunction);
                return 1;
            }
            entry = entry->next;
        }
    }

  //  fprintf(stderr, "[areVariablesMatched] Variable '%s' in '%s' not matched.\n", teacherVar, teacherFunction);
    return 0;
}

int checkFunctionCallMatch(MatchTable* table, FunctionCallMatchTable* fcmt, VarEntry* variableTable1, VarEntry* variableTable2,
                           ScoreConfig* config, FunctionCallScore* fcScore) {
    if (!fcScore) {
      //  fprintf(stderr, "[checkFunctionCallMatch] fcScore is NULL.\n");
        return 0;
    }

    char* teacherCallStr = buildTeacherCallString(fcScore->teacher_call.function_name,
                                                  fcScore->teacher_call.param_count,
                                                  fcScore->teacher_call.params);
    if (!teacherCallStr) {
      //  fprintf(stderr, "[checkFunctionCallMatch] Failed to build teacherCallStr.\n");
        return 0;
    }

   // fprintf(stderr,"[checkFunctionCallMatch] Teacher call string: %s\n", teacherCallStr);

    int matched = 0;
    for (int i = 0; i < fcmt->size && !matched; i++) {
        FunctionCallMatchEntry* entry = fcmt->entries[i];
        while (entry) {
            if ((strcmp(entry->functionCall1, teacherCallStr) == 0) ||
                (strcmp(entry->functionCall2, teacherCallStr) == 0)) {
                matched = 1;
                break;
            }
            entry = entry->next;
        }
    }

    free(teacherCallStr);
    return matched;
}

char* buildTeacherCallString(const char* funcName, int paramCount, TeacherCallParam* params) {
    if (!funcName) {
        fprintf(stderr, "[buildTeacherCallString] funcName is NULL\n");
        return strdup("unknown()");
    }

    size_t totalLen = strlen(funcName) + 3;

    char** paramStrings = (char**)calloc(paramCount, sizeof(char*));
    if (!paramStrings && paramCount > 0) {
        fprintf(stderr, "[buildTeacherCallString] Memory allocation failed for paramStrings.\n");
        return strdup("unknown()");
    }

    for (int i = 0; i < paramCount; i++) {
        char* currentParamStr = strdup("unknown_param");
        if (!currentParamStr) {
            fprintf(stderr, "[buildTeacherCallString] Memory allocation failed for currentParamStr.\n");
            for (int k = 0; k < i; k++) free(paramStrings[k]);
            free(paramStrings);
            return strdup("unknown()");
        }
        free(currentParamStr);

        if (strcmp(params[i].type, "function_call") == 0) {
            if (params[i].fcall_function_name[0] == '\0' || params[i].fcall_param_count < 0 || !params[i].fcall_params) {
                fprintf(stderr, "[buildTeacherCallString] Invalid function_call param data.\n");
                currentParamStr = strdup("unknown()");
            } else {
                currentParamStr = buildTeacherCallString(params[i].fcall_function_name,
                                                         params[i].fcall_param_count,
                                                         params[i].fcall_params);
                if (!currentParamStr) {
                    fprintf(stderr, "[buildTeacherCallString] Nested call returned NULL.\n");
                    currentParamStr = strdup("unknown()");
                }
            }
        } else if (strcmp(params[i].type, "constant") == 0) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%d", params[i].value);
            currentParamStr = strdup(buffer);
        } else if (strcmp(params[i].type, "variable_expression") == 0) {
            if (params[i].expr[0] == '\0') {
                fprintf(stderr, "[buildTeacherCallString] Variable expression is empty.\n");
                currentParamStr = strdup("unknown_expr");
            } else {
                currentParamStr = strdup(params[i].expr);
            }
        } else {
            fprintf(stderr, "[buildTeacherCallString] Unknown param type: %s\n", params[i].type);
            currentParamStr = strdup("unknown_param");
        }

        if (!currentParamStr) {
            fprintf(stderr, "[buildTeacherCallString] Memory allocation failed for param.\n");
            currentParamStr = strdup("unknown_param");
        }

        paramStrings[i] = currentParamStr;
        totalLen += strlen(paramStrings[i]) + 1;
    }

    char* result = (char*)malloc(totalLen);
    if (!result) {
        fprintf(stderr, "[buildTeacherCallString] Memory allocation failed for result.\n");
        for (int i = 0; i < paramCount; i++) {
            if (paramStrings[i]) free(paramStrings[i]);
        }
        free(paramStrings);
        return strdup("unknown()");
    }

    snprintf(result, totalLen, "%s(", funcName);
    for (int i = 0; i < paramCount; i++) {
        strcat(result, paramStrings[i]);
        if (i < paramCount - 1)
            strcat(result, ",");
    }
    strcat(result, ")");

    for (int i = 0; i < paramCount; i++) {
        free(paramStrings[i]);
    }
    free(paramStrings);

    return result;
}

static void safeStringCopy(char* dest, size_t destSize, const char* src) {
    if (!dest || destSize == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, destSize-1);
    dest[destSize-1] = '\0';
}

static void flattenAdditiveExpression(ASTNode* expr, int parentSign, AddTermList* list, 
                                      const char* functionName, VarEntry* variableTable, int depth)
{
    if (!expr) return;

    // expr->type AdditiveExpression ise ve expr->name '+' veya '-'
    if (expr->type == NodeType_AdditiveExpression &&
        (strcmp(expr->name, "+") == 0 || strcmp(expr->name, "-") == 0))
    {
        ASTNode* left = expr->children[0];
        ASTNode* right = expr->children[1];

        // Soldaki terim
        if (left->type == NodeType_AdditiveExpression &&
           (strcmp(left->name, "+") == 0 || strcmp(left->name, "-") == 0))
        {
            flattenAdditiveExpression(left, parentSign, list, functionName, variableTable, depth+1);
        } else {
            pushAddTerm(list, parentSign, left);
        }

        // '-' ise sağ terimin işaretini çevir
        int signForRight = parentSign;
        if (strcmp(expr->name, "-") == 0) {
            signForRight = (parentSign == +1) ? -1 : +1;
        }
        if (right->type == NodeType_AdditiveExpression &&
           (strcmp(right->name, "+") == 0 || strcmp(right->name, "-") == 0))
        {
            flattenAdditiveExpression(right, signForRight, list, functionName, variableTable, depth+1);
        } else {
            pushAddTerm(list, signForRight, right);
        }
    }
    else {
        pushAddTerm(list, parentSign, expr);
    }
}

static int compareFlattenedAdditive(AddTermList* list1, AddTermList* list2, 
                                    const char* functionName1, const char* functionName2,
                                    MatchTable* table, VarEntry* varTable1, VarEntry* varTable2,
                                    FunctionCallMatchTable* fcMatch, int depth,
                                    int isReturnComparison, int isParameterComparison)
{
    if (list1->count != list2->count) {
        return 0;
    }

    int used2[256];
    memset(used2, 0, sizeof(used2));

    int matchedCount = 0;

    for (int i = 0; i < list1->count; i++) {
        int foundMatch = 0;
        for (int j = 0; j < list2->count; j++) {
            if (!used2[j]) {
                if (list1->terms[i].sign == list2->terms[j].sign) {
                    int score = compareExpressionsInDetail(
                        list1->terms[i].operand,
                        list2->terms[j].operand,
                        functionName1, functionName2,
                        table, varTable1, varTable2,
                        isReturnComparison, isParameterComparison,
                        fcMatch, depth+1
                    );
                    if (score > 0) {
                        used2[j] = 1;
                        matchedCount++;
                        foundMatch = 1;
                        break;
                    }
                }
            }
        }
        if (!foundMatch) {
            return 0;
        }
    }

    return (matchedCount == list1->count) ? 1 : 0;
}





























