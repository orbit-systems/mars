#include "crash.h"
#include "orbit.h"

void crash(char* error, ...) {
    printf("INTERNAL COMPILER ERROR: ");
    va_list args;
    va_start(args, error);
    vprintf(error, args);
    va_end(args);

#ifndef __WIN32__
    void* array[256]; // hold 256 stack traces
    char** strings;
    int size;

    size = backtrace(array, 256);
    strings = backtrace_symbols(array, size);

    if (strings != NULL) {
        printf("Obtained %d stack frames\n", size);
        for (int i = 0; i < size; i++) {
            string output_str = str(strings[i]);
            if (output_str.raw[0] == '.') {
                // we need to trim
                for (; output_str.raw[0] != '('; output_str.raw++)
                    ;
                output_str.raw++;
                int close_bracket = 0;
                for (; output_str.raw[close_bracket] != ')'; close_bracket++)
                    ;
                output_str.len = close_bracket;
            }
            printf("frame %d:\t" str_fmt "\n", i, str_arg(output_str));
        }
    }

    free(strings); // this is partially unsafe?
#endif
    exit(-1); // lmao
}

#ifndef __WIN32__
void signal_handler(int sig, siginfo_t* info, void* ucontext) {
    ucontext = ucontext;
    switch (sig) {
    case SIGSEGV:
        crash("SIGSEGV at addr 0x%lx\n", (long)info->si_addr);

    case SIGINT:
        printf("Debug interupt caught");
        return;

    case SIGFPE:
        printf("Fatal arithmetic error! (its probably a division by zero)");
        crash("");

    default:
        crash("Unhandled signal %s caught\n", strsignal(sig));
    }
}

void init_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigfillset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        printf("sigaction to catch SIGSEGV failed.");
        exit(-1);
    }
    if (sigaction(SIGFPE, &sa, NULL) == -1) {
        printf("sigaction to catch SIGFPE failed.");
        exit(-1);
    }
}
#endif
