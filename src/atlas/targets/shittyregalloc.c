#include "atlas.h"
#include "ptrmap.h"

// bad regalloc for single-block programs
// redo as soon as possible

typedef struct LiveRange {
    u32 start;
    u32 end; // inclusive
} LiveRange;
// a vreg's live range ends right before its last use so that a physical reg can be
// reallocated on the same 

static TargetInfo* target;

static bool is_defined_by(AsmInst* inst, VReg* vreg) {
    for_range(i, 0, inst->template->num_outs) {
        if (inst->outs[i] == vreg) return true;
    }
    return false;
}

static bool is_used_by(AsmInst* inst, VReg* vreg) {
    for_range(i, 0, inst->template->num_ins) {
        if (inst->ins[i] == vreg) return true;
    }
    return false;
}

static void debug_ptrmap(PtrMap* pm) {
    printf("PTRMAP DEBUG:\n");
    for_range(i, 0, pm->cap) {
        printf("\t%p -> %p\n", pm->keys[i], pm->vals[i]);
    }
}

static void calculate_live_ranges(AsmFunction* f, PtrMap* vreg2liverange) {

    AsmBlock* block = f->blocks[0];

    foreach(VReg* v, f->vregs) {
    
        LiveRange* range = mars_alloc(sizeof(*range));

        for_range(i, 0, block->len) {
            if (is_defined_by(block->at[i], v)) {
                range->start = i;
                break;
            }
        }

        for_range(i, 0, block->len) {
            if (is_used_by(block->at[i], v)) {
                range->end = max(i - 1, range->start);
            }
        }

        switch (v->special){
        case VSPEC_NONE: break;
        case VSPEC_PARAMVAL:
            range->start = 0;
            break;
        case VSPEC_RETURNVAL:
            range->end = block->len-1;
            break;
        default:
            CRASH("unhandled vspec");
            break;
        }
        ptrmap_put(vreg2liverange, v, range);
        LiveRange* r = ptrmap_get(vreg2liverange, v);
        if (r != range) printf("FUCK something went wrong with ptrmap\n");



        // printf("(%d -> %d)\n", r->start, r->end);
    }

    // debug_ptrmap(vreg2liverange);
}

static bool is_reg_occupied_at(AsmFunction* f, PtrMap* vreg2liverange, u32 regclass, u32 physreg, u32 inst_index) {    
    foreach(VReg* v, f->vregs) {
        LiveRange* range = ptrmap_get(vreg2liverange, v);
        assert(range != PTRMAP_NOT_FOUND);

        if (range->start <= inst_index && range->end >= inst_index) {
            // live range interferes

            if(range->start == 0 && range->end == 0) return false;

            if (v->required_regclass == regclass && v->real == physreg) {
                return true;
            }
        }
    }
    return false;
}

static void alloc(AsmFunction* f, PtrMap* vreg2liverange) {
    foreach(VReg* v, f->vregs) {
        if (v->real != ATLAS_PHYS_UNASSIGNED) continue;

        LiveRange* range = ptrmap_get(vreg2liverange, v);
        assert(range != PTRMAP_NOT_FOUND);
        
        TargetRegisterClass* regclass = &target->regclasses[v->required_regclass];


        if(!is_reg_occupied_at(f, vreg2liverange, v->required_regclass, v->hint, range->start)) {
            v->real = v->hint;
        } else for(int i = regclass->regs_len - 1; i >= 0; i--) {
            if(!is_reg_occupied_at(f, vreg2liverange, v->required_regclass, i, range->start)) {
                v->real = i;
                break;
            }
        }
    }
}

void shit_regalloc(AsmModule* m, AsmFunction* f) {
    target = m->target; // load into target slot

    assert(f->num_blocks == 1);

    PtrMap vreg2liverange = {0};
    ptrmap_init(&vreg2liverange, f->vregs.len * 2); // extra padding

    calculate_live_ranges(f, &vreg2liverange);

    alloc(f, &vreg2liverange);

}