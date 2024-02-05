#include "orbit.h"
#include "deimos.h"
#include "mil.h"

size_t num_places (u64 n) {
    size_t r = 1;
    while (n > 9) {
        n /= 10;
        r++;
    }
    return r;
}

string new_temp_name(mil_module* mod) {

    static size_t temp_number = 0;
    size_t np = num_places(temp_number);
    char* str = arena_alloc(&mod->str_alloca, np+1, 1);
    snprintf(str, np+1, "t%zu", temp_number);

    temp_number++;
    return (string){str, np};
}

var_index new_temp(mil_module* mod, );