#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <signal.h>

template<int SIGNAL>
class SignalHandler {
    struct sigaction new_action_;
    struct sigaction old_action_;

public:
    explicit SignalHandler(void(*handler)(int)) {
        new_action_.sa_handler = handler;
        sigemptyset(&new_action_.sa_mask);
        new_action_.sa_flags = 0;
        sigaction(SIGNAL, NULL, &old_action_);
        if (old_action_.sa_handler != SIG_IGN) sigaction(SIGNAL, &new_action_, NULL);
    }

    ~SignalHandler() {
        sigaction(SIGNAL, &old_action_, NULL);
    }
};

#endif