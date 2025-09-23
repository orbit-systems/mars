#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef __linux__
    #include <sys/types.h>
    #include <execinfo.h>
    #include <errno.h>
#else
    #include <windows.h>
    // #include <dbghelp.h>
#endif

#include "iron/iron.h"

[[noreturn]] void fe_runtime_crash(const char* error, ...) {
    fflush(stdout);

    fprintf(stderr, "iron: ");

    va_list args;
    va_start(args, error);
    vfprintf(stderr, error, args);
    va_end(args);
    printf("\n");

#ifndef _WIN32
    void* array[256]; // hold 256 stack traces
    char** strings;
    int size;

    size = backtrace(array, 256);
    strings = backtrace_symbols(array, size);

    if (strings != nullptr) {
        printf("obtained %d stack frames\n", size);
        for (int i = 0; i < size; i++) {
            char* output_str = strings[i];
            // if (output_str.raw[0] == '.') {
            //     // we need to trim
            //     for (; output_str.raw[0] != '('; output_str.raw++)
            //         ;
            //     output_str.raw++;
            //     int close_bracket = 0;
            //     for (; output_str.raw[close_bracket] != ')'; close_bracket++)
            //         ;
            //     output_str.len = close_bracket;
            // }
            printf("| %s\n", output_str);
        }
    }
#else
    #define MAX_BACKTRACES 255
    void* frames[MAX_BACKTRACES];

    HANDLE process = GetCurrentProcess();

    usize frames_len = CaptureStackBackTrace(0, MAX_BACKTRACES, frames, nullptr);

    // printf("WINDOWS STACK TRACES BABY\n");

    for_n (i, 0, frames_len) {
        printf("| %p\n", frames[i]);
    }

    // todo: figure out windows stack traces!

    __debugbreak();
#endif

    exit(-1);
}

#ifdef _WIN32
static void signal_handler(int sig) {
    switch (sig) {
    case SIGSEGV:
        FE_CRASH("segfault");
        break;
    case SIGINT:
        FE_CRASH("debug interrupt");
        break;
    case SIGFPE:
        FE_CRASH("fatal arithmetic exception");
        break;
    case SIGTERM:
        exit(0);
        break;
    default:
        FE_CRASH("unhandled signal %d", sig);
        break;
    }
}
void fe_init_signal_handler() {
    signal(SIGSEGV, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGFPE, signal_handler);
    signal(SIGTERM, signal_handler); 
}
#else
static void signal_handler(int sig, siginfo_t* info, void* ucontext) {
    // ucontext = ucontext;
    switch (sig) {
    case SIGSEGV:
        FE_CRASH("segfault at addr 0x%lx", (long)info->si_addr);
        break;
    case SIGINT:
        printf("debug interrupt caught");
        break;
    case SIGFPE:
        FE_CRASH("fatal arithmetic exception");
        break;
    case SIGABRT:
        FE_CRASH("sigabrt lmao");
        break;
    default:
        FE_CRASH("unhandled signal %s caught", strsignal(sig));
        break;
    }
}

void fe_init_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigfillset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, nullptr) == -1) {
        printf("sigaction to catch SIGSEGV failed.");
        exit(-1);
    }
    if (sigaction(SIGFPE, &sa, nullptr) == -1) {
        printf("sigaction to catch SIGFPE failed.");
        exit(-1);
    }
    if (sigaction(SIGABRT, &sa, nullptr) == -1) {
        printf("sigaction to catch SIGABRT failed.");
        exit(-1);
    }
}
#endif
