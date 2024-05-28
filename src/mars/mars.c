// generate the actual orbit.h code here
#define ORBIT_IMPLEMENTATION

#include "orbit.h"
#include "mars.h"
#include "irgen.h"

#include "phobos/phobos.h"
#include "phobos/dot.h"
#include "phobos/parse.h"
#include "phobos/sema.h"

#include "atlas/atlas.h"
#include "atlas/targettriples.h"

#include "llta/lexer.h"

flag_set mars_flags;

int main(int argc, char** argv) {
    load_arguments(argc, argv, &mars_flags);


    AtlasModule* atlas_module;

    if (!mars_flags.use_llta) {

        mars_module* main_mod = parse_module(mars_flags.input_path);    

        if (mars_flags.output_dot == true) {  
            emit_dot(str("test"), main_mod->program_tree);
        }   
    
        // recursive check
        check_module_and_dependencies(main_mod);

        if (mars_flags.dump_AST) for_urange(i, 0, main_mod->program_tree.len) {
            dump_tree(main_mod->program_tree.at[i], 0);
        }
        

        TargetInfo* atlas_target;

        switch (mars_flags.target_arch){
        case TARGET_ARCH_APHELION: 
            atlas_target = &aphelion_target_info; 
            break;
        default:
            CRASH("");
            break;
        }

        atlas_module = atlas_new_module(main_mod->module_name, atlas_target);
        generate_atlas_from_mars(main_mod, atlas_module);
    } else {
        atlas_module = llta_parse_ir(mars_flags.input_path);
    }

    atlas_append_pass(atlas_module, &ir_pass_trme);
    atlas_append_pass(atlas_module, &ir_pass_tdce);
    atlas_append_pass(atlas_module, &ir_pass_movprop);
    atlas_append_pass(atlas_module, &ir_pass_elim);

    atlas_append_pass(atlas_module, &asm_pass_aphelion_cg);

    atlas_run_all_passes(atlas_module);

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
    printf("-llta                             use llta instead of phobos\n");
}

cmd_arg make_argument(char* s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == ':') {
            s[i] = '\0';
            return (cmd_arg){str(s), str(s+i+1)};
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

    if (argc <= 2) {
        printf("No target selected, defaulting to "str_fmt "\n", str_arg(DEFAULT_TARGET));
        set_target_triple(DEFAULT_TARGET, fl);
        return;
    }

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
        } else if (string_eq(a.key, str("-llta"))) {
            fl->use_llta = true;
        } else {
            general_error("unrecognized option \""str_fmt"\"", str_arg(a.key));
        }
    }
}

/*
void parse_target_triple(string tt, flag_set* fl) {
    string arch = str("");
    string system = str("");
    string product = str("");
    int curr_anchor = 0;
    int curr_str = 0;
    for (int i = 0; i < tt.len; i++) {
        if (tt.raw[i] == '-' && curr_str == 0) {
            arch = string_clone(substring(tt, curr_anchor, i));
            curr_str++;
            curr_anchor = i + 1;
            continue;
        }
        if (tt.raw[i] == '-' && curr_str == 1) {
            system = string_clone(substring(tt, curr_anchor, i));
            curr_str++;
            curr_anchor = i + 1;
            continue;
        }
    }
    product = string_clone(substring(tt, curr_anchor, tt.len));
    fl->target_arch = arch_from_str(arch);
    fl->target_system = sys_from_str(system);
    fl->target_product = product_from_str(product);
    if (fl->target_arch == -1)    general_error("Unrecognized architecture: " str_fmt, str_arg(arch));
    if (fl->target_system == -1)  general_error("Unrecognized system: " str_fmt, str_arg(system));
    if (fl->target_product == -1) general_error("Unrecognized product: " str_fmt, str_arg(product));
    return;
}*/