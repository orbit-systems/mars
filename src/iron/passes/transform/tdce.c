#include "iron/iron.h"

/* pass "tdce" - trivial dead code elimination

    recursively eliminate instructions with no uses or side-effects.

*/

// FIXME: probably die
// ^ i stopped writing this comment in the middle
//   and idk what i was going to say
//   its funnier like this i think
static bool has_side_effects(FeInst* ir) {
    return !(_FE_INST_NO_SIDE_EFFECTS_BEGIN < ir->kind && ir->kind < _FE_INST_NO_SIDE_EFFECTS_END);
}

static void register_uses(FeInst* ir) {
    switch (ir->kind) {
    case FE_INST_ADD:
    case FE_INST_SUB:
    case FE_INST_UMUL:
    case FE_INST_IMUL:
    case FE_INST_UDIV:
    case FE_INST_IDIV:
    case FE_INST_ASR:
    case FE_INST_LSR:
    case FE_INST_SHL:
    case FE_INST_XOR:
    case FE_INST_OR:
    case FE_INST_AND:
        FeInstBinop* binop = (FeInstBinop*) ir;
        binop->lhs->use_count++;
        binop->rhs->use_count++;
        break;
    case FE_INST_LOAD:
        FeInstLoad* load = (FeInstLoad*) ir;
        load->location->use_count++;
        break;
    case FE_INST_STORE:
        FeInstStore* store = (FeInstStore*) ir;
        store->location->use_count++;
        store->value->use_count++;
        break;
    case FE_INST_RETURNVAL:
        FeInstReturnval* retval = (FeInstReturnval*) ir;
        if (retval->source) retval->source->use_count++;
        break;
    case FE_INST_MOV:
        FeInstMov* mov = (FeInstMov*) ir;
        if (mov->source) mov->source->use_count++;
        break;
    case FE_INST_NOT:
    case FE_INST_NEG:
    case FE_INST_CAST:
        FeInstUnop* unop = (FeInstUnop*) ir;
        if (unop->source) unop->source->use_count++;
        break;

    case FE_INST_INVALID:
    case FE_INST_CONST:
    case FE_INST_PARAMVAL:
    case FE_INST_STACK_ADDR:
    case FE_INST_RETURN:
        break;
    default:
        CRASH("unhandled inst type %d", ir->kind);
        break;
    }
}

static void reset_use_counts(FeFunction* f) {
    for_urange(i, 0, f->blocks.len) {
        for_inst(inst, *f->blocks.at[i]) {
            inst->use_count = 0;
        }
    }
}

static void count_uses_func(FeFunction* f) {
    for_urange(i, 0, f->blocks.len) {
        for_inst(inst, *f->blocks.at[i]) {
            register_uses(inst);
        }
    }
}

static void try_eliminate(FeInst* ir) {
    // recursively attempt to eliminate dead code
    if (ir == NULL || ir->use_count != 0 || has_side_effects(ir)) return;

    switch (ir->kind) {
    case FE_INST_ADD:
    case FE_INST_SUB:
    case FE_INST_UMUL:
    case FE_INST_IMUL:
    case FE_INST_UDIV:
    case FE_INST_IDIV:
    case FE_INST_ASR:
    case FE_INST_LSR:
    case FE_INST_SHL:
    case FE_INST_XOR:
    case FE_INST_OR:
    case FE_INST_AND:
        FeInstBinop* binop = (FeInstBinop*) ir;
        binop->lhs->use_count--;
        binop->rhs->use_count--;
        try_eliminate(binop->lhs);
        try_eliminate(binop->rhs);
        break;
    case FE_INST_STORE:
        FeInstStore* store = (FeInstStore*) ir;
        store->location->use_count--;
        store->value->use_count--;
        try_eliminate(store->location);
        try_eliminate(store->value);
        break;
    case FE_INST_RETURNVAL:
        FeInstReturnval* retval = (FeInstReturnval*) ir;
        if (retval->source) retval->source->use_count--;
        try_eliminate(retval->source);
        break;
    case FE_INST_MOV:
        FeInstMov* mov = (FeInstMov*) ir;
        if (mov->source) mov->source->use_count--;
        try_eliminate(mov->source);
        break;
    case FE_INST_NEG:
    case FE_INST_NOT:
        FeInstUnop* unop = (FeInstUnop*) ir;
        if (unop->source) unop->source->use_count--;
        try_eliminate(unop->source);
        break;
    case FE_INST_INVALID:
    case FE_INST_CONST:
    case FE_INST_PARAMVAL:
    case FE_INST_RETURN:
        break;
    default:
        CRASH("unhandled inst type %d", ir->kind);
        break;
    }
    // ir->kind = FE_INST_ELIMINATED;
    fe_remove(ir);
}

static void tdce_on_function(FeFunction* f) {
    // FIXME: this should be done as a separate pass!
    reset_use_counts(f);
    count_uses_func(f);

    for_urange(i, 0, f->blocks.len) {
        for_inst(inst, *f->blocks.at[i]) {
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
    .callback = run_pass_tdce,
};