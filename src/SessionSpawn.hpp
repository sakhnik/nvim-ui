#pragma once

#include "Session.hpp"
#include <uv.h>

class SessionSpawn : public Session
{
public:
    SessionSpawn(int argc, char *argv[]);
    ~SessionSpawn() override;

    const std::string& GetDescription() const override;

private:
    std::string _description;
    std::unique_ptr<uv_pipe_t> _stdin_pipe, _stdout_pipe;
    std::unique_ptr<uv_process_t> _child_req;
};
