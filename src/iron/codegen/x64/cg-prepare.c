#include "iron/codegen/mach.h"
#include "iron/codegen/x64/x64.h"

// run IR passes to transform into a form that is more easily translated
// and introduce arch-specific instructions because the IR framework is
// much more suited to optimization and data-flow analysis than mach ir

