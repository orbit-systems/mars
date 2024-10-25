// generate the actual orbit.h code here
#define ORBIT_IMPLEMENTATION

#include "common/orbit.h"
#include "mars/mars.h"
#include "irgen.h"
#include "targettriples.h"

#include "phobos/phobos.h"
#include "phobos/dot.h"
#include "phobos/parse/parse.h"
#include "phobos/analysis/sema.h"

#include "common/crash.h"

#include "iron/iron.h"
#include "iron/codegen/x64/x64.h"

#include "common/ptrmap.h"

flag_set mars_flags;

FeModule* irgen_module(mars_module* mars);

void apply_current_arch(mars_module* mod) {
    foreach (mars_module* curr_mod, mod->import_list) {
        curr_mod->current_architecture = mod->current_architecture;
        apply_current_arch(curr_mod);
    }
}

int main(int argc, char** argv) {
#ifndef _WIN32
    init_signal_handler();
#endif

    load_arguments(argc, argv, &mars_flags);

    mars_module* main_mod = parse_module(mars_flags.input_path);

    main_mod->current_architecture = mars_arch_to_fe(mars_flags.target_arch);
    apply_current_arch(main_mod);

    if (mars_flags.output_dot == true) {
        emit_dot(str("test"), main_mod->program_tree);
    }
    // recursive check
    check_module(main_mod);

    printf("attempt IR generation\n");

    FeModule* iron_module = irgen_module(main_mod);

    printf("IR generated\n");
    printf("attempt passes\n");

    MARS_STANDARD_PASSES(iron_module);

    fe_run_all_passes(iron_module, true);

    printf("passes done\n");

    printf("attempt codegen\n");

    iron_module->target.arch = main_mod->current_architecture;
    iron_module->target.system = mars_sys_to_fe(mars_flags.target_system);

    FeMachBuffer mb = fe_mach_codegen(iron_module);

    FeDataBuffer db = fe_db_new(128);

    fe_mach_emit_text(&db, &mb);

    printf("\n%s\n", fe_db_clone_to_cstring(&db));

    return 0;
}

void print_help() {
    printf("usage: mars (directory) [flags]\n\n");
    printf("(directory)                       where the main_mod module is.\n");
    printf("\n");
    printf("-o:(path)                         specify an output path\n");
    printf("-help                             display this text\n");
    printf("-target:(arch)-(system)-(product) specify the target triple you are using, e.g aphelion-unknown-asm\n");
    printf("\n");
    printf("-timings                          print stage timings\n");
    printf("-dump-AST                         print readable AST\n");
    printf("-dot                              convert the AST to a graphviz .dot file\n");
}

cmd_arg make_argument(char* s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == ':') {
            s[i] = '\0';
            return (cmd_arg){str(s), str(s + i + 1)};
        }
    }
    return (cmd_arg){str(s), NULL_STR};
}

void load_arguments(int argc, char* argv[], flag_set* fl) {
    if (argc < 2) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    *fl = (flag_set){0};

    fl->target_arch = -1;
    fl->target_system = -1;
    fl->target_product = -1;

    cmd_arg input_directory_arg = make_argument(argv[1]);
    if (string_eq(input_directory_arg.key, str("-help"))) {
        print_help();
        exit(EXIT_SUCCESS);
    }
    if (!is_null_str(input_directory_arg.val)) {
        general_error("error: expected an input path, got \"%s\"", argv[1]);
    }
    char dumbass_shit_buffer[PATH_MAX] = {0};

    char* got = realpath(input_directory_arg.key.raw, dumbass_shit_buffer);
    if (got == NULL) {
        general_error("could not find '%s'", input_directory_arg.key.raw);
    }

    fl->input_path = string_clone(str(dumbass_shit_buffer));

    int flag_start_index = 2;
    for_range(i, flag_start_index, argc) {
        cmd_arg a = make_argument(argv[i]);
        if (string_eq(a.key, str("-help"))) {
            print_help();
            exit(EXIT_SUCCESS);
        } else if (string_eq(a.key, str("-o"))) {

            char dumbass_shit_buffer[PATH_MAX] = {0};

            char* got = realpath(a.val.raw, dumbass_shit_buffer);
            if (got == NULL) {
                general_error("could not find '%.*s'", a.val.len, a.val.raw);
            }

            fl->output_path = string_clone(str(dumbass_shit_buffer));
        } else if (string_eq(a.key, str("-dot"))) {
            fl->output_dot = true;
        } else if (string_eq(a.key, str("-timings"))) {
            fl->print_timings = true;
        } else if (string_eq(a.key, str("-dump-AST"))) {
            fl->dump_AST = true;
        } else if (string_eq(a.key, str("-target"))) {
            set_target_triple(a.val, fl);
        } else {
            general_error("unrecognized option \"" str_fmt "\"", str_arg(a.key));
        }
    }

    if (fl->target_arch == -1 || fl->target_system == -1 || fl->target_product == -1) {
        printf("No target selected, defaulting to " str_fmt "\n", str_arg(DEFAULT_TARGET));
        set_target_triple(DEFAULT_TARGET, fl);
        return;
    }
}