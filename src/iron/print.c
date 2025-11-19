#include <stdlib.h>
#include <string.h>

#include "common/ansi.h"

#include "iron/iron.h"

static const char* ty_name[] = {
    [FE_TY_VOID] = "void",

    [FE_TY_BOOL] = "bool",

    [FE_TY_I8]  = "i8",
    [FE_TY_I16] = "i16",
    [FE_TY_I32] = "i32",
    [FE_TY_I64] = "i64",

    [FE_TY_F32] = "f32",
    [FE_TY_F64] = "f64",

    [FE_TY_TUPLE]  = "...",
    [FE_TY_RECORD] = "{...}",
    [FE_TY_ARRAY]  = "[...]",

    [FE_TY_I8x16] = "i8x16",
    [FE_TY_I16x8] = "i16x8",
    [FE_TY_I32x4] = "i32x4",
    [FE_TY_I64x2] = "i64x2",
    [FE_TY_F32x4] = "f32x4",
    [FE_TY_F64x2] = "f64x2",

    [FE_TY_I8x32]  = "i8x32",
    [FE_TY_I16x16] = "i16x16",
    [FE_TY_I32x8]  = "i32x8",
    [FE_TY_I64x4]  = "i64x4",
    [FE_TY_F32x8]  = "f32x8",
    [FE_TY_F64x4]  = "f64x4",

    [FE_TY_I8x64]  = "i8x64",
    [FE_TY_I16x32] = "i16x32",
    [FE_TY_I32x16] = "i32x16",
    [FE_TY_I64x8]  = "i64x8",
    [FE_TY_F32x16] = "f32x16",
    [FE_TY_F64x8]  = "f64x8",
};

static const char* inst_name[FE__BASE_INST_END] = {
    [FE__BOOKEND] = "<bookend>",

    [FE_CONST] = "const",

    [FE_STACK_ADDR] = "stack-addr",
    [FE_SYM_ADDR] = "sym-addr",

    [FE__MACH_MOV] = "mach-mov",
    [FE__MACH_UPSILON] = "mach-upsilon",
    [FE__MACH_REG] = "mach-reg",
    [FE__MACH_RETURN] = "mach-return",

    [FE__ROOT] = "root",

    [FE_PROJ] = "proj",

    [FE_IADD] = "iadd",
    [FE_ISUB] = "isub",
    [FE_IMUL] = "imul",
    [FE_IDIV] = "idiv",
    [FE_UDIV] = "udiv",
    [FE_IREM] = "irem",
    [FE_UREM] = "urem",

    [FE_AND] = "and",
    [FE_OR]  = "or",
    [FE_XOR] = "xor",
    [FE_SHL] = "shl",
    [FE_USR] = "usr",
    [FE_ISR] = "isr",
    [FE_ILT] = "ilt",
    [FE_ULT] = "ult",
    [FE_ILE] = "ile",
    [FE_ULE] = "ule",
    [FE_IEQ] = "ieq",

    [FE_FADD] = "fadd",
    [FE_FSUB] = "fsub",
    [FE_FMUL] = "fmul",
    [FE_FDIV] = "fdiv",
    [FE_FREM] = "frem",

    [FE_MOV] = "mov",
    [FE_TRUNC] = "trunc",
    [FE_SIGN_EXT] = "sign-ext",
    [FE_ZERO_EXT] = "zero-ext",
    [FE_I2F] = "i2f",
    [FE_F2I] = "f2i",
    [FE_U2F] = "u2f",
    [FE_F2U] = "f2u",

    [FE_LOAD] = "load",
    [FE_STORE] = "store",
    [FE_MEM_BARRIER] = "mem-barrier",

    [FE_UNREACHABLE] = "unreachable",
    [FE_BRANCH] = "branch",
    [FE_JUMP] = "jump",
    [FE_RETURN] = "return",

    [FE_PHI] = "phi",
    [FE_MEM_PHI] = "mem-phi",
    [FE_MEM_MERGE] = "mem-merge",

    [FE_CALL] = "call",
};

