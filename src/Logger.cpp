#include "Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

spdlog::logger& Logger()
{
#ifndef _WIN32
    static auto logger = spdlog::stderr_color_st("nvim-ui");
#else
    static auto sink = std::make_shared<spdlog::sinks::msvc_sink_st>();
    static auto logger = std::make_shared<spdlog::logger>("nvim-ui", sink);
#endif
    return *logger.get();
}
