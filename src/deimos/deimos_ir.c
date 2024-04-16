#include "deimos_ir.h"

size_t ir_type_size[] = {
    0,
#define IR_TYPE(ident, identstr, structdef) sizeof(ir_##ident),
    IR_NODES
#undef IR_TYPE
    0
};

char* ir_type_str[] = {
    "invalid",
#define IR_TYPE(ident, identstr, structdef) identstr,
    IR_NODES
#undef IR_TYPE
    "COUNT",
};

// allocate and zero a new IR node with an arena
IR new_ir_entry(arena* restrict alloca, ir_type type) {
    IR entry;
    void* entry_ptr = arena_alloc(alloca, ir_type_size[type], 8);
    if (entry_ptr == NULL) {
        general_error("internal: new_ir_entry() could not allocate deimos IR entry of type '%s' with size %d", ir_type_str[type], ir_type_size[type]);
    }
    memset(entry_ptr, 0, ir_type_size[type]);
    entry.rawptr = entry_ptr;
    entry.type = type;
    return entry;
}

char* operator_strings[] = {
    "invalid",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "COUNT"
};

void print_ir(da(IR) ir_elems) {
    printf("printing %d ir elements:\n", ir_elems.len);
    FOR_URANGE(i, 0, ir_elems.len) {
        switch (ir_elems.at[i].type) {
            case ir_type_function_label:
                printf("%s:\n", clone_to_cstring(ir_elems.at[i].as_function_label->label_text));
                break;
            case ir_type_arg_mov:
                printf("%s = %%arg%d\n", clone_to_cstring(ir_elems.at[i].as_arg_mov->identifier_name), ir_elems.at[i].as_arg_mov->argument);
                break;
            case ir_type_binary_op:
                printf("%s = BINARY_OP %s, %s, %s\n", clone_to_cstring(ir_elems.at[i].as_binary_op->out),
                                                      clone_to_cstring(ir_elems.at[i].as_binary_op->lhs),
                                                      clone_to_cstring(ir_elems.at[i].as_binary_op->rhs),
                                                      operator_strings[ir_elems.at[i].as_binary_op->op]);
                break;
            case ir_type_return_stmt:
                printf("return %s\n", clone_to_cstring(ir_elems.at[i].as_return_stmt->identifier_name));
                break;
            default:
                general_warning("FIXME: print_ir() unhandled IR element %s", ir_type_str[ir_elems.at[i].type]);
                break;
        }
    }
}