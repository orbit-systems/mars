#define ORBIT_IMPLEMENTATION
#define DONT_USE_MARS_ALLOC

#include "src/common/orbit.h"
#include "src/common/strmap.c"
#include <time.h>

/* 
    TODO: ./mars-build iron-dynamic    build iron as a dynamic library 

    note: building iron as a static library requires the 'ar' utility,
    which may not be present on non-linux platforms.

    a lot of this code is inefficient and leaks memory ALL over the place, 
    but thats okay, its not doing a whole lot of heavy lifting. it just
    needs to work and be relatively easy to modify.

*/

#define STYLE_Reset  "\x1b[0m"
#define STYLE_Bold   "\x1b[1m"
#define STYLE_Dim    "\x1b[2m"
#define STYLE_Italic "\x1b[3m"

#define STYLE_FG_Black    "\x1b[30m"
#define STYLE_FG_Red      "\x1b[31m"
#define STYLE_FG_Green    "\x1b[32m"
#define STYLE_FG_Yellow   "\x1b[33m"
#define STYLE_FG_Blue     "\x1b[34m"
#define STYLE_FG_Magenta  "\x1b[35m"
#define STYLE_FG_Cyan     "\x1b[36m"
#define STYLE_FG_White    "\x1b[37m"
#define STYLE_FG_Default  "\x1b[39m"

#define STYLE_BG_Black    "\x1b[40m"
#define STYLE_BG_Red      "\x1b[41m"
#define STYLE_BG_Green    "\x1b[42m"
#define STYLE_BG_Yellow   "\x1b[43m"
#define STYLE_BG_Blue     "\x1b[44m"
#define STYLE_BG_Magenta  "\x1b[45m"
#define STYLE_BG_Cyan     "\x1b[46m"
#define STYLE_BG_White    "\x1b[47m"
#define STYLE_BG_Default  "\x1b[49m"

#ifdef __linux__
    #define mkdir(x) mkdir(x, S_IRWXU);
#endif

char* cc = "gcc";

char* mars_sources[] = {
    "src/common",
    "src/mars",
    "src/mars/phobos",
    "src/mars/phobos/analysis",
    "src/mars/phobos/parse",
};

char* iron_sources[] = {
    "src/common",
    "src/iron",
    "src/iron/io",
    "src/iron/passes",
    "src/iron/passes/analysis",
    "src/iron/passes/transform",
    "src/iron/codegen",
    "src/iron/codegen/x64",
};

// char* opt = " -O3 -flto";
char* opt = "";

char* cflags = 
    " -std=c17 -D_XOPEN_SOURCE=700 -fwrapv "
    " -fno-delete-null-pointer-checks -fno-strict-overflow -fno-strict-aliasing "
    " -Wall -Wno-format -Wno-unused -Werror=incompatible-pointer-types -Wno-discarded-qualifiers "
;
char* lflags =
    " -lm "
;

typedef char* cstr;
da_typedef(cstr);

typedef struct timespec Timespec;
typedef struct BuildFile {
    char* realpath;
    Timespec last_modified;
} BuildFile;

BuildFile retrieve_file(char* path) {
    BuildFile f = {0};
    
    f.realpath = realpath(path, malloc(PATH_MAX));
    if (f.realpath == NULL) {
        CRASH("realpath error");
    }

    struct stat statbuf = {0};
    if (stat(f.realpath, &statbuf) != 0) {
        CRASH("stat error");
    }

    f.last_modified = (Timespec){.tv_sec = statbuf.st_mtime};

    return f;
}

bool timespec_greater(Timespec a, Timespec b) {
    return (a.tv_sec > b.tv_sec || (a.tv_sec == b.tv_sec && a.tv_nsec > b.tv_nsec));
}

enum {
    BUILD_MODE_MARS,
    BUILD_MODE_IRON_EXE,
    BUILD_MODE_IRON_STATIC,
    BUILD_MODE_FORMAT,
};

void print_usage() {
    printf(
        "./mbuild mars            build mars with iron\n"
        "./mbuild iron            build iron as a standalone application\n"
        "./mbuild iron-static     build iron as a static library\n"
        "./mbuild format          use clang-format on the whole project. all flags are ignored\n"
        "\n"
        "-opt [flags]            set optimization flags (default -O0)\n"
        "-clean                  delete the build folder OR force rebuild from scratch\n"
        "-release                sets -opt \"-O3 -flto\", removes debug info flags\n"
        "-cflags [flags]         add more flags for the c compiler\n"
        "-cc [cc]                specify a c compiler to use (default gcc)\n"
    );
}

