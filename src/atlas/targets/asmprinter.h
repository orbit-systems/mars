#pragma once
#define ATLAS_TARGET_ASMPRINTER_H

#include "target.h"
#include "term.h"

void debug_asm_printer(AsmModule* m);
void asm_printer(AsmModule* m, bool debug_mode);