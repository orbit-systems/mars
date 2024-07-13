#include "iron.h"

/* pass "tdce" - trivial dead code elimination

    recursively eliminate instructions with no uses or side-effects.

*/

// FIXME: probably die
// ^ i stopped writing this comment in the middle
//   and idk what i was going to say
//   its funnier like this i think
static bool has_side_effects(FeInst* ir) {
    switch (ir->tag) {
    case FE_INST_ADD:
    case FE_INST_SUB:
    case FE_INST_MUL:
    case FE_INST_DIV:
    case FE_INST_MOV:
    case FE_INST_CONST:
    case FE_INST_PARAMVAL:
    case FE_INST_STACKADDR:
        return false;

    default: // assume side effects until proven otherwise
        return true;
    }
}

static void register_uses(FeInst* ir) {
    switch (ir->tag) {
    case FE_INST_ADD:
    case FE_INST_SUB:
    case FE_INST_MUL:
    case FE_INST_DIV:
        FeBinop* binop = (FeBinop*) ir;
        binop->lhs->use_count++;
        binop->rhs->use_count++;
        break;
    case FE_INST_LOAD:
        FeLoad* load = (FeLoad*) ir;
        load->location->use_count++;
        break;
    case FE_INST_STORE:
        FeStore* store = (FeStore*) ir;
        store->location->use_count++;
        store->value->use_count++;
        break;
    case FE_INST_RETURNVAL:
        FeReturnVal* retval = (FeReturnVal*) ir;
        if (retval->source) retval->source->use_count++;
        break;
    case FE_INST_MOV:
        FeMov* mov = (FeMov*) ir;
        if (mov->source) mov->source->use_count++;
        break;

    case FE_INST_INVALID:
    case FE_INST_ELIMINATED:
    case FE_INST_CONST:
    case FE_INST_PARAMVAL:
    case FE_INST_STACKADDR:
    case FE_INST_RETURN:
        break;
    default:
        CRASH("unhandled AIR type %d", ir->tag);
        break;
    }
}

static void reset_use_counts(FeFunction* f) {
    for_urange(i, 0, f->blocks.len) {
        for_urange(j, 0, f->blocks.at[i]->len) {
            f->blocks.at[i]->at[j]->use_count = 0;
        }
    }
}

static void count_uses_func(FeFunction* f) {
    for_urange(i, 0, f->blocks.len) {
        for_urange(j, 0, f->blocks.at[i]->len) {
            register_uses(f->blocks.at[i]->at[j]);
        }
    }
}

static void try_eliminate(FeInst* ir) {
    // recursively attempt to eliminate dead code
    if (ir == NULL || ir->use_count != 0 || has_side_effects(ir)) return;

    switch (ir->tag) {
    case FE_INST_ADD:
    case FE_INST_SUB:
    case FE_INST_MUL:
    case FE_INST_DIV:
        FeBinop* binop = (FeBinop*) ir;
        binop->lhs->use_count--;
        binop->rhs->use_count--;
        try_eliminate(binop->lhs);
        try_eliminate(binop->rhs);
        break;
    case FE_INST_STORE:
        FeStore* store = (FeStore*) ir;
        store->location->use_count--;
        store->value->use_count--;
        try_eliminate(store->location);
        try_eliminate(store->value);
        break;
    case FE_INST_RETURNVAL:
        FeReturnVal* retval = (FeReturnVal*) ir;
        if (retval->source) retval->source->use_count--;
        try_eliminate(retval->source);
        break;
    case FE_INST_MOV:
        FeMov* mov = (FeMov*) ir;
        if (mov->source) mov->source->use_count--;
        try_eliminate(mov->source);
        break;

    case FE_INST_INVALID:
    case FE_INST_ELIMINATED:
    case FE_INST_CONST:
    case FE_INST_PARAMVAL:
    case FE_INST_RETURN:
        break;
    default:
        CRASH("unhandled AIR type %d", ir->tag);
        break;
    }
    ir->tag = FE_INST_ELIMINATED;
}

static void tdce_on_function(FeFunction* f) {
    // FIXME: this should be done as a separate pass!
    reset_use_counts(f);
    count_uses_func(f);

    for_urange(i, 0, f->blocks.len) {
        for_urange(j, 0, f->blocks.at[i]->len) {
            FeInst* ir = f->blocks.at[i]->at[j];
            if (ir->tag == FE_INST_ELIMINATED) continue;
            try_eliminate(ir);
        }
    }
    // if (elimd) tdce_on_function(f);
}

void run_pass_tdce(FeModule* mod) {

    for_urange(i, 0, mod->ir_module->functions_len) {
        tdce_on_function(mod->ir_module->functions[i]);
    }
}

FePass air_pass_tdce = {
    .name = "tdce",
    .callback = run_pass_tdce,
};