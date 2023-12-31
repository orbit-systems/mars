#include "orbit.h"
#include "mars.h"
#include "deimos/tac.h"
#include "phobos/phobos.h"

flag_set mars_flags;

int main(int argc, char** argv) {
    
    load_arguments(argc, argv, &mars_flags);

    parse_program();



    //testTAC();
    return 0;
}





void print_help() {
    printf("usage: mars (directory) [module name] [flags]\n\n");
    printf("(directory)         where the target module(s) are.\n");
    printf("[module name]       specify a module to compile. leave this blank to compile every module in a directory.\n");
    printf("\n");
    printf("-o:(path)           specify an output path\n");
    printf("-help               display this text\n");
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

    // set default values
    fl->input_path = NULL_STR;
    fl->module_name = NULL_STR;
    fl->output_path = NULL_STR;

    cmd_arg input_directory_arg = make_argument(argv[1]);
    if (string_eq(input_directory_arg.key, to_string("-help"))) {
        print_help();
        exit(EXIT_SUCCESS);
    }
    if (!is_null_str(input_directory_arg.val)) {
        printf("error: expected an input path, got \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    fl->input_path = input_directory_arg.key;

    if (argc <= 2) return;

    int flag_start_index = 2;
    cmd_arg module_name_arg = make_argument(argv[2]);
    if (is_null_str(module_name_arg.val) && module_name_arg.key.raw[0] != '-') {
        fl->module_name = module_name_arg.key;
        flag_start_index++;
    }

    FOR_RANGE_EXCL(i, flag_start_index, argc) {
        cmd_arg a = make_argument(argv[i]);
        if (string_eq(a.key, to_string("-help"))) {
            print_help();
            exit(EXIT_SUCCESS);
        } else if (string_eq(a.key, to_string("-o"))) {
            fl->output_path = a.val;
        } else {
            printf("error: unrecognized option \"%s\"\n", to_cstring(a.key));
            exit(EXIT_FAILURE);
        }
    }
};