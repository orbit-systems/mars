#pragma once
#define ORBITFS_H

// filesystem utils
// god this thing is a mess but it works ig

#include "orbit.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

typedef u8 fs_file_type; enum {
    oft_invalid = 0,
    oft_regular,
    oft_directory,
    oft_pipe,
    oft_symlink,
    oft_other,
};

typedef struct fs_file_s {
    string          path;
    size_t          size;
    FILE*           handle;
    fs_file_type    type;
    bool            opened;
} fs_file;

// check if a file exists at the specified path.
bool fs_exists(string path);

// retrieves information about a file or filesystem entity.
// this may allocate information, like the name of a file, which will be cleaned up and freed by drop_file.
// returns true if the operation was successful.
bool fs_get(string path, fs_file* file);

// drops information about a file, frees any relevant memory and zeroes the structure. 
// this also closes the file if it is opened.
// returns true if the operation was successful.
bool fs_drop(fs_file* file);

// creates a file at (path) and writes its information into (*file).
// this may allocate information, like the name of a file, which will be cleaned up and freed by drop_file.
// returns true if the operation was successful.
bool fs_create(string path, fs_file_type type, fs_file* file);

// macro for creating a directory
#define fs_create_dir(path, file) \
        fs_create((path), oft_directory, (file))

// deletes a file from the filesystem. This will not work if the file is opened.
// this will also drop the file (call fs_drop).
// returns true if the operation was successful.
bool fs_delete(fs_file* file);

// open a file if it is closed. does not perform any other operations
bool fs_open(fs_file* file, char* mode);

// close a file if it is opened. does not perform any other operations
bool fs_close(fs_file* file);

// self explanatory
#define fs_is_regular(file)    ((file)->type == oft_regular)
#define fs_is_directory(file)  ((file)->type == oft_directory)
#define fs_is_pipe(file)       ((file)->type == oft_pipe)
#define fs_is_symlink(file)    ((file)->type == oft_symlink)
#define fs_is_other(file)      ((file)->type == oft_other)
#define fs_is_invalid(file)      ((file)->type == oft_invalid)

// sets *count to how many subfiles/subdirectories are in a directory
// returns true if the operation was successful.
int fs_subfile_count(fs_file* file);

// write a file array full of subfiles
bool fs_get_subfiles(fs_file* file, fs_file* file_array);

// attempts to read (file) entirely into memory at (buf)
// returns true if the operation was successful.
bool fs_read_entire(fs_file* file, void* buf);

// attempt to read (len) bytes from file (file) into (buf).
// this sets the file cursor.
bool fs_read(fs_file* file, void* buf, size_t len);

// sends (len) bytes of data at (buf) to (file).
bool fs_write(fs_file* file, void* buf, size_t len);

// set file cursor to a specific location.
bool fs_set_cursor(fs_file* file, int new_cursor);

// get the value of the file cursor, relative to how
int fs_get_cursor(fs_file* file);



// use #define ORBITFS_IMPLEMENTATION to generate the actual code

#ifdef ORBITFS_IMPLEMENTATION

bool fs_exists(string path) {

    struct stat statbuffer;
    bool exists;

    if (can_be_cstring(path)) {
        exists = stat(path.raw, &statbuffer) == 0;
    } else {
        char* path_cstr = clone_to_cstring(path);
        exists = stat(path_cstr, &statbuffer) == 0;
        free(path_cstr);
    }
    return exists;
}

