#include "iron/iron.h"

/* pass "tdce" - trivial dead code elimination

    recursively eliminate instructions with no uses or side-effects.

*/

// FIXME: probably die
// ^ i stopped writing this comment in the middle
//   and idk what i was going to say
//   its funnier like this i think
static bool has_side_effects(FeIr* ir) {
    return !(_FE_IR_NO_SIDE_EFFECTS_BEGIN < ir->kind && ir->kind < _FE_IR_NO_SIDE_EFFECTS_END);
}

static void register_uses(FeIr* ir) {
    switch (ir->kind) {
    case FE_IR_ADD:
    case FE_IR_SUB:
    case FE_IR_UMUL:
    case FE_IR_IMUL:
    case FE_IR_UDIV:
    case FE_IR_IDIV:
    case FE_IR_ASR:
    case FE_IR_LSR:
    case FE_IR_SHL:
    case FE_IR_XOR:
    case FE_IR_OR:
    case FE_IR_AND:
        FeIrBinop* binop = (FeIrBinop*)ir;
        binop->lhs->use_count++;
        binop->rhs->use_count++;
        break;
    case FE_IR_LOAD:
        FeIrLoad* load = (FeIrLoad*)ir;
        load->location->use_count++;
        break;
    case FE_IR_STORE:
        FeIrStore* store = (FeIrStore*)ir;
        store->location->use_count++;
        store->value->use_count++;
        break;
    case FE_IR_RETURNVAL:
        FeIrReturnVal* retval = (FeIrReturnVal*)ir;
        if (retval->source) retval->source->use_count++;
        break;
    case FE_IR_MOV:
        FeIrMov* mov = (FeIrMov*)ir;
        if (mov->source) mov->source->use_count++;
        break;
    case FE_IR_NOT:
    case FE_IR_NEG:
    case FE_IR_SIGNEXT:
    case FE_IR_ZEROEXT:
    case FE_IR_TRUNC:
        FeIrUnop* unop = (FeIrUnop*)ir;
        if (unop->source) unop->source->use_count++;
        break;
    case FE_IR_INVALID:
    case FE_IR_CONST:
    case FE_IR_PARAMVAL:
    case FE_IR_STACK_ADDR:
    case FE_IR_RETURN:
        break;
    default:
        CRASH("unhandled inst type %d", ir->kind);
        break;
    }
}

static void reset_use_counts(FeFunction* f) {
    for_urange(i, 0, f->blocks.len) {
        for_fe_ir(inst, *f->blocks.at[i]) {
            inst->use_count = 0;
        }
    }
}

static void count_uses_func(FeFunction* f) {
    for_urange(i, 0, f->blocks.len) {
        for_fe_ir(inst, *f->blocks.at[i]) {
            register_uses(inst);
        }
    }
}

static void try_eliminate(FeIr* ir) {
    // recursively attempt to eliminate dead code
    if (ir == NULL || ir->use_count != 0 || has_side_effects(ir)) return;

    switch (ir->kind) {
    case FE_IR_ADD:
    case FE_IR_SUB:
    case FE_IR_UMUL:
    case FE_IR_IMUL:
    case FE_IR_UDIV:
    case FE_IR_IDIV:
    case FE_IR_ASR:
    case FE_IR_LSR:
    case FE_IR_SHL:
    case FE_IR_XOR:
    case FE_IR_OR:
    case FE_IR_AND:
        FeIrBinop* binop = (FeIrBinop*)ir;
        binop->lhs->use_count--;
        binop->rhs->use_count--;
        try_eliminate(binop->lhs);
        try_eliminate(binop->rhs);
        break;
    case FE_IR_STORE:
        FeIrStore* store = (FeIrStore*)ir;
        store->location->use_count--;
        store->value->use_count--;
        try_eliminate(store->location);
        try_eliminate(store->value);
        break;
    case FE_IR_RETURNVAL:
        FeIrReturnVal* retval = (FeIrReturnVal*)ir;
        if (retval->source) retval->source->use_count--;
        try_eliminate(retval->source);
        break;
    case FE_IR_MOV:
        FeIrMov* mov = (FeIrMov*)ir;
        if (mov->source) mov->source->use_count--;
        try_eliminate(mov->source);
        break;
    case FE_IR_NEG:
    case FE_IR_NOT:
        FeIrUnop* unop = (FeIrUnop*)ir;
        if (unop->source) unop->source->use_count--;
        try_eliminate(unop->source);
        break;
    case FE_IR_INVALID:
    case FE_IR_CONST:
    case FE_IR_PARAMVAL:
        break;
    default:
        CRASH("unhandled inst type %d", ir->kind);
        break;
    }
    // ir->kind = FE_IR_ELIMINATED;
    fe_remove_ir(ir);
}

static void tdce_on_function(FeFunction* f) {
    // FIXME: this should be done as a separate pass!
    reset_use_counts(f);
    count_uses_func(f);

    for_urange(i, 0, f->blocks.len) {
        for_fe_ir(inst, *f->blocks.at[i]) {
            try_eliminate(inst);
        }
    }
    // if (elimd) tdce_on_function(f);
}

static void run_pass_tdce(FeModule* mod) {
    for_urange(i, 0, mod->functions_len) {
        tdce_on_function(mod->functions[i]);
    }
}

FePass fe_pass_tdce = {
    .name = "tdce",
    .module = run_pass_tdce,
    .function = tdce_on_function,
};