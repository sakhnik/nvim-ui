#include "Timer.hpp"
#include "Logger.hpp"

Timer::Timer(uv_loop_t *loop)
{
    if (int err = uv_timer_init(loop, &_timer))
        throw std::runtime_error(fmt::format("Failed to init timer: {}", uv_strerror(err)));
    _timer.data = this;
}

Timer::~Timer()
{
    if (int err = uv_timer_stop(&_timer))
        Logger().warn("~Timer: failed to stop timer: {}", uv_strerror(err));
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
    if (int err = uv_timer_start(&_timer, on_timeout, delay, period))
        throw std::runtime_error(fmt::format("Failed to start timer: {}", uv_strerror(err)));
}

void Timer::Stop()
{
    if (int err = uv_timer_stop(&_timer))
        Logger().error("Failed to stop timer: {}", uv_strerror(err));
}