bool fs_get(string path, fs_file* file) {

    struct stat statbuffer;

    {
        bool exists;
        if (can_be_cstring(path)) {
            exists = stat(path.raw, &statbuffer) == 0;
        } else {
            char* path_cstr = clone_to_cstring(path);
            exists = stat(path_cstr, &statbuffer) == 0;
            free(path_cstr);
        }
        if (!exists) return false;
    }

    file->path = string_clone(path);
    file->size = statbuffer.st_size;

#ifdef S_ISREG
    if      (S_ISREG(statbuffer.st_mode))  file->type = oft_regular;
#endif
#ifdef S_ISDIR
    else if (S_ISDIR(statbuffer.st_mode))  file->type = oft_directory;
#endif
#ifdef S_ISLNK
    else if (S_ISLNK(statbuffer.st_mode))  file->type = oft_symlink;
#endif
#ifdef S_ISFIFO
    else if (S_ISFIFO(statbuffer.st_mode)) file->type = oft_pipe;
#endif
    else                                   file->type = oft_other;

    file->handle = NULL;
    file->opened = false;
    
    return true;
}

bool fs_create(string path, fs_file_type type, fs_file* file) {
    if (fs_exists(path)) return false;

    bool creation_success = false;
    FILE* handle = NULL;

    switch (type) {
    case oft_directory:
        if (can_be_cstring(path)) {
            creation_success = mkdir(path.raw
            #if !(defined(MINGW32) || defined(__MINGW32__))
                , S_IRWXU | S_IRWXG | S_IRWXO
            #endif
            ) == 0;
        } else {
            char* path_cstr = clone_to_cstring(path);
            creation_success = mkdir(path_cstr
            #if !(defined(MINGW32) || defined(__MINGW32__))
                , S_IRWXU | S_IRWXG | S_IRWXO
            #endif
            ) == 0;
            free(path_cstr);
        }
        if (!creation_success) return false;
        break;
    case oft_regular:
        if (can_be_cstring(path)) {
            handle = fopen(path.raw, "w");
        } else {
            char* path_cstr = clone_to_cstring(path);
            handle = fopen(path_cstr, "w");
            free(path_cstr);
        }
        if (handle == NULL) return false;
        break;

    case oft_symlink: CRASH("orbitfs does not currently support creating symlinks\n");
    case oft_pipe:    CRASH("orbitfs does not currently support creating pipes\n");
    default:
        return false;
        break;
    }
    
    if (handle != NULL) fclose(handle);

    return fs_get(path, file);
}

bool fs_drop(fs_file* file) {

    if (file->opened) {
        bool closed = fs_close(file);
        if (!closed) return false;
    }

    string_free(file->path);

    *file = (fs_file){};

    file->handle = NULL;

    return true;
}

bool fs_open(fs_file* file, char* mode) {
    if (file->opened) return false;


    FILE* handle = NULL;
    if (can_be_cstring(file->path)) {
        handle = fopen(file->path.raw, mode);
    } else {
        char* path_cstr = clone_to_cstring(file->path);
        handle = fopen(path_cstr, mode);
        free(path_cstr);
    }

    if (handle == NULL) return false;

    file->handle = handle;

    file->opened = true;

    return true;
}

bool fs_close(fs_file* file) {
    if (!file->opened) return false;

    bool closed = fclose(file->handle) == 0;
    if (!closed) return false;

    file->handle = NULL;
    file->opened = false;
    return true;
}

bool fs_delete(fs_file* file) {
    if (file->opened) return false;

    bool success;

    switch (file->type) {
    case oft_directory:
        if (can_be_cstring(file->path)) {
            success = rmdir(file->path.raw) == 0;
        } else {
            char* path_cstr = clone_to_cstring(file->path);
            success = rmdir(path_cstr) == 0;
            free(path_cstr);
        }
        break;
    case oft_regular:
        if (can_be_cstring(file->path)) {
            success = remove(file->path.raw) == 0;
        } else {
            char* path_cstr = clone_to_cstring(file->path);
            success = remove(path_cstr) == 0;
            free(path_cstr);
        }
        break;

    default:
        return false;
        break;
    }

    return success && fs_drop(file);
}

int fs_subfile_count(fs_file* file) {
    int count = 0;
    if (!fs_is_directory(file)) return 0;
    
    DIR* d;
    struct dirent* dir;
    
    if (can_be_cstring(file->path)) {
        d = opendir(file->path.raw);
    } else {
        char* path_cstr = clone_to_cstring(file->path);
        d = opendir(path_cstr);
        free(path_cstr);
    }
    if (!d) return false;

    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0) continue;
        if (strcmp(dir->d_name, "..") == 0) continue;
        count++;
    }

    closedir(d);
    return count; // account for the . and .. dirs
}

