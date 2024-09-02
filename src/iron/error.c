#include "iron/iron.h"
#include "common/crash.h"

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

void fe_clear_message_buffer(FeModule* m) {
    m->messages.len = 0;
}

FeMessage fe_pop_message(FeModule* m) {
    if (m->messages.len == 0) return (FeMessage){0};
    return m->messages.at[--m->messages.len];
}

void fe_push_message(FeModule* m, FeMessage msg) {
    if (m->messages.at == NULL) {
        da_init(&m->messages, 32);
    }
    da_append(&m->messages, msg);
    if (msg.severity == FE_MSG_SEVERITY_FATAL) {
        fe_print_message(msg);
        crash("iron: fatal message");
    }
}

void fe_print_message(FeMessage msg) {
    printf("IRON [ ");
    switch (msg.severity) {
    case FE_MSG_SEVERITY_FATAL: printf(STYLE_FG_Red STYLE_Bold "FATAL" STYLE_Reset); break;
    case FE_MSG_SEVERITY_ERROR: printf(STYLE_FG_Red "ERROR" STYLE_Reset); break;
    case FE_MSG_SEVERITY_WARNING: printf(STYLE_FG_Yellow "WARNING" STYLE_Reset); break;
    case FE_MSG_SEVERITY_LOG: printf("LOG"); break;
    }
    printf(" ] %s()\n", msg.function_of_origin);
    printf("| %s\n", msg.message);
}