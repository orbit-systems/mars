#ifndef MARS_COMPILER_H
#define MARS_COMPILER_H

#include <assert.h>
#include "common/type.h"
#include "common/vec.h"
#include "common/str.h"
#include "common/arena.h"

typedef struct SourceFile {
    string full_path;
    string source;
} SourceFile;

typedef struct SourceFileId {u32 _;} SourceFileId;

typedef struct MarsCompiler {
    Vec(SourceFile) files;
    Arena permanent;
} MarsCompiler;

#endif // MARS_COMPILER_H
