#include "common/util.h"
#include "iron/iron.h"

// various local transformations

static void push_uses(FeInstSet* wlist, FeInst* inst) {
    for_n(i, 0, inst->use_len) {
        fe_iset_push(wlist, FE_USE_PTR(inst->uses[i]));
    }
}

// returns new value or nullptr if didnt work
static FeInst* try_load_elim(FeFunc* f, FeInstSet* wlist, FeInst* load) {
    if (load->kind != FE_LOAD) {
        return nullptr;
    }
    /*
        %0 = ...
        %1 = store [%0] %ptr, %val
        %2 = load [%1] %ptr
        %3 = ... %2
    ->
        %0 = ...
        %1 = store [%0] %ptr, %val
        %3 = ... %val
    */

    FeInst* dependent_store = load->inputs[0];
    // dependent operation is not a store
    if (dependent_store->kind != FE_STORE) {
        return nullptr;
    }
    // pointers dont match
    if (dependent_store->inputs[1] != load->inputs[1]) {
        return nullptr;
    }

    FeInst* new_val = dependent_store->inputs[2];

    fe_replace_uses(f, load, new_val);
    fe_inst_destroy(f, load);

    // add the store and its uses to the worklist
    fe_iset_push(wlist, dependent_store);
    for_n(i, 0, dependent_store->use_len) {
        fe_iset_push(wlist, FE_USE_PTR(dependent_store->uses[i]));
    }

    return new_val;
}

static FeInst* try_store_elim(FeFunc* f, FeInstSet* wlist, FeInst* store) {
    if (store->kind != FE_STORE) {
        return nullptr;
    }
    /*
        %0 = ...
        %1 = store [%0] %ptr, %val
        %2 = store [%1] %ptr, %val2
    ->
        %1 = store [%0] %ptr, %val2
    */


    FeInst* dependent_store = store->inputs[0];
    // dependent operation is not a store
    if (dependent_store->kind != FE_STORE) {
        return nullptr;
    }
    // pointers dont match
    if (dependent_store->inputs[1] != store->inputs[1]) {
        return nullptr;
    }

    // our store isnt the only thing using the dependent store
    if (dependent_store->use_len != 1) {
        return nullptr;
    }

    // use the dependent store's memory link
    fe_set_input(f, store, 0, dependent_store->inputs[0]);
    fe_inst_destroy(f, dependent_store);

    // add the store and its uses to the worklist
    fe_iset_push(wlist, store);
    for_n(i, 0, store->use_len) {
        fe_iset_push(wlist, FE_USE_PTR(store->uses[i]));
    }

    return store;
}

static bool try_tdce(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    if (inst->use_len == 0 && !fe_inst_has_trait(inst->kind, FE_TRAIT_VOLATILE)) {
        for_n(i, 0, inst->in_len) {
            if (inst->inputs[i] == nullptr) {
                continue;
            }
            fe_iset_push(wlist, inst->inputs[i]);
        }
        fe_inst_destroy(f, inst);
    }
    return false;
}

/*
    strength reduction:
        x - const -> x + (-const)
        x imul 2     -> x << 1 // extend to powers of two
        x udiv 2     -> x >> 1 // extend to powers of two
        x umod 2     -> x & 1  // extend to powers of two
*/ 


static inline usize u64_next_pow_2(usize x) {
    return 1 << ((sizeof(x) * 8) - __builtin_clzll(x - 1));
}

static inline usize u64_log2(usize x) {
    return (sizeof(x) * 8 - 1) - __builtin_clzll(x);
}

static bool try_strength_binop(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    if (!fe_inst_has_trait(inst->kind, FE_TRAIT_BINOP)) {
        return false;
    }

    FeInst* lhs = inst->inputs[0];
    FeInst* rhs = inst->inputs[1];

    bool modified = false;

    switch (inst->kind) {
    case FE_ISUB:
        if (rhs->kind == FE_CONST) {
            // fe_extra(rhs, FeInstConst)->val = -fe_extra(rhs, FeInstConst)->val;
            FeInst* new_const = fe_inst_const(f, rhs->ty, 
                -fe_extra(rhs, FeInstConst)->val
            );
            fe_insert_before(inst, new_const);
            
            fe_iset_push(wlist, rhs);
            fe_set_input(f, inst, 1, new_const);

            inst->kind = FE_IADD;
            modified = true;
        }
    }

    if (modified) {
        fe_iset_push(wlist, inst);
    }

    return modified;
}

