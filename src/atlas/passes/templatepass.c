#include "atlas.h"

/* pass "template" - do nothing

    this is a template pass to copy when making a new one.

*/

void run_pass_template(AIR_Module* mod) {
    /* code here! */
}

AtlasPass ir_pass_template = {
    .name = "template",
    .ir2ir_callback = run_pass_template,
    .kind = PASS_IR_TO_IR,
};