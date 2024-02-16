#pragma once
#define ORBIT_H

// orbit systems utility header

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <sys/time.h>
#include <stdatomic.h>
#include <assert.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;
typedef float f32;
typedef double f64;

#if !defined(bool)
typedef uint8_t  bool;
#define false ((bool)0)
#define true ((bool)1)
#endif

#ifdef _MSC_VER
#define forceinline __forceinline
#elif defined(__GNUC__)
#define forceinline inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
#if __has_attribute(__always_inline__)
        #define forceinline inline __attribute__((__always_inline__))
    #else
        #define forceinline inline
    #endif
#else
    #define forceinline inline
#endif

#define TODO(msg) do {\
    printf("\x1b[36m\x1b[1mTODO\x1b[0m: \"%s\" at %s:%d\n", (msg), (__FILE__), (__LINE__)); \
    exit(EXIT_FAILURE); } while (0)

#define CRASH(msg) do { \
    printf("\x1b[31m\x1b[1mCRASH\x1b[0m: \"%s\" at %s:%d\n", (msg), (__FILE__), (__LINE__)); \
    exit(EXIT_FAILURE); } while (0)

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

#define FOR_RANGE_INCL(iterator, start, end) for (intptr_t iterator = (start); iterator <= (end); iterator++)
#define FOR_RANGE(iterator, start, end) for (intptr_t iterator = (start); iterator < (end); iterator++)

#define FOR_URANGE_INCL(iterator, start, end) for (uintptr_t iterator = (start); iterator <= (end); iterator++)
#define FOR_URANGE(iterator, start, end) for (uintptr_t iterator = (start); iterator < (end); iterator++)

#define is_pow_2(i) ((i) != 0 && ((i) & ((i)-1)) == 0)


// sandwich's shitty """polymorphic""" dynamic array lib V2
// lean and mean w/ new functions
// all the functions are now macros, inspired by mista zozin himself (https://github.com/tsoding)

#define da(type) da_##type

#define da_typedef(type) typedef struct da_##type { \
    type * at; \
    size_t len; \
    size_t cap; \
} da_##type

// the array pointer is called 'at' so that you can use it like 'dynarray.at[i]'

#define da_init(da_ptr, capacity) do { \
    size_t c = (size_t)(capacity); \
    if ((capacity) <= 0) c = 1; \
    (da_ptr)->len = 0; \
    (da_ptr)->cap = c; \
    (da_ptr)->at = malloc(sizeof((da_ptr)->at[0]) * c); \
    if ((da_ptr)->at == NULL) { \
        printf("(%s:%d) da_init malloc failed for capacity %zu", (__FILE__), (__LINE__), c); \
        exit(1); \
    } \
} while (0)

#define da_append(da_ptr, element) do { \
    if ((da_ptr)->len == (da_ptr)->cap) { \
        (da_ptr)->cap *= 2; \
        (da_ptr)->at = realloc((da_ptr)->at, sizeof((da_ptr)->at[0]) * (da_ptr)->cap); \
        if ((da_ptr)->at == NULL) { \
            printf("(%s:%d) da_append realloc failed for capacity %zu", (__FILE__), (__LINE__), (da_ptr)->cap); \
            exit(1); \
        } \
    } \
    (da_ptr)->at[(da_ptr)->len++] = (element); \
} while (0)

#define da_shrink(da_ptr) do { \
    if ((da_ptr)->len == (da_ptr)->cap) break; \
    (da_ptr)->cap = (da_ptr)->len; \
    (da_ptr)->at = realloc((da_ptr)->at, sizeof((da_ptr)->at[0]) * (da_ptr)->cap); \
    if ((da_ptr)->at == NULL) { \
        printf("(%s:%d) da_init realloc failed for capacity %zu", (__FILE__), (__LINE__), (da_ptr)->cap); \
        exit(1); \
    } \
} while (0)

#define da_reserve(da_ptr, num_slots) do { \
    (da_ptr)->cap += num_slots; \
    (da_ptr)->at *= realloc((da_ptr)->at, sizeof((da_ptr)->at[0]) * (da_ptr)->cap); \
    if ((da_ptr)->at == NULL) { \
        printf("(%s:%d) da_reserve realloc failed for capacity %zu", (__FILE__), (__LINE__), (da_ptr)->cap); \
        exit(1); \
    } \
} while (0)

#define da_destroy(da_ptr) do { \
    if ((da_ptr)->at == NULL) break; \
    free((da_ptr)->at); \
} while (0)

#define da_insert_at(da_ptr, element, index) do { \
    da_append(da_ptr, element); \
    memmove(&(da_ptr)->at[index + 1], &(da_ptr)->at[index], sizeof((da_ptr)->at[0]) * ((da_ptr)->len-index-1)); \
    (da_ptr)->at[index] = element; \
} while (0)

#define da_remove_at(da_ptr, index) do { \
    memmove(&(da_ptr)->at[index], &(da_ptr)->at[index + 1], sizeof((da_ptr)->at[0]) * ((da_ptr)->len-index-1)); \
    (da_ptr)->len--; \
} while (0)

#define da_unordered_remove_at(da_ptr, index) do { \
    (da_ptr)->at[index] = (da_ptr)->at[(da_ptr)->len - 1]; \
    (da_ptr)->len--; \
} while (0)


// strings and string-related utils.

// #define CSTRING_COMPATIBILITY_MODE
// allocates an extra character for null termination outside of the string bounds.
// probably recommended if you interface a lot with standard C APIs and dont want clone_to_cstring allocations everywhere.

