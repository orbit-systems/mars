#pragma once
#define ATLAS_IRGEN_H

#include "iron.h"
#include "phobos/sema.h"

void generate_ir_atlas_from_mars(mars_module* mod, FeModule* atmod);

FeData* generate_ir_global_from_stmt_decl(FeModule* mod, AST ast);
FeInst* generate_ir_expr_literal(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_value(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_binop(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_ident_load(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_value(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_address(FeFunction* f, FeBasicBlock* bb, AST ast);
void generate_ir_stmt_assign(FeFunction* f, FeBasicBlock* bb, AST ast);
void generate_ir_stmt_return(FeFunction* f, FeBasicBlock* bb, AST ast);
FeFunction* generate_ir_function(FeModule* mod, AST ast);

FeType* translate_type(FeModule* m, type* t);