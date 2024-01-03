#include "orbit.h"
#include "../mars.h"
#include "../error.h"

#include "phobos.h"
#include "lexer.h"

extern flag_set mars_flags;

program_tree* phobos_perform_frontend() {

    if (!fs_exists(mars_flags.input_path)) {
        general_error("input directory \"%s\" does not exist", to_cstring(mars_flags.input_path));
    }

    fs_file input_dir = {0};
    fs_get(mars_flags.input_path, &input_dir);
    if (!fs_is_directory(&input_dir)) {
        general_error("input path \"%s\" is not a directory", to_cstring(mars_flags.input_path));
    }

    size_t subfile_count = fs_subfile_count(&input_dir);
    if (subfile_count == 0) {
        general_error("input path \"%s\" has no files", to_cstring(mars_flags.input_path));
    }

    fs_file* subfiles = malloc(sizeof(fs_file) * subfile_count);
    fs_get_subfiles(&input_dir, subfiles);

    FOR_RANGE_EXCL(i, 0, subfile_count) {

        // filter out non-files and non-mars files.
        if (!fs_is_regular(&subfiles[i])) continue;
        if (!string_ends_with(subfiles[i].path, to_string(".mars"))) continue;



    }

    FOR_RANGE_EXCL(i, 0, subfile_count) fs_drop(&subfiles[i]);
    free(subfiles);
    fs_drop(&input_dir);

    lexer_state lex = new_lexer(to_string("path/lmao"), to_string(
        "\n module test;"
        "\n "
        "\n import mem \"./memory\";"
        "\n import neptune \"./neptune\";"
        "\n "
        "\n let main = fn #section(\"text.boot\") #link_name(\"__main\") () {"
        "\n     "
        "\n     let str = \"bruh\";"
        "\n     let str2 = []u8{0x62, 0x72, 0x75, 0x68};"
        "\n "
        "\n     if mem::equal(str.base, str2.base, str.len) {"
        "\n         neptune::kprint(\"equal\");"
        "\n     } else {"
        "\n         neptune::kprint(\"not equal\");"
        "\n     }"
        "\n };"
    ));

    construct_token_buffer(&lex);
    FOR_RANGE_EXCL(i, 0, lex.buffer.len) {
        printstr(lex.buffer.base[i].text);
        printf(" ");
    }

    return NULL;
}