#include "similarity.h"

int compareFunctions(ASTNode* func1, ASTNode* func2) {
    if (strcmp(func1->name, func2->name) != 0) return 0;  // İsimler farklı ise benzer değiller.
    if (func1->paramCount != func2->paramCount) return 0;  // Parametre sayısı farklı ise benzer değiller.

    // Parametre tiplerini karşılaştır
    for (int i = 0; i < func1->paramCount; i++) {
        if (strcmp(func1->params[i]->name, func2->params[i]->name) != 0) return 0;
    }

    // Fonksiyon gövdeleri (opsiyonel olarak daha detaylı bir karşılaştırma yapılabilir)
    return 1;
}


