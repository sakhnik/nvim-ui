#pragma once

#include <thread>
#include <uv.h>

class UvLoop
{
public:
    UvLoop();
    ~UvLoop();

    void RunAsync();

protected:
    uv_loop_t _loop;
    std::unique_ptr<std::thread> _thread;
};
