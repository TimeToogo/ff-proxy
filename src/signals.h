#include <signal.h>

#ifndef FF_SIGNALS_H
#define FF_SIGNALS_H

volatile sig_atomic_t interrupt_fired = 0;

void ff_sigint_handler(int signal);

#endif