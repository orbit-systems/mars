#include "orbit.h"
#include "mars.h"

#include "term.h"

#include "phobos.h"
#include "lex.h"
#include "parse.h"
#include "sema.h"
#include "ast.h"

module_list active_modules;

da(cstring) cwd_stack;

void restore_cwd() {
    if (cwd_stack.len == 0) {
        CRASH("cwd_stack popped at len == 0");
    }

    char* cwd = cwd_stack.at[cwd_stack.len-1];

    chdir(cwd);
    da_pop(&cwd_stack);
}

void change_cwd(char* dir) {
    if (cwd_stack.at == NULL) {
        da_init(&cwd_stack, 1);
    }

    // relying on gnu behavior, this may cause a problem
    char* cwd = getcwd(NULL, 0);
    da_append(&cwd_stack, cwd);

    chdir(dir);
}


/*tune this probably*/
#define PARSER_ARENA_SIZE 0x100000

string search_for_module(mars_module* mod, string relpath) {
    // search locally first
    change_cwd(clone_to_cstring(mod->module_path));
    if (fs_exists(relpath)) {
        string mod_realpath = string_alloc(PATH_MAX);
        realpath(clone_to_cstring(relpath), mod_realpath.raw);
        restore_cwd();
        return mod_realpath;
    }
    restore_cwd();

    // look in cwd
    if (fs_exists(relpath)) {
        string mod_realpath = string_alloc(PATH_MAX);
        realpath(clone_to_cstring(relpath), mod_realpath.raw);
        return mod_realpath;
    }

    return NULL_STR;

}

mars_module* parse_module(string input_path) {

    // path checks
    if (!fs_exists(input_path))
        general_error("module \""str_fmt"\" does not exist", str_arg(input_path));

    fs_file input_dir = {0};
    fs_get(input_path, &input_dir);
    if (!fs_is_directory(&input_dir)) {
        general_error("path \""str_fmt"\" is not a directory", str_arg(input_path));
    }

    int subfile_count = fs_subfile_count(&input_dir);
    if (subfile_count == 0) {
        general_error("path \""str_fmt"\" has no files", str_arg(input_path));
    }

    fs_file* subfiles = malloc(sizeof(fs_file) * subfile_count);
    fs_get_subfiles(&input_dir, subfiles);

    change_cwd(clone_to_cstring(input_path));

    da(lexer) lexers;
    da_init(&lexers, subfile_count);

    int mars_file_count = 0;
    FOR_RANGE(i, 0, subfile_count) {

        // filter out non-files and non-mars files.
        if (!fs_is_regular(&subfiles[i])) continue;
        if (!string_ends_with(subfiles[i].path, str(".mars"))) continue;

        mars_file_count++;

        string loaded_file;

        // stop the lexer from shitting itself
        if (subfiles[i].size == 0) 
            loaded_file = string_clone(str(" \n "));
        else 
            loaded_file = string_alloc(subfiles[i].size);

        fs_open(&subfiles[i], "rb");

        bool read_success = true;
        if (subfiles[i].size != 0) {
            read_success = fs_read(&subfiles[i], loaded_file.raw, subfiles[i].size);
        }

        fs_close(&subfiles[i]);

        if (read_success == false)
            general_error("cannot read from \""str_fmt"\"", str_arg(subfiles[i].path));


        lexer this_lexer = new_lexer(subfiles[i].path, loaded_file);

        da_append(&lexers, this_lexer);

    }
    if (mars_file_count == 0)
        general_error("path \""str_fmt"\" has no \".mars\" files", str_arg(input_path));

    // timing
    struct timeval lex_begin, lex_end;
    if (mars_flags.print_timings) gettimeofday(&lex_begin, 0);        
    size_t tokens_lexed = 0;

    FOR_URANGE(i, 0, lexers.len) {
        construct_token_buffer(&lexers.at[i]);
        tokens_lexed += lexers.at[i].buffer.len;
    }

    if (mars_flags.print_timings) {
        gettimeofday(&lex_end, 0);
        long seconds = lex_end.tv_sec - lex_begin.tv_sec;
        long microseconds = lex_end.tv_usec - lex_begin.tv_usec;
        double elapsed = (double) seconds + (double) microseconds*1e-6;
        printf(STYLE_FG_Cyan STYLE_Bold "LEXING" STYLE_Reset);
        printf("\t  time      : %fs\n", elapsed);
        printf("\t  tokens    : %zu\n", tokens_lexed);
        printf("\t  tok/s     : %.3f\n", (double) tokens_lexed / elapsed);
    }

    restore_cwd();

    da(parser) parsers;
    da_init(&parsers, lexers.len);

    arena alloca = arena_make(PARSER_ARENA_SIZE);

    FOR_URANGE(i, 0, lexers.len) {
        
        parser p = make_parser(&lexers.at[i], &alloca);

        da_append(&parsers, p);
    }

    // timing
    struct timeval parse_begin, parse_end;
    if (mars_flags.print_timings) gettimeofday(&parse_begin, 0);
    size_t ast_nodes_created = 0;

    FOR_URANGE(i, 0, parsers.len) {
        parse_file(&parsers.at[i]);
        ast_nodes_created += parsers.at[i].num_nodes;
    }

    mars_module* module = create_module(&parsers, alloca);
    module->module_path = input_path;
    module->visited = true;

    if (active_modules.at == NULL) {
        da_init(&active_modules, 1);
    }
    da_append(&active_modules, module);

    /* display timing */ 
    if (mars_flags.print_timings) {
        gettimeofday(&parse_end, 0);
        long seconds = parse_end.tv_sec - parse_begin.tv_sec;
        long microseconds = parse_end.tv_usec - parse_begin.tv_usec;
        double elapsed = (double) seconds + (double) microseconds*1e-6;
        printf(STYLE_FG_Blue STYLE_Bold "PARSING" STYLE_Reset);
        printf("\t  time      : %fs\n", elapsed);
        printf("\t  AST nodes : %zu\n", ast_nodes_created);
        printf("\t  nodes/s   : %.3f\n", (double) ast_nodes_created / elapsed);
    }

    // index and parse imports
    FOR_URANGE(i, 0, module->program_tree.len) {
        if (module->program_tree.at[i].type == AST_import_stmt) {
            string importpath = search_for_module(
                module, 
                module->program_tree.at[i].as_import_stmt->path.as_literal_expr->value.as_string
            );

            // does module exist?
            if (is_null_str(importpath)) {
                error_at_node(module, module->program_tree.at[i], "path not found");
            }
            // has it been imported yet?
            int found_imported_module = -1;
            FOR_URANGE(j, 0, active_modules.len) {
                if (string_eq(active_modules.at[j]->module_path, importpath)) {
                    found_imported_module = j;
                }
            }

            module->program_tree.at[i].as_import_stmt->realpath = importpath;

            if (found_imported_module != -1) {
                if (active_modules.at[found_imported_module]->visited) {
                    error_at_node(module, module->program_tree.at[i], "cyclic imports are not allowed");
                }
                da_append(&module->import_list, active_modules.at[found_imported_module]);
            } else {
                // parse new module
                mars_module* import_module = parse_module(importpath);
                da_append(&module->import_list, import_module);

                // check module name conflicts
                FOR_URANGE(j, 0, active_modules.len) {
                    if (import_module == active_modules.at[j]) continue;
                    if (string_eq(active_modules.at[j]->module_name, import_module->module_name)) {
                        warning_at_node(module, module->program_tree.at[i], 
                            "imported module may cause symbol conflicts with module at \""str_fmt"\"", str_arg(active_modules.at[j]->module_path));
                    }
                }
            }
        }
    }

    module->visited = false;

    // cleanup
    // FOR_RANGE(i, 0, subfile_count) fs_drop(&subfiles[i]);
    // free(subfiles);
    // fs_drop(&input_dir);

    return module;
}