char* add_cstr(char* a, char* b) {
    char* s = malloc(strlen(a) + strlen(b) + 1);
    strcpy(s, a);
    strcat(s, b);
    return s;
}

void add_if_not_exists(da(cstr)* list, char* s) {
    foreach(char* item, *list) {
        if (strcmp(item, s) == 0) {
            return;
        }
    }
    da_append(list, s);
}

void add_source_collection(da(cstr)* list, char** folders, int len) {
    for_range(i, 0, len) {
        char* real = realpath(folders[i], NULL);
        if (real == NULL) {
            printf("cannot find %s\n", folders[i]);
            exit(1);
        }
        add_if_not_exists(list, real);
    }
}

bool ends_with(char* s, char* suffix) {
    return strlen(s) >= strlen(suffix) && strcmp(suffix, s + strlen(s) - strlen(suffix)) == 0;
}

char* dir_of(char* path) {
    int last_slash = -1;
    int len = strlen(path);
    for (int i = len - 1; i > -1; i--) {
        if (path[i] == '\\' || path[i] == '/') {
            last_slash = i;
            break;
        }
    }

    if (last_slash == -1) return NULL;

    char* dir = malloc(last_slash);
    strncpy(dir, path, last_slash);
    dir[last_slash] = '\0';
    return dir;
}

string dir_of_str(string path) {
    int last_slash = -1;
    for (int i = path.len - 1; i > -1; i--) {
        if (path.raw[i] == '\\' || path.raw[i] == '/') {
            last_slash = i;
            break;
        }
    }

    if (last_slash == -1) return NULL_STR;

    return (string){.len = last_slash, .raw = path.raw};
}

char saved_cwd[500] = {0};

void ensure_directory(string file) {
    string dir = dir_of_str(file);
    if (!fs_exists(dir)) {
        ensure_directory(dir);
        char* cdir = clone_to_cstring(dir);
        mkdir(cdir);
        free(cdir);
    }
}

da_typedef(string);

bool should_rebuild_object(string object_path);

StrMap file_data = {0};

