#pragma once
#define ERROR_H

#include "orbit.h"

// enum ANSI_STYLE {
//     STYLE_Reset       = 0,

//     STYLE_Bold        = 1,
//     STYLE_Dim         = 2,
//     STYLE_Italic      = 3,

//     STYLE_FG_Black    = 30,
//     STYLE_FG_Red      = 31,
//     STYLE_FG_Green    = 32,
//     STYLE_FG_Yellow   = 33,
//     STYLE_FG_Blue     = 34,
//     STYLE_FG_Magenta  = 35,
//     STYLE_FG_Cyan     = 36,
//     STYLE_FG_White    = 37,
//     STYLE_FG_Default  = 39,

//     STYLE_BG_Black    = 40,
//     STYLE_BG_Red      = 41,
//     STYLE_BG_Green    = 42,
//     STYLE_BG_Yellow   = 43,
//     STYLE_BG_Blue     = 44,
//     STYLE_BG_Magenta  = 45,
//     STYLE_BG_Cyan     = 46,
//     STYLE_BG_White    = 47,
//     STYLE_BG_Default  = 49,
// };

// #define set_style(style) printf("\x1b[%dm", style)

// use c's string literal concatenation

#define style_Reset  "\x1b[0m"
#define style_Bold   "\x1b[1m"
#define style_Dim    "\x1b[2m"
#define style_Italic "\x1b[3m"

#define style_FG_Black    "\x1b[30m"
#define style_FG_Red      "\x1b[31m"
#define style_FG_Green    "\x1b[32m"
#define style_FG_Yellow   "\x1b[33m"
#define style_FG_Blue     "\x1b[34m"
#define style_FG_Magenta  "\x1b[35m"
#define style_FG_Cyan     "\x1b[36m"
#define style_FG_White    "\x1b[37m"
#define style_FG_Default  "\x1b[39m"

#define style_BG_Black    "\x1b[40m"
#define style_BG_Red      "\x1b[41m"
#define style_BG_Green    "\x1b[42m"
#define style_BG_Yellow   "\x1b[43m"
#define style_BG_Blue     "\x1b[44m"
#define style_BG_Magenta  "\x1b[45m"
#define style_BG_Cyan     "\x1b[46m"
#define style_BG_White    "\x1b[47m"
#define style_BG_Default  "\x1b[49m"

void error_at_string(string path, string text, string pos, char* message, ...);
void warning_at_string(string path, string text, string pos, char* message, ...);

void general_error(char* message, ...);
void general_warning(char* message, ...);

void line_and_col(string text, size_t position, char** last_newline, int* line_len, int* line, int* col);