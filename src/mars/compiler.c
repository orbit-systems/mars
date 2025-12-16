#include "compiler.h"
#include "reporting.h"

#include "common/fs.h"
#include "common/str.h"
#include "common/vec.h"

#include <stdio.h>


MarsCompiler* marsc_new() {
    MarsCompiler* c = malloc(sizeof(*c));

    arena_init(&c->permanent);

    const char* working_dir = fs_get_current_dir();
    c->working_dir = arena_strdup(&c->permanent, string_wrap(working_dir));
    c->files = vec_new(SourceFile, 16);
    c->reports = vec_new(Report*, 16);

    return c;
}

SourceFileId marsc_get_file(MarsCompiler* marsc, string path) {
    char buf_c[path.len + 1];
    memcpy(buf_c, path.raw, path.len);
    buf_c[path.len] = '\0';
    FsFile* file = fs_open(buf_c, false, false);

    if (file == nullptr) {
        printf("could not open file '%s'\n", buf_c);
        exit(0);
    }

    SourceFile srcfile = {
        .source = fs_read_entire(file),
        .path = arena_strdup(&marsc->permanent, (string){
            .len = file->path.len,
            .raw = file->path.raw,
        })
    };

    vec_append(&marsc->files, srcfile);

    return (SourceFileId){vec_len(marsc->files) - 1};
}
