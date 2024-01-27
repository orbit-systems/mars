#include "orbit.h"
#include "term.h"

void general_error(char* message, ...) {
    char ERROR_MSG_BUFFER[500] = {0};
    va_list args;
    va_start(args, message);
    vsprintf(ERROR_MSG_BUFFER, message, args);
    va_end(args);


    printf(STYLE_FG_Red STYLE_Bold "ERROR" STYLE_Reset);

    printf(STYLE_Dim " | " STYLE_Reset "%s", ERROR_MSG_BUFFER);


    printf("\n");
    exit(EXIT_FAILURE);
}

void general_warning(char* message, ...) {
    char ERROR_MSG_BUFFER[500] = {0};
    va_list args;
    va_start(args, message);
    vsprintf(ERROR_MSG_BUFFER, message, args);
    va_end(args);

    printf(STYLE_FG_Yellow STYLE_Bold "WARNING" STYLE_Reset);


    printf("\n");
}

void error_at_string(string path, string text, string pos, char* message, ...) {

    char ERROR_MSG_BUFFER[500] = {0};
    va_list args;
    va_start(args, message);
    vsprintf(ERROR_MSG_BUFFER, message, args);
    va_end(args);

    printf(STYLE_FG_Red STYLE_Bold "ERROR" STYLE_Reset);

    printf(STYLE_Dim " | " STYLE_Reset);

    int line = 1;
    int column = 1;
    char* line_ptr = text.raw - 1;
    int line_len = 0;
    line_and_col(text, pos.raw-text.raw, &line_ptr, &line_len, &line, &column);
    if (line_ptr[line_len-1] == '\r') line_len--; // stupid windows line breaks

    printf(STYLE_Italic STYLE_Bold);
    printstr(path);
    printf(" @ %d:%d ", line, column);
    printf(STYLE_Reset);

    printf(STYLE_Dim "-> " STYLE_Reset);

    printf(STYLE_Italic STYLE_Bold "%s" STYLE_Reset, ERROR_MSG_BUFFER);

    printf(STYLE_Dim);
    // printf("\n      | ");
    printf("\n %4d | ", line);
    printf(STYLE_Reset);
    printstr(string_make(line_ptr, line_len));
    printf(STYLE_Dim);
    printf("\n      | ");
    printf(STYLE_Reset);

    while (column-- > 1) printf(" ");

    printf(STYLE_FG_Red);
    printf(STYLE_Bold);
    if (pos.len > 0) {
        printf("^");
    }
    if (pos.len > 2) {
        for (size_t i = 2; i < pos.len; i++)
            printf("~");
    }
    if (pos.len > 1) {
        printf("^");
    }
    printf(STYLE_Reset);

    // printf(STYLE_Italic);
    // printf(STYLE_Bold);
    // printf(" %s\n", ERROR_MSG_BUFFER);
    // printf(STYLE_Reset);
    printf("\n");

    exit(EXIT_FAILURE);
}

void warning_at_string(string path, string text, string pos, char* message, ...) {
    
    char ERROR_MSG_BUFFER[500] = {0};
    va_list args;
    va_start(args, message);
    vsprintf(ERROR_MSG_BUFFER, message, args);
    va_end(args);

    printf(STYLE_FG_Yellow STYLE_Bold "WARNING" STYLE_Reset);

    printf(STYLE_Dim " | " STYLE_Reset);

    int line = 1;
    int column = 1;
    char* line_ptr = text.raw;
    int line_len = 0;
    line_and_col(text, pos.raw-text.raw, &line_ptr, &line_len, &line, &column);

    printstr(path);
    printf(" @ %d:%d ", line, column);

    printf(STYLE_Dim "-> " STYLE_Reset);



    printf(STYLE_Italic STYLE_Bold "%s" STYLE_Reset, ERROR_MSG_BUFFER);

    printf(STYLE_Dim);
    // printf("\n        | ");
    printf("\n   % 4d | ", line);
    printf(STYLE_Reset);
    printstr(string_make(line_ptr, line_len));
    printf(STYLE_Dim);
    printf("\n        | ");
    printf(STYLE_Reset);
    
    while (column-- > 1) printf(" ");


    printf(STYLE_FG_Yellow);
    printf(STYLE_Bold);
    if (pos.len > 0) {
        printf("^");
    }
    if (pos.len > 2) {
        for (size_t i = 2; i < pos.len; i++)
            printf("~");
    }
    if (pos.len > 1) {
        printf("^");
    }
    printf(STYLE_Reset);

    // printf(STYLE_Italic);
    // printf(STYLE_Bold);
    // printf(" %s\n", ERROR_MSG_BUFFER);
    // printf(STYLE_Reset);
    printf("\n");

}

void line_and_col(string text, size_t position, char** restrict last_newline, int* restrict line_len, int* restrict line, int* restrict col) {
    
    int l = 0;
    int c = 0;

    for (size_t i = 0; i <= position; i++) {
        if (text.raw[i] == '\n') {
            *last_newline = &text.raw[i];
            l++;
            c = 0;
            continue;
        }
        if (text.raw[i] == '\t') {
            c += 4;
            continue;
        }
        c++;
    }

    *last_newline += 1;
    *line = l + 1;
    *col = c;

    *line_len = 0;
    while ((*last_newline)[*line_len] != '\0' && (*last_newline)[*line_len] != '\n') {
        if (*line_len + *last_newline - text.raw >= text.len) break;
        *line_len += 1;
    }
}