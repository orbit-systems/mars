#pragma once
#define ORBITFS_H

// im gonna try to make a filesystem wrapper lib

#include "../orbit.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

typedef u8 ofs_file_type; enum {
    oft_regular,
    oft_directory,
    oft_chardev,
    oft_blockdev,
    oft_pipe,
    oft_symlink,
    oft_socket,
};

typedef struct ofs_file_s {
    string          filename;
    int             size;
    FILE*           handle;
    ofs_file_type   type;

    bool            opened;
} ofs_file;


// check if a file exists at the specified path.
bool file_exists(string path);

// retrieves information about a file or filesystem entity.
// this also opens the file if it can be opened.
// this may allocate information, like the name of a file, which will be cleaned up and freed by drop_file.
// returns true if the operation was successful.
bool get_file(string path, ofs_file* file);

// creates a file at (path) and writes its information into (*file).
bool create_file(string path, ofs_file* file);

// drops information about a file, frees any relevant memory and zeroes the structure. 
// this also closes the file if it is opened.
// returns true if the operation was successful.
bool drop_file(ofs_file* file);

// self explanatory
bool is_regular(ofs_file* file);
bool is_directory(ofs_file* file);
bool is_chardev(ofs_file* file);
bool is_blockdev(ofs_file* file);
bool is_pipe(ofs_file* file);
bool is_symlink(ofs_file* file);
bool is_socket(ofs_file* file);

// sets *count to how many subfiles/subdirectories are in a directory
// returns true if the operation was successful.
bool get_subfile_count(ofs_file* dir, int* count);

// write a file array full of 
bool get_subfiles(ofs_file* file_array);

// attempts to read (file) entirely into memory at (buf)
// returns true if the operation was successful.
bool load_entire_file(ofs_file* file, void* buf);

// attempt to read (len) bytes at location (at) from file (file) into (buf).
// this should NOT set the file cursor.
bool read_from_file_at(ofs_file* file, void* buf, size_t at, size_t len);

// attempt to read (len) bytes from file (file) into (buf).
// this sets the file cursor.
bool read_from_file(ofs_file* file, void* buf, size_t len);