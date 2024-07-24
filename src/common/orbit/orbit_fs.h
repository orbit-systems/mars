#pragma once
#define ORBIT_FS_H

#if !defined(PATH_MAX)
#   define PATH_MAX 4096
#endif

// filesystem utils
// god this thing is a mess but it works ig

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#ifdef _WIN32
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>

#define __S_ISTYPE(mode, mask)  (((mode) & S_IFMT) == (mask))
#undef S_ISREG
#undef S_ISDIR
#define S_ISREG(mode)    __S_ISTYPE((mode), S_IFREG)
#define S_ISDIR(mode)    __S_ISTYPE((mode), S_IFDIR)

#define fs_mkdir _mkdir
#define chdir _chdir
#define PATH_MAX 260
#else
#include <dirent.h>
#include <unistd.h>

#define fs_mkdir mkdir
#endif

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

#ifdef _WIN32
#include <stdlib.h>
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif


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

// self-explanatory
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


#ifdef ORBIT_IMPLEMENTATION

bool fs_exists(string path) {

    struct stat statbuffer;
    bool exists;

    char* path_cstr = clone_to_cstring(path);
    exists = stat(path_cstr, &statbuffer) == 0;
    free(path_cstr);
    return exists;
}

bool fs_get(string path, fs_file* file) {

    struct stat statbuffer;

    {
        bool exists;
        char* path_cstr = clone_to_cstring(path);
        exists = stat(path_cstr, &statbuffer) == 0;
        free(path_cstr);
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
    case oft_directory: {
        char* path_cstr = clone_to_cstring(path);
        creation_success = mkdir(path_cstr
        #if !(defined(MINGW32) || defined(__MINGW32__))
            , S_IRWXU | S_IRWXG | S_IRWXO
        #endif
        ) == 0;
        free(path_cstr);
        if (!creation_success) return false;
        } break;
    case oft_regular: {
            char* path_cstr = clone_to_cstring(path);
            handle = fopen(path_cstr, "w");
            free(path_cstr);
            if (handle == NULL) return false;
        } break;

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

    *file = (fs_file){0};

    file->handle = NULL;

    return true;
}

bool fs_open(fs_file* file, char* mode) {
    if (file->opened) return false;


    FILE* handle = NULL;
    char* path_cstr = clone_to_cstring(file->path);
    handle = fopen(path_cstr, mode);
    free(path_cstr);

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
    case oft_directory: {
            char* path_cstr = clone_to_cstring(file->path);
            success = rmdir(path_cstr) == 0;
            free(path_cstr);
        } break;
    case oft_regular: {
        char* path_cstr = clone_to_cstring(file->path);
        success = remove(path_cstr) == 0;
        free(path_cstr);
        } break;
    default:
        return false;
        break;
    }

    return success && fs_drop(file);
}

int fs_subfile_count(fs_file* file) {
    int count = 0;
    if (!fs_is_directory(file)) return 0;

#ifdef _WIN32
    HANDLE find = NULL;
    WIN32_FIND_DATA find_data = {0};

    char* path_cstr = clone_to_cstring(file->path);
    char path[MAX_PATH] = {0};
    snprintf(path, MAX_PATH, "%s\\*", path_cstr);
    find = FindFirstFile(path, &find_data);
    free(path_cstr);

    if(find != INVALID_HANDLE_VALUE){
        do {
            if (strcmp(find_data.cFileName, ".") == 0) continue;
            if (strcmp(find_data.cFileName, "..") == 0) continue;
            count++;
        } while(FindNextFile(find, &find_data));
        FindClose(find);
    }
#else
    DIR* d;
    struct dirent* dir;

    char* path_cstr = clone_to_cstring(file->path);
    d = opendir(path_cstr);
    free(path_cstr);

    if (!d) return false;

    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0) continue;
        if (strcmp(dir->d_name, "..") == 0) continue;
        count++;
    }

    closedir(d);
#endif
    return count; // account for default directories
}

bool fs_get_subfiles(fs_file* file, fs_file* file_array) {

    char file_realpath[PATH_MAX] = {0};
    realpath(clone_to_cstring(file->path), file_realpath);

    if (!fs_is_directory(file)) return false;

#ifdef _WIN32
    HANDLE find = NULL;
    WIN32_FIND_DATA find_data = {0};

    char* path_cstr = clone_to_cstring(file->path);
    char path[MAX_PATH] = {0};
    snprintf(path, MAX_PATH, "%s\\*", path_cstr);
    find = FindFirstFile(path, &find_data);
    free(path_cstr);

    if(find == INVALID_HANDLE_VALUE) return false;
#else
    DIR* directory;
    struct dirent* dair_entry;
    {
        char* path_cstr = clone_to_cstring(file->path);
        directory = opendir(path_cstr);
        free(path_cstr);
    }
    if (!directory) return false;
#endif
    {
        char* path_cstr = clone_to_cstring(file->path);
        chdir(path_cstr);
        free(path_cstr);
    }
#ifdef _WIN32
    int i = 0;
    do
    {
        if (strcmp(find_data.cFileName, ".") == 0) continue;
        if (strcmp(find_data.cFileName, "..") == 0) continue;
//        string path = str(find_data.cFileName);
        char* pathcstr = malloc(PATH_MAX);
        realpath(find_data.cFileName, pathcstr);
        string path = str(pathcstr);
        bool success = fs_get(path, &file_array[i]);
        if (!success) {
            chdir(file_realpath);
            return false;
        }
        i++;
    } while(FindNextFile(find, &find_data));
#else
    for (int i = 0; (dair_entry = readdir(directory)) != NULL;) {
        if (strcmp(dair_entry->d_name, ".") == 0) continue;
        if (strcmp(dair_entry->d_name, "..") == 0) continue;
        //string temp1 = string_concat(file->path, str("/"));
        //string path = string_concat(temp1, str(dair_entry->d_name));
        //printf("\ny\n[%s]\n\n", dair_entry->d_name);
        char* pathcstr = malloc(PATH_MAX);
        realpath(dair_entry->d_name, pathcstr);
        string path = str(pathcstr);
        bool success = fs_get(path, &file_array[i]);
        if (!success) {
            chdir(file_realpath);
            return false;
        }
        //string_free(temp1);
        // string_free(path);
        i++;
    }
#endif

    chdir(file_realpath);
    chdir("..");

#ifdef _WIN32
    FindClose(find);
#else
    closedir(directory);
#endif
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
#endif