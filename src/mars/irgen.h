#pragma once
#define ATLAS_IRGEN_H

#include "atlas.h"
#include "atlas/passes/passes.h"
#include "phobos/sema.h"

void generate_ir_atlas_from_mars(mars_module* mod, AtlasModule* atmod);

AIR_Global* generate_ir_global_from_stmt_decl(AtlasModule* mod, AST ast);
AIR* generate_ir_expr_literal(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* generate_ir_expr_value(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* generate_ir_expr_binop(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* generate_ir_expr_ident_load(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* generate_ir_expr_value(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR* generate_ir_expr_address(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
void generate_ir_stmt_assign(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
void generate_ir_stmt_return(AIR_Function* f, AIR_BasicBlock* bb, AST ast);
AIR_Function* generate_ir_function(AtlasModule* mod, AST ast);

AIR_Type* translate_type(AtlasModule* m, type* t);