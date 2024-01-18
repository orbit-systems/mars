#include "orbit.h"
#include "mars.h"
#include "dynarr.h"
#include "term.h"

#include "phobos.h"
#include "lexer.h"
#include "parser.h"
#include "type.h"

/*tune this probably*/
#define PARSER_ARENA_SIZE 0x100000

compilation_unit* phobos_perform_frontend() {

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

    dynarr(lexer) lexers;
    dynarr_init(lexer, &lexers, subfile_count);

    int mars_file_count = 0;
    FOR_RANGE_EXCL(i, 0, subfile_count) {

        // filter out non-files and non-mars files.
        if (!fs_is_regular(&subfiles[i])) continue;
        if (!string_ends_with(subfiles[i].path, to_string(".mars"))) continue;

        mars_file_count++;

        string loaded_file;

        // stop the lexer from shitting itself
        if (subfiles[i].size == 0) 
            loaded_file = to_string(" ");
        else 
            loaded_file = string_alloc(subfiles[i].size);

        fs_open(&subfiles[i], "r"
        #if (defined(MINGW32) || defined(__MINGW32__))
            "b"  /* stupid fucking windows permission that makes it so it doesn't stop loading the bullshit file */
                 /* can you tell im angry about this */
        #endif
        );

        bool read_success = true;
        if (subfiles[i].size != 0) {
            read_success = fs_read(&subfiles[i], loaded_file.raw, subfiles[i].size);
        }

        fs_close(&subfiles[i]);

        if (read_success == false)
            general_error("cannot read from \"%s\"", clone_to_cstring(subfiles[i].path));


        lexer this_lexer = new_lexer(subfiles[i].path, loaded_file);

        dynarr_append(lexer, &lexers, this_lexer);

    }
    if (mars_file_count == 0)
        general_error("input path \"%s\" has no \".mars\" files", clone_to_cstring(mars_flags.input_path));

    // timing
    struct timeval lex_begin, lex_end;
    if (mars_flags.print_timings) gettimeofday(&lex_begin, 0);        
    size_t tokens_lexed = 0;

    FOR_URANGE_EXCL(i, 0, lexers.len) {
        construct_token_buffer(&lexers.raw[i]);
        tokens_lexed += lexers.raw[i].buffer.len;
    }

    if (mars_flags.print_timings) {
        gettimeofday(&lex_end, 0);
        long seconds = lex_end.tv_sec - lex_begin.tv_sec;
        long microseconds = lex_end.tv_usec - lex_begin.tv_usec;
        double elapsed = seconds + microseconds*1e-6;
        printf(style_FG_Cyan style_Bold "LEXING" style_Reset);
        printf("\t  time      : %fs\n", elapsed);
        printf("\t  tokens    : %lu\n", tokens_lexed);
        printf("\t  tok/s     : %.3f\n", tokens_lexed / elapsed);
    }

    dynarr(parser) parsers;
    dynarr_init(parser, &parsers, lexers.len);

    FOR_URANGE_EXCL(i, 0, lexers.len) {
        arena alloca = arena_make(PARSER_ARENA_SIZE);

        parser p = make_parser(&lexers.raw[i], alloca);


        dynarr_append(parser, &parsers, p);
    }

    // timing
    struct timeval parse_begin, parse_end;
    if (mars_flags.print_timings) gettimeofday(&parse_begin, 0);
    size_t ast_nodes_created = 0;

    FOR_URANGE_EXCL(i, 0, parsers.len) {
        parse_file(&parsers.raw[i]);
        ast_nodes_created += parsers.raw[i].num_nodes;
    }

    /* display timing */ 
    if (mars_flags.print_timings) {
        gettimeofday(&parse_end, 0);
        long seconds = parse_end.tv_sec - parse_begin.tv_sec;
        long microseconds = parse_end.tv_usec - parse_begin.tv_usec;
        double elapsed = seconds + microseconds*1e-6;
        printf(style_FG_Blue style_Bold "PARSING" style_Reset);
        printf("\t  time      : %fs\n", elapsed);
        printf("\t  AST nodes : %lu\n", ast_nodes_created);
        printf("\t  nodes/s   : %.3f\n", ast_nodes_created / elapsed);
    }

    // cleanup
    FOR_RANGE_EXCL(i, 0, subfile_count) fs_drop(&subfiles[i]);
    free(subfiles);
    fs_drop(&input_dir);

    return NULL;
}