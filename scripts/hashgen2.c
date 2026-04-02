#include <ctype.h>
#include <stdlib.h>

#include "common/portability.h"
#include "common/type.h"
#include "common/util.h"
#include "common/fs.h"
#include "../common/src/common/fs_win.c"
#include "../common/src/common/fs_unix.c"
#include "../common/src/common/str.c"
#include "../common/src/common/vec.c"

usize hashfunc(const char* key, usize key_len, usize offset, usize mult, usize table_size) {
    usize hash = offset;

    for (usize i = 0; i < key_len; ++i) {
        hash ^= key[i];
        hash *= mult;
    }
    return hash % table_size;
}

#define func_text \
    "usize hashfunc(const char* key, usize key_len) {\n" \
    "    usize hash = %zu;\n" \
    "\n" \
    "    for (usize i = 0; i < key_len; ++i) {\n" \
    "        hash ^= key[i];\n" \
    "        hash *= %zu;\n" \
    "    }\n" \
    "    return hash %% %zu;\n" \
    "}\n" \

Vec(string) keywords = {};

int main(int argc, char** argv) {

    keywords = vec_new(string, 256);
    // intake keywords file

    if (argc <= 1) {
        printf("provide a file name.\n");
        return 1;
    }
    
    // if (argc <= 2) {
    //     printf("provide a max number for search parameters.\n");
    //     return 1;
    // }

    FsFile* file = fs_open(argv[1], false, false);
    string text = fs_read_entire(file, false);
    text = string_concat(text, strlit("\n"));

    usize i = 0;
    while (i < text.len) {
        if (isspace(text.raw[i])) {
            ++i;
            continue;
        }
        // scan a keyword
        usize start = i;
        while (!isspace(text.raw[i])) {
            ++i;
        }
        string kw = {
            .raw = &text.raw[start],
            .len = i - start
        };
        vec_append(&keywords, kw);
    }


    // work backwards
    for (isize table_size = (isize)(vec_len(keywords) * 2); table_size >= vec_len(keywords); table_size--) {
        next_table_size:

        bool* occupied = malloc(sizeof(bool) * table_size);
        memset(occupied, 0, sizeof(bool) * table_size);
    
        srand(0x12345678);

        while (true) {
            usize offset = rand() + ((usize)rand() << 32);
            usize mult = rand() + ((usize)rand() << 32);

            // try this table configuration.
            for_n(i, 0, vec_len(keywords)) {
                string kw = keywords[i];
                usize index = hashfunc(kw.raw, kw.len, offset, mult, table_size);
                if (occupied[index]) {
                    goto next_config;
                }
                occupied[index] = true;
            }
            printf("CONFIG FOUND FOR LENGTH %zi:\n" func_text"\n", table_size, offset, mult, table_size);
            table_size--;
            goto next_table_size;
                
            next_config:
            memset(occupied, 0, sizeof(bool) * table_size);
        }
        printf("tried table size %zi\n", table_size);
    }

    fs_close(file);
}
