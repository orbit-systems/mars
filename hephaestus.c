#define ORBIT_IMPLEMENTATION
#include "src/orbit.h"

// simple C build tool inspired by tsoding's nobuild

// to build the project, just compile and run this program!

char* project_name = "mars";

char* cc           = "gcc"; // swap this out with the compiler of your choice
char* flags        = "-O3 -Isrc/";
char* build_dir    = "build";
char* output_dir   = "";
char* link_flags   = "-lm";
char* source_directories[] = {
    "src",
    "src/phobos",
    "src/deimos",
};

// here be dragons















// seriously, im not proud of this code
// dont @ me if you find bad shit in here

#define MAX_CMD 1000
char cmd[MAX_CMD] = {0};

void clear(char* buf) {
    while(*buf != '\0') *(buf++) = '\0';
}

// realpath()s of the source directories that i need because filesystem shit is so dumb
char* real_src_dirs[sizeof(source_directories) / sizeof(*source_directories)];
char base_dir[PATH_MAX] = {0}; // this is the current working directory at the start of execution

#define error(msg, ...) do { \
    printf("error: ");       \
    printf(msg __VA_OPT__(,) __VA_ARGS__); \
    printf("\n");            \
    exit(EXIT_FAILURE);      \
} while (0)

int main() {

    // clean build directory
    if (fs_exists(to_string(build_dir))) {
        strcat(cmd, "rm -rf ");
        strcat(cmd, build_dir);
        system(cmd);
        clear(cmd);
    }

    // create build directory
    {
        strcat(cmd, "mkdir ");
        strcat(cmd, build_dir);
        system(cmd);
        clear(cmd);
    }

    // translate relative paths into realpaths (im so fucking done)
    FOR_RANGE(i, 0, sizeof(source_directories) / sizeof(*source_directories)) {
        fs_file source_directory;
        if (!fs_get(to_string(source_directories[i]), &source_directory))
            error("could not open source directory '%s'", source_directories[i]);

        if (!fs_is_directory(&source_directory))
            error("source directory '%s' is not a directory", source_directories[i]);

        real_src_dirs[i] = malloc(PATH_MAX);
        realpath(source_directories[i], real_src_dirs[i]);
        printf("\t%s\n", real_src_dirs[i]);
        fs_drop(&source_directory);
    }

    printf("building %s using %s with flags %s\n", project_name, cc, flags);

    // build the individual object files
    FOR_RANGE(i, 0, sizeof(source_directories) / sizeof(*source_directories)) {
        fs_file source_directory;
        if (!fs_get(to_string(real_src_dirs[i]), &source_directory))
            error("could not open source directory '%s'", source_directories[i]);

        int src_dir_subfile_count = fs_subfile_count(&source_directory);
        if (src_dir_subfile_count == 0) {
            fs_drop(&source_directory);
            continue;
        }

        fs_file* src_dir_subfiles = malloc(sizeof(fs_file) * src_dir_subfile_count);
        fs_get_subfiles(&source_directory, src_dir_subfiles);
        FOR_RANGE(j, 0, src_dir_subfile_count) {
            if (string_ends_with(src_dir_subfiles[j].path, to_string(".c"))) {
                // build file!
                string outpath = string_clone(src_dir_subfiles[j].path);
                outpath.raw[outpath.len-1] = 'o';
                printf("\t%s/"str_fmt"\n", real_src_dirs[i], str_arg(outpath));
                // sprintf(cmd, "%s -c -o %s %s %s", cc, );
                clear(cmd);
            }
        }

        FOR_RANGE(j, 0, src_dir_subfile_count) fs_drop(&src_dir_subfiles[j]);
        free(src_dir_subfiles);

        fs_drop(&source_directory);
    }

    exit(EXIT_SUCCESS);
}