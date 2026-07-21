#if !defined(SIGACTION_H)
#define SIGACTION_H

#include <signal.h>

extern volatile sig_atomic_t caught_sigint;
extern volatile sig_atomic_t caught_sigterm;

#if 1
void signal_handler(int signal_number);
void init_sigaction(struct sigaction* action, void (*sig_handler)(int));
bool register_sigaction(struct sigaction* action);
void log_sigaction();
#endif

#endif