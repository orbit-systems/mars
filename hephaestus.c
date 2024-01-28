// to build the project, just compile and run this file!

// hephaestus by sandwichman - single-file, slim build tool for multi-file C projects

//////////////////////////////////////////////////////////////////////////////

char* project_name  = "mars";

char* cc            = "gcc";
char* flags         = "-O3";
char* source_dirs[] = {
        "src/",
        "src/phobos/",
        "src/deimos/",
};
char* include_dir   = "src";
char* build_dir     = "build";
char* output_dir    = "./";
char* link_flags    = "-lm";
int transparency_mode = 0;

//////////////////////////////////////////////////////////////////////////////

// TODO read dependency files for separate compilation

// this is basically my orbit.h header but slimmed down and copied in
// because single-file means single-file

// here be dragons

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

typedef uint32_t u32;
typedef uint8_t  u8;
typedef uint8_t  bool;
#define false ((bool)0)
#define true ((bool)!false)

#define CRASH(msg) do { \
    printf("\x1b[31m\x1b[1mCRASH\x1b[0m: \"%s\" at %s:%d\n", (msg), (__FILE__), (__LINE__)); \
    exit(EXIT_FAILURE); } while (0)

#define FOR_RANGE(iterator, start, end) for (intptr_t iterator = (start); iterator < (end); iterator++)

// #define CSTRING_COMPATIBILITY_MODE
// ^ allocates an extra character for null termination outside the string bounds.
// probably recommended if you interface a lot with standard C APIs and dont want clone_to_cstring allocations everywhere.

typedef struct string_s {
    char* raw;
    u32   len;
} string;

#define NULL_STR ((string){NULL, 0})
#define is_null_str(str) ((str).raw == NULL)

#define str_fmt "%.*s"
#define str_arg(str) (int)(str).len, (str).raw

#define substring_len(str, start, len) ((string){(str).raw + (start), (len)})
#define can_be_cstring(str) ((str).raw[(str).len] == '\0')
#define to_string(cstring) ((string){(cstring), strlen((cstring))})

char*  clone_to_cstring(string str); // this allocates

string  string_alloc(size_t len);
#define string_free(str) free(str.raw)
string  string_clone(string str); // this allocates as well

bool string_eq(string a, string b);
bool string_ends_with(string source, string ending);

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

bool fs_exists(string path);
bool fs_get(string path, fs_file* file);
bool fs_drop(fs_file* file);

#ifdef _WIN32
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif

bool fs_close(fs_file* file);

#define fs_is_regular(file)    ((file)->type == oft_regular)
#define fs_is_directory(file)  ((file)->type == oft_directory)

int fs_subfile_count(fs_file* file);

bool fs_get_subfiles(fs_file* file, fs_file* file_array);

#define MAX_CMD 1000
char cmd[MAX_CMD] = {0};
char objects[MAX_CMD] = {0};

void clear(char* buf) {
    while(*buf != '\0') *(buf++) = '\0';
}

// realpath()s of the source directories that i need because filesystem shit is so dumb
char* real_src_dirs[sizeof(source_dirs) / sizeof(*source_dirs)];
char* real_build_dir;
char* real_include_dir;
char* real_output_dir;

#define error(msg, ...) do { \
    printf("error: ");       \
    printf(msg __VA_OPT__(,) __VA_ARGS__); \
    printf("\n");            \
    exit(EXIT_FAILURE);      \
} while (0)

int execute(char* command) {
    if (transparency_mode) printf("%s\n", command);
    return system(command);
}

