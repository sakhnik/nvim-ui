#pragma once

#include <functional>
#include <uv.h>

class Timer
{
public:
    Timer(uv_loop_t *);
    ~Timer();

    using ActionT = std::function<void(void)>;
    void Start(int delay, int period, ActionT action);
    void Stop();

private:
    uv_timer_t _timer;
    ActionT _action;
};
