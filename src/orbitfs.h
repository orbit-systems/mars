#pragma once
#define ORBITFS_H

// im gonna try to make a filesystem wrapper lib

#include "orbit.h"

#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

typedef u8 ofs_file_type; enum {
    oft_invalid = 0,
    oft_regular,
    oft_directory,
    oft_pipe,
    oft_symlink,
    oft_other,
};

typedef struct ofs_file_s {
    string          path;
    size_t          size;
    FILE*           handle;
    ofs_file_type   type;

    time_t          last_access;
    time_t          last_mod;
    time_t          last_status;

    bool            opened;
} ofs_file;

// check if a file exists at the specified path.
bool ofs_file_exists(string path);

// retrieves information about a file or filesystem entity.
// this may allocate information, like the name of a file, which will be cleaned up and freed by drop_file.
// returns true if the operation was successful.
bool ofs_get_file(string path, ofs_file* file);

// drops information about a file, frees any relevant memory and zeroes the structure. 
// this also closes the file if it is opened.
// returns true if the operation was successful.
bool ofs_drop_file(ofs_file* file);

// creates a file at (path) and writes its information into (*file).
// this may allocate information, like the name of a file, which will be cleaned up and freed by drop_file.
// returns true if the operation was successful.
bool ofs_create_file(string path, ofs_file_type type, ofs_file* file);

// macro for creating a directory
#define ofs_create_dir(path, file) \
        ofs_create_file((path), oft_directory, (file))

// deletes a file from the filesystem. This will not work if the file is opened.
// this will also drop the file (call ofs_drop_file).
// returns true if the operation was successful.
bool ofs_delete_file(ofs_file* file);

// open a file if it is closed. does not perform any other operations
bool ofs_open_file(ofs_file* file, char* mode);

// close a file if it is opened. does not perform any other operations
bool ofs_close_file(ofs_file* file);

// self explanatory
#define ofs_is_regular(file)    ((file)->type == oft_regular)
#define ofs_is_directory(file)  ((file)->type == oft_directory)
#define ofs_is_pipe(file)       ((file)->type == oft_pipe)
#define ofs_is_symlink(file)    ((file)->type == oft_symlink)
#define ofs_is_other(file)      ((file)->type == oft_other)
#define ofs_is_invalid(file)      ((file)->type == oft_invalid)

// sets *count to how many subfiles/subdirectories are in a directory
// returns true if the operation was successful.
size_t ofs_get_subfile_count(ofs_file* file);

// write a file array full of subfiles
bool ofs_get_subfiles(ofs_file* file, ofs_file* file_array);

// attempts to read (file) entirely into memory at (buf)
// returns true if the operation was successful.
bool ofs_read_entire_file(ofs_file* file, void* buf);

// attempt to read (len) bytes from file (file) into (buf).
// this sets the file cursor.
bool ofs_read_from_file(ofs_file* file, void* buf, size_t len);

// sends (len) bytes of data at (buf) to (file).
bool ofs_write_to_file(ofs_file* file, void* buf, size_t len);

// set file cursor to a specific location.
bool ofs_set_cursor(ofs_file* file, size_t new_cursor);

// get the value of the file cursor, relative to how
size_t ofs_get_cursor(ofs_file* file);