typedef struct string_s {
    char* raw;
    u32   len;
} string;

#define NULL_STR ((string){NULL, 0})
#define is_null_str(str) ((str).raw == NULL)

#define str_fmt "%.*s"
#define str_arg(str) (int)(str).len, (str).raw

#define string_make(ptr, len) ((string){(ptr), (len)})
#define string_len(s) ((s).len)
#define string_raw(s) ((s).raw)
#define is_within(haystack, needle) (((haystack).raw <= (needle).raw) && ((haystack).raw + (haystack).len >= (needle).raw + (needle).len))
#define substring(str, start, end) ((string){(str).raw + (start), (end) - (start)})
#define substring_len(str, start, len) ((string){(str).raw + (start), (len)})
#define can_be_cstring(str) ((str).raw[(str).len] == '\0')
#define to_string(cstring) ((string){(cstring), strlen((cstring))})

char*  clone_to_cstring(string str); // this allocates
void   printstr(string str);
string strprintf(char* format, ...);

string  string_alloc(size_t len);
#define string_free(str) free(str.raw)
string  string_clone(string str); // this allocates as well
string  string_concat(string a, string b); // allocates
void  string_concat_buf(string buf, string a, string b); // this does not

int  string_cmp(string a, string b);
bool string_eq(string a, string b);
bool string_ends_with(string source, string ending);

#ifdef ORBIT_IMPLEMENTATION
string strprintf(char* format, ...) {
    string c = NULL_STR;
    va_list a;
    va_start(a, format);
    va_list b;
    va_copy(b, a);
    size_t bufferlen = 1 + vsnprintf("", 0, format, a);
    c = string_alloc(bufferlen);
    vsnprintf(c.raw, c.len, format, b);
    c.len--;
    va_end(a);
    va_end(b);
    return c;
}

string string_concat(string a, string b) {
    string c = string_alloc(a.len + b.len);
    FOR_RANGE(i, 0, a.len) c.raw[i] = a.raw[i];
    FOR_RANGE(i, 0, b.len) c.raw[a.len + i] = b.raw[i];
    return c;
}

void string_concat_buf(string buf, string a, string b) {
    FOR_RANGE(i, 0, a.len) buf.raw[i] = a.raw[i];
    FOR_RANGE(i, 0, b.len) buf.raw[a.len + i] = b.raw[i];
}

bool string_ends_with(string source, string ending) {
    if (source.len < ending.len) return false;

    return string_eq(substring_len(source, source.len-ending.len, ending.len), ending);
}

string string_alloc(size_t len) {
    #ifdef CSTRING_COMPATIBILITY_MODE
    char* raw = malloc(len + 1);
    #else
    char* raw = malloc(len);
    #endif

    if (raw == NULL) return NULL_STR;

    memset(raw, ' ', len);

    #ifdef CSTRING_COMPATIBILITY_MODE
    raw[len] = '\0';
    #endif

    return (string){raw, len};

}

int string_cmp(string a, string b) {
    // copied from odin's implementation lmfao
    int res = memcmp(a.raw, b.raw, min(a.len, b.len));
    if (res == 0 && a.len != b.len) return a.len <= b.len ? -1 : 1;
	else if (a.len == 0 && b.len == 0) return 0;
	return res;
}

bool string_eq(string a, string b) {
    if (a.len != b.len) return false;
    FOR_RANGE(i, 0, a.len) {
        if (a.raw[i] != b.raw[i]) return false;
    }
    return true;
}

char* clone_to_cstring(string str) {
    if (is_null_str(str)) return "";

    char* cstr = malloc(str.len + 1);
    if (cstr == NULL) return NULL;
    memcpy(cstr, str.raw, str.len);
    cstr[str.len] = '\0';
    return cstr;
}

string string_clone(string str) {
    string new_str = string_alloc(str.len);
    if (memmove(new_str.raw, str.raw, str.len) != new_str.raw) return NULL_STR;
    return new_str;
}

void printn(char* text, size_t len) {
    size_t c = 0;
    while (text[c] != '\0' && c < len)
        putchar(text[c++]);
}

void printstr(string str) {
    printn(str.raw, str.len);
}
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

    *file = (fs_file){0};

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
    struct dirent* dir_entry;

    if (can_be_cstring(file->path)) {
        directory = opendir(file->path.raw);
    } else {
        char* path_cstr = clone_to_cstring(file->path);
        directory = opendir(path_cstr);
        free(path_cstr);
    }
    if (!directory) return false;
#endif
    if (can_be_cstring(file->path)) {
        chdir(file->path.raw);
    } else {
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
//        string path = to_string(find_data.cFileName);
        char* pathcstr = malloc(PATH_MAX);
        realpath(find_data.cFileName, pathcstr);
        string path = to_string(pathcstr);
        bool success = fs_get(path, &file_array[i]);
        if (!success) {
            chdir(file_realpath);
            return false;
        }
        i++;
    } while(FindNextFile(find, &find_data));
#else
    for (int i = 0; (dir_entry = readdir(directory)) != NULL;) {
        if (strcmp(dir_entry->d_name, ".") == 0) continue;
        if (strcmp(dir_entry->d_name, "..") == 0) continue;
        //string temp1 = string_concat(file->path, to_string("/"));
        //string path = string_concat(temp1, to_string(dir_entry->d_name));
        //printf("\ny\n[%s]\n\n", dir_entry->d_name);
        char* pathcstr = malloc(PATH_MAX);
        realpath(dir_entry->d_name, pathcstr);
        string path = to_string(pathcstr);
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

