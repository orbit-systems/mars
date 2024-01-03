#include "orbit.h"
#include "../mars.h"
#include "../error.h"
#include "../dynarr.h"

#include "phobos.h"
#include "lexer.h"

dynarr_lib(lexer_state);

extern flag_set mars_flags;

program_tree* phobos_perform_frontend() {

    if (!fs_exists(mars_flags.input_path))
        general_error("input directory \"%s\" does not exist", to_cstring(mars_flags.input_path));

    fs_file input_dir = {0};
    fs_get(mars_flags.input_path, &input_dir);
    if (!fs_is_directory(&input_dir))
        general_error("input path \"%s\" is not a directory", to_cstring(mars_flags.input_path));

    int subfile_count = fs_subfile_count(&input_dir);
    if (subfile_count == 0)
        general_error("input path \"%s\" has no files", to_cstring(mars_flags.input_path));

    fs_file* subfiles = malloc(sizeof(fs_file) * subfile_count);
    fs_get_subfiles(&input_dir, subfiles);


    int mars_file_count = 0;
    FOR_RANGE_EXCL(i, 0, subfile_count) {

        // filter out non-files and non-mars files.
        if (!fs_is_regular(&subfiles[i])) continue;
        if (!string_ends_with(subfiles[i].path, to_string(".mars"))) continue;
        mars_file_count++;

        string loaded_file = string_alloc(subfiles[i].size);

        fs_open(&subfiles[i], "r");
        bool read_success = fs_read_entire(&subfiles[i], loaded_file.raw);
        fs_close(&subfiles[i]);
        if (!read_success)
            general_error("cannot load file \"%s\" (it might be opened already?)", to_cstring(subfiles[i].path));


        lexer_state lex = new_lexer(subfiles[i].path, loaded_file);
        construct_token_buffer(&lex);
        printf("%d tokens\n", lex.buffer.len);

    }
    if (mars_file_count == 0)
        general_error("input path \"%s\" has no \".mars\" files", to_cstring(mars_flags.input_path));

    FOR_RANGE_EXCL(i, 0, subfile_count) fs_drop(&subfiles[i]);
    free(subfiles);
    fs_drop(&input_dir);

    return NULL;
}