// idea stolen from TB lmao
static int ansi(usize x) {
    usize hash = (x) % 11;
    if (hash >= 6) {
        return hash - 6 + 91;
    } else {
        return hash + 31;
    }
}

thread_local static bool should_ansi = true;

static void print_ty(FeDataBuffer* db, FeTy ty, FeComplexTy* cty) {
    if (ty == FE_TY_RECORD) {
        if (cty != nullptr) {
            FE_CRASH("ty is record but no FeComplexTy was provided");
        }
        fe_db_writecstr(db, "{ ");

        for_n (i, 0, cty->record.fields_len) {
            FeRecordField* field = &cty->record.fields[i];
            if (i != 0) {
                fe_db_writecstr(db, ", ");
            }
            fe_db_writef(db, "%u: ", field->offset);
            print_ty(db, field->ty, field->complex_ty);
        }

        fe_db_writecstr(db, " }");
    } else if (ty == FE_TY_ARRAY) {
        if (cty != nullptr) {
            FE_CRASH("ty is array but no FeComplexTy was provided");
        }

        fe_db_writecstr(db, "[");
        fe_db_writef(db, "%u * ", cty->array.len);
        print_ty(db, cty->array.elem_ty, cty->array.complex_elem_ty);
        fe_db_writecstr(db, "]");
    } else {
        fe_db_writecstr(db, ty_name[ty]);
    }
}

static void print_inst_ty(FeDataBuffer* db, FeInst* inst) {
    if (inst->ty == FE_TY_TUPLE) {
        usize index = 0;
        for (FeTy ty = fe_proj_ty(inst, index); ty != FE_TY_VOID; index++, ty = fe_proj_ty(inst, index)) {
            if (index != 0) {
                fe_db_writecstr(db, ", ");
            }
            fe_db_writecstr(db, ty_name[ty]);
        }
    } else {
        fe_db_writecstr(db, ty_name[inst->ty]);
    }
}

void fe__emit_ir_stack_label(FeDataBuffer* db, FeStackItem* item) {
    if (item == nullptr) {
        if (should_ansi) {
            fe_db_writecstr(db, "\x1b[90m");
        }
        fe_db_writecstr(db, "null:");
        if (should_ansi) {
            fe_db_writecstr(db, "\x1b[0m");
        }
        return;
    }

    if (should_ansi) fe_db_writef(db, "\x1b[%dm", ansi(item->flags));
    fe_db_writef(db, "s%u", item->flags);
    if (should_ansi) fe_db_writecstr(db, "\x1b[0m");
}

void fe__emit_ir_block_label(FeDataBuffer* db, FeFunc* f, FeBlock* ref) {
    if (ref == nullptr) {
        if (should_ansi) {
            fe_db_writecstr(db, "\x1b[90m");
        }
        fe_db_writecstr(db, "null:");
        if (should_ansi) {
            fe_db_writecstr(db, "\x1b[0m");
        }
        return;
    }

    if (should_ansi) fe_db_writef(db, "\x1b[%dm", ansi(ref->id));
    fe_db_writef(db, "%u:", ref->id);
    if (should_ansi) fe_db_writecstr(db, "\x1b[0m");
}

void fe__emit_vreg_def(FeDataBuffer* db, FeFunc* f, FeVReg vr) {
        fe_db_writef(db, "v%u", vr);

        // BAD ASSUMPTION
        if (fe_vreg(f->vregs, vr)->real != FE_VREG_REAL_UNASSIGNED) {
            FeVirtualReg* vreg = fe_vreg(f->vregs, vr);
            fe_db_writef(db, "/%s", f->mod->target->reg_name(vreg->class, vreg->real));
        }
}

