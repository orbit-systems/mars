#include "iron/iron.h"

/* pass "template" - do nothing

    this is a template pass to copy when making a new one.

*/

void run_pass_template(FeModule* mod) {
    /* code here! */
}

FePass air_pass_template = {
    .name = "template",
    .callback = run_pass_template,
};