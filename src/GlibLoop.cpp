#include "GlibLoop.hpp"
#include "Logger.hpp"
#include <stdexcept>

GlibLoop::GlibLoop(uv_loop_t *loop, GMainContext *context)
    : _loop{loop}
    , _context{context}
    , _timer{loop}
{
}

namespace {

int GlibToUv(int events)
{
    int res{0};
    if ((events & G_IO_IN))
        res |= UV_READABLE;
    if ((events & G_IO_OUT))
        res |= UV_WRITABLE;
    if ((events & G_IO_HUP))
        res |= UV_DISCONNECT;
    if ((events & G_IO_PRI))
        res |= UV_PRIORITIZED;
    return res;
}

int UvToGlib(int events)
{
    int res{0};
    if ((events & UV_READABLE))
        res |= G_IO_IN;
    if ((events & UV_WRITABLE))
        res |= G_IO_OUT;
    if ((events & UV_DISCONNECT))
        res |= G_IO_HUP;
    if ((events & UV_PRIORITIZED))
        res |= G_IO_PRI;
    return res;
}

} //namespace

void GlibLoop::Prepare()
{
    if (!::g_main_context_acquire(_context))
        throw std::runtime_error("Couldn't acquire main context");
    while (::g_main_context_prepare(_context, &_max_priority))
        ::g_main_context_dispatch(_context);
    gint timeout{0};
    _fds_count = ::g_main_context_query(_context, _max_priority, &timeout, _fds.data(), _fds.size());
    if (static_cast<size_t>(_fds_count) > _fds.size())
    {
        _fds.resize(_fds_count);
        _fds_count = ::g_main_context_query(_context, _max_priority, &timeout, _fds.data(), _fds.size());
    }
    _timer.Start(timeout, 0, [this] { _OnTimeout(); });

    auto start_poll = [&](int fd, int events) {
        _polls.emplace_back();
        auto &poll = _polls.back();
        poll.fd = fd;
        poll.gio_events = events;

        ::uv_poll_init_socket(_loop, &poll.poll, fd);
        poll.poll.data = this;
        ::uv_poll_start(&poll.poll, GlibToUv(events), _OnPollReady);
    };

    for (int i = 0; i < _fds_count; ++i)
    {
        auto fd = _fds[i].fd;
        int gio_events = _fds[i].events;

        auto it = _polls.begin() + i;
        if (it == _polls.end())
        {
            start_poll(fd, _fds[i].events);
            continue;
        }

        auto it2 = std::find_if(it, _polls.end(),
                               [fd](const _Poll &p) {
                                   return p.fd == fd;
                               });
        if (it != it2)
        {
            std::for_each(it, it2, [](_Poll &p) { ::uv_poll_stop(&p.poll); });
            _polls.erase(it, it2);
        }
        if (it == _polls.end())
        {
            start_poll(fd, gio_events);
        }
        else
        {
            if (it->gio_events != gio_events)
                Logger().error("Polling events changed {:x} -> {:x}, not implemented",
                               it->gio_events, gio_events);
        }

        // Assume for now that the events set doesn't change once set.
    }

    ::g_main_context_release(_context);
}

void GlibLoop::_Check()
{
    _timer.Stop();

    if (!::g_main_context_acquire(_context))
        throw std::runtime_error("Couldn't acquire main context");
    if (::g_main_context_check(_context, _max_priority, _fds.data(), _fds_count))
        ::g_main_context_dispatch(_context);
    Prepare();
    ::g_main_context_release(_context);
}

void GlibLoop::_OnTimeout()
{
    std::for_each(_fds.begin(), _fds.begin() + _fds_count,
                  [](GPollFD &fd) { fd.revents = 0; });
    _Check();
}

void GlibLoop::_OnPollReady(uv_poll_t *poll, int status, int events)
{
    static_cast<GlibLoop *>(poll->data)->_OnPollReady2(poll, status, events);
}

void GlibLoop::_OnPollReady2(uv_poll_t *poll, int status, int events)
{
    std::for_each(_fds.begin(), _fds.begin() + _fds_count,
                  [](GPollFD &fd) { fd.revents = 0; });

    auto it = std::find_if(_polls.begin(), _polls.end(),
                           [poll](const _Poll &p) { return &p.poll == poll; });
    assert(it != _polls.end());
    if (status < 0)
        throw std::runtime_error("Polling error");

    int idx = it - _polls.begin();
    _fds[idx].revents = UvToGlib(events);
    _Check();
}
