#include "Timer.hpp"

Timer::Timer(uv_loop_t *loop)
{
    uv_timer_init(loop, &_timer);
    _timer.data = this;
}

Timer::~Timer()
{
    uv_timer_stop(&_timer);
    auto nop = [](uv_handle_t *) { };
    uv_close(reinterpret_cast<uv_handle_t *>(&_timer), nop);
}

void Timer::Start(int delay, int period, ActionT action)
{
    auto on_timeout = [](uv_timer_t *handle) {
        Timer *self = reinterpret_cast<Timer*>(handle->data);
        self->_action();
    };

    _action = action;
    uv_timer_start(&_timer, on_timeout, delay, period);
}

void Timer::Stop()
{
    uv_timer_stop(&_timer);
}