int main() {

    // translate relative paths into realpaths (im so fucking done)
    real_output_dir = malloc(PATH_MAX);
    if (output_dir[0] == '\0') output_dir = "./";
    realpath(output_dir, real_output_dir);

    real_build_dir = malloc(PATH_MAX);
    if (build_dir[0] == '\0') {
        error("an explicit build path must be provided");
    }
    realpath(build_dir, real_build_dir);

    real_include_dir = malloc(PATH_MAX);
    if (include_dir[0] == '\0') real_include_dir = "";
    else realpath(include_dir, real_include_dir);

    // clean build directory
    if (fs_exists(to_string(build_dir))) {
        strcat(cmd, "rm -rf ");
        strcat(cmd, build_dir);
        execute(cmd);
        clear(cmd);
    }

    // create build directory
    {
        strcat(cmd, "mkdir ");
        strcat(cmd, build_dir);
        execute(cmd);
        clear(cmd);
    }

    FOR_RANGE(i, 0, sizeof(source_dirs) / sizeof(*source_dirs)) {
        fs_file source_directory;
        if (!fs_get(to_string(source_dirs[i]), &source_directory))
            error("could not open source directory '%s'", source_dirs[i]);

        if (!fs_is_directory(&source_directory))
            error("source directory '%s' is not a directory", source_dirs[i]);

        real_src_dirs[i] = malloc(PATH_MAX);
        realpath(source_dirs[i], real_src_dirs[i]);
//        printf("\t%s\n", real_src_dirs[i]);
        fs_drop(&source_directory);
    }

    if (!transparency_mode) printf("compiling using %s with flags %s\n", cc, flags);


    // count the files
    int total_files_to_build = 0;
    FOR_RANGE(i, 0, sizeof(source_dirs) / sizeof(*source_dirs)) {
        fs_file source_directory;
        if (!fs_get(to_string(real_src_dirs[i]), &source_directory))
            error("could not open source directory '%s'", source_dirs[i]);

        int src_dir_subfile_count = fs_subfile_count(&source_directory);
        if (src_dir_subfile_count == 0) {
            fs_drop(&source_directory);
            continue;
        }

        fs_file* src_dir_subfiles = malloc(sizeof(fs_file) * src_dir_subfile_count);
        fs_get_subfiles(&source_directory, src_dir_subfiles);
        FOR_RANGE(j, 0, src_dir_subfile_count) {
            if (!fs_is_regular(&src_dir_subfiles[j])) continue;
            if (string_ends_with(src_dir_subfiles[j].path, to_string(".c"))) {
                total_files_to_build++;
            }
        }

        FOR_RANGE(j, 0, src_dir_subfile_count) fs_drop(&src_dir_subfiles[j]);
        free(src_dir_subfiles);

        fs_drop(&source_directory);
    }

    // build the individual object files
    int file_num = 1;
    FOR_RANGE(i, 0, sizeof(source_dirs) / sizeof(*source_dirs)) {
        fs_file source_directory;
        if (!fs_get(to_string(real_src_dirs[i]), &source_directory))
            error("could not open source directory '%s'", source_dirs[i]);

        int src_dir_subfile_count = fs_subfile_count(&source_directory);
        if (src_dir_subfile_count == 0) {
            fs_drop(&source_directory);
            continue;
        }

        fs_file* src_dir_subfiles = malloc(sizeof(fs_file) * src_dir_subfile_count);
        fs_get_subfiles(&source_directory, src_dir_subfiles);
        FOR_RANGE(j, 0, src_dir_subfile_count) {
            if (!fs_is_regular(&src_dir_subfiles[j])) continue;
            if (string_ends_with(src_dir_subfiles[j].path, to_string(".c"))) {
                // build file!
                char* slashchar;
                if (real_src_dirs[i][strlen(real_src_dirs[i])-1] == '\\' ||
                    real_src_dirs[i][strlen(real_src_dirs[i])-1] == '/') {
                    slashchar = "";
                } else {
#if defined(_WIN32) || defined(_WIN64)
                    slashchar = "\\";
#else
                    slashchar = "/";
#endif
                }
                printf("[ %d / %d ] ", file_num++, total_files_to_build);
                if (!transparency_mode) printf("compiling %s%s"str_fmt"\n", real_src_dirs[i], slashchar, str_arg(src_dir_subfiles[j].path));

                string outpath = string_clone(src_dir_subfiles[j].path);
                outpath.raw[outpath.len-1] = 'o';
                sprintf(cmd, "%s -c -o %s/"str_fmt" %s/"str_fmt" %s",
                        cc, real_build_dir, str_arg(outpath), real_src_dirs[i], str_arg(src_dir_subfiles[j].path), flags);
                if (real_include_dir[0] != '\0') {
                    strcat(cmd, " -I");
                    strcat(cmd, real_include_dir);
                }

                // record object file name for link stage
                sprintf(&objects[strlen(objects)], "%s/"str_fmt" ", real_build_dir, str_arg(outpath));

                if (execute(cmd)) {
                    error("building "str_fmt" failed", str_arg(src_dir_subfiles[j].path));
                }
                clear(cmd);
            }
        }

        FOR_RANGE(j, 0, src_dir_subfile_count) fs_drop(&src_dir_subfiles[j]);
        free(src_dir_subfiles);

        fs_drop(&source_directory);
    }

    if (!transparency_mode) printf("linking using %s with flags %s\n", cc, link_flags);

    char output_name[PATH_MAX] = {0};
    sprintf(output_name, "%s/%s",  real_output_dir, project_name);

    if (fs_exists(to_string(output_name))) {
        strcat(cmd, "rm ");
        strcat(cmd, output_name);
        execute(cmd);
        clear(cmd);
    }

    sprintf(cmd, "%s %s -o %s %s",
            cc, objects, output_name, link_flags
    );
    if (system(cmd)) {
        error("linking failed");
    }
    clear(cmd);

    if (!transparency_mode) printf("%s built successfully!\n", project_name);
    exit(EXIT_SUCCESS);
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


bool fs_close(fs_file* file) {
    if (!file->opened) return false;

    bool closed = fclose(file->handle) == 0;
    if (!closed) return false;

    file->handle = NULL;
    file->opened = false;
    return true;
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

    char file_realpath[PATH_MAX] = {0};
    realpath(clone_to_cstring(file->path), file_realpath);

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
            chdir(file_realpath);
            return false;
        }
        //string_free(temp1);
        // string_free(path);
        i++;
    }

    chdir(file_realpath);
    chdir("..");

    closedir(directory);
    return true;
}