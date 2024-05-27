#pragma once
#define DEIMOS_IRGEN_H

#include "deimos.h"
#include "deimos/passes/passes.h"
#include "phobos/sema.h"

AIR_Module* air_generate(mars_module* mod);
AIR_Global* air_generate_global_from_stmt_decl(AIR_Module* mod, AST ast);
AIR* air_generate_expr_literal(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* air_generate_expr_value(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* air_generate_expr_binop(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* air_generate_expr_ident_load(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* air_generate_expr_value(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* air_generate_expr_address(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
void air_generate_stmt_assign(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
void air_generate_stmt_return(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR_Function* air_generate_function(AIR_Module* mod, AST ast);