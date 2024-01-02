#include "orbit.h"
#include "error.h"

void general_error(char* message, ...) {
    char ERROR_MSG_BUFFER[500] = {};
    va_list args;
    va_start(args, message);
    vsprintf(ERROR_MSG_BUFFER, message, args);
    va_end(args);

    set_style(STYLE_FG_Red);
    set_style(STYLE_Bold);
    printf("ERROR");
    set_style(STYLE_Reset);

    set_style(STYLE_Dim);
    printf(" | ");
    set_style(STYLE_Reset);

    printf("%s", ERROR_MSG_BUFFER);

    printf("\n");
    exit(EXIT_FAILURE);
}

void general_warning(char* message, ...) {
    char ERROR_MSG_BUFFER[500] = {};
    va_list args;
    va_start(args, message);
    vsprintf(ERROR_MSG_BUFFER, message, args);
    va_end(args);

    set_style(STYLE_FG_Yellow);
    set_style(STYLE_Bold);
    printf("WARNING");
    set_style(STYLE_Reset);

    set_style(STYLE_Dim);
    printf(" | ");
    set_style(STYLE_Reset);

    printf("%s", ERROR_MSG_BUFFER);

    printf("\n");
    exit(EXIT_FAILURE);
}

void error_at_string(string path, string text, string pos, char* message, ...) {
    
    char ERROR_MSG_BUFFER[500] = {};
    va_list args;
    va_start(args, message);
    vsprintf(ERROR_MSG_BUFFER, message, args);
    va_end(args);

    set_style(STYLE_FG_Red);
    set_style(STYLE_Bold);
    printf("ERROR");
    set_style(STYLE_Reset);

    set_style(STYLE_Dim);
    printf(" |");
    set_style(STYLE_Reset);

    int line;
    int column;
    char* line_ptr;
    int line_len;
    line_and_col(text, pos.raw-text.raw, &line_ptr, &line_len, &line, &column);

    printf(" "); printstr(path);
    printf(" @ %d:%d ", line, column);

    set_style(STYLE_Dim);
    printf("-> ");
    set_style(STYLE_Reset);

    set_style(STYLE_Italic);
    set_style(STYLE_Bold);
    printf("%s", ERROR_MSG_BUFFER);
    set_style(STYLE_Reset);

    set_style(STYLE_Dim);
    printf("\n      | ");
    printf("\n % 4d | ", line);
    set_style(STYLE_Reset);
    printstr(string_make(line_ptr, line_len));
    set_style(STYLE_Dim);
    printf("\n      | ");
    set_style(STYLE_Reset);
    
    while (column-- > 1) printf(" ");


    set_style(STYLE_FG_Red);
    set_style(STYLE_Bold);
    if (pos.len > 0) {
        printf("^");
    }
    if (pos.len > 2) {
        for (int i = 2; i < pos.len; i++)
            printf("~");
    }
    if (pos.len > 1) {
        printf("^");
    }
    set_style(STYLE_Reset);

    set_style(STYLE_Italic);
    set_style(STYLE_Bold);
    printf(" %s\n", ERROR_MSG_BUFFER);
    set_style(STYLE_Reset);

    exit(EXIT_FAILURE);

}

void warning_at_string(string path, string text, string pos, char* message, ...) {
    
    char ERROR_MSG_BUFFER[500] = {};
    va_list args;
    va_start(args, message);
    vsprintf(ERROR_MSG_BUFFER, message, args);
    va_end(args);

    set_style(STYLE_FG_Red);
    set_style(STYLE_Bold);
    printf("WARNING");
    set_style(STYLE_Reset);

    set_style(STYLE_Dim);
    printf("     |");
    set_style(STYLE_Reset);

    int line;
    int column;
    char* line_ptr;
    int line_len;
    line_and_col(text, pos.raw-text.raw, &line_ptr, &line_len, &line, &column);

    printf(" %s @ %d:%d ", path, line, column);

    set_style(STYLE_Dim);
    printf("-> ");
    set_style(STYLE_Reset);

    set_style(STYLE_Italic);
    set_style(STYLE_Bold);
    printf("%s", ERROR_MSG_BUFFER);
    set_style(STYLE_Reset);

    set_style(STYLE_Dim);
    printf("\n        | ");
    printf("\n   % 4d | ", line);
    set_style(STYLE_Reset);
    printstr(string_make(line_ptr, line_len));
    set_style(STYLE_Dim);
    printf("\n        | ");
    set_style(STYLE_Reset);
    
    while (column-- > 1) printf(" ");


    set_style(STYLE_FG_Red);
    set_style(STYLE_Bold);
    if (pos.len > 0) {
        printf("^");
    }
    if (pos.len > 2) {
        for (int i = 2; i < pos.len; i++)
            printf("~");
    }
    if (pos.len > 1) {
        printf("^");
    }
    set_style(STYLE_Reset);

    set_style(STYLE_Italic);
    set_style(STYLE_Bold);
    printf(" %s\n", ERROR_MSG_BUFFER);
    set_style(STYLE_Reset);

    exit(EXIT_FAILURE);

}

void line_and_col(string text, size_t position, char** last_newline, int* line_len, int* line, int* col) {
    
    int l = 0;
    int c = 0;

    for (int i = 0; i <= position; i++) {
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
    while ((*last_newline)[*line_len] != '\0' && (*last_newline)[*line_len] != '\n') 
        *line_len += 1;
}