void fe__emit_ir_ref(FeDataBuffer* db, FeFunc* f, FeInst* ref) {
    if (ref == nullptr) {
        if (should_ansi) {
            fe_db_writecstr(db, "\x1b[90m");
        }
        fe_db_writecstr(db, "null");
        if (should_ansi) {
            fe_db_writecstr(db, "\x1b[0m");
        }
        return;
    }

    if (should_ansi) fe_db_writef(db, "\x1b[%dm", ansi(ref->id));
    if (ref->kind == FE__MACH_REG) {
        FE_ASSERT(ref->vr_def != FE_VREG_NONE);
        FeVirtualReg* vr = fe_vreg(f->vregs, ref->vr_def);
        FE_ASSERT(vr->real != FE_VREG_REAL_UNASSIGNED);
        fe_db_writef(db, "%s", f->mod->target->reg_name(vr->class, vr->real));
        if (should_ansi) fe_db_writecstr(db, "\x1b[0m");
        return;
    }

    fe_db_writef(db, "%%%u", ref->id);
    if (ref->vr_def != FE_VREG_NONE) {
        fe_db_writecstr(db, "/");
        fe__emit_vreg_def(db, f, ref->vr_def);
    }
    if (should_ansi) fe_db_writecstr(db, "\x1b[0m");
}

const char* fe_inst_name(const FeTarget* target, FeInstKind kind) {
    if (kind < FE__BASE_INST_END) {
        return inst_name[kind];
    } else {
        return "<target-specific>";
    }
}

const char* fe_ty_name(FeTy ty) {
    if (ty < FE__TY_END) {
        return ty_name[ty];
    }
    return nullptr;
}

static void print_input_list(FeDataBuffer* db, FeFunc* f, FeInst* inst, usize start_at) {
    for_n(i, start_at, inst->in_len) {
        if (i != start_at) fe_db_writecstr(db, ", ");
        fe__emit_ir_ref(db, f, inst->inputs[i]);
    }
}

