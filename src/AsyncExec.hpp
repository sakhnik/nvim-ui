#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include <fmt/format.h>
#include <uv.h>

class AsyncExec
{
public:
    AsyncExec(uv_loop_t *);
    ~AsyncExec();

    using TaskT = std::function<void(void)>;

    template <typename T>
    void Post(T &&task)
    {
        {
            std::lock_guard<std::mutex> guard{_mut};
            _tasks.push_back(std::forward<T>(task));
        }
        if (int err = uv_async_send(_async.get()))
            throw std::runtime_error(fmt::format("Failed to send async: {}", uv_strerror(err)));
    }

private:
    std::unique_ptr<uv_async_t> _async;
    std::vector<TaskT> _tasks;
    std::mutex _mut;

    static void _Execute(uv_async_t *);
    void _Execute2();
};
