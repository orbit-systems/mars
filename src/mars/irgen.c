#include "irgen.h"

#include "phobos/analysis/sema.h"
#include "common/ptrmap.h"

static mars_module* mars_mod;
static FeModule* am;

static PtrMap entity2stackoffset;

// replacing all of this with TODO stubs because i cant be fucked to update this every single time i make changes
void generate_ir_atlas_from_mars(mars_module* mod, FeModule* atmod) {
    TODO("");
}

FeData* generate_ir_global_from_stmt_decl(FeModule* mod, AST ast) { //FIXME: add sanity
    TODO("");
}

//FIXME: add generate_ir_local_from_stmt_decl

FeInst* generate_ir_expr_literal(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeInst* generate_ir_expr_binop(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeInst* generate_ir_expr_ident_load(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeInst* generate_ir_expr_value(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeInst* generate_ir_expr_address(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

void generate_ir_stmt_assign(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

void generate_ir_stmt_return(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeFunction* generate_ir_function(FeModule* mod, AST ast) {
    TODO("");
}

FeType* translate_type(FeModule* m, Type* t) {
    TODO("");
}