int main(int argc, char** argv) {
    strmap_init(&file_data, 64);

    getcwd(saved_cwd, sizeof(saved_cwd));

    BuildFile self = retrieve_file(argv[0]);
    BuildFile self_source = retrieve_file(__FILE__);

    // self-rebuild if necessary
    if (timespec_greater(self_source.last_modified, self.last_modified)) {

        // remove old executable
        char cmd[400] = {0};
        sprintf(cmd, "rm -f %s", argv[0]);
        system(cmd);
        memset(cmd, 0, sizeof(cmd));

        // recompile and restart
        sprintf(cmd, "%s " __FILE__ " -o %s -Isrc", cc, argv[0]); 
        system(cmd);
        memset(cmd, 0, sizeof(cmd));
        char* cmdp = cmd;
        for_range(i, 0, argc) {
            cmdp = add_cstr(cmdp, argv[i]);
            cmdp = add_cstr(cmdp, " ");
        }
        return system(cmdp);
    }

    // set build mode
    int build_mode = -1;
    bool clean_build = false;
    bool release_build = false;
    if (argc <= 1) {
        print_usage();
        return 0;
    }
    // if (argc >= 2 && strcmp(argv[1], "mars") == 0) {
    //     build_mode = BUILD_MODE_MARS;
    // }
    // if (argc >= 2 && strcmp(argv[1], "iron") == 0) {
    //     build_mode = BUILD_MODE_IRON_EXE;
    // }
    // if (argc >= 3 && strcmp(argv[1], "iron-static") == 0 && strcmp(argv[2], "static") == 0) {
    //     build_mode = BUILD_MODE_IRON_STATIC;
    // }
    for_range(i, 1, argc) {
        char* arg = argv[i];
        if (strcmp(arg, "mars") == 0) {
            build_mode = BUILD_MODE_MARS;
        } else if (strcmp(arg, "iron") == 0) {
            build_mode = BUILD_MODE_IRON_EXE;
        } else if (strcmp(arg, "iron-static") == 0) {
            build_mode = BUILD_MODE_IRON_STATIC;
        } else if (strcmp(arg, "format") == 0) {
            build_mode = BUILD_MODE_FORMAT;
            break;
        } else if (strcmp(arg, "-clean") == 0) {
            clean_build = true;
            system("rm -rf build");
        } else if (strcmp(arg, "-release") == 0) {
            opt = " -O3 -flto ";
            release_build = true;
        } else if (strcmp(arg, "-opt") == 0) {
            opt = argv[i+1];
            i++;
        } else if (strcmp(arg, "-cc") == 0) {
            cc = argv[i+1];
            i++;
        } else if (strcmp(arg, "-cflags") == 0) {
            if (i + 1 == argc) {
                printf("-cflags needs an argument list\n");
                exit(-1);
            }
            char* new_cflags = malloc(strlen(cflags) + strlen(argv[i+1]) + 1);
            strcpy(new_cflags, cflags);
            strcat(new_cflags, argv[i+1]);
            cflags = new_cflags;
            i++;
        } else {
            printf("unknown flag '%s'\n", arg);
            exit(1);
        }
    }
    if (build_mode == -1) {
        return 0;
    }
    if (!release_build) {
        #ifndef _WIN32
            char* debug_flags = " -pg -g -rdynamic";
        #else
            char* debug_flags = " -pg -g";
        #endif
        cflags = add_cstr(cflags, debug_flags);
        opt = " -O0 ";
    }

    da(cstr) source_folders = {0};
    da_init(&source_folders, 16);

    if (build_mode == BUILD_MODE_FORMAT) {
        add_source_collection(&source_folders, mars_sources, sizeof(mars_sources)/sizeof(mars_sources[0]));
    } else if (build_mode == BUILD_MODE_MARS) {
        add_source_collection(&source_folders, mars_sources, sizeof(mars_sources)/sizeof(mars_sources[0]));
    } else if (build_mode == BUILD_MODE_IRON_EXE) {
        da_append(&source_folders, realpath("src/iron/driver", NULL));
    }
    add_source_collection(&source_folders, iron_sources, sizeof(iron_sources)/sizeof(iron_sources[0]));

    da_typedef(string);
    da(string) files_to_compile = {0};
    da_init(&files_to_compile, 16);

    da(string) obj_paths;
    da_init(&obj_paths, files_to_compile.len);
    obj_paths.len = files_to_compile.len;

    foreach(char* folder, source_folders) {
        // printf("%s\n", folder);
        string folderstr = str(folder);

        fs_file folderfile = {0};
        assert(fs_get(str(folder), &folderfile));
        if (folderfile.type != oft_directory) {
            printf("%s is not a directory\n");
            exit(1);
        }
        
        int num_subfiles = fs_subfile_count(&folderfile);
        if (num_subfiles == 0) continue;

        fs_file* subfiles = malloc(sizeof(fs_file) * num_subfiles);
        fs_get_subfiles(&folderfile, subfiles);
        for_range(i, 0, num_subfiles) {
            fs_file f = subfiles[i];
            if (f.type == oft_directory || !string_ends_with(f.path, constr(".c")) ) continue;
            // printf("\t"str_fmt"\n", str_arg(subfiles[i].path));
            da_append(&files_to_compile, f.path);

            string obj_path = string_clone(f.path);
            // crazy shit but i cannot think of a situation where it would not work
            obj_path.raw += strlen(saved_cwd)-1;
            obj_path.len -= strlen(saved_cwd)-1;
            memcpy(obj_path.raw, "build", 5); 
            obj_path.raw[obj_path.len-1] = 'o';

            da_append(&obj_paths, obj_path);

            strmap_put(&file_data, f.path, &subfiles[i]);
        }
    }
    chdir(saved_cwd);

    if (build_mode == BUILD_MODE_FORMAT) {
        foreach (string src_path, files_to_compile) {
            string format_command = strprintf("clang-format -i "str_fmt, str_arg(src_path));
            int return_code = system(clone_to_cstring(format_command));
            if (return_code != 0) return return_code;
        }
        return 0;
    }
    
    if (!fs_exists(constr("build"))) {
        mkdir("build");
    }

    bool* needs_to_compile = malloc(sizeof(bool)*files_to_compile.len);
    memset(needs_to_compile, 0, sizeof(bool)*files_to_compile.len);

    int how_many_to_compile = 0;
    foreach (string src_path, files_to_compile) {
        string obj_path = obj_paths.at[count];

        if (!fs_exists(obj_path) || should_rebuild_object(obj_path)) {
            needs_to_compile[count] = true;
            how_many_to_compile++;
            continue;
        }
        // TODO("timespec check against dependencies against object file");
    }

    // if (files_to_compile.len == 0) return 0;

    int file_counter = 1;
    for_range(i, 0, files_to_compile.len) {
        if (!needs_to_compile[i]) continue;
        string compile_path = files_to_compile.at[i];
        string short_compile_path = compile_path;
        short_compile_path.raw += strlen(saved_cwd)+1;
        short_compile_path.len -= strlen(saved_cwd)+1;

        ensure_directory(obj_paths.at[i]);

        printf(STYLE_Reset"["STYLE_FG_Green"%d/%d"STYLE_Reset"] compiling "STYLE_Bold str_fmt STYLE_Reset"\n",
            file_counter++, how_many_to_compile, str_arg(short_compile_path)
        );

        // we have to actually MAKE THE COMPILER COMMAND LMAO
        // {cc} {source} -o {output} -MD {cflags}
        
        string compile_command = strprintf("%s -c "str_fmt" -o "str_fmt" -Isrc -MD %s %s",
            cc, str_arg(compile_path), str_arg(obj_paths.at[i]), opt, cflags
        );
        int compile_return_code = system(clone_to_cstring(compile_command));
        if (compile_return_code != 0) return compile_return_code;

        // printstr(compile_command);
        // printf("\n");
    }

    string obj_list;
    for_range(i, 0, obj_paths.len) {
        obj_list.len += obj_paths.at[i].len + 1;
    }
    obj_list = string_alloc(obj_list.len);
    {
        int cursor = 0;
        for_range(i, 0, obj_paths.len) {
            memcpy(obj_list.raw + cursor, obj_paths.at[i].raw, obj_paths.at[i].len);
            cursor += obj_paths.at[i].len;
            obj_list.raw[cursor++] = ' ';
        }
    }    

    switch (build_mode) {
    case BUILD_MODE_MARS: {
        if (!clean_build && how_many_to_compile == 0 && (fs_exists(str("mars")) || fs_exists(str("mars.exe")))) {
            break;
        }
        string compile_command = strprintf("%s "str_fmt" -o mars %s %s",
            cc, str_arg(obj_list), opt, cflags
        );
        int compile_return_code = system(clone_to_cstring(compile_command));
        if (compile_return_code != 0) return compile_return_code;
    } break;
    case BUILD_MODE_IRON_EXE:{
        if (!clean_build && how_many_to_compile == 0 && (fs_exists(str("iron")) || fs_exists(str("iron.exe")))) {
            break;
        }
        string compile_command = strprintf("%s "str_fmt" -o iron %s %s",
            cc, str_arg(obj_list), opt, cflags
        );
        int compile_return_code = system(clone_to_cstring(compile_command));
        if (compile_return_code != 0) return compile_return_code;
    } break;
    case BUILD_MODE_IRON_STATIC:
        TODO("iron static");
    default:
        break;
    }
}

