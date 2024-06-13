#include "orbit.h"
#include "arena.h"

// better way to do this but whatever

typedef struct StringBuilder {
    string* at;
    size_t len;
    size_t cap;

    Arena alloca; // copies strings to permanent allocator
} StringBuilder;

#define SB_ARENA_BLOCK_SIZE 0x1000
#define SB_INITIAL_CAPACITY 16

void sb_init(StringBuilder* sb);
void sb_destroy(StringBuilder* sb);

void sb_append(StringBuilder* sb, string s);
void sb_append_c(StringBuilder* sb, char* s);
void sb_printf(StringBuilder* sb, char* fmt, ...);

size_t sb_len(StringBuilder* sb);
void sb_write(StringBuilder* sb, char* buffer);