static void print_inst(FeFunc* f, FeDataBuffer* db, FeInst* inst) {

    const FeTarget* target = f->mod->target;

    if (!(inst->ty == FE_TY_VOID && inst->use_len == 0)) {
        fe__emit_ir_ref(db, f, inst);
        if (inst->ty != FE_TY_VOID) {
            fe_db_writef(db, ": ");
            print_inst_ty(db, inst);
        }
        fe_db_writecstr(db, " = ");
    }
    // if (inst->use_len != 0) {
    //     fe__emit_ir_ref(db, f, inst);
    // } else {
    //     if (should_ansi) {
    //         fe_db_writecstr(db, "\x1b[90m");
    //     }
    //     fe_db_writef(db, "(%%%u)", inst->id);
    //     if (should_ansi) {
    //         fe_db_writecstr(db, "\x1b[0m");
    //     }
    // }
    // if (inst->ty != FE_TY_VOID) {
    //     fe_db_writef(db, ": ");
    //     print_inst_ty(db, inst);
    // }
    // fe_db_writecstr(db, " = ");
    
    if (inst->kind < FE__BASE_INST_END) {
        const char* name = inst_name[inst->kind];
        
        if (name) fe_db_writecstr(db, name);
        else fe_db_writef(db, "<kind %d>", inst->kind);
    } else {
        target->ir_print_inst(db, f, inst);
        fe_db_writecstr(db, "\n");
        return;
    }

    fe_db_writecstr(db, " ");

    switch (inst->kind) {
    case FE__ROOT:
        break;
    case FE_PROJ:
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writef(db, ", %u", fe_extra(inst, FeInstProj)->index);
        break;
    case FE_IADD ... FE_FREM:
        print_input_list(db, f, inst, 0);
        break;
    case FE_MOV ... FE_F2U:
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        break;
    case FE_RETURN:
        ;
        fe_db_writecstr(db, "^");
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writecstr(db, ", ");
        for_n(i, 1, inst->in_len) {
            if (i != 1) fe_db_writecstr(db, ", ");
            fe__emit_ir_ref(db, f, inst->inputs[i]);
        }
        break;
    case FE_CALL:
        ;
        fe_db_writecstr(db, "^");
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writecstr(db, ", ");
        for_n(i, 1, inst->in_len) {
            if (i != 1) fe_db_writecstr(db, ", ");
            fe__emit_ir_ref(db, f, inst->inputs[i]);
        }
        break;
    case FE_BRANCH:
        ;
        FeInstBranch* branch = fe_extra(inst);
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_block_label(db, f, branch->if_true);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_block_label(db, f, branch->if_false);
        break;
    case FE_JUMP:
        ;
        FeInstJump* jump = fe_extra(inst);
        fe__emit_ir_block_label(db, f, jump->to);
        break;
    case FE_PHI:
        ;
        FeInstPhi* phi = fe_extra(inst);
        for_n(i, 0, inst->in_len) {
            if (i != 0) {
                fe_db_writecstr(db, ", ");
            }
            FeBlock* src_block = phi->blocks[i];
            FeInst* src = inst->inputs[i];
            fe__emit_ir_block_label(db, f, src_block);
            fe_db_writecstr(db, " ");
            fe__emit_ir_ref(db, f, src);
        }
        break;
    case FE_SYM_ADDR:
        ;
        FeSymbol* sym = fe_extra(inst, FeInstSymAddr)->sym;
        fe_db_writecstr(db, "\"");
        fe_db_write(db, fe_compstr_data(sym->name), sym->name.len);
        fe_db_writecstr(db, "\"");
        break;
    case FE_STACK_ADDR:
        ;
        FeStackItem* item = fe_extra(inst, FeInstStack)->item;
        fe__emit_ir_stack_label(db, item);
        break;
    case FE_LOAD:
        ;
        FeInstMemop* load = fe_extra(inst);
        fe_db_writecstr(db, "^");
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writecstr(db, ", ");

        fe__emit_ir_ref(db, f, inst->inputs[1]);
        fe_db_writef(db, " align(%u)", load->align);
        if (load->offset) {
            fe_db_writef(db, " offset(%u) ", load->offset);
        }
        break;
    case FE_STORE:
        ;
        FeInstMemop* store = fe_extra(inst);
        fe_db_writecstr(db, "^");
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_ref(db, f, inst->inputs[1]);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_ref(db, f, inst->inputs[2]);
        fe_db_writef(db, " align(%u)", store->align);
        if (store->offset) {
            fe_db_writef(db, " offset(%u) ", store->offset);
        }
        break;
    case FE_MEM_BARRIER:
        fe_db_writecstr(db, "^");
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        break;
    case FE_MEM_PHI:
        ;
        FeInstPhi* memphi = fe_extra(inst);
        for_n(i, 0, inst->in_len) {
            if (i != 0) {
                fe_db_writecstr(db, ", ");
            }
            FeBlock* src_block = memphi->blocks[i];
            FeInst* src = inst->inputs[i];
            fe__emit_ir_block_label(db, f, src_block);
            fe_db_writecstr(db, " ^");
            fe__emit_ir_ref(db, f, src);
        }
        break;
    case FE_MEM_MERGE:
        ;
        for_n(i, 0, inst->in_len) {
            if (i != 0) {
                fe_db_writecstr(db, ", ");
            }
            FeInst* src = inst->inputs[i];
            fe_db_writecstr(db, "^");
            fe__emit_ir_ref(db, f, src);
        }
        break;
    case FE_CONST:
        switch (inst->ty) {
        case FE_TY_BOOL: fe_db_writef(db, "%s", fe_extra(inst, FeInstConst)->val ? "true" : "false"); break;
        case FE_TY_F64:  fe_db_writef(db, "%lf", (f64)fe_extra(inst, FeInstConst)->val_f64); break;
        case FE_TY_F32:  fe_db_writef(db, "%lf", (f64)fe_extra(inst, FeInstConst)->val_f32); break;
        case FE_TY_I64:  fe_db_writef(db, "%llu", (u64)fe_extra(inst, FeInstConst)->val); break;
        case FE_TY_I32:  fe_db_writef(db, "%llu", (u64)(u32)fe_extra(inst, FeInstConst)->val); break;
        case FE_TY_I16:  fe_db_writef(db, "%llu", (u64)(u16)fe_extra(inst, FeInstConst)->val); break;
        case FE_TY_I8:   fe_db_writef(db, "%llu", (u64)(u8)fe_extra(inst, FeInstConst)->val); break;
        default:
            fe_db_writef(db, "[TODO]");
            break;
        }
        break;
    case FE__MACH_REG:
        ;
        FeVReg vr = inst->vr_def;
        FeVirtualReg* vreg = fe_vreg(f->vregs, vr);
        u8 class = vreg->class;
        u16 real = vreg->real;
        fe_db_writecstr(db, f->mod->target->reg_name(class, real));
        break;
    case FE__MACH_RETURN:
        break;
    default:
        fe_db_writef(db, "[TODO LMFAO]");
    }
    fe_db_writecstr(db, "\n");
}

