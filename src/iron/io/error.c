#include "iron/iron.h"
#include "common/crash.h"
#include "style.h"

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
        crash("iron: fatal message\n");
    }
}

void fe_print_message(FeMessage msg) {
    printf("IRON ");
    switch (msg.severity) {
    case FE_MSG_SEVERITY_FATAL: printf(STYLE_FG_Red STYLE_Bold "FATAL" STYLE_Reset); break;
    case FE_MSG_SEVERITY_ERROR: printf(STYLE_FG_Red "ERROR" STYLE_Reset); break;
    case FE_MSG_SEVERITY_WARNING: printf(STYLE_FG_Yellow "WARNING" STYLE_Reset); break;
    case FE_MSG_SEVERITY_LOG: printf("LOG"); break;
    }
    printf(" from "STYLE_Bold"%s"STYLE_Reset"(): ", msg.function_of_origin);
    printf("%s\n", msg.message);
}