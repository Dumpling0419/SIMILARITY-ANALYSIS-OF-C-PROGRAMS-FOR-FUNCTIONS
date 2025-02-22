#include <stdio.h>
#include "ast.h"
#include <stdlib.h>
#include <string.h>

const int initial_capacity = 10;

int max(int a, int b) {
    return (a > b) ? a : b;
}

ASTNode* createFunctionNode(char* name, ASTNode* paramList, ASTNode* body) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for function node.\n");
        return NULL;
    }

    node->type = NodeType_FunctionDef;
    node->name = name ? strdup(name) : NULL;
    node->children = malloc(sizeof(ASTNode*) * 2); // One for paramList, one for body
    node->childCount = 0;

    if (paramList) {
        node->children[node->childCount++] = paramList;
        fprintf(stderr, "Param list added to function node: %s\n", name);
    }

    ASTNode* bodyNode = createASTNode(NodeType_Body, "Body");
    if (!bodyNode) {
        fprintf(stderr, "Failed to create body node.\n");
        free(node);
        return NULL;
    }
    
    for (int i = 0; i < body->childCount; i++) {
        addASTChild(bodyNode, body->children[i]);
    }

    node->children[node->childCount++] = bodyNode;

    fprintf(stderr, "Created Function Node: %s with %d children\n", name, node->childCount);
    return node;
}

ASTNode* createMainFunctionNode(char* name, ASTNode* compoundStatement) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for main function node.\n");
        return NULL;
    }

    node->type = NodeType_MainFunction;
    node->name = strdup(name);
    node->children = malloc(sizeof(ASTNode*));
    node->children[0] = compoundStatement;
    node->childCount = 1;  // Sadece bir body var.

    return node;
}

ASTNode* createFunctionCallNode(char* name, ASTNode* params) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for function call node.\n");
        return NULL;
    }

    node->type = NodeType_FunctionCall;
    node->name = strdup(name);
    node->childCount = params->childCount;
    node->children = malloc(sizeof(ASTNode*) * node->childCount);

    if (!node->children) {
        fprintf(stderr, "Failed to allocate memory for children of function call node.\n");
        free(node->name);
        free(node);
        return NULL;
    }

    for (int i = 0; i < node->childCount; i++) {
        node->children[i] = params->children[i];
    }

    return node;
}

ASTNode* createParameterNode(char* name) {
    return createASTNode(NodeType_Parameter, name);
}

ASTNode* createCallNode(char* name, ASTNode** args, int argCount) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NodeType_FunctionCall;
    node->name = strdup(name);
    node->params = args;
    node->paramCount = argCount;
    return node;
}

ASTNode* findReturnStatement(ASTNode* body) {
    if (!body || body->type != NodeType_Statements) return NULL;
    for (int i = 0; i < body->childCount; i++) {
        if (body->children[i]->type == NodeType_Return) {
            return body->children[i];
        }
    }
    return NULL;
}

ASTNode* findFunctionCall(ASTNode *node) {
    for (int i = 0; i < node->childCount; i++) {
        if (node->children[i]->type == NodeType_FunctionCall) {
            return node->children[i];
        }
    }
    return NULL;
}

ASTNode* createASTNode(NodeType type, char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for ASTNode.\n");
        return NULL; 
    }
    node->type = type;
    node->name = name ? strdup(name) : NULL;
    node->children = NULL;
    node->childCount = 0;
    node->params = NULL;
    node->paramCount = 0;

    if (type == NodeType_Functions || type == NodeType_MainFunction) {
        node->children = malloc(sizeof(ASTNode*) * initial_capacity);
        if (!node->children) {
            fprintf(stderr, "Failed to allocate memory for children nodes.\n");
            free(node->name);
            free(node);
            return NULL;
        }
    }

    fprintf(stderr, "Created AST node: Type=%d, Name=%s\n", type, node->name ? node->name : "NULL");
    return node;
}

