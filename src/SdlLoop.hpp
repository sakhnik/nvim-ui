#pragma once

#include "Timer.hpp"
#include <uv.h>

class Renderer;

class SdlLoop
{
public:
    SdlLoop(uv_loop_t *, Renderer *);
    ~SdlLoop();

    void Start();

private:
    Renderer *_renderer;
    Timer _timer;
    bool _shift = false;
    bool _control = false;

    void _StartTimer();
    void _PollEvents();
    void _RawInput(const char *input);
};
