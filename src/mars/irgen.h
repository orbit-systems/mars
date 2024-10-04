#pragma once
#define IRGEN_H

#include "common/ptrmap.h"
#include "phobos/analysis/sema.h"
#include "iron/iron.h"

typedef struct IrBuilder {
    mars_module* mars;

    FeModule* mod;
    FeFunction* fn; // current function;
    FeBasicBlock* bb; // current basic block
    FeBasicBlock* backlink; // control flow backlink

    // for return statements, which need immediate access to the stack objects for return values
    FeStackObject** fn_returns;
    u64 fn_returns_len;

    PtrMap entity2stackobj;
} IrBuilder;

FeModule* irgen_module(mars_module* mars);
FeFunction* irgen_function(IrBuilder* builder, ast_func_literal_expr* fn_literal, FeSymbol* sym);
void irgen_global_decl(IrBuilder* builder, ast_decl_stmt* global_decl);
void irgen_block(IrBuilder* builder, ast_stmt_block* block);
FeIr* irgen_value_expr(IrBuilder* builder, AST expr);
void irgen_stmt(IrBuilder* builder, AST stmt);
void irgen_global_fn_decl(IrBuilder* builder, ast_func_literal_expr* func);

FeType irgen_mars_to_iron_type(IrBuilder* builder, Type* t);
string irgen_mangle_identifer(IrBuilder* builder, string ident);