#include "mach.h"
#include "x64/x64.h"

/*
    the liveness analyzer assumes well-formed programs.
    this means that for every use, there must be a preceding definition along all control flow paths that lead to it.
    you cannot use a regiser that is only defined in some, but not all, predecessors.

    valid IR will never generate ill-formed programs

    I could design a more complicated analyzer that detects this, but the overhead is kinda horrendous

*/

typedef struct LiveRange {
    u32 from; // inclusive
    u32 to;   // exclusive
    u32 vreg;
} LiveRange;

typedef struct LifetimeSet {
    union {
        LiveRange* at;
        LiveRange single_lifetime;
    };
    // if we're only storing one lifetime, we dont actually have to allocate the space for it
    // this is a very common occurance so it's worth the optimizaiton
    u32 len;
    u32 cap;

    u32 vreg;

    struct {
        struct LifetimeSet** at;
        u32 len;
        u32 cap;
    } interference;

} LifetimeSet;

static u32 total_range_count = 0;
static LifetimeSet* lifetimes;
static LiveRange* pending;

static LiveRange range_from_set(LifetimeSet* lts, u32 index) {
    if (lts->cap == 0)
        return lts->single_lifetime;
    return lts->at[index];
}

static void add_range(LifetimeSet* lts, LiveRange range) {
    range.vreg = lts->vreg;
    total_range_count++;
    if (lts->cap == 0 && lts->len == 0) {
        lts->single_lifetime = range;
        lts->len = 1;
        return;
    }
    if (lts->cap == 0 && lts->len == 1) {
        LiveRange og = lts->single_lifetime;
        lts->cap = 4;
        lts->len = 2;
        lts->at = fe_malloc(sizeof(LiveRange) * lts->cap);
        lts->at[0] = og;
        lts->at[1] = range;
        return;
    } else {
        da_append(lts, range);
    }
}

static LiveRange get_range(LifetimeSet* lts, u32 index) {
    if (lts->cap == 0) return lts->single_lifetime;
    else return lts->at[index];
}
static LiveRange* get_range_ptr(LifetimeSet* lts, u32 index) {
    if (lts->cap == 0) return &lts->single_lifetime;
    else return &lts->at[index];
}

static void add_lifetime_point(u32 here, u32 vreg_index, bool is_def, bool is_use) {
    // printf("%d: %s for vreg %d\n", here, !is_use ? "def" : "use", vreg_index);

    LiveRange* pending_range = &pending[vreg_index];

    if (is_use) {
        // if its a use, extend the pending live range from here
        if (pending_range->from == 0) CRASH("vreg use before def");
        pending_range->to = here;
    } else if (is_def) {
        // if its a def, push the current pending live range and start a new one
        // only push current range if the most recent def has actually been used
        if (pending_range->to != 0) add_range(&lifetimes[vreg_index], pending[vreg_index]);
        pending[vreg_index].from = here;
        pending[vreg_index].to = here;
    } else CRASH("vreg not def or use");
}

static void liveness_analysis(FeMachBuffer* buf) {
    // assume a single basic block at the moment
    // i know, i know
    // i need to figure out how lifetime traces work for an entire function

    lifetimes = fe_malloc(sizeof(LifetimeSet) * buf->vreg_lists.len);
    for_range(i, 0, buf->vreg_lists.len) lifetimes[i].vreg = i;
    pending = fe_malloc(sizeof(LiveRange) * buf->vreg_lists.len);

    bool regalloc_enabled = false;
    for_range(here, 0, buf->buf.len) {
        FeMach* elem = buf->buf.at[here];

        if (!regalloc_enabled && elem->kind != FE_MACH_CFG_BEGIN)
            continue;

        switch (elem->kind) {
        case FE_MACH_CFG_BEGIN:
            regalloc_enabled = true;
            break;
        case FE_MACH_CFG_END:
            // push pending ranges
            for_range(r, 0, buf->vregs.len) {
                if (pending[r].to != 0) {
                    add_range(&lifetimes[r], pending[r]);
                }
            }
            regalloc_enabled = false;
            break;
        case FE_MACH_CFG_BRANCH:
        case FE_MACH_CFG_JUMP:
        case FE_MACH_CFG_TARGET:
            TODO("complex cfgs not handled yet");
            break;
        case FE_MACH_INST:
            FeMachInst* inst = (FeMachInst*)elem;
            const FeMachInstTemplate templ = buf->target.inst_templates[inst->template];
            u32 vreg_0 = inst->regs;
            for_range(r, 0, templ.regs_len) {
                u32 vreg_index = buf->vreg_lists.at[vreg_0 + r];
                bool is_def = (templ.defs & (1ull << r)) != 0;
                bool is_use = (templ.uses & (1ull << r)) != 0;
                add_lifetime_point(here, vreg_index, is_def, is_use);
            }
            break;
        case FE_MACH_LIFETIME_BEGIN:
            FeMachLifetimePoint* ltp = (FeMachLifetimePoint*)elem;
            add_lifetime_point(here, ltp->vreg, true, false);
            break;
        case FE_MACH_LIFETIME_END:
            ltp = (FeMachLifetimePoint*)elem;
            add_lifetime_point(here, ltp->vreg, false, true);
            break;
        default:
            break;
        }
    }

    fe_free(pending);
}

