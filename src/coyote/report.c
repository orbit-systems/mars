#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common/fs.h"
#include "common/str.h"
#include "lex.h"
#include "common/ansi.h"

static u32 line_number(string src, string snippet) {
    u32 line = 1;
    char* current = src.raw;
    while (current != snippet.raw) {
        if (*current == '\n') line++;
        current++;
    }

    return line;
}

static u32 col_number(string src, string snippet) {
    u32 col = 1;
    char* current = src.raw;
    while (current != snippet.raw) {
        if (*current == '\n') col = 0;
        col++;
        current++;
    }

    return col;
}

static void print_snippet(string line, string snippet, const char* color, usize pad, string msg) {
    
    fprintf(stderr, Blue"| "Reset);

    for_n(i, 0, line.len) {
        if (line.raw + i == snippet.raw) {
            fprintf(stderr, Bold "%s", color);
        }
        if (line.raw + i == snippet.raw + snippet.len) {
            fprintf(stderr, Reset);
        }
        fprintf(stderr, "%c", line.raw[i]);
    }
    fprintf(stderr, Reset);
    fprintf(stderr, "\n");
    for_n(i, 0, pad) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, Blue"| "Reset);
    for_n(i, 0, line.len) {
        if (line.raw + i < snippet.raw) {
            fprintf(stderr, " ");
        }
        if (line.raw + i == snippet.raw) {
            fprintf(stderr, Bold "%s^", color);
        }
        if (line.raw + i == snippet.raw + snippet.len) {
            break;
        }
        if (line.raw + i > snippet.raw) {
            fprintf(stderr, "~");
        }
    }
    // fprintf(stderr, Reset"\n");
    fprintf(stderr, Bold " "str_fmt Reset"\n", str_arg(msg));
    // fprintf(stderr, Bold " "str_fmt Reset"\n", str_arg(msg));
}

bool is_whitespace(char c) {
    switch (c) {
    case ' ':
    case '\r':
    case '\n':
    case '\t':
    case '\v':
        return true;
    }
    return false;
}

string snippet_line(string src, string snippet) {
    string line = snippet;
    // expand line backwards
    while (true) {
        if (line.raw == src.raw) break;
        if (*line.raw == '\n') {
            line.raw++;
            break;
        }
        line.raw--;
    }
    // trim leading whitespace
    while (is_whitespace(line.raw[0])) {
        line.raw++;
    }
    // expand line forwards
    while (true) {
        if (line.raw + line.len == src.raw + src.len) break;
        if (line.raw[line.len] == '\n') {
            // line.len--;
            break;
        }
        line.len++;
    }
    return line;
}

string try_localize_path(string path) {
    const char* current_dir = fs_get_current_dir();
    usize curdir_len = strlen(current_dir);

    if (path.len <= curdir_len) {
        return path;
    }

    if (strncmp(current_dir, path.raw, curdir_len) == 0) {
        path.raw += curdir_len;
        path.len -= curdir_len;

        if (path.raw[0] == '/') {
            path.raw++;
            path.len--;
        }
    }

    return path;
}

static usize digits(usize n) {
    usize d = 1;
    while (n >= 10) {
        n /= 10;
        d++;
    }
    return d;
}

void report_line(ReportLine* report) {

    const char* color = White;

    report->path = try_localize_path(report->path);

    switch (report->kind) {
    case REPORT_ERROR: fprintf(stderr, Bold Red"error"Reset); color = Red; break;
    case REPORT_WARNING: fprintf(stderr, Bold Yellow"warning"Reset); color = Yellow; break;
    case REPORT_NOTE: fprintf(stderr, Bold Cyan"note"Reset); color = Cyan; break;
    }

    u32 line_num = line_number(report->src, report->snippet);
    u32 col_num  = col_number(report->src, report->snippet);

    // fprintf(stderr, " -> "str_fmt":%u:%u ", str_arg(report->path), line_num, col_num);
    fprintf(stderr, ": "Bold str_fmt Reset, str_arg(report->msg));
    fprintf(stderr, "\n");
    
    usize line_digits = digits(line_num);

    for_n(i, 0, line_digits) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, Blue"--> "Reset str_fmt":%u:%u\n", str_arg(report->path), line_num, col_num);
    for_n(i, 0, line_digits) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, Blue" |\n"Reset);

    string line = snippet_line(report->src, report->snippet);

    fprintf(stderr, Blue "%u ", line_num);

    if (report->reconstructed_line.raw != nullptr) {
        print_snippet(line, report->snippet, color, line_digits + 1, strlit("in this macro invocation"));
    } else {
        print_snippet(line, report->snippet, color, line_digits + 1, report->msg);
    }

    
    if (report->reconstructed_line.raw != nullptr) {

        string line = snippet_line(report->reconstructed_line, report->reconstructed_snippet);
        
        for_n(i, 0, line_digits) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, Blue"--> "Reset "expands to: "Reset"\n");
        for_n(i, 0, line_digits) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, Blue" |\n"Reset);
        fprintf(stderr, Blue"%u ", line_num);
        print_snippet(line, report->reconstructed_snippet, color, line_digits + 1, report->msg);
    }
    for_n(i, 0, line_digits) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, Blue" |\n"Reset);
}
