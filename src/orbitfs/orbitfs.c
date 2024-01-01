#include "../orbit.h"
#include "orbitfs.h"

// NOT GOOD BUT OK FOR TESTING
#include "../string.c"

bool ofs_file_exists(string path) {

    struct stat statbuffer;
    bool exists;

    if (can_be_cstring(path)) {
        exists = stat(path.raw, &statbuffer) == 0;
    } else {
        char* path_cstr = to_cstring(path);
        exists = stat(path_cstr, &statbuffer) == 0;
        free(path_cstr);
    }
    return exists;
}

bool ofs_get_file(string path, ofs_file* file) {

    struct stat statbuffer;

    {
        bool exists;
        if (can_be_cstring(path)) {
            exists = stat(path.raw, &statbuffer) == 0;
        } else {
            char* path_cstr = to_cstring(path);
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

    file->last_access = statbuffer.st_atime;
    file->last_status = statbuffer.st_ctime;
    file->last_mod    = statbuffer.st_mtime;

    file->handle = NULL;
    file->opened = false;
    
    return true;
}

bool ofs_create_file(string path, ofs_file_type type, ofs_file* file) {
    if (ofs_file_exists(path)) return false;

    bool creation_success = false;
    FILE* handle = NULL;

    switch (type) {
    case oft_directory:
        if (can_be_cstring(path)) {
            creation_success = mkdir(path.raw
            #if (!(defined(_WIN64) || defined(_WIN32)))
                , S_IRWXU | S_IRWXG | S_IRWXO
            #endif
            ) == 0;
        } else {
            char* path_cstr = to_cstring(path);
            creation_success = mkdir(path_cstr
            #if (!(defined(_WIN64) || defined(_WIN32)))
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
            char* path_cstr = to_cstring(path);
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

    return ofs_get_file(path, file);
}

bool ofs_drop_file(ofs_file* file) {

    if (file->opened) {
        bool closed = ofs_close_file(file);
        if (!closed) return false;
    }

    string_free(file->path);

    *file = (ofs_file){};

    file->handle = NULL;

    return true;
}

bool ofs_open_file(ofs_file* file, char* mode) {
    if (file->opened) return false;

    FILE* handle = NULL;
    if (can_be_cstring(file->path)) {
        handle = fopen(file->path.raw, mode);
    } else {
        char* path_cstr = to_cstring(file->path);
        printf("((%s))", path_cstr);
        handle = fopen(path_cstr, mode);
        free(path_cstr);
    }


    if (handle == NULL) return false;

    file->handle = handle;

    file->opened = true;

    return true;
}

bool ofs_close_file(ofs_file* file) {
    if (!file->opened) return false;

    bool closed = fclose(file->handle) == 0;
    if (!closed) return false;

    file->handle = NULL;
    file->opened = false;
    return true;
}

bool ofs_delete_file(ofs_file* file) {
    if (file->opened) return false;

    bool success;

    switch (file->type) {
    case oft_directory:
        if (can_be_cstring(file->path)) {
            success = rmdir(file->path.raw) == 0;
        } else {
            char* path_cstr = to_cstring(file->path);
            success = rmdir(path_cstr) == 0;
            free(path_cstr);
        }
        break;
    case oft_regular:
        if (can_be_cstring(file->path)) {
            success = remove(file->path.raw) == 0;
        } else {
            char* path_cstr = to_cstring(file->path);
            success = remove(path_cstr) == 0;
            free(path_cstr);
        }
        break;

    default:
        return false;
        break;
    }

    return success && ofs_drop_file(file);
}

size_t ofs_get_subfile_count(ofs_file* file) {
    int count = 0;
    if (!ofs_is_directory(file)) return 0;
    
    DIR* d;
    struct dirent* dir;
    
    if (can_be_cstring(file->path)) {
        d = opendir(file->path.raw);
    } else {
        char* path_cstr = to_cstring(file->path);
        d = opendir(path_cstr);
        free(path_cstr);
    }
    if (!d) return false;

    while ((dir = readdir(d)) != NULL) count++;
    closedir(d);
    count -= 2; // account for the . and .. dirs
    return count;
}

bool ofs_get_subfiles(ofs_file* file, ofs_file* file_array) {

    if (!ofs_is_directory(file)) return false;

    DIR* d;
    struct dirent* dir;
    
    if (can_be_cstring(file->path)) {
        d = opendir(file->path.raw);
    } else {
        char* path_cstr = to_cstring(file->path);
        d = opendir(path_cstr);
        free(path_cstr);
    }
    if (!d) return false;

    if (can_be_cstring(file->path)) {
        chdir(file->path.raw);
    } else {
        char* path_cstr = to_cstring(file->path);
        chdir(path_cstr);
        free(path_cstr);
    }

    dir = readdir(d); // skip .
    dir = readdir(d); // skip ..

    for (int i = 0; (dir = readdir(d)) != NULL; i++) {
        bool success = ofs_get_file(to_string(dir->d_name), &file_array[i]);
        if (!success) {
            return false;
        }
    }

    closedir(d);
    return true;
}

bool ofs_set_cursor(ofs_file* file, size_t new_cursor) {
    if (!file->opened) return false;
    return fseek(file->handle, new_cursor, SEEK_SET) == 0;
}

size_t ofs_get_cursor(ofs_file* file) {
    if (!file->opened) return 0;
    return ftell(file->handle);
}

bool ofs_read_entire_file(ofs_file* file, void* buf) {
    if (!file->opened) return false;

    size_t size_read = fread(buf, 1, file->size, file->handle);
    return (size_read == file->size);
}

bool ofs_read_from_file(ofs_file* file, void* buf, size_t len) {
    if (!file->opened) return false;

    size_t size_read = fread(buf, 1, len, file->handle);
    return (size_read == len);
}

bool ofs_write_to_file(ofs_file* file, void* buf, size_t len) {
    if (!file->opened) return false;

    size_t size_written = fwrite(buf, 1, len, file->handle);
    
    return (size_written == len);
}

#ifdef ORBIT_FS_RUN_TEST
int test() {

    printf("orbitfs\n");
    {
        printf("\tofs_file_exists\n");
        char* path;
        bool  exists;
        bool  passed = true;
        
        path = "Makefile";
        exists = ofs_file_exists(to_string(path));
        printf("\t\t%s exists: %s\n", path, exists ? "true" : "false");
        passed = passed && exists;

        path = "mars_code";
        exists = ofs_file_exists(to_string(path));
        printf("\t\t%s exists: %s\n", path, exists ? "true" : "false");
        passed = passed && exists;

        path = "mars_code/../mars_code/test.mars";
        exists = ofs_file_exists(to_string(path));
        printf("\t\t%s exists: %s\n", path, exists ? "true" : "false");
        passed = passed && exists;

        path = "does/not/exist";
        exists = ofs_file_exists(to_string(path));
        printf("\t\t%s exists: %s\n", path, exists ? "true" : "false");
        passed = passed && !exists;

        printf(passed ? "PASSED\n" : "FAILED\n");
    }

    {
        printf("\tofs_get_file\n");
        char* path;
        bool got;
        ofs_file f;
        bool  passed = true;

        path = "Makefile";
        got = ofs_get_file(to_string(path), &f);
        printf("\t\tgot %s, is a regular file: %s\n", path, f.type == oft_regular ? "true" : "false");
        passed = passed && got && f.type == oft_regular;
        ofs_drop_file(&f);

        path = "mars_code";
        got = ofs_get_file(to_string(path), &f);
        printf("\t\tgot %s, is a directory: %s\n", path, f.type == oft_directory ? "true" : "false");
        passed = passed && got && f.type == oft_directory;
        ofs_drop_file(&f);

        printf(passed ? "PASSED\n" : "FAILED\n");
    }
    
    {
        
        printf("\tofs_create_file\n");
        char* path;
        bool got;
        ofs_file f;
        bool passed = true;

        path = "random_file.txt";
        got = ofs_create_file(to_string(path), oft_regular, &f);
        printf("\t\tcreated %s as a regular file, does exist: %s\n", path, f.type == oft_regular ? "true" : "false");
        passed = passed && got && f.type == oft_regular;
        ofs_drop_file(&f);

        path = "random_dir";
        got = ofs_create_file(to_string(path), oft_directory, &f);
        printf("\t\tcreated %s as a directory, does exist: %s\n", path, (got && f.type == oft_directory) ? "true" : "false");
        passed = passed && got && f.type == oft_directory;
        ofs_drop_file(&f);

        printf(passed ? "PASSED\n" : "FAILED\n");
        
    }

    {
        printf("\tofs_write_to_file\n");
        char* path;
        bool got;
        ofs_file f;
        bool passed = true;
        string sample_text = to_string("ok this is for testing! lets hope this works");
        string sample_read = string_alloc(sample_text.len);

        path = "random_file.txt";
        got = ofs_get_file(to_string(path), &f);
        printf("\t\tgot %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;

        bool opened = ofs_open_file(&f, "a");
        printf("\t\topened %s? %s", path, opened ? "yes\n" : "no\n");
        passed = passed && opened;

        bool wrote = ofs_write_to_file(&f, sample_text.raw, sample_text.len);
        printf("\t\twrote to %s? %s", path, wrote ? "yes\n" : "no\n");
        passed = passed && wrote;

        ofs_drop_file(&f);
        printf(passed ? "PASSED\n" : "FAILED\n");



        printf("\tofs_read_from_file\n");
        passed = true;

        got = ofs_get_file(to_string(path), &f);
        printf("\t\tgot %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;

        opened = ofs_open_file(&f, "r");
        printf("\t\topened %s? %s", path, opened ? "yes\n" : "no\n");
        passed = passed && opened;

        bool read = ofs_read_from_file(&f, sample_read.raw, sample_read.len);
        printf("\t\tread from %s? %s", path, read ? "yes\n" : "no\n");
        passed = passed && read;

        bool equal = string_eq(sample_text, sample_read);
        printf("\t\tread equal? %s", equal ? "yes\n" : "no\n");
        passed = passed && equal;

        ofs_drop_file(&f);
        printf(passed ? "PASSED\n" : "FAILED\n");
    }

    {
        printf("\tofs_delete_file\n");
        char* path;
        bool got;
        ofs_file f;
        bool passed = true;

        path = "random_file.txt";
        got = ofs_get_file(to_string(path), &f);
        printf("\t\tgot %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;
        got = ofs_delete_file(&f);
        printf("\t\tdeleted %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;
        ofs_drop_file(&f);

        path = "random_dir";
        got = ofs_get_file(to_string(path),  &f);
        printf("\t\tgot %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;
        got = ofs_delete_file(&f);
        printf("\t\tdeleted %s? %s", path, got ? "yes\n" : "no\n");
        passed = passed && got;
        ofs_drop_file(&f);

        printf(passed ? "PASSED\n" : "FAILED\n");
    }

    {
        printf("\tofs_subfiles\n");
        char* path;
        bool got;
        ofs_file f;
        bool passed = true;

        path = ".";
        ofs_get_file(to_string(path), &f);
        int count = ofs_get_subfile_count(&f);
        passed = passed && count != 0;
        printf("\t\t%s has %d subfiles\n", path, count);
        
        ofs_file* subfile_array = malloc(sizeof(ofs_file) * count);
        passed = passed && ofs_get_subfiles(&f, subfile_array);
        FOR_RANGE_EXCL(i, 0, count) {
            printf("\t\t- %s\n", to_cstring(subfile_array[i].path));
            ofs_drop_file(&subfile_array[i]);
        }
        free(subfile_array);
        ofs_drop_file(&f);

        printf(passed ? "PASSED\n" : "FAILED\n");
    }

}
#endif