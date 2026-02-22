#include "common/util.h"
#include "iron/iron.h"

// various local transformations
// this has ballooned into everything lmfao

static bool try_all(FeFunc* f, FeInstSet* wlist, FeInst* inst);

static void push_uses(FeInstSet* wlist, FeInst* inst) {
    for_n(i, 0, inst->use_len) {
        fe_iset_push(wlist, FE_USE_PTR(inst->uses[i]));
    }
}

static void push_inputs(FeInstSet* wlist, FeInst* inst) {
    for_n(i, 0, inst->in_len) {
        fe_iset_push(wlist, inst->inputs[i]);
    }
}

static bool load_elim_store(FeFunc* f, FeInstSet* wlist, FeInst* dependent_store, FeInst* load) {
    if (dependent_store->inputs[1] != load->inputs[1]) {
        return false;
    }

    FeInst* new_val = dependent_store->inputs[2];

    fe_replace_uses(f, load, new_val);
    fe_inst_destroy(f, load);

    // add the store and its uses to the worklist
    fe_iset_push(wlist, dependent_store);
    push_uses(wlist, dependent_store);

    // add the new value and its uses to the worklist
    fe_iset_push(wlist, new_val);
    push_uses(wlist, new_val);
    push_inputs(wlist, new_val);

    return true;
}

static bool load_elim_phi(FeFunc* f, FeInstSet* wlist, FeInst* dependent_phi, FeInst* load) {

    for_n(i, 0, dependent_phi->in_len) {
        FeInst* dependent_store = dependent_phi->inputs[i];
        if (dependent_store->kind != FE_STORE) {
            return false;
        }
        if (dependent_store->inputs[1] != load->inputs[1]) {
            return false;
        }
    }

    // good to go
    FeInst* phi = fe_insert_after(dependent_phi, 
        fe_inst_phi(f, load->ty, dependent_phi->in_len)
    );

    for_n(i, 0, dependent_phi->in_len) {
        FeInst* dependent_store = dependent_phi->inputs[i];
        FeInst* stored_value = dependent_store->inputs[2];
        fe_phi_add_src(f, 
            phi, 
            stored_value,
            fe_extra(dependent_phi, FeInstPhi)->blocks[i]
        );
    }

    fe_replace_uses(f, load, phi);

    fe_iset_push(wlist, dependent_phi);
    push_uses(wlist, dependent_phi);
    
    fe_iset_push(wlist, load);
    fe_iset_push(wlist, phi);
    push_uses(wlist, phi);

    return true;
}