bool fs_get_subfiles(fs_file* file, fs_file* file_array) {

    if (!fs_is_directory(file)) return false;

    DIR* directory;
    struct dirent* dir_entry;
    
    if (can_be_cstring(file->path)) {
        directory = opendir(file->path.raw);
    } else {
        char* path_cstr = clone_to_cstring(file->path);
        directory = opendir(path_cstr);
        free(path_cstr);
    }
    if (!directory) return false;

    if (can_be_cstring(file->path)) {
        chdir(file->path.raw);
    } else {
        char* path_cstr = clone_to_cstring(file->path);
        chdir(path_cstr);
        free(path_cstr);
    }

    for (int i = 0; (dir_entry = readdir(directory)) != NULL;) {
        if (strcmp(dir_entry->d_name, ".") == 0) continue;
        if (strcmp(dir_entry->d_name, "..") == 0) continue;
        //string temp1 = string_concat(file->path, to_string("/"));
        //string path = string_concat(temp1, to_string(dir_entry->d_name));
        //printf("\ny\n[%s]\n\n", dir_entry->d_name);
        string path = to_string(dir_entry->d_name);
        bool success = fs_get(path, &file_array[i]);
        if (!success) {
            return false;
        }
        //string_free(temp1);
        // string_free(path);
        i++;
    }

    closedir(directory);
    return true;
}

bool fs_set_cursor(fs_file* file, int new_cursor) {
    if (!file->opened) return false;
    return fseek(file->handle, new_cursor, SEEK_SET) == 0;
}

int fs_get_cursor(fs_file* file) {
    if (!file->opened) return 0;
    return ftell(file->handle);
}

bool fs_read_entire(fs_file* file, void* buf) {
    if (!file->opened) return false;

    size_t size_read = fread(buf, 1, file->size, file->handle);
    return (size_read == file->size);
}

bool fs_read(fs_file* file, void* buf, size_t len) {
    if (!file->opened) return false;

    size_t size_read = fread(buf, 1, len, file->handle);
    return (size_read == len);
}

bool fs_write(fs_file* file, void* buf, size_t len) {
    if (!file->opened) return false;

    size_t size_written = fwrite(buf, 1, len, file->handle);
    
    return (size_written == len);
}

