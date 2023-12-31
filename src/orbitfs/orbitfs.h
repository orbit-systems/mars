#pragma once
#define ORBITFS_H

// im gonna try to make a filesystem wrapper lib

#include "../orbit.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

typedef u8 ofs_file_type; enum {
    ofs_regular,
    ofs_directory,
    ofs_chardev,
    ofs_blockdev,
    ofs_pipe,
    ofs_symlink,
    ofs_socket,
};

typedef struct ofs_file_s {
    string        filename;
    int           size;
    ofs_file_type type;
    FILE*         handle;
} ofs_file;

bool open_file(string path, ofs_file* file);
bool close_file(string path);