#include "iron/iron.h"
#include "common/crash.h"
#include "style.h"

void fe_clear_report_buffer(FeModule* m) {
    m->messages.len = 0;
}

FeReport fe_pop_report(FeModule* m) {
    if (m->messages.len == 0) return (FeReport){0};
    return m->messages.at[--m->messages.len];
}

void fe_push_report(FeModule* m, FeReport msg) {
    if (m->messages.at == NULL) {
        da_init(&m->messages, 32);
    }
    da_append(&m->messages, msg);
    if (msg.severity == FE_MSG_SEVERITY_FATAL) {
        fe_print_report(msg);
        crash("iron: fatal message\n");
    }
}

void fe_print_report(FeReport msg) {
    printf("IRON ");
    switch (msg.severity) {
    case FE_MSG_SEVERITY_FATAL: printf(STYLE_FG_Red STYLE_Bold "FATAL" STYLE_Reset); break;
    case FE_MSG_SEVERITY_ERROR: printf(STYLE_FG_Red "ERROR" STYLE_Reset); break;
    case FE_MSG_SEVERITY_WARNING: printf(STYLE_FG_Yellow "WARNING" STYLE_Reset); break;
    case FE_MSG_SEVERITY_LOG: printf(STYLE_FG_Green "LOG" STYLE_Reset); break;
    }
    printf(" in "STYLE_Bold"%s"STYLE_Reset"(): ", msg.function_of_origin);
    printf("%s\n", msg.message);
}