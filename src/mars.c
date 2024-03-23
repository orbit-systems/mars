// generate the actual orbit.h code here
#define ORBIT_IMPLEMENTATION

#include "orbit.h"
#include "mars.h"
#include "phobos/phobos.h"
#include "phobos/checker.h"

flag_set mars_flags;

int main(int argc, char** argv) {
    
    load_arguments(argc, argv, &mars_flags);

    mars_module* target = parse_target_module(mars_flags.input_path);

    FOR_URANGE(i, 0, target->program_tree.len) {
        dump_tree(target->program_tree.at[i], 0);
    }

    printf("target module parsed %p\n", target);

    return 0;
}

void print_help() {
    printf("usage: mars (directory) [flags]\n\n");
    printf("(directory)         where the target module is.\n");
    printf("\n");
    printf("-o:(path)           specify an output path\n");
    printf("-help               display this text\n\n");
    printf("-timings            print compiler stage timings\n");
    printf("-dot                convert the parse tree to a DOT file for graphviz\n");
}

cmd_arg make_argument(char* s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == ':') {
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
    fl->input_path = input_directory_arg.key;

    if (argc <= 2) return;

    int flag_start_index = 2;
    FOR_RANGE(i, flag_start_index, argc) {
        cmd_arg a = make_argument(argv[i]);
        if (string_eq(a.key, to_string("-help"))) {
            print_help();
            exit(EXIT_SUCCESS);
        } else if (string_eq(a.key, to_string("-o"))) {
            fl->output_path = a.val;
        } else if (string_eq(a.key, to_string("-dot"))) {
            fl->output_dot = true;
        } else if (string_eq(a.key, to_string("-timings"))) {
            fl->print_timings = true;
        } else {
            general_error("unrecognized option \""str_fmt"\"", str_arg(a.key));
        }
    }
}