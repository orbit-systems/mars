#pragma once
#define ATLAS_IRGEN_H

#include "iron/iron.h"
#include "phobos/analysis/sema.h"

void generate_ir_iron_from_mars(mars_module* mod, FeModule* femod);

FeData* generate_ir_global_from_stmt_decl(FeModule* mod, AST ast);
FeInst* generate_ir_expr(FeFunction* func, FeBasicBlock* bb, AST ast);
FeFunction* generate_ir_function(FeModule* mod, AST ast);
FeBasicBlock* generate_ir_stmt_block(FeFunction* func, FeBasicBlock* bb, AST stmt_block);

FeInst* generate_ir_expr_literal(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_value(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_binop(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_ident_load(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_value(FeFunction* f, FeBasicBlock* bb, AST ast);
FeInst* generate_ir_expr_address(FeFunction* f, FeBasicBlock* bb, AST ast);
void generate_ir_stmt_assign(FeFunction* f, FeBasicBlock* bb, AST ast);
void generate_ir_stmt_return(FeFunction* f, FeBasicBlock* bb, AST ast);

FeType* TEType_to_iron(FeModule* mod, Type* t);