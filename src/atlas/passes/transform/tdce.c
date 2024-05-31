#include "atlas.h"

/* pass "tdce" - trivial dead code elimination

    recursively eliminate instructions with no uses or side-effects.

*/

// FIXME: probably die
// ^ i stopped writing this comment in the middle
//   and idk what i was going to say
//   its funnier like this i think
static bool has_side_effects(AIR* ir) {
    switch (ir->tag) {
    case AIR_ADD:
    case AIR_SUB:
    case AIR_MUL:
    case AIR_DIV:
    case AIR_MOV:
    case AIR_CONST:
    case AIR_PARAMVAL:
    case AIR_STACKOFFSET:
        return false;

    default: // assume side effects until proven otherwise
        return true;
    }
}

static void register_uses(AIR* ir) {
    switch (ir->tag) {
    case AIR_ADD:
    case AIR_SUB:
    case AIR_MUL:
    case AIR_DIV:
        AIR_BinOp* binop = (AIR_BinOp*) ir;
        binop->lhs->use_count++;
        binop->rhs->use_count++;
        break;
    case AIR_LOAD:
        AIR_Load* load = (AIR_Load*) ir;
        load->location->use_count++;
        break;
    case AIR_STORE:
        AIR_Store* store = (AIR_Store*) ir;
        store->location->use_count++;
        store->value->use_count++;
        break;
    case AIR_RETURNVAL:
        AIR_ReturnVal* retval = (AIR_ReturnVal*) ir;
        if (retval->source) retval->source->use_count++;
        break;
    case AIR_MOV:
        AIR_Mov* mov = (AIR_Mov*) ir;
        if (mov->source) mov->source->use_count++;
        break;

    case AIR_INVALID:
    case AIR_ELIMINATED:
    case AIR_CONST:
    case AIR_PARAMVAL:
    case AIR_STACKOFFSET:
    case AIR_RETURN:
        break;
    default:
        CRASH("unhandled AIR type %d", ir->tag);
        break;
    }
}

static void reset_use_counts(AIR_Function* f) {
    for_urange(i, 0, f->blocks.len) {
        for_urange(j, 0, f->blocks.at[i]->len) {
            f->blocks.at[i]->at[j]->use_count = 0;
        }
    }
}

static void count_uses_func(AIR_Function* f) {
    for_urange(i, 0, f->blocks.len) {
        for_urange(j, 0, f->blocks.at[i]->len) {
            register_uses(f->blocks.at[i]->at[j]);
        }
    }
}

static void try_eliminate(AIR* ir) {
    // recursively attempt to eliminate dead code
    if (ir == NULL || ir->use_count != 0 || has_side_effects(ir)) return;

    switch (ir->tag) {
    case AIR_ADD:
    case AIR_SUB:
    case AIR_MUL:
    case AIR_DIV:
        AIR_BinOp* binop = (AIR_BinOp*) ir;
        binop->lhs->use_count--;
        binop->rhs->use_count--;
        try_eliminate(binop->lhs);
        try_eliminate(binop->rhs);
        break;
    case AIR_STORE:
        AIR_Store* store = (AIR_Store*) ir;
        store->location->use_count--;
        store->value->use_count--;
        try_eliminate(store->location);
        try_eliminate(store->value);
        break;
    case AIR_RETURNVAL:
        AIR_ReturnVal* retval = (AIR_ReturnVal*) ir;
        if (retval->source) retval->source->use_count--;
        try_eliminate(retval->source);
        break;
    case AIR_MOV:
        AIR_Mov* mov = (AIR_Mov*) ir;
        if (mov->source) mov->source->use_count--;
        try_eliminate(mov->source);
        break;

    case AIR_INVALID:
    case AIR_ELIMINATED:
    case AIR_CONST:
    case AIR_PARAMVAL:
    case AIR_RETURN:
        break;
    default:
        CRASH("unhandled AIR type %d", ir->tag);
        break;
    }
    ir->tag = AIR_ELIMINATED;
}

static void tdce_on_function(AIR_Function* f) {
    // FIXME: this should be done as a separate pass!
    reset_use_counts(f);
    count_uses_func(f);

    for_urange(i, 0, f->blocks.len) {
        for_urange(j, 0, f->blocks.at[i]->len) {
            AIR* ir = f->blocks.at[i]->at[j];
            if (ir->tag == AIR_ELIMINATED) continue;
            try_eliminate(ir);
        }
    }
    // if (elimd) tdce_on_function(f);
}

void run_pass_tdce(AtlasModule* mod) {

    for_urange(i, 0, mod->ir_module->functions_len) {
        tdce_on_function(mod->ir_module->functions[i]);
    }
}

AtlasPass air_pass_tdce = {
    .name = "tdce",
    .callback = run_pass_tdce,
};