void printASTNode(ASTNode* node, int level) {
    if (!node) {
        printf("Node is NULL.\n");
        return;
    }

    for (int i = 0; i < level; i++) {
        printf("  ");
    }

    printf("Node Type: %d, Name: %s", node->type, node->name);

    if (node->params) {
        printf(", Parameters: ");
        for (int i = 0; i < node->paramCount; i++) {
        	printf("%s ", node->params[i]->name);
    	}
    }

    printf(", Child Count: %d\n", node->childCount);

    for (int i = 0; i < node->childCount; i++) {
        printASTNode(node->children[i], level + 1);
    }
}

void printAST(ASTNode* root) {
    if (!root) {
        printf("Root is NULL.\n");
        return;
    }

    printf("Printing AST:\n");
    printASTNode(root, 0);
}

int countParams(ASTNode* paramList) {
    if (paramList == NULL) {
        return 0;
    }
    return paramList->childCount;
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

int isFunctionDefinition(ASTNode* node) {
    return node && node->type == NodeType_FunctionDef;
}

void addASTChild(ASTNode* parent, ASTNode* child) {
    if (!parent || !child) {
        fprintf(stderr, "Error: NULL parent or child in addASTChild.\n");
        return;
    }

    if (parent->children == NULL) {
        parent->children = malloc(sizeof(ASTNode*) * initial_capacity);
        parent->childCount = 0;
        if (!parent->children) {
            fprintf(stderr, "Memory allocation failed for children array in addASTChild.\n");
            return;
        }
    }

    if (parent->childCount % initial_capacity == 0 && parent->childCount > 0) {
        ASTNode** temp = realloc(parent->children, sizeof(ASTNode*) * (parent->childCount + initial_capacity));
        if (!temp) {
            fprintf(stderr, "Memory reallocation failed for children array in addASTChild.\n");
            return;
        }
        parent->children = temp;
    }

    parent->children[parent->childCount++] = child;
    fprintf(stderr, "Log: Added child node type %d, name %s to parent node type %d, name %s\n", child->type, child->name, parent->type, parent->name);
}

int compareASTNodes(ASTNode *node1, ASTNode *node2) {
    if (!node1 || !node2) {
        fprintf(stderr, "Error: One or both nodes are NULL in compareASTNodes.\n");
        return 0;
    }

    int score = 0;
    if (node1->type == node2->type) {
        score = compareSameTypeNodes(node1, node2);
    } else {
        score = compareDifferentTypeNodes(node1, node2);
    }
    
    fprintf(stderr, "Comparing nodes %s and %s: Score %d\n", node1->name, node2->name, score);
    return score;
}

int compareSameTypeNodes(ASTNode *node1, ASTNode *node2) {
    int score = 0;
    switch (node1->type) {
        case NodeType_Expression:
            score = compareExpressions(node1, node2) * 10;
            break;
        case NodeType_Assignment:
            score = compareAssignments(node1, node2) * 5;
            break;
        case NodeType_FunctionCall:
            score = compareFunctionCalls(node1, node2) * 15;
            break;
        case NodeType_Return:
            score = compareReturnStatements(node1, node2) * 10;
            break;
        case NodeType_FunctionDef:
            score += compareFunctionDefinition(node1, node2);
            break;
        default:
            score = genericNodeComparison(node1, node2);
            break;
    }
    return score;
}

int compareDifferentTypeNodes(ASTNode* node1, ASTNode* node2) {
    if (!node1 || !node2 || node1->type == node2->type) {
        return 0;
    }

    if ((node1->type == NodeType_Assignment && node2->type == NodeType_FunctionCall) ||
        (node2->type == NodeType_Assignment && node1->type == NodeType_FunctionCall)) {
        return compareAssignmentAndFunctionCall(node1, node2);
    }
        if ((node1->type == NodeType_Assignment && node2->type == NodeType_Return) ||
        (node2->type == NodeType_Assignment && node1->type == NodeType_Return)) {
        return compareAssignmentAndReturn(node1, node2);
    }

    return 0;
}


int compareAssignmentAndFunctionCall(ASTNode* assign, ASTNode* funcCall) {
    ASTNode *assignment = (assign->type == NodeType_Assignment) ? assign : funcCall;
    ASTNode *functionCall = (funcCall->type == NodeType_FunctionCall) ? funcCall : assign;

    if (assignment->children[0] && functionCall->children[0]) {
        return compareExpressionsIgnoringNames(assignment->children[1], functionCall->children[0]);
    }
    return 0;
}

int compareAssignmentAndReturn(ASTNode* assign, ASTNode* ret) {
    ASTNode *assignment = (assign->type == NodeType_Assignment) ? assign : ret;
    ASTNode *returnStmt = (ret->type == NodeType_Return) ? ret : assign;

    if (assignment->children[1] && returnStmt->children[0]) {
        return compareExpressionsIgnoringNames(assignment->children[1], returnStmt->children[0]) * 10;
    }
    return 0;
}

/*
int deepCompareASTNodes(ASTNode *node1, ASTNode *node2) {
    if (node1->type != node2->type) return 0;
    int score = 0;
    switch (node1->type) {
        case NodeType_Expression:
        case NodeType_Statements:
            score = compareExpressions(node1, node2) ? 10 : 0;
            break;
        case NodeType_FunctionCall:
            score = compareExpressions(node1, node2) ? 30 : 0;
            break;
        default:
            score = compareFunctionBodies(node1, node2);
            break;
    }
    return score;
}
*/
int calculateFunctionSimilarity(ASTNode *node1, ASTNode *node2) {
    if (!node1 || !node2 || node1->type != NodeType_FunctionDef || node2->type != NodeType_FunctionDef) {
        return 0;
    }

    fprintf(stderr, "Comparing function definitions: %s vs %s\n", node1->name, node2->name);
    int score = 0;

    if (node1->paramCount == node2->paramCount) {
        score += 30;
        fprintf(stderr, "Parameter counts match. Added 30 points.\n");
    }

    ASTNode* body1 = node1->children[1];
    ASTNode* body2 = node2->children[1];
    if (body1 && body2) {
        int bodyScore = compareBodies(body1, body2);
        score += bodyScore;
        if (bodyScore >= 70) {
            fprintf(stderr, "Function bodies match completely.\n");
        } else {
            fprintf(stderr, "Function bodies do not match completely.\n");
        }
    } else {
        fprintf(stderr, "One or both function bodies are NULL.\n");
    }

    ASTNode* return1 = findReturnStatement(node1->body);
    ASTNode* return2 = findReturnStatement(node2->body);
    if (return1 && return2) {
        int returnScore = compareReturnStatements(return1, return2);
        if (returnScore > 0) {
            score += 50;
            fprintf(stderr, "Return statements match. Added 50 points.\n");
        } else {
            fprintf(stderr, "Return statements do not match.\n");
        }
    }

    return score;
}

int compareControlStructures(ASTNode* node1, ASTNode* node2) {
    if (node1->type != node2->type) {
        return 0;
    }

    int score = 10;
    if (node1->condition && node2->condition) {
        score += compareExpressions(node1->condition, node2->condition);
    } else if (node1->condition || node2->condition) {
        score -= 5;
    }

    if (node1->body && node2->body) {
        score += compareFunctionBodies(node1->body, node2->body);
    } else if (node1->body || node2->body) {
        score -= 5;
    }

    return score;
}

int compareFunctionSignatures(ASTNode* node1, ASTNode* node2) {
    if (!node1 || !node2) return 0;
    if (node1->type != NodeType_FunctionDef || node2->type != NodeType_FunctionDef) return 0;
    if (node1->paramCount != node2->paramCount) return 0;
    int score = 10;
    for (int i = 0; i < node1->paramCount; i++) {
        if (strcmp(node1->params[i]->dataType, node2->params[i]->dataType) != 0) {
            score -= 5;
        }
    }
    return score > 0 ? score : 0;
}

int compareReturnStatements(ASTNode *node1, ASTNode *node2) {
    if (!node1 || !node2) {
        fprintf(stderr, "Error: One or both of the return statement nodes are NULL.\n");
        return 0;
    }

    if (node1->type != NodeType_Return || node2->type != NodeType_Return) {
        fprintf(stderr, "Error: Comparing non-return type nodes in return statements.\n");
        return 0;
    }

    if (node1->children && node2->children && node1->childCount > 0 && node2->childCount > 0) {
        int score = compareExpressions(node1->children[0], node2->children[0]);
        fprintf(stderr, "Return statement comparison score: %d\n", score);
        return score;
    } else {
        fprintf(stderr, "Error: One or both return statements do not have enough children.\n");
        return 0;
    }
}

int compareFunctionBodies(ASTNode* body1, ASTNode* body2) {
    if (!body1 || !body2) return 0;
    if (body1->childCount != body2->childCount) return 0;

    int score = 0;
    for (int i = 0; i < body1->childCount; i++) {
    	if (body1->children[i]->type != body2->children[i]->type) {
            score += compareDifferentTypeNodes(body1->children[i], body2->children[i]);
        } else {
        score += compareASTNodes(body1->children[i], body2->children[i]);
        }
    }
    return score;
}

int compareExpressions(ASTNode *expr1, ASTNode *expr2) {
    if (!expr1 || !expr2) return 0;
    if (expr1->type != expr2->type) return 0;
    fprintf(stderr, "Comparing Expressions: %s vs %s\n", expr1->name, expr2->name);

    if (expr1->type == NodeType_Expression) {
        if (!strcmp(expr1->name, expr2->name)) {
            int leftScore = compareExpressionsIgnoringNames(expr1->children[0], expr2->children[0]);
            int rightScore = compareExpressionsIgnoringNames(expr1->children[1], expr2->children[1]);
            fprintf(stderr, "Operator match: %s, scores: %d (left), %d (right)\n", expr1->name, leftScore, rightScore);
            return (leftScore + rightScore) / 2;
        } else if (isCommutative(expr1->name) && isCommutative(expr2->name)) {
            int score1 = compareExpressionsIgnoringNames(expr1->children[0], expr2->children[1]);
            int score2 = compareExpressionsIgnoringNames(expr1->children[1], expr2->children[0]);
            fprintf(stderr, "Commutative match: %s, scores: %d, %d\n", expr1->name, score1, score2);
            return (score1 && score2) ? ((score1 + score2) / 2) : 0;
        }
    } else if (expr1->type == NodeType_Constant) {
        int result = strcmp(expr1->name, expr2->name) == 0 ? 10 : 0;
        fprintf(stderr, "Constant comparison: %s vs %s, result: %d\n", expr1->name, expr2->name, result);
        return result;
    }
    return 0;
}

int compareExpressionsIgnoringNames(ASTNode *expr1, ASTNode *expr2) {
    if (!expr1 || !expr2) return 0;
    if (expr1->type != expr2->type) return 0;

    if (expr1->type == NodeType_Expression) {
        if (strcmp(expr1->name, expr2->name) == 0 || (isCommutative(expr1->name) && isCommutative(expr2->name))) {
            int leftScore = compareExpressionsIgnoringNames(expr1->children[0], expr2->children[1]);
            int rightScore = compareExpressionsIgnoringNames(expr1->children[1], expr2->children[0]);
            int normalOrderScore = compareExpressionsIgnoringNames(expr1->children[0], expr2->children[0]);
            int reverseOrderScore = compareExpressionsIgnoringNames(expr1->children[1], expr2->children[1]);
            return max(max(leftScore, rightScore), max(normalOrderScore, reverseOrderScore));
        }
    } else if (expr1->type == NodeType_Constant) {
        return strcmp(expr1->name, expr2->name) == 0 ? 10 : 0;
    }
    return 0;
}

int isCommutative(const char* opName) {
    return (strcmp(opName, "+") == 0 || strcmp(opName, "*") == 0);
}
/*
int recursiveBodyComparison(ASTNode* node1, ASTNode* node2) {
    if (node1 == NULL && node2 == NULL) {
        return 1;
    } else if (node1 == NULL || node2 == NULL) {
        return 0;
    }

    if (node1->type != node2->type) {
        return 0;
    }

    if (node1->childCount != node2->childCount) {
        return 0;
    }

    int score = 1;
    for (int i = 0; i < node1->childCount; i++) {
        score &= recursiveBodyComparison(node1->children[i], node2->children[i]);
    }
    return score;
}
*/
int compareFunctionCalls(ASTNode* call1, ASTNode* call2) {
    if (!call1 || !call2) return 0;
    fprintf(stderr, "Comparing Function Calls: %s vs %s\n", call1->name, call2->name);

    if (call1->type != NodeType_FunctionCall || call2->type != NodeType_FunctionCall) return 0;
    if (call1->paramCount != call2->paramCount) return 0;

    int score = 0;
    for (int i = 0; i < call1->paramCount; i++) {
        int paramScore = compareExpressions(call1->params[i], call2->params[i]);
        fprintf(stderr, "Param %d: %s vs %s, score: %d\n", i+1, call1->params[i]->name, call2->params[i]->name, paramScore);
        score += paramScore;
    }
    return (score == call1->paramCount * 10) ? 1 : 0;
}

int evaluateFunctionCallContext(ASTNode* call, ASTNode* context) {
    if (!call || !context) return 0;
    int score = 0;
    ASTNode* current = context;
    while (current) {
        if (current->type == NodeType_FunctionCall) {
            score += compareFunctionCalls(call, current);
        }
        current = current->parent;
    }
    return score;
}

int compareMainFunctions(ASTNode *main1, ASTNode *main2) {
    if (!main1 || !main2) return 0;
    if (main1->childCount != main2->childCount) return 0;

    int score = 0;
    for (int i = 0; i < main1->childCount; i++) {
        ASTNode* child1 = main1->children[i];
        ASTNode* child2 = main2->children[i];

        if (child1->type == NodeType_FunctionCall && child2->type == NodeType_FunctionCall) {
            score += compareFunctionCalls(child1, child2) * 20;
        } else if (child1->type == NodeType_Condition && child2->type == NodeType_Condition) {
            score += compareControlStructures(child1, child2) * 15;
        } else {
            score += compareASTNodes(child1, child2);
        }
    }
    return score;
}
/*
int compareStatementBlocks(ASTNode* node1, ASTNode* node2) {
    if (!node1 || !node2) return 0;
    if (node1->childCount != node2->childCount) return 0;

    int score = 0;
    for (int i = 0; i < node1->childCount; i++) {
        score += compareASTNodes(node1->children[i], node2->children[i]);
    }
    return score;
}
*/
int genericNodeComparison(ASTNode* node1, ASTNode* node2) {
    if (!node1 || !node2) return 0;
    if (node1->type != node2->type) return 0;

    int score = 0;
    switch(node1->type) {
        //case NodeType_Variable:
        //case NodeType_Function:
        case NodeType_Constant:
            score = compareConstants(node1, node2);
            break;
        default:
            score = basicSemanticMatch(node1, node2);
            break;
    }

    return score;
}

int compareParameters(ASTNode** params1, ASTNode** params2, int paramCount1, int paramCount2) {

    if (paramCount1 != paramCount2) return 0;
    int score = 0;
    for (int i = 0; i < paramCount1; i++) {
        score += compareASTNodes(params1[i], params2[i]);
    }
    return score;
}

int compareConstants(ASTNode* node1, ASTNode* node2) {
    if (strcmp(node1->name, node2->name) == 0)
        return 10;
    else
        return 0;
}

int basicSemanticMatch(ASTNode* node1, ASTNode* node2) {
    return 5;
}

int compareBodies(ASTNode* body1, ASTNode* body2) {
    if (!body1 || !body2) return 0;

    int maxScore = 10 * (body1->childCount > body2->childCount ? body1->childCount : body2->childCount);
    int score = 0;

    for (int i = 0; i < body1->childCount; i++) {
        int bestMatchScore = 0;
        for (int j = 0; j < body2->childCount; j++) {
            int matchScore = compareASTNodes(body1->children[i], body2->children[j]);
            if (matchScore > bestMatchScore) {
                bestMatchScore = matchScore;
            }
        }
        score += bestMatchScore;
    }

    int normalizedScore = (score * 70) / maxScore;
    fprintf(stderr, "Computed body score: %d (out of 70)\n", normalizedScore);
    return normalizedScore >= 35 ? normalizedScore : 0; 
}

int compareBodyNodes(ASTNode* node1, ASTNode* node2) {
    if (!node1 || !node2) return 0;
    if (node1->childCount != node2->childCount) return 0;

    int score = 0;
    for (int i = 0; i < node1->childCount; i++) {
        score += compareASTNodes(node1->children[i], node2->children[i]);
    }
    return score;
}

int compareValues(ASTNode* value1, ASTNode* value2) {
    if (!value1 || !value2) return 0;  // Null kontrolü
    return compareExpressions(value1, value2);
}

int compareAssignments(ASTNode* node1, ASTNode* node2) {
    if (!node1 || !node2) return 0;
    fprintf(stderr, "Comparing Assignments: %s = %s vs %s = %s\n",
            node1->children[0]->name, node1->children[1]->name,
            node2->children[0]->name, node2->children[1]->name);

    if (node1->type != NodeType_Assignment || node2->type != NodeType_Assignment) return 0;

    int leftScore = compareExpressions(node1->children[0], node2->children[0]);
    int rightScore = compareExpressions(node1->children[1], node2->children[1]);
    fprintf(stderr, "Assignment scores: %d (left), %d (right)\n", leftScore, rightScore);

    return (leftScore + rightScore) / 2;
}

int compareFunctionDefinition(ASTNode* node1, ASTNode* node2) {
    if (!node1 || !node2) return 0;
    fprintf(stderr, "Comparing Function Definitions: %s vs %s\n", node1->name, node2->name);

    if (node1->type != NodeType_FunctionDef || node2->type != NodeType_FunctionDef) return 0;
    int paramScore = compareParameters(node1->params, node2->params, node1->paramCount, node2->paramCount);
    int bodyScore = compareFunctionBodies(node1->body, node2->body);
    fprintf(stderr, "Function definition scores: %d (params), %d (body)\n", paramScore, bodyScore);

    return paramScore + bodyScore;
}

int compareASTs(ASTNode *root1, ASTNode *root2) {
    if (!root1 || !root2) {
        fprintf(stderr, "Error: One or both ASTs are NULL.\n");
        return 0;
    }

    int totalScore = 0;

    if (root1->childCount >= 2 && root2->childCount >= 2) {
        ASTNode* functionsNode1 = root1->children[0];
        ASTNode* functionsNode2 = root2->children[0];
        for (int i = 0; i < functionsNode1->childCount && i < functionsNode2->childCount; i++) {
            totalScore += calculateFunctionSimilarity(functionsNode1->children[i], functionsNode2->children[i]);
        }

        ASTNode* mainNode1 = root1->children[1];
        ASTNode* mainNode2 = root2->children[1];
        totalScore += compareMainFunctions(mainNode1, mainNode2);
    }

    return totalScore;
}

