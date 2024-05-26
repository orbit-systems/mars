#include "orbit.h"
#include "lexer.h"

#define skip_whitespace(c, i) while (lex_src.raw[i] == ' ' && i < lex_src.len) {i++; c = lex_src.raw[i];}

#define skip_until(char, i) while(lex_src.raw[i] != (char) && i < lex_src.len) {i++;}

da(icarus_token) lltd_lex(string path);
u8 str_to_type(string str);

IR_Module* lltd_parse_ir(string path) {
    da(icarus_token) tokens = lltd_lex(path);

    IR_Module* mod = ir_new_module(str("test"));

    foreach (icarus_token token, tokens) {
        if (string_eq(token.tok, str("define"))) {
            //we got a global! scan for the func name, we assume i32 typing for now
            if (string_eq(tokens.at[count + 1].tok, str("dso_local"))) {
                //we're probably in a function! this is horribly hardcoded.
                //type is up next
                u8 type = str_to_type(tokens.at[count + 2].tok);
                
            }
        }
    }
}

u8 str_to_type(string str) {
    if (string_eq(str, str("i32"))) return 5;
    general_error("Unknown string type: "str_fmt, str_arg(str));
}

da(icarus_token) lltd_lex(string path) {
    if (!fs_exists(path)) general_error("File '" str_fmt "' does not exist!", str_arg(path));
    fs_file ll_file = {0};
    fs_get(path, &ll_file);
    if (fs_is_directory(&ll_file)) general_error("'" str_fmt "' is a directory, not a file!", str_arg(path));
    fs_open(&ll_file, "r");
    string lex_src = string_alloc(ll_file.size);
    fs_read(&ll_file, lex_src.raw, ll_file.size);
    //printf(str_fmt, str_arg(lex_src));

    da(string) src_lines;
    da_init(&src_lines, 1);

    int anchor = 0;
    for (int i = 0; i < lex_src.len; i++) {
        if (lex_src.raw[i] == '\n') {
            if (lex_src.raw[anchor] != ';' && (i - anchor) != 0) { 
                da_append(&src_lines, string_clone((string){(lex_src.raw + anchor), (i - anchor)}));
            }
            anchor = i + 1;
            continue;
        }
    }

    da(icarus_token) tokens;
    da_init(&tokens, 1);

    foreach (string line, src_lines) {
        int anchor = 0;  
        for (int i = 0; i < (line.len + 1); i++) {
            if (line.raw[0] == ' ') {
                //we need to start trimming trailing space, and restart the loop
                for (int j = 0; j < line.len; j++) {
                    if (line.raw[0] == ' ') {
                        line.raw++;
                        line.len--;
                    }
                    i = 0;
                    continue;
                }
            }

            if (line.raw[i] == '!') {
                icarus_token tok = (icarus_token){str("!"), count};
                da_append(&tokens, tok);
                anchor = i + 1;
                continue;
            }

            if (line.raw[i] == '{') {
                icarus_token tok = (icarus_token){str("{"), count};
                da_append(&tokens, tok);
                anchor = i + 1;
                continue;
            }

            if (line.raw[i] == ' ' || 
                line.raw[i] == '(' ||
                line.raw[i] == ')' ||
                line.raw[i] == '}' ||
                i == line.len) {
                if ((i - anchor) == 0) continue;
                icarus_token tok = (icarus_token){string_clone((string){(line.raw + anchor), (i - anchor)}), count};
                da_append(&tokens, tok);

                if (line.raw[i] == '(') {
                    tok = (icarus_token){str("("), count};
                    da_append(&tokens, tok);
                }

                if (line.raw[i] == ')') {
                    tok = (icarus_token){str(")"), count};
                    da_append(&tokens, tok);
                }

                if (line.raw[i] == '}') {
                    tok = (icarus_token){str("}"), count};
                    da_append(&tokens, tok);
                }

                anchor = i + 1;
                continue;
            }
        }
    }

    foreach (icarus_token token, tokens) {
        printf("line: %03d, '"str_fmt"'\n", token.line, str_arg(token.tok));
    }

    return tokens;
}