void fe_emit_ir_func(FeDataBuffer* db, FeFunc* f, bool fancy) {
    should_ansi = fancy;

    switch (f->sym->bind) {
    case FE_BIND_EXTERN: fe_db_writecstr(db, "extern "); break;
    case FE_BIND_GLOBAL: fe_db_writecstr(db, "global "); break;
    case FE_BIND_LOCAL:  fe_db_writecstr(db, "local "); break;
    case FE_BIND_WEAK:  fe_db_writecstr(db, "weak "); break;
    case FE_BIND_SHARED_EXPORT: fe_db_writecstr(db, "shared_export "); break;
    case FE_BIND_SHARED_IMPORT: fe_db_writecstr(db, "shared_import "); break;
    }

    // write function signature
    fe_db_writef(db, "func \"%.*s\"", f->sym->name.len, fe_compstr_data(f->sym->name));
    if (f->sig->param_len) {
        fe_db_writecstr(db, " ");
        for_n(i, 0, f->sig->param_len) {
            if (i != 0) fe_db_writecstr(db, ", ");
            fe_db_writecstr(db, ty_name[fe_funcsig_param(f->sig, i)->ty]);
        }
    }
    if (f->sig->return_len) {
        fe_db_writecstr(db, " -> ");
        for_n(i, 0, f->sig->return_len) {
            if (i != 0) fe_db_writecstr(db, ", ");
            fe_db_writecstr(db, ty_name[fe_funcsig_return(f->sig, i)->ty]);
        }
    }

    if (f->sym->bind == FE_BIND_EXTERN) {
        return;
    }

    // write function body
    fe_db_writecstr(db, " {\n");
    
    // write stack frame
    u32 stack_counter = 1;
    for (FeStackItem* item = f->stack_bottom; item != nullptr; item = item->next) {
        item->flags = stack_counter;
        fe_db_writef(db, "    s%d: ", stack_counter);
        print_ty(db, item->ty, item->complex_ty);
        fe_db_writecstr(db, "\n");
        stack_counter++;
    }

    bool print_liveness = false;

    for_blocks(block, f) {
        fe_db_writecstr(db, "  ");
        fe__emit_ir_block_label(db, f, block);
        fe_db_writecstr(db, "\n");
        
        // print live-in set if present
        if (print_liveness && block->live && block->live->in_len > 0) {
            fe_db_writecstr(db, "   [live-in ");
            for_n(i, 0, block->live->in_len) {
                if (i != 0) {
                    fe_db_writecstr(db, ", ");
                }
                FeVReg in = block->live->in[i];
                fe__emit_vreg_def(db, f, in);
            }
            fe_db_writecstr(db, "]\n");
        }

        for_inst(inst, block) {            
            if (inst->kind == FE__MACH_REG) {
                continue;
            }
            fe_db_writecstr(db, "    ");
            print_inst(f, db, inst);
        }
        // print live-in set if present
        if (print_liveness && block->live && block->live->out_len > 0) {
            fe_db_writecstr(db, "   [live-out ");
            for_n(i, 0, block->live->out_len) {
                if (i != 0) {
                    fe_db_writecstr(db, ", ");
                }
                FeVReg out = block->live->out[i];
                fe__emit_vreg_def(db, f, out);
                
            }
            fe_db_writecstr(db, "]\n");
        }
    }
    fe_db_writecstr(db, "}\n");
}
