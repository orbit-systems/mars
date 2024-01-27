#include "orbit.h"
#include "mars.h"
#include "da.h"
#include "term.h"

#include "phobos.h"
#include "lexer.h"
#include "parser.h"

/*tune this probably*/
#define PARSER_ARENA_SIZE 0x100000

mars_module* phobos_parse_target_module() {

    // path checks
    if (!fs_exists(mars_flags.input_path))
        general_error("input directory \"%s\" does not exist", clone_to_cstring(mars_flags.input_path));

    fs_file input_dir = {0};
    fs_get(mars_flags.input_path, &input_dir);
    if (!fs_is_directory(&input_dir))
        general_error("input path \"%s\" is not a directory", clone_to_cstring(mars_flags.input_path));

    int subfile_count = fs_subfile_count(&input_dir);
    if (subfile_count == 0)
        general_error("input path \"%s\" has no files", clone_to_cstring(mars_flags.input_path));

    fs_file* subfiles = malloc(sizeof(fs_file) * subfile_count);
    fs_get_subfiles(&input_dir, subfiles);

    da(lexer) lexers;
    da_init(&lexers, subfile_count);

    int mars_file_count = 0;
    FOR_RANGE(i, 0, subfile_count) {

        // filter out non-files and non-mars files.
        if (!fs_is_regular(&subfiles[i])) continue;
        if (!string_ends_with(subfiles[i].path, to_string(".mars"))) continue;

        mars_file_count++;

        string loaded_file;

        // stop the lexer from shitting itself
        if (subfiles[i].size == 0) 
            loaded_file = string_clone(to_string(" "));
        else 
            loaded_file = string_alloc(subfiles[i].size);

        fs_open(&subfiles[i], "rb");

        bool read_success = true;
        if (subfiles[i].size != 0) {
            read_success = fs_read(&subfiles[i], loaded_file.raw, subfiles[i].size);
        }

        fs_close(&subfiles[i]);

        if (read_success == false)
            general_error("cannot read from \"%s\"", clone_to_cstring(subfiles[i].path));


        lexer this_lexer = new_lexer(subfiles[i].path, loaded_file);

        da_append(&lexers, this_lexer);

    }
    if (mars_file_count == 0)
        general_error("input path \"%s\" has no \".mars\" files", clone_to_cstring(mars_flags.input_path));

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
        printf("\t  tokens    : %lu\n", tokens_lexed);
        printf("\t  tok/s     : %.3f\n", (double) tokens_lexed / elapsed);
    }

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

    /* display timing */ 
    if (mars_flags.print_timings) {
        gettimeofday(&parse_end, 0);
        long seconds = parse_end.tv_sec - parse_begin.tv_sec;
        long microseconds = parse_end.tv_usec - parse_begin.tv_usec;
        double elapsed = (double) seconds + (double) microseconds*1e-6;
        printf(STYLE_FG_Blue STYLE_Bold "PARSING" STYLE_Reset);
        printf("\t  time      : %fs\n", elapsed);
        printf("\t  AST nodes : %lu\n", ast_nodes_created);
        printf("\t  nodes/s   : %.3f\n", (double) ast_nodes_created / elapsed);
    }

    // cleanup
    FOR_RANGE(i, 0, subfile_count) fs_drop(&subfiles[i]);
    free(subfiles);
    fs_drop(&input_dir);

    return module;
}

mars_module* create_module(da(parser)* pl, arena alloca) {
    if (pl == NULL) CRASH("build_module() provided with null parser list pointer");
    if (pl->len == 0) CRASH("build_module() provided with parser list of length 0");

    mars_module* mod = malloc(sizeof(mars_module));
    if (mod == NULL) CRASH("build_module() module alloc failed");

    mod->alloca = alloca;

    mod->module_name = pl->at[0].module_decl.as_module_decl->name->text;
    if (!string_eq(mod->module_name, to_string("mars"))) {}

    da_init(&mod->files, pl->len);
    FOR_URANGE(i, 0, pl->len) {
        if (!string_eq(pl->at[i].module_decl.as_module_decl->name->text, mod->module_name)) {
            printf("WE HERE\n");
            error_at_string(pl->at[i].path, pl->at[i].src, pl->at[i].module_decl.as_module_decl->name->text,
                "mismatched module name, expected '"str_fmt"'", str_arg(mod->module_name));
        }

        mod->files.at[i].path = pl->at[i].path;
        mod->files.at[i].src  = pl->at[i].src;
    }

    if (string_eq(mod->module_name, to_string("mars")))
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

mars_file* find_source_file(mars_module* cu, string snippet) {
    FOR_URANGE(i, 0, cu->files.len) {
        if (is_within(cu->files.at[i].src, snippet)) {
            return &cu->files.at[i];
        }
    }
    return NULL;
}