// returns new value or nullptr if didnt work
static bool try_load_elim(FeFunc* f, FeInstSet* wlist, FeInst* load) {
    if (load->kind != FE_LOAD) {
        return false;
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

    FeInst* dependent = load->inputs[0];

    if (dependent->kind == FE_STORE) {
        return load_elim_store(f, wlist, dependent, load);
    }
    if (dependent->kind == FE_MEM_PHI) {
        return load_elim_phi(f, wlist, dependent, load);
    }

    return false;
}

static bool try_store_elim(FeFunc* f, FeInstSet* wlist, FeInst* store) {
    if (store->kind != FE_STORE) {
        return false;
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
        return false;
    }
    // pointers dont match
    if (dependent_store->inputs[1] != store->inputs[1]) {
        return false;
    }

    // our store isnt the only thing using the dependent store
    if (dependent_store->use_len != 1) {
        return false;
    }

    // use the dependent store's memory link
    fe_set_input(f, store, 0, dependent_store->inputs[0]);
    // fe_inst_destroy(f, dependent_store);

    // add the store and its uses to the worklist
    fe_iset_push(wlist, store);
    fe_iset_push(wlist, dependent_store);
    push_uses(wlist, store);
    push_inputs(wlist, store);

    return true;
}

static bool try_tdce(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    if (inst->use_len == 0) {
        if (fe_inst_has_trait(inst->kind, FE_TRAIT_VOLATILE)) {
            return false;
        }
        if (fe_inst_has_trait(inst->kind, FE_TRAIT_TERMINATOR)) {
            return false;
        }
        for_n(i, 0, inst->in_len) {
            if (inst->inputs[i] == nullptr) {
                continue;
            }
            fe_iset_push(wlist, inst->inputs[i]);
        }
        fe_inst_destroy(f, inst);
        return true;
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
        if (is_const(rhs, 0)) {
            replace = lhs;
        }
        break;
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

    // first operation must only be used by this operation
    if (first->use_len != 1) {
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

static bool try_alias_relax(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    if (!fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_SINGLE_USE)) {
        return false;
    }

    // see if we can turn
    //  W dep THIS dep Y dep Z
    // into
    //  W dep (THIS, Y), THIS dep Z, Y dep Z
    FeInst* inst_dependency = inst->inputs[0];
    if (fe_inst_has_trait(inst_dependency->kind, FE_TRAIT_MEM_SINGLE_USE)) {

        if (!fe_insts_may_alias(f, inst, inst_dependency)) {
            // this operation doesn't alias its dependency, we can try to relax it
            // FE_CRASH("can relax!!\n");

            // create a mem_merge for the dependencies of `inst` to use
            FeInst* merge = fe_inst_mem_merge(f, 2);
            fe_insert_after(inst, merge);

            fe_replace_uses(f, inst, merge);
            
            fe_add_input(f, merge, inst);
            fe_add_input(f, merge, inst_dependency);

            fe_set_input(f, inst, 0, inst_dependency->inputs[0]);

            fe_iset_push(wlist, inst);
            fe_iset_push(wlist, merge);

            push_uses(wlist, merge);

            return true;
        }
    }

    // if the dependency is a mem_merge, we want to investiage what things 
    // in the mem_merge our instruction actually depends on, and conditionally 
    // make a new, reduced mem_merge based on that.
    if (inst_dependency->kind == FE_MEM_MERGE) {
        bool actually_depends_like_for_real[inst_dependency->in_len];
        usize dependency_count = 0;

        FeInst* last_dep = nullptr;
        for_n(i, 0, inst_dependency->in_len) {
            FeInst* dependency = inst_dependency->inputs[i];
            bool does_depend =  fe_insts_may_alias(f, inst, dependency);
            actually_depends_like_for_real[i] = does_depend;
            if (does_depend) {
                last_dep = dependency;
                dependency_count += 1;
            }
        }

        if (dependency_count == 1) {
            // just depend on the single thing, no need to make a new mem_merge
            fe_set_input(f, inst, 0, last_dep);

            fe_iset_push(wlist, inst);
            fe_iset_push(wlist, last_dep);
            fe_iset_push(wlist, inst_dependency);
            push_uses(wlist, inst);
            push_inputs(wlist, inst);
            return true;
        } else if (dependency_count != inst_dependency->in_len) {
            // we can make a new mem_merge!
            FE_CRASH("todo!");
        }
    }

    return false;
}

static bool try_phi_merge_reduce(FeFunc* f, FeInstSet* wlist, FeInst* inst) {

    if (inst->kind != FE_PHI && inst->kind != FE_MEM_MERGE && inst->kind != FE_MEM_PHI) {
        return false;
    }

    if (inst->in_len == 1) {
        push_uses(wlist, inst);
        fe_replace_uses(f, inst, inst->inputs[0]);
        fe_inst_destroy(f, inst);
        return true;
    }

    bool all_inputs_same = true;
    for_n(i, 1, inst->in_len) {
        if (inst->inputs[0] != inst->inputs[i]) {
            // check
            all_inputs_same = false;
            break;
        }
    }
    if (all_inputs_same) {
        push_uses(wlist, inst);
        fe_replace_uses(f, inst, inst->inputs[0]);
        fe_inst_destroy(f, inst);
        return true;
    }

    return false;
}

static bool try_coalesce_mem_merge(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    if (inst->kind != FE_MEM_MERGE) {
        return false;
    }

    // the goal is to turn:
    //  %1 = mem-merge ^x, ^y
    //  %2 = mem-merge ^%1, ^z
    // into
    //  %2 = mem-merge ^x, ^y, ^z

    bool can_do_anything = false;
    for_n(i, 0, inst->in_len) {
        FeInst* dep = inst->inputs[i];
        if (dep->kind == FE_MEM_MERGE && dep->use_len == 1) {
            can_do_anything = true;

            fe_set_input(f, inst, i, dep->inputs[0]);
            for_n(j, 1, dep->in_len) {
                fe_add_input(f, inst, dep->inputs[j]);
            }
        }
    }

    return can_do_anything;
}

static bool try_phi_elim(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    if (inst->kind != FE_PHI && inst->kind != FE_MEM_PHI) {
        return false;
    }


    FeInst* other = nullptr;
    for_n(i, 0, inst->in_len) {
        if (inst->inputs[i] != inst) {
            other = inst->inputs[i];
            break;
        }
    }

    bool all_inputs_self_or_other = true;
    for_n(i, 0, inst->in_len) {
        if (inst->inputs[i] != inst && inst->inputs[i] != other) {
            all_inputs_self_or_other = false;
            break;
        }
    }

    if (all_inputs_self_or_other) {
        push_uses(wlist, inst);
        fe_iset_push(wlist, other);
        fe_replace_uses(f, inst, other);
        fe_iset_push(wlist, inst);
        return true;
    }

    return false;
}

static bool try_all(FeFunc* f, FeInstSet* wlist, FeInst* inst) {
    return false
        || try_tdce(f, wlist, inst)
        || try_commute(f, wlist, inst)
        || try_reassoc(f, wlist, inst)
        || try_strength_binop(f, wlist, inst)
        || try_identity_binop(f, wlist, inst)
        || try_consteval_binop(f, wlist, inst)
        || try_coalesce_mem_merge(f, wlist, inst)
        || try_phi_merge_reduce(f, wlist, inst)
        || try_phi_elim(f, wlist, inst)
        || try_alias_relax(f, wlist, inst)
        || try_load_elim(f, wlist, inst)
        || try_store_elim(f, wlist, inst)
    ;
}

void fe_opt_local(FeFunc* f) {
    FeInstSet wlist;
    fe_iset_init(&wlist);

    for_blocks(block, f) {
        fe_iset_push(&wlist, block->bookend);
        for_inst(inst, block) {
            fe_iset_push(&wlist, inst);
        }
    }

    while (true) {
        FeInst* inst = fe_iset_pop(&wlist);
        if (inst == nullptr) {
            break;
        }

        try_all(f, &wlist, inst);
    }
    fe_iset_destroy(&wlist);

    // fe_opt_compact_ids(f);

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
