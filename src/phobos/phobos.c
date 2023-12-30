#include "orbit.h"
#include "../mars.h"
#include "phobos.h"
#include "lexer.h"

extern flag_set mars_flags;

program_tree* parse_program() {

    char* input_path_cstr = to_cstring(mars_flags.input_path);
    char* module_name_cstr = to_cstring(mars_flags.module_name);

    /*
    struct stat s;
    int err = stat("/path/to/possible_dir", &s);
    if(-1 == err) {
        if(ENOENT == errno) {
            // does not exist
        } else {
            perror("stat");
            exit(1);
        }
    } else {
        if(S_ISDIR(s.st_mode)) {
            // it's a dir
        } else {
            // exists but is no dir
        }
    }
    */

    free(input_path_cstr);
    free(module_name_cstr);
    

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
        "\n     if mem::equal(str.base, str2.base, str.len); {"
        "\n         neptune::kprint(\"equal\");"
        "\n     } else {"
        "\n         neptune::kprint(\"not equal\");"
        "\n     }"
        "\n };"
    ));

    // append_next_token(&lex);
    // append_next_token(&lex);
    construct_token_buffer(&lex);


    return NULL;
}