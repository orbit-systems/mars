#pragma once
#define ARENA_H

#include <execinfo.h>
#include <signal.h>
#include <errno.h>

void crash(char* error, ...);
void signal_handler(int sig, siginfo_t *info, void *ucontext);
void init_signal_handler();