static void conflict_edge(u32 x, u32 y) {
    printf("v%d and v%d conflict\n", x, y);
    LifetimeSet* x_set = &lifetimes[x];
    LifetimeSet* y_set = &lifetimes[y];

    if (x_set->interference.at == NULL) da_init(&x_set->interference, 8);
    if (y_set->interference.at == NULL) da_init(&y_set->interference, 8);

    da_append(&x_set->interference, y_set);
    da_append(&y_set->interference, x_set);
}

#define ranges_interfere(xptr, yptr) ((xptr->from < yptr->to && yptr->from < xptr->to))

static void build_conflict_graph(FeMachBuffer* buf) {

    LiveRange** sorted_ranges = fe_malloc(sizeof(LiveRange*) * total_range_count);

    // copy ranges in and sort
    {
        u32 len = 0;
        for_range(r, 0, buf->vregs.len) {
            for_range(i, 0, lifetimes[r].len) {
                sorted_ranges[len++] = get_range_ptr(&lifetimes[r], i);
            }
        }

        for_range(i, 0, len) {
            LiveRange* x = sorted_ranges[i];
            u32 j = i;
            while (j > 0 && sorted_ranges[j - 1]->from > x->from) {
                sorted_ranges[j] = sorted_ranges[j - 1];
                j--;
            }
            sorted_ranges[j] = x;
        }

        for_range(i, 0, len) {
            printf("% 2d [%d, %d)\n", sorted_ranges[i]->vreg, sorted_ranges[i]->from, sorted_ranges[i]->to);
        }
    }

    // active range set
    struct {
        LiveRange** at;
        u32 len;
        u32 cap;
    } active_ranges;
    da_init(&active_ranges, 64);

    // build interference graph
    for_range(i, 0, total_range_count) {
        LiveRange* new = sorted_ranges[i];

        // kill any dead ranges
        for_range(j, 0, active_ranges.len) {
            LiveRange* maybe_dead = active_ranges.at[j];
            // kill range if its endpoint is earlier than right now
            if (maybe_dead->to <= new->from) {
                da_unordered_remove_at(&active_ranges, j);
                j--;
            }
        }

        // see if current range interferes with active ones
        for_range(j, 0, active_ranges.len) {
            LiveRange* maybe_interferes = active_ranges.at[j];
            if (ranges_interfere(maybe_interferes, new)) {
                conflict_edge(maybe_interferes->vreg, new->vreg);
            }
        }

        // add this range to the active ranges
        da_append(&active_ranges, new);
    }
}

static u8 real_max(FeArchInfo* arch, u8 regclass) {
    return arch->regclasses.at[regclass].len;
}

static void assign_concrete(FeMachBuffer* buf) {
    // shitty ish graph colorer

    // remember to skip the null vreg
    for_range(r, 1, buf->vregs.len) {
        LifetimeSet* lts = &lifetimes[r];
        FeMachVReg* reg = &buf->vregs.at[r];
        // unknown regclass?
        if (reg->class == 0) {
            CRASH("unknown regclass encountered");
        }

        // this shit already assigned
        if (reg->real != 0) continue;

        // if theres a hint, check if that interferes with anything already assigned
        if (reg->hint != 0) {
            bool interferes = false;
            foreach (LifetimeSet* maybe_interferes, lts->interference) {
                FeMachVReg* maybe_reg = &buf->vregs.at[maybe_interferes->vreg];
                if (maybe_reg->real == reg->hint) {
                    interferes = true;
                    break;
                }
            }
            if (!interferes) {
                reg->real = reg->hint;
                continue;
            }
            CRASH("cannot select a real register");
        }

        // look through and see if we can put a real reg to these bitches
    }
}

void fe_mach_regalloc(FeMachBuffer* buf) {
    total_range_count = 0;
    liveness_analysis(buf);
    build_conflict_graph(buf);
    assign_concrete(buf);
}