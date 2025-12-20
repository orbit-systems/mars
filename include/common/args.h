#ifndef ORBIT_ARGS_H
#define ORBIT_ARGS_H

#include "common/type.h"
#include "common/str.h"

typedef enum ArgCaptureKind: u8 {
    ARGVAL_STRING,
    ARGVAL_INTEGER,
    ARGVAL_FLOATING,
    ARGVAL_ENUMERATION,
} ArgCaptureKind;

typedef struct ArgCapture {
    ArgCaptureKind kind;
    union {
        string str;
        usize integer;
        f64 floating;
        string enumeration;
    } val;
} ArgCapture;

typedef struct Argument {
    ArgCapture* values;
    u32 values_len;
    u32 arg_id;
} Argument;

/* 
    Arguments which accept values are created via template strings. 
    
    {s} - string
    {i} - integer
    {f} - floating
    {e} - enumeration

    Examples:
        iteration-limit {s} {i} {f}
        arch {e: rv32 rv64 {alias: x86-64 x64 ia64}}
        warn {...f}

    Valid invocations would be:
        --iteration-limit bruh 102 3.14
        --arch rv32
        --warn bruh={f} lmao={f}
*/

typedef struct ArgBuilder ArgBuilder;
typedef struct ParsedArgs ParsedArgs;

/// Create a new ArgBuilder.
ArgBuilder* args_new();
/// Destroy and deallocate an ArgBuilder
void argb_destroy(ArgBuilder* ab);

/// TODO write a comprehensive description for this :sweat:
void argb_create(ArgBuilder* ab, u32 arg_id, string template, bool required);

/// Create a simple 'switch' argument that does not take arguments.
/// Can be used `-c`, where `c` is `short_form`, or `--l` where `l` is `long_form`
/// When used in short form, it can be combined with other switch arguments, like `-xyz`
void argb_create_switch(ArgBuilder* ab, u32 arg_id, char short_form, string long_form);

/// Parse an argument list with `ArgBuilder ab`.
ParsedArgs* args_parse(ArgBuilder* ab, int argc, const char** argv);
void args_destroy(ParsedArgs* pa);

Argument* args_get(ParsedArgs* pa, u32 arg_id);
string args_get_uncaptured(ParsedArgs* pa, u32 index);

#endif // ORBIT_ARGS_H
