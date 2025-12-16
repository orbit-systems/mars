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

    FeAliasMap* amap;

    struct {
        FeAliasCategory stack;
        FeAliasCategory memory;
    } alias;
} IRGen;

typedef struct {
    FeTy ty;
    FeComplexTy* cty;
} TySelectResult;

static TySelectResult select_iron_type(IRGen* ig, TyIndex ty_index);
static FeFuncSig* generate_signature(IRGen* ig);
static void irgen_function(IRGen* ig);


typedef struct {
    bool diverges;
} StmtInfo;
#define STMTINFO(...) (StmtInfo){__VA_ARGS__}



static FeInst* irgen_value(IRGen* ig, Expr* expr);
static StmtInfo irgen_stmt(IRGen* ig, Stmt* stmt);

#endif // IRGEN_H