int stringchr(string s, char c) {
    for_range(i, 0, s.len) {
        if (s.raw[i] == c) {
            return i;
        }
    }
    return -1;
}

bool should_rebuild_object(string object_path) {
    // retrieve dependency file. we expect it to be near the object file
    BuildFile object = retrieve_file(clone_to_cstring(object_path));

    string dep_path = string_clone(object_path);
    dep_path.raw[dep_path.len-1] = 'd';
    fs_file dep;
    if (!fs_get(dep_path, &dep)) {
        return true;
    }
    fs_open(&dep, "rb");
    string dep_text = string_alloc(dep.size);
    fs_read_entire(&dep, dep_text.raw);

    // printstr(dep_text);
    // skip initial :
    int colon_pos = stringchr(dep_text, ':') + 1;
    dep_text.raw += colon_pos;
    dep_text.len -= colon_pos;

    // printstr(dep_text);

    da(string) deps;
    da_init(&deps, 32);

    for_range(i, 0, dep_text.len) {
        switch (dep_text.raw[i]) {
        case ' ':
        case '\r':
        case '\n':
        case '\t':
        case '\v':
            i++;
            // continue;
        }

        if (dep_text.raw[i] == '\\' && dep_text.raw[i+1] == '\n') {
            i += 2;
            continue;
        }
        if (dep_text.raw[i] == '\\' && dep_text.raw[i+1] == '\r' && dep_text.raw[i+2] == '\n') {
            i += 3;
            continue;
        }

        i64 start_index = i;
        while (i < dep_text.len) {
            switch (dep_text.raw[i]) {
            case ' ':
            case '\r':
            case '\n':
            case '\t':
            case '\v':
                goto restart_loop;
            }
            i++;
        }

        restart_loop:
        da_append(&deps, string_make(&dep_text.raw[start_index], i - start_index));
    }
    
    foreach(string dep_path, deps) {
        // printstr(dep_path);
        // printf("\n");
        if (dep_path.len == 0) continue;
        BuildFile dep_file = retrieve_file(clone_to_cstring(dep_path));
        
        if (timespec_greater(dep_file.last_modified, object.last_modified)) {
            return true;
        }
    }
    return false;
}