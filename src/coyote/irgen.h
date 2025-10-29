#ifndef IRGEN_H
#define IRGEN_H

#include "lex.h"
#include "parse.h"

#include "iron/iron.h"

FeModule* irgen(CompilationUnit* cu);

typedef struct {
    CompilationUnit* cu;

    FeSection* default_text;
    FeSection* default_data;
    FeSection* default_rodata;

    Entity* e;
    FeModule* m;
    FeFunc* f;

    // current block
    FeBlock* b;


} IRGen;

typedef struct {
    FeTy ty;
    FeComplexTy* cty;
} TySelectResult;

static TySelectResult select_iron_type(TyIndex ty_index);
static FeFuncSig* generate_signature(CompilationUnit* cu, FeModule* m, Entity* e);
static void irgen_function(IRGen* ig);

#endif // IRGEN_H
