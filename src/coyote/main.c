#include <stdio.h>

#include "coyote.h"

#include "common/util.h"
#include "lex.h"
#include "parse.h"

#include "iron/iron.h"

thread_local const char* filepath = nullptr;
thread_local FlagSet flags = {};

static void print_help() {
    puts("coyote path/file.jkl [options]");
    puts(" --help              Display this info.");
    puts(" --version           Display version and copyright information.");
    puts(" --xrsdk             Warn on code that would not compile with the");
    puts("                     original XR/SDK compiler.");
    puts(" --error-on-warn     Turn warnings into errors.");
    puts(" --preproc,          Only perform the preprocessor. This strips");
    puts("                     all hygenic macro scope information and may");
    puts("                     not produce re-compilable code.");
    puts(" --incdir=/path/     Add a directory to search for INCLUDE");
    puts("                     directives with '<inc>/'");
    puts(" --libdir=/path/     Add a directory to search for INCLUDE");
    puts("                     directives with '<ll>/'");
    puts(" --arch=...          Specify the target architecture:");
    puts("                      xr17032");
    puts("                      fox32");
    puts("                      x86_64/x64");
    puts("                      aarch64");
    puts(" --system=...        Specify the target system:");
    puts("                      freestanding/none");
    puts("                      linux");
    puts("                      windows");
}

static void print_version() {
    printf("Coyote v%d.%d using Iron v%d.%d\n", COYOTE_MAJOR, COYOTE_MINOR, FE_VERSION_MAJOR, FE_VERSION_MINOR);
}

static void parse_args(int argc, char** argv) {
    if (argc == 1) {
        print_help();
        exit(0);
    }
    filepath = argv[1];
    for_n(i, 2, argc) {
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
        } else if (strcmp(arg, "--error-on-warn") == 0) {
            flags.error_on_warn = true;
        } else {
            printf("unknown flag '%s'\n", arg);
            exit(1);
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
        .path = fs_from_path(&file->path),
    };

    Parser p = lex_entrypoint(&f);
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
}
