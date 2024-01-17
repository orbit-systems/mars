#pragma once
#define MARS_H

#include "orbit.h"

typedef struct cmd_arg_s {
    string key;
    string val;
} cmd_arg;

typedef struct flag_set_s {
    string input_path;
    string output_path;
    bool output_dot;
    bool print_timings;
} flag_set;

cmd_arg make_argument(char* s);
void load_arguments(int argc, char* argv[], flag_set* fl);

void print_help();
u8* load_file(FILE* asm_file);

extern flag_set mars_flags;