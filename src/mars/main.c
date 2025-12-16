#include <stdio.h>

#include "common/util.h"
#include "compiler.h"
#include "lex.h"

#include "reporting.h"

int main() {
    auto marsc = marsc_new();

    auto id = marsc_get_file(marsc, strlit("tests/mars/brackets.mars"));

    auto l = lexer_new(marsc, id);

    while (lexer_next_token(&l)) {
    }

    for_n(i, 0, vec_len(marsc->reports)) {
        Report* r = marsc->reports[i];
        report_render(stdout, r);
    }
}
