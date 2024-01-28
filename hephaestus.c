// to build the project, just compile and run this file!

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

//////////////////////////////////////////////////////////////////////////////

// HEPHAESTUS by sandwichman - single-file, slim build tool for multi-file C projects

// here be dragons

// seriously, im not proud of this code
// dont @ me if you find bad shit in here

#define ORBIT_IMPLEMENTATION
#include "src/orbit.h"

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
char base_dir[PATH_MAX] = {0}; // this is the current working directory at the start of execution

#define error(msg, ...) do { \
    printf("error: ");       \
    printf(msg __VA_OPT__(,) __VA_ARGS__); \
    printf("\n");            \
    exit(EXIT_FAILURE);      \
} while (0)

int main() {

    // translate relative paths into realpaths (im so fucking done)
    real_output_dir = malloc(PATH_MAX);
    if (output_dir[0] == '\0') output_dir = "./";
    realpath(output_dir, real_output_dir);

    real_build_dir = malloc(PATH_MAX);
    if (build_dir[0] == '\0') {
        if (fs_exists(to_string("./build"))) {
            error("an explicit build path must be provided");
        }
        printf("default build directory created\n");
        build_dir = "./build";
    }
    realpath(build_dir, real_build_dir);

    real_include_dir = malloc(PATH_MAX);
    if (include_dir[0] == '\0') real_include_dir = "";
    else realpath(include_dir, real_include_dir);

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

    printf("compiling using %s with flags %s\n", cc, flags);


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
                printf("[ %d / %d ] compiling %s/"str_fmt"\n", file_num++, total_files_to_build, real_src_dirs[i], str_arg(src_dir_subfiles[j].path));
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

                if (system(cmd)) {
                    error("building "str_fmt" failed", str_arg(src_dir_subfiles[j].path));
                }
                clear(cmd);
            }
        }

        FOR_RANGE(j, 0, src_dir_subfile_count) fs_drop(&src_dir_subfiles[j]);
        free(src_dir_subfiles);

        fs_drop(&source_directory);
    }

    printf("linking using %s with flags %s\n", cc, link_flags);

    char output_name[PATH_MAX] = {0};
    sprintf(output_name, "%s/%s",  real_output_dir, project_name);

    if (fs_exists(to_string(output_name))) {
        strcat(cmd, "rm ");
        strcat(cmd, output_name);
        system(cmd);
        clear(cmd);
    }

    sprintf(cmd, "%s %s -o %s %s",
        cc, objects, output_name, link_flags
    );
    if (system(cmd)) {
        error("linking failed");
    }
    clear(cmd);


    printf("%s built successfully!\n", project_name);
    exit(EXIT_SUCCESS);
}