/*
    identity reduction:
        x + 0      -> x
        x - 0      -> x
        x * 0      -> 0
        x * 1      -> x
        x / 1      -> x
        0 / x      -> 0 -- watch out for x == 0 ???
        x & 0      -> 0
        x | 0      -> x
        x ^ 0      -> x
        x << 0     -> x
        x >> 0     -> x
        0 << x     -> 0
        0 >> x     -> 0
        ~(~x)      -> x
        -(-x)      -> x

        (these only applies to bool):
            x & false -> false
            x & true  -> x
            x | false -> x
            x | true  -> true
*/

static bool is_const(FeInst* inst, u64 val) {
    return inst->kind == FE_CONST && fe_extra(inst, FeInstConst)->val == val;
}

static bool try_identity_binop(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    if (!fe_inst_has_trait(inst->kind, FE_TRAIT_BINOP)) {
        return false;
    }

    FeInst* lhs = inst->inputs[0];
    FeInst* rhs = inst->inputs[1];

    FeInst* replace = nullptr;

    switch (inst->kind) {
    case FE_IADD:
    case FE_ISUB:
        if (is_const(rhs, 0)) {
            replace = lhs;
        }
        break;
    case FE_IMUL:
        if (is_const(rhs, 0)) {
            replace = rhs;
        } else if (is_const(lhs, 1)) {
            replace = lhs;
        }
        break;
    case FE_SHL:
    case FE_ISR:
    case FE_USR:
        if (is_const(rhs, 0)) {
            replace = lhs;
        } else if (is_const(lhs, 0)) {
            replace = rhs;
        }
        break;
    case FE_XOR:
        if (is_const(rhs, 0)) {
            replace = lhs;
        }
        break;
    }

    if (replace) {
        fe_replace_uses(f, inst, replace);

        fe_iset_push(wlist, inst);
        push_uses(wlist, replace);
    }

    return false;
}

/*
    reassociation: (helps with const eval)
        (x + y) + z  ->  x + (y + z)
        (x * y) * z  ->  x * (y * z)
        (x & y) & z  ->  x & (y & z)
        (x | y) | z  ->  x | (y | z)
        (x ^ y) ^ z  ->  x ^ (y ^ z)
*/

static bool try_reassoc(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    /*
        %2 = op %x, %y
        %3 = op %2, %z
    ->
        %2 = op %y, %z
        %3 = op %2, %x
    */

    if (!fe_inst_has_trait(inst->kind, FE_TRAIT_ASSOCIATIVE)) {
        return false;
    }

    FeInst* first = inst->inputs[0];

    // two operations must be the same kind
    if (inst->kind != first->kind) {
        return false;
    }

    FeInst* x = first->inputs[0];
    FeInst* y = first->inputs[1];
    FeInst* z = inst->inputs[1];

    if (y->kind != FE_CONST || z->kind != FE_CONST) {
        return false;
    }

    fe_set_input(f, first, 0, y);
    fe_set_input(f, first, 1, z);
    fe_set_input(f, inst, 1, x);

    fe_inst_remove_from_block(first);
    fe_insert_before(inst, first);

    fe_iset_push(wlist, inst);
    fe_iset_push(wlist, first);

    return true;
}

/*
    constant evaluation:
        1 + 2   -> 3
        1 - 2   -> -1
        2 * 2   -> 4
        6 / 2   -> 3
        (neg) 1 -> -1
        ...
*/

