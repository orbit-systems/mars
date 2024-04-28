#include "deimos.h"

// trivial dead code elimination

// FIXME: probably die

// ^ i stopped writing this comment in the middle
//   and idk what i was going to say
//   its funnier like this i think
static bool has_side_effects(IR* ir) {
    switch (ir->tag) {
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
    case IR_MOV:
    case IR_CONST:
        return false;

    default: // assume side effects until proven otherwise
        return true;
    }
}

static void register_uses(IR* ir) {
    switch (ir->tag) {
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
        IR_BinOp* binop = (IR_BinOp*) ir;
        binop->lhs->use_count++;
        binop->rhs->use_count++;
        break;
    case IR_STORE:
        IR_Store* store = (IR_Store*) ir;
        store->location->use_count++;
        store->value->use_count++;
        break;
    case IR_RETURNVAL:
        IR_ReturnVal* retval = (IR_ReturnVal*) ir;
        if (retval->source) retval->source->use_count++;
        break;
    case IR_MOV:
        IR_Mov* mov = (IR_Mov*) ir;
        if (mov->source) mov->source->use_count++;
        break;

    case IR_INVALID:
    case IR_ELIMINATED:
    case IR_CONST:
    case IR_PARAMVAL:
    case IR_RETURN:
        break;
    default:
        general_warning("unhandled IR type %d", ir->tag);
        CRASH("unhandled IR type");
        break;
    }
}

static void reset_use_counts(IR_Function* f) {
    FOR_URANGE(i, 0, f->blocks.len) {
        FOR_URANGE(j, 0, f->blocks.at[i]->len) {
            f->blocks.at[i]->at[j]->use_count = 0;
        }
    }
}

static void count_uses_func(IR_Function* f) {
    FOR_URANGE(i, 0, f->blocks.len) {
        FOR_URANGE(j, 0, f->blocks.at[i]->len) {
            register_uses(f->blocks.at[i]->at[j]);
        }
    }
}

static void try_eliminate(IR* ir) {
    // recursively attempt to eliminate dead code
    if (ir == NULL || ir->use_count != 0 || has_side_effects(ir)) return;

    switch (ir->tag) {
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
        IR_BinOp* binop = (IR_BinOp*) ir;
        binop->lhs->use_count--;
        binop->rhs->use_count--;
        try_eliminate(binop->lhs);
        try_eliminate(binop->rhs);
        break;
    case IR_STORE:
        IR_Store* store = (IR_Store*) ir;
        store->location->use_count--;
        store->value->use_count--;
        try_eliminate(store->location);
        try_eliminate(store->value);
        break;
    case IR_RETURNVAL:
        IR_ReturnVal* retval = (IR_ReturnVal*) ir;
        if (retval->source) retval->source->use_count--;
        try_eliminate(retval->source);
        break;
    case IR_MOV:
        IR_Mov* mov = (IR_Mov*) ir;
        if (mov->source) mov->source->use_count--;
        try_eliminate(mov->source);
        break;

    case IR_INVALID:
    case IR_ELIMINATED:
    case IR_CONST:
    case IR_PARAMVAL:
    case IR_RETURN:
        break;
    default:
        general_warning("unhandled IR type %d", ir->tag);
        CRASH("unhandled IR type");
        break;
    }
    ir->tag = IR_ELIMINATED;
}

static void tdce_on_function(IR_Function* f) {
    // FIXME: this should be done as a separate pass!
    reset_use_counts(f);
    count_uses_func(f);

    FOR_URANGE(i, 0, f->blocks.len) {
        FOR_URANGE(j, 0, f->blocks.at[i]->len) {
            IR* ir = f->blocks.at[i]->at[j];
            if (ir->tag == IR_ELIMINATED) continue;
            try_eliminate(ir);
        }
    }
    // if (elimd) tdce_on_function(f);
}

IR_Module* ir_pass_tdce(IR_Module* mod) {

    FOR_URANGE(i, 0, mod->functions_len) {
        tdce_on_function(mod->functions[i]);
    }

    return mod;
}