#include "common/orbit.h"
#include "common/arena.h"

// better way to do this but whatever

typedef struct StringBuilder {
    char* buffer;
    size_t len;
    size_t cap;
} StringBuilder;

void sb_init(StringBuilder* sb);
void sb_destroy(StringBuilder* sb);

void sb_append(StringBuilder* sb, string s);
void sb_append_c(StringBuilder* sb, char* s);
void sb_printf(StringBuilder* sb, char* fmt, ...);

size_t sb_len(StringBuilder* sb);
void sb_write(StringBuilder* sb, char* buffer);