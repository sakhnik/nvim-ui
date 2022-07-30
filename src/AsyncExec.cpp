#include "AsyncExec.hpp"

AsyncExec::AsyncExec(uv_loop_t *loop)
    : _async{new uv_async_t}
{
    if (int err = uv_async_init(loop, _async.get(), _Execute))
        throw std::runtime_error(fmt::format("Failed to init async: {}", uv_strerror(err)));
    _async->data = this;
}

AsyncExec::~AsyncExec()
{
    auto nop = [](uv_handle_t *h) {
        delete reinterpret_cast<uv_async_t*>(h);
    };
    uv_close(reinterpret_cast<uv_handle_t *>(_async.release()), nop);
}

void AsyncExec::_Execute(uv_async_t *a)
{
    reinterpret_cast<AsyncExec *>(a->data)->_Execute2();
}

void AsyncExec::_Execute2()
{
    std::lock_guard<std::mutex> guard{_mut};
    for (const auto &t : _tasks)
        t();
    _tasks.clear();
}
