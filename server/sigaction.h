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
int create_sigaction(struct sigaction* action);
#endif

// Blocks SIGINT/SIGTERM on the calling thread, saving the previous mask to
// *old_set. Call this before spawning a thread so the new thread
// inherits a mask where these signals are blocked
void block_termination_signals(sigset_t *old_set);

// Restores a mask previously saved by block_termination_signals(). Call this
// on the main thread right before it needs to be interruptible again.
void restore_signal_mask(const sigset_t *old_set);

#endif