/*
int fs_self_test() {

    printf("orbitfs test\n");
    {
        printf("\tfs_exists\n");
        char* path;
        bool  exists;
        bool  passed = true;
        
        path = "Makefile";
        exists = fs_exists(to_string(path));
        printf("\t\t%s exists: %s\n", path, exists ? "true" : "false");
        passed = passed && exists;

        path = "mars_code";
        exists = fs_exists(to_string(path));
        printf("\t\t%s exists: %s\n", path, exists ? "true" : "false");
        passed = passed && exists;

        path = "mars_code/../mars_code/test.mars";
        exists = fs_exists(to_string(path));
        printf("\t\t%s exists: %s\n", path, exists ? "true" : "false");
        passed = passed && exists;

        path = "does/not/exist";
        exists = fs_exists(to_string(path));
        printf("\t\t%s exists: %s\n", path, exists ? "true" : "false");
        passed = passed && !exists;

        printf(passed ? "PASSED\n" : "FAILED\n");
    }

    {
        printf("\tfs_get\n");
        char* path;
        bool got;
        fs_file f;
        bool  passed = true;

        path = "Makefile";
        got = fs_get(to_string(path), &f);
        printf("\t\tgot %s, is a regular file: %s\n", path, f.type == oft_regular ? "true" : "false");
        passed = passed && got && f.type == oft_regular;
        fs_drop(&f);

        path = "mars_code";
        got = fs_get(to_string(path), &f);
        printf("\t\tgot %s, is a directory: %s\n", path, f.type == oft_directory ? "true" : "false");
        passed = passed && got && f.type == oft_directory;
        fs_drop(&f);

        printf(passed ? "PASSED\n" : "FAILED\n");
    }
    
    {
        
        printf("\tfs_create\n");
        char* path;
        bool got;
        fs_file f;
        bool passed = true;

        path = "random_file.txt";
        got = fs_create(to_string(path), oft_regular, &f);
        printf("\t\tcreated %s as a regular file, does exist: %s\n", path, f.type == oft_regular ? "true" : "false");
        passed = passed && got && f.type == oft_regular;
        fs_drop(&f);

        path = "random_dir";
        got = fs_create(to_string(path), oft_directory, &f);
        printf("\t\tcreated %s as a directory, does exist: %s\n", path, (got && f.type == oft_directory) ? "true" : "false");
        passed = passed && got && f.type == oft_directory;
        fs_drop(&f);

        printf(passed ? "PASSED\n" : "FAILED\n");
        
    }

    {
        printf("\tfs_write\n");
        char* path;
        bool got;
        fs_file f;
        bool passed = true;
        string sample_text = to_string("ok this is for testing! lets hope this works");
        string sample_read = string_alloc(sample_text.len);

        path = "random_file.txt";
        got = fs_get(to_string(path), &f);
        printf("\t\tgot %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;

        bool opened = fs_open(&f, "a");
        printf("\t\topened %s? %s", path, opened ? "yes\n" : "no\n");
        passed = passed && opened;

        bool wrote = fs_write(&f, sample_text.raw, sample_text.len);
        printf("\t\twrote to %s? %s", path, wrote ? "yes\n" : "no\n");
        passed = passed && wrote;

        fs_drop(&f);
        printf(passed ? "PASSED\n" : "FAILED\n");



        printf("\tfs_read\n");
        passed = true;

        got = fs_get(to_string(path), &f);
        printf("\t\tgot %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;

        opened = fs_open(&f, "r");
        printf("\t\topened %s? %s", path, opened ? "yes\n" : "no\n");
        passed = passed && opened;

        bool read = fs_read(&f, sample_read.raw, sample_read.len);
        printf("\t\tread from %s? %s", path, read ? "yes\n" : "no\n");
        passed = passed && read;

        bool equal = string_eq(sample_text, sample_read);
        printf("\t\tread equal? %s", equal ? "yes\n" : "no\n");
        passed = passed && equal;

        fs_drop(&f);
        printf(passed ? "PASSED\n" : "FAILED\n");
    }

    {
        printf("\tfs_delete\n");
        char* path;
        bool got;
        fs_file f;
        bool passed = true;

        path = "random_file.txt";
        got = fs_get(to_string(path), &f);
        printf("\t\tgot %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;
        got = fs_delete(&f);
        printf("\t\tdeleted %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;
        fs_drop(&f);

        path = "random_dir";
        got = fs_get(to_string(path),  &f);
        printf("\t\tgot %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;
        got = fs_delete(&f);
        printf("\t\tdeleted %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;
        fs_drop(&f);

        printf(passed ? "PASSED\n" : "FAILED\n");
    }

    {
        printf("\tfs_subfiles\n");
        char* path;
        bool got;
        fs_file f;
        bool passed = true;

        path = ".";
        fs_get(to_string(path), &f);
        int count = fs_subfile_count(&f);
        passed = passed && count != 0;
        printf("\t\t%s has %d subfiles\n", path, count);
        
        fs_file* subfile_array = malloc(sizeof(fs_file) * count);
        passed = passed && fs_get_subfiles(&f, subfile_array);
        FOR_RANGE_EXCL(i, 0, count) {
            printf("\t\t- %s\n", clone_to_cstring(subfile_array[i].path));
            fs_drop(&subfile_array[i]);
        }
        free(subfile_array);
        fs_drop(&f);

        printf(passed ? "PASSED\n" : "FAILED\n");
    }

}
*/
#endif