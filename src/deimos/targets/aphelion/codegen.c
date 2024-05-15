#include "aphelion.h"
#include "target.h"
#include "ir.h"

AsmModule* aphelion_translate_module(IR_Module* irmod) {
    AsmModule* mod = asm_new_module(&aphelion_target_info);


}

AsmFunction* aphelion_translate_function(AsmModule* m, IR_Function* f) {
    
}

AsmBlock* aphelion_translate_block(AsmModule* m, IR_Function* f, IR_BasicBlock* bb) {

    


}