#pragma once

#include "MsgPackRpc.hpp"
#include <stdexcept>
#include <fstream>
#include <curses.h>

std::ofstream ofs("/tmp/n.log");

class Renderer
{
public:
    Renderer(MsgPackRpc *rpc)
        : _rpc{rpc}
    {
        _wnd = ::initscr();
        if (!_wnd)
            throw std::runtime_error("Couldn't init screen");

        ::keypad (stdscr, TRUE);
        ::raw();
        ::noecho();
    }

    ~Renderer()
    {
        ::endwin();
    }

    void AttachUI()
    {
        _rpc->OnNotifications(
            [this] (std::string method, const msgpack::object &obj) {
                _OnNotification(std::move(method), obj);
            }
        );

        int width{}, height{};
        getmaxyx(_wnd, height, width);

        _rpc->Request(
            [=](auto &pk) {
                pk.pack(std::string{"nvim_ui_attach"});
                pk.pack_array(3);
                pk.pack(width);
                pk.pack(height);
                pk.pack_map(2);
                pk.pack("rgb");
                pk.pack(true);
                pk.pack("ext_linegrid");
                pk.pack(true);
            },
            [](const msgpack::object &err, const auto &resp) {
                if (!err.is_nil())
                {
                    std::ostringstream oss;
                    oss << "Failed to attach UI " << err << std::endl;
                    throw std::runtime_error(oss.str());
                }
            }
        );
    }

private:
    MsgPackRpc *_rpc;
    WINDOW *_wnd;

    void _OnNotification(std::string method, const msgpack::object &obj)
    {
        if (method != "redraw")
        {
            ofs << "Notification " << method << " " << obj << std::endl;
            return;
        }

        const auto &arr = obj.via.array;
        for (size_t i = 0; i < arr.size; ++i)
        {
            const auto &event = arr.ptr[i].via.array;
            std::string subtype = event.ptr[0].as<std::string>();
            ofs << subtype << " " << event.size << std::endl;
            if (subtype == "flush")
            {
                ::wrefresh(_wnd);
            }
            else if (subtype == "grid_cursor_goto")
            {
                for (size_t j = 1; j < event.size; ++j)
                {
                    const auto &inst = event.ptr[j].via.array;
                    int grid = inst.ptr[0].as<int>();
                    if (grid != 1)
                        throw std::runtime_error("Multigrid not supported");
                    int row = inst.ptr[1].as<int>();
                    int col = inst.ptr[2].as<int>();

                    ::wmove(_wnd, row, col);
                }
            }
            else if (subtype == "grid_line")
            {
                int y{}, x{};
                getyx(_wnd, y, x);

                for (size_t j = 1; j < event.size; ++j)
                {
                    const auto &inst = event.ptr[j].via.array;
                    int grid = inst.ptr[0].as<int>();
                    if (grid != 1)
                        throw std::runtime_error("Multigrid not supported");
                    int row = inst.ptr[1].as<int>();
                    int col = inst.ptr[2].as<int>();
                    const auto &cells = inst.ptr[3].via.array;

                    int hl_id;
                    for (size_t c = 0; c < cells.size; ++c)
                    {
                        const auto &cell = cells.ptr[c].via.array;
                        int repeat = 1;
                        std::string text = cell.ptr[0].as<std::string>();
                        // if repeat is greater than 1, we are guaranteed to send an hl_id
                        // https://github.com/neovim/neovim/blob/master/src/nvim/api/ui.c#L483
                        if (cell.size > 1)
                            hl_id = cell.ptr[1].as<int>();
                        if (cell.size > 2)
                            repeat = cell.ptr[2].as<int>();
                        ::wmove(_wnd, row, col);
                        size_t char_count = std::count_if(text.begin(), text.end(),
                                                          [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; });
                        col += repeat * char_count;
                        while (repeat--)
                        {
//                attr = self.attributes.get(hl_id, curses.color_pair(hl_id))
                            ::waddstr(_wnd, text.c_str());
                        }
                    }
                }

                ::wmove(_wnd, y, x);
            }
        }

    }
};
