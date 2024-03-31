// generate the actual orbit.h code here
#define ORBIT_IMPLEMENTATION

#include "orbit.h"
#include "mars.h"

#include "phobos/phobos.h"
#include "phobos/dot.h"
#include "phobos/parse.h"
#include "phobos/sema.h"

#include "deimos/deimos.h"

flag_set mars_flags;

int main(int argc, char** argv) {
    
    load_arguments(argc, argv, &mars_flags);

    mars_module* main_mod = parse_module(mars_flags.input_path);

    FOR_URANGE(i, 0, main_mod->program_tree.len) {
        dump_tree(main_mod->program_tree.at[i], 0);
        // process_ast(main_mod->program_tree.at[i]);
    }

    if (mars_flags.output_dot == true) {  
        emit_dot(to_string("test"), main_mod->program_tree);
    }

    // printf("main_mod module parsed %p\n", main_mod);

    // FOR_URANGE(i, 0, main_mod->program_tree.len) {
    //     dump_tree(main_mod->program_tree.at[i], 0);
    // }

    // recursive check
    checked_expr e = {0};
    check_expr(
        main_mod, 
        NULL, 
        main_mod->program_tree.at[0].as_decl_stmt->rhs,
        &e,
        true,
        NULL
    );

    printf("%p %p\n", e.ev->kind, e.ev->as_untyped_int);


    // check_module_and_dependencies(main_mod);



    // process_ast(main_mod->program_tree);


    return 0;
}

void print_help() {
    printf("usage: mars (directory) [flags]\n\n");
    printf("(directory)         where the main_mod module is.\n");
    printf("\n");
    printf("-o:(path)           specify an output path\n");
    printf("-help               display this text\n\n");
    printf("-timings            print compiler stage timings\n");
    printf("-dot                convert the parse tree to a DOT file for graphviz\n");
}

cmd_arg make_argument(char* s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == ':') {
            s[i] = '\0';
            return (cmd_arg){to_string(s), to_string(s+i+1)};
        }
    }
    return (cmd_arg){to_string(s), NULL_STR};
}

void load_arguments(int argc, char* argv[], flag_set* fl) {
    if (argc < 2) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    *fl = (flag_set){0};

    cmd_arg input_directory_arg = make_argument(argv[1]);
    if (string_eq(input_directory_arg.key, to_string("-help"))) {
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

    fl->input_path = string_clone(to_string(dumbass_shit_buffer));

    if (argc <= 2) return;

    int flag_start_index = 2;
    FOR_RANGE(i, flag_start_index, argc) {
        cmd_arg a = make_argument(argv[i]);
        if (string_eq(a.key, to_string("-help"))) {
            print_help();
            exit(EXIT_SUCCESS);
        } else if (string_eq(a.key, to_string("-o"))) {

            char dumbass_shit_buffer[PATH_MAX] = {0};

            char* got = realpath(a.val.raw, dumbass_shit_buffer);
            if (got == NULL) {
                general_error("could not find '%s'", a.val.raw);
            }

            fl->output_path = string_clone(to_string(dumbass_shit_buffer));
        } else if (string_eq(a.key, to_string("-dot"))) {
            fl->output_dot = true;
        } else if (string_eq(a.key, to_string("-timings"))) {
            fl->print_timings = true;
        } else {
            general_error("unrecognized option \""str_fmt"\"", str_arg(a.key));
        }
    }
}