#include "common/args.h"
#include "common/vec.h"
#include "common/util.h"

typedef struct LongArgInfo {
    string format;
    u32 arg_id;
} LongArgInfo;

typedef struct ShortArgInfo {
    char short_form;
    u32 arg_id;
} ShortArgInfo;

struct ArgBuilder {
    Vec(LongArgInfo) long_args;
    Vec(ShortArgInfo) short_args;
};

struct ParsedArgs {

};

ArgBuilder* args_new() {

}

void argb_destroy(ArgBuilder* ab);

void argb_create(ArgBuilder* ab, u32 arg_id, string template, bool required);
void argb_create_switch(ArgBuilder* ab, u32 arg_id, char short_form, string long_form);

ParsedArgs* args_parse(ArgBuilder* ab, int argc, const char** argv) {
    Vec(char) argstring = vec_new(char, 128);

    for_n(i, 0, argc) {
        const char* arg = argv[i];
        usize len = strlen(arg);
        vec_append_many(&argstring, arg, len);
        vec_append(&argstring, ' ');
    }

    TODO("fuck");

    vec_destroy(&argstring);
}
void args_destroy(ParsedArgs* pa);

Argument* args_get(ParsedArgs* pa, u32 arg_id);
string args_get_uncaptured(ParsedArgs* pa, u32 index);

