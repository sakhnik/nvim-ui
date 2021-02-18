#pragma once

#include <chrono>
#include <string>
#include <iostream>

struct Timer
{
    std::string _msg;
    using PointT = std::chrono::time_point<std::chrono::system_clock>;
    PointT _start;

    Timer(std::string msg)
        : _msg(msg)
    {
        _start = std::chrono::system_clock::now();
        ofs << "> " << _msg << std::endl;
    }

    ~Timer()
    {
        auto finish = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = finish - _start;
        ofs << "< " << _msg << " " << diff.count() << std::endl;
    }
};