static bool try_consteval_binop(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    if (!fe_inst_has_trait(inst->kind, FE_TRAIT_BINOP)) {
        return false;
    }
    
    FeInst* lhs_inst = inst->inputs[0];
    FeInst* rhs_inst = inst->inputs[1];

    if (lhs_inst->kind != FE_CONST || rhs_inst->kind != FE_CONST) {
        return false;
    }

    u64 lhs = fe_extra(lhs_inst, FeInstConst)->val;
    u64 rhs = fe_extra(rhs_inst, FeInstConst)->val;
    u64 result = 0;

    bool can_eval = true;
    
    if (inst->kind == FE_IDIV && rhs == 0) {
        can_eval = false;
    }

    if (can_eval) switch (inst->kind) {
    case FE_IADD: result = lhs + rhs; break;
    case FE_ISUB: result = lhs - rhs; break;
    case FE_IMUL: result = lhs * rhs; break;
    case FE_IDIV: result = (i64)lhs / (i64)rhs; break;
    case FE_IREM: result = (i64)lhs % (i64)rhs; break;
    case FE_UDIV: result = lhs / rhs; break;
    case FE_UREM: result = lhs % rhs; break;
    case FE_AND:  result = lhs & rhs; break;
    case FE_OR:   result = lhs | rhs; break;
    case FE_XOR:  result = lhs ^ rhs; break;
    case FE_SHL:  result = lhs << rhs; break;
    case FE_USR:  result = lhs >> rhs; break;
    case FE_ISR:  result = (i64)lhs >> (i64)rhs; break;
    case FE_ILT:  result = (bool)((i64)lhs >  (i64)rhs); break;
    case FE_ILE:  result = (bool)((i64)lhs >= (i64)rhs); break;
    case FE_ULT:  result = (bool)(lhs >  rhs); break;
    case FE_ULE:  result = (bool)(lhs >= rhs); break;
    case FE_IEQ:  result = (bool)(lhs == rhs); break;
    default:
        can_eval = false;
        break;
    }

    if (!can_eval) {
        return false;
    }

    FeInst* c = fe_inst_const(f, inst->ty, result);
    fe_insert_before(inst, c);

    // transfer all uses of inst to c
    fe_replace_uses(f, inst, c);
    
    fe_iset_push(wlist, inst);

    for_n(i, 0, c->use_len) {
        fe_iset_push(wlist, FE_USE_PTR(c->uses[i]));
    }

    return true;
}

static bool try_commute(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    if (!fe_inst_has_trait(inst->kind, FE_TRAIT_BINOP)) {
        return false;
    }

    if (!fe_inst_has_trait(inst->kind, FE_TRAIT_COMMUTATIVE)) {
        return false;
    }

    FeInst* lhs_inst = inst->inputs[0];
    FeInst* rhs_inst = inst->inputs[1];

    if (!(lhs_inst->kind == FE_CONST && rhs_inst->kind != FE_CONST)) {
        return false;
    }

    fe_set_input(f, inst, 0, rhs_inst);
    fe_set_input(f, inst, 1, lhs_inst);

    fe_iset_push(wlist, inst);

    return true;
}

void fe_opt_tdce(FeFunc* f) {
    FeInstSet wlist;
    fe_iset_init(&wlist);

    for_blocks(block, f) {
        for_inst(inst, block) {
            fe_iset_push(&wlist, inst);
        }
    }

    while (true) {
        FeInst* inst = fe_iset_pop(&wlist);
        if (inst == nullptr) {
            break;
        }
        
        try_tdce(f, &wlist, inst);
    }

    fe_iset_destroy(&wlist);
}

void fe_opt_local(FeFunc* f) {
    FeInstSet wlist;
    fe_iset_init(&wlist);

    for_blocks(block, f) {
        for_inst(inst, block) {
            fe_iset_push(&wlist, inst);
        }
    }

    while (true) {
        FeInst* inst = fe_iset_pop(&wlist);
        if (inst == nullptr) {
            break;
        }

        bool opts = false
            || try_tdce(f, &wlist, inst)
            || try_commute(f, &wlist, inst)
            || try_reassoc(f, &wlist, inst)
            || try_strength_binop(f, &wlist, inst)
            || try_identity_binop(f, &wlist, inst)
            || try_consteval_binop(f, &wlist, inst)
            || try_load_elim(f, &wlist, inst)
            || try_store_elim(f, &wlist, inst)
        ;
    }

    fe_iset_destroy(&wlist);
}
