#include "iron/iron.h"

/* pass "template" - do nothing

    this is a template pass to copy when making a new one.

*/

static void function_template(FeFunction* fn) {
    /* code here! */
}

static void module_template(FeModule* mod) {
    /* code here! */
}

FePass fe_pass_template = {
    .name = "template",
    .module = module_template,
    .function = function_template,
};