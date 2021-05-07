#include "AsyncExec.hpp"

AsyncExec::AsyncExec(uv_loop_t *loop)
{
    uv_async_init(loop, &_async, _Execute);
    _async.data = this;
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