#include "aphelion.h"
#include "target.h"
#include "ir.h"

AsmModule* aphelion_translate_module(IR_Module* irmod) {
    AsmModule* mod = asm_new_module(&aphelion_target_info);
}