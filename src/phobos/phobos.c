#include "orbit.h"
#include "../mars.h"
#include "../error.h"
#include "../dynarr.h"

#include "phobos.h"
#include "lexer.h"
extern flag_set mars_flags;

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
    dynarr_init_lexer(&lexers, subfile_count);

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
        construct_token_buffer(&this_lexer);

        dynarr_append(lexer, &lexers, this_lexer);

    }
    if (mars_file_count == 0)
        general_error("input path \"%s\" has no \".mars\" files", clone_to_cstring(mars_flags.input_path));

    FOR_RANGE_EXCL(i, 0, subfile_count) fs_drop(&subfiles[i]);
    free(subfiles);
    fs_drop(&input_dir);

    return NULL;
}

dynarr_lib(lexer)