mars_module* create_module(da(parser)* pl, arena alloca) {
    if (pl == NULL) CRASH("build_module() provided with null parser list pointer");
    if (pl->len == 0) CRASH("build_module() provided with parser list of length 0");

    mars_module* mod = malloc(sizeof(mars_module));
    if (mod == NULL) CRASH("build_module() module alloc failed");

    mod->AST_alloca = alloca;

    mod->module_name = pl->at[0].module_decl.as_module_decl->name->text;

    da_init(&mod->import_list, 1);

    da_init(&mod->files, pl->len);
    FOR_URANGE(i, 0, pl->len) {
        if (!string_eq(pl->at[i].module_decl.as_module_decl->name->text, mod->module_name)) {
            error_at_string(pl->at[i].path, pl->at[i].src, pl->at[i].module_decl.as_module_decl->name->text,
                "mismatched module name, expected '"str_fmt"'", str_arg(mod->module_name));
        }

        da_append(&mod->files, ((mars_file){
            pl->at[i].path,
            pl->at[i].src
        }));
    }

    if (string_eq(mod->module_name, str("mars")))
        error_at_string(pl->at[0].path, pl->at[0].src, pl->at[0].module_decl.as_module_decl->name->text,
            "module name 'mars' is reserved");

    // stitch ASTs together
    da_init(&mod->program_tree, pl->len);
    FOR_URANGE(file, 0, pl->len) {
        FOR_URANGE(stmt, 0, pl->at[file].stmts.len) {
            da_append(&mod->program_tree, pl->at[file].stmts.at[stmt]);
        }
    }

    return mod;
}

mars_file *find_source_file(mars_module* cu, string snippet) {
    FOR_URANGE(i, 0, cu->files.len) {
        if (is_within(cu->files.at[i].src, snippet)) {
            return &cu->files.at[i];
        }
    }
    return NULL;
}
