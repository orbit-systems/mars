#include <stdio.h>

#include "common/fs.h"
#include "common/util.h"

#include "coyote.h"
#include "lex.h"
#include "parse.h"
#include "irgen.h"

#include "iron/iron.h"

thread_local const char* filepath = nullptr;
thread_local FlagSet flags = {};

static void print_help() {
    puts("coyote [path/file.jkl] [options]");
    puts(" --help              Display this info.");
    puts(" --version           Display version information.");
    puts(" --xrsdk             Warn on code that would not compile with the");
    puts("                       original XR/SDK compiler.");
    puts(" --error-on-warn     Turn warnings into errors.");
    puts(" --preproc,          Only perform the preprocessor. This strips");
    puts("                       all hygenic macro scope information and may");
    puts("                       not produce re-compilable code.");
    puts(" --incdir=/path/     Add a directory to search for INCLUDE");
    puts("                       directives with '<inc>/'");
    puts(" --libdir=/path/     Add a directory to search for INCLUDE");
    puts("                       directives with '<ll>/'");
    puts(" --arch=...          Specify the target architecture:");
    puts("                       x86-64/x64");
    puts("                       aarch64/arm64");
    puts("                       xr17032");
    puts("                       fox32");
    puts(" --system=...        Specify the target system:");
    puts("                       freestanding/none");
    puts("                       linux");
    puts("                       windows");
    puts(" --passes            Emit pass information to stdout.");
}

static void print_version() {
    printf("Coyote v%d.%d\n", COYOTE_MAJOR, COYOTE_MINOR);
    printf("Iron   v%d.%d\n", FE_VERSION_MAJOR, FE_VERSION_MINOR);
}

static void parse_args(int argc, char** argv) {
    if (argc == 1) {
        print_help();
        exit(0);
    }
    // filepath = argv[1];
    for_n(i, 1, argc) {
        char* arg = argv[i];
        if (strcmp(arg, "--help") == 0) {
            print_help();
            exit(0);
        } else if (strcmp(arg, "--version") == 0) {
            print_version();
            exit(0);
        } else if (strcmp(arg, "--xrsdk") == 0) {
            flags.xrsdk = true;
        } else if (strcmp(arg, "--preproc") == 0) {
            flags.preproc = true;
        } else if (strcmp(arg, "--passes") == 0) {
            flags.passes = true;
        } else if (strcmp(arg, "--error-on-warn") == 0) {
            flags.error_on_warn = true;
        } else if (arg[0] == '-') {
            printf("unknown flag '%s'\n", arg);
            exit(1);
        } else {
            filepath = arg;
        }
    }
}

int main(int argc, char** argv) {

    parse_args(argc, argv);

    FsFile* file = fs_open(filepath, false, false);
    if (file == nullptr) {
        printf("cannot open file %s\n", filepath);
        return 1;
    }

    SrcFile f = {
        .src = fs_read_entire(file),
        .path = fs_strref_from_path(&file->path),
    };

    LexState lex_state;
    Arena arena;
    arena_init(&arena);

    lex_state.incdirs = vec_new(string, 16);
    lex_state.libdirs = vec_new(string, 16);
    lex_state.tokens = vec_new(Token, 512);
    lex_state.arena = &arena;
    lex_state.current_file = arena_strdup(&arena, fs_strref_from_path(&file->path));
    strmap_init(&lex_state.included_files, 128);

    Parser p = lex_entrypoint(&f, &lex_state);
    p.flags = flags;

    // p.flags.xrsdk = true;
    // p.flags.error_on_warn = true;

    if (flags.preproc) {
        for_n(i, 0, p.tokens_len) {
            Token* t = &p.tokens[i];
            if (TOK__PARSE_IGNORE < t->kind) {
                if (t->kind == TOK_NEWLINE) {
                    printf("\n");
                    // continue;
                }
                // printf("%s ", token_kind[t->kind]);
                continue;
            }
            if (t->kind == TOK_STRING) {
                printf("\"");
            }
            printf(str_fmt, str_arg(tok_span(*t)));
            if (t->kind == TOK_STRING) {
                printf("\"");
            }
            printf(" ");
        }
        printf("\n");
        return 0;
    }

    CompilationUnit cu = parse_unit(&p);

    FeModule* m = irgen(&cu);

    FeDataBuffer db;
    fe_db_init(&db, 512);
    if (flags.passes) {
        for_funcs(func, m) {
            fe_emit_ir_func(&db, func, true);
        }
        printf("%.*s", (int)db.len, db.at);
    }

    db.len = 0;
    for_funcs(func, m) {
        fe_opt_local(func);
    }
    if (flags.passes) {
        for_funcs(func, m) {
            fe_emit_ir_func(&db, func, true);
        }
        printf("%.*s", (int)db.len, db.at);
    }
    
    
    db.len = 0;for_funcs(func, m) {
        fe_codegen(func);
    }
    if (flags.passes) {
        for_funcs(func, m) {
            fe_emit_ir_func(&db, func, true);
        }
        printf("%.*s", (int)db.len, db.at);
    }
    db.len = 0;

    fe_codegen_print_text(&db, m);
    if (flags.passes) {
        printf("%.*s", (int)db.len, db.at);
    }


    string out_path = arena_strcat(&arena, lex_state.current_file, strlit(".s"), true);

    FsFile* outfile = fs_open(out_path.raw, true, true);

    fs_write(outfile, db.at, db.len);

    fs_close(outfile);
    fs_destroy(outfile);
}
