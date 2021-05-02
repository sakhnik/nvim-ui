#pragma once

#include "Timer.hpp"

#include <vector>
#include <unordered_map>
#include <glib.h>
#include <uv.h>

class GlibLoop
{
public:
    GlibLoop(uv_loop_t *, GMainContext * = g_main_context_default());

    void Prepare();

private:
    uv_loop_t *_loop;
    GMainContext *_context;
    Timer _timer;

    gint _max_priority = 0;
    std::vector<GPollFD> _fds;
    gint _fds_count = 0;

    struct _Poll
    {
        decltype(_fds[0].fd) fd;
        uv_poll_t poll;
        int gio_events = 0;
    };
    std::vector<_Poll> _polls;

    void _Check();
    void _OnTimeout();
    static void _OnPollReady(uv_poll_t *, int status, int events);
    void _OnPollReady2(uv_poll_t *, int status, int events);
};
