#include <stdio.h>

#include "compiler.h"
#include "lex.h"

#include "reporting.h"

int main() {
    SourceFile src = {
        .full_path = strlit("foo.mars"),
        .source = strlit(
            "let y = if x == 0 {\n"
            "    \"hello there\"\n"
            "} else {\n"
            "    100\n"
            "};\n"
        ),
    };
    SourceFileId src_id = {0};
    SourceFile src2 = {
        .full_path = strlit("fart.mars"),
        .source = strlit(
            "let y = if x == 0 {\n"
            "    \"hiiii there\"\n"
            "} else {\n"
            "    124\n"
            "};\n"
        ),
    };
    SourceFileId src2_id = {1};
    auto files = vec_new(SourceFile, 1);
    vec_append(&files, src);
    vec_append(&files, src2);

    auto report = report_new(
        REPORT_ERROR, 
        strlit("`else` value has incompatible type"), 
        files
    );

    report_add_label(report,
        REPORT_LABEL_PRIMARY,
        src_id,
        50, 53,
        strlit("expected `[:0]const u8`, got `isize`")
    );
    
    report_add_label(report,
        REPORT_LABEL_SECONDARY,
        src_id,
        8, 38,
        strlit("this is of type `?[:0]const u8`")
    );

    report_add_note(report, strlit(
        "`else` follows type rule `?T else T` and returns `T`"
    ));

    report_render(stdout, report);
}
