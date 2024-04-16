#include "../deimos.h"

string generate_temporary() {
    static u64 temp_number = 0;
    return strprintf("t%zu", temp_number++);
}

/* append operations to ir_stream and return identifier with the value of the expression.
NOTE: this is just a demo

for example:
    a * b + c

generates:
    t0 = a * b
    t1 = t0 + c

and returns t1.
*/
string generate_expression(AST node, da(IR)* ir_stream) {
    switch (node.type) {
    case AST_identifier_expr:
        // if its just an identifier, we can use that directly
        return node.as_identifier_expr->tok->text;
        break;
    case AST_binary_op_expr:
        IR op = new_ir_entry(&deimos_alloca, ir_type_binary_op);

        switch (node.as_binary_op_expr->op->type) {
        case TOK_ADD:   op.as_binary_op->op = ADD; break;
        case TOK_MUL:   op.as_binary_op->op = MUL; break;
        case TOK_SUB:   op.as_binary_op->op = SUB; break;
        case TOK_DIV:   op.as_binary_op->op = DIV; break;
        default:
            TODO("sandwich's demo isnt built for that lmao");
        }

        // generate value dependencies
        op.as_binary_op->lhs = generate_expression(
            node.as_binary_op_expr->lhs,
            ir_stream
        );
        op.as_binary_op->rhs = generate_expression(
            node.as_binary_op_expr->rhs,
            ir_stream
        );
        
        op.as_binary_op->out = generate_temporary();

        da_append(ir_stream, op);
        return op.as_binary_op->out;
        break;
    default:
        TODO("WHAT\n");
        break;
    }
}


// append return and necessary opreations to ir_stream
void generate_return(AST node, da(IR)* ir_stream) {
    // assuming single-value returns at the moment
    
    // generate expression for return value
    string retval = generate_expression(node.as_return_stmt->returns.at[0], ir_stream);

    IR ret = new_ir_entry(&deimos_alloca, ir_type_return_stmt);
    ret.as_return_stmt->identifier_name = retval;

    da_append(ir_stream, ret);
}