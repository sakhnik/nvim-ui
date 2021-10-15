#pragma once

#include "Session.hpp"
#include <uv.h>

class SessionSpawn : public Session
{
public:
    SessionSpawn(int argc, char *argv[]);

private:
    uv_pipe_t _stdin_pipe, _stdout_pipe;
    uv_process_t _child_req;
};
