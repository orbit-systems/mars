#ifndef MARS_COMPILER_H
#define MARS_COMPILER_H

#include <assert.h>
#include "common/type.h"
#include "common/vec.h"
#include "common/str.h"
#include "common/arena.h"

// A source file the compiler is analyzing.
typedef struct SourceFile {
    string full_path;
    string source;
} SourceFile;

// A handle used to refer to a source file.
typedef struct SourceFileId {u32 _;} SourceFileId;

// An instance of the Mars compiler.
typedef struct MarsCompiler {

    // all the source files being processed by the compiler at the moment.
    Vec(SourceFile) files;

    // data that persists across the entire compilation.
    // not the only arena (more to be added).
    Arena permanent;
} MarsCompiler;

#endif // MARS_COMPILER_H
