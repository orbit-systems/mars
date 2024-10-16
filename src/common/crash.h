#pragma once

#ifndef __WIN32__
#    include <execinfo.h>
#    include <signal.h>
#    include <errno.h>

void signal_handler(int sig, siginfo_t* info, void* ucontext);
void init_signal_handler();
#endif

void crash(char* error, ...);