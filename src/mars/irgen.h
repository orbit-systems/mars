#pragma once
#define ATLAS_IRGEN_H

#include "atlas.h"
#include "atlas/passes/passes.h"
#include "phobos/sema.h"

void atlas_run(mars_module* main_mod, AIR_Module* passthrough);

AtlasModule* generate_atlas_from_mars(mars_module* mod);
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