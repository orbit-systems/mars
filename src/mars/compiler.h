#ifndef MARS_COMPILER_H
#define MARS_COMPILER_H

#include <assert.h>
#include "common/type.h"
#include "common/vec.h"
#include "common/str.h"
#include "common/arena.h"

#if __STDC_HOSTED__ && (defined(__x86_64__) || defined(__aarch64__))
    /// Pointers are sign-extended from 48-bits.
    #define HOST_POINTER_48_BITS
#endif

/// An opaque handle used to refer to a source file with a MarsCompiler.
typedef struct SourceFileId {u32 _;} SourceFileId;

/// A source file the compiler is analyzing.
typedef struct SourceFile {
    /// The absolute path to the source file.
    string path;
    /// The full source text.
    string source;
} SourceFile;

/// An instance of the Mars compiler.
typedef struct MarsCompiler {
    /// Full path of the current working directory.
    string working_dir;

    /// All the source files being processed by the compiler at the moment.
    Vec(SourceFile) files;

    /// All reports that accumulate throughout the compilation process.
    Vec(struct Report*) reports;

    /// Storage for data that persists across the entire compilation.
    /// Examples include persistent strings, file paths, etc.
    Arena permanent;
} MarsCompiler;

/// Create a new mars compiler instance.
MarsCompiler* marsc_new();

// Load a file into MarsCompiler `marsc`.
SourceFileId marsc_get_file(MarsCompiler* marsc, string path);

#endif // MARS_COMPILER_H
