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
    std::unordered_map<int, int> _attributes;

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
            if (subtype == "flush")
            {
                ::wrefresh(_wnd);
            }
            else if (subtype == "grid_cursor_goto")
            {
                _GridCursorGoto(event);
            }
            else if (subtype == "grid_line")
            {
                int y{}, x{};
                getyx(_wnd, y, x);
                _GridLine(event);
                ::wmove(_wnd, y, x);
            }
            else if (subtype == "grid_scroll")
            {
                int y{}, x{};
                getyx(_wnd, y, x);
                _GridScroll(event);
                ::wmove(_wnd, y, x);
            }
            else if (subtype == "hl_attr_define")
            {
                _HlAttrDefine(event);
            }
            else
                ofs << subtype << " " << event.size << std::endl;
        }

    }

    void _GridCursorGoto(const msgpack::object_array &event)
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

    void _GridLine(const msgpack::object_array &event)
    {
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
                auto attr = COLOR_PAIR(hl_id);
                auto it = _attributes.find(hl_id);
                if (it != _attributes.end())
                    attr = it->second;
                ::wattron(_wnd, attr);
                size_t char_count = std::count_if(text.begin(), text.end(),
                                                  [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; });
                col += repeat * char_count;
                while (repeat--)
                {
                    ::waddstr(_wnd, text.c_str());
                }
                ::wattroff(_wnd, attr);
            }
        }
    }

    void _GridScroll(const msgpack::object_array &event)
    {
        for (size_t j = 1; j < event.size; ++j)
        {
            const auto &inst = event.ptr[j].via.array;
            int grid = inst.ptr[0].as<int>();
            if (grid != 1)
                throw std::runtime_error("Multigrid not supported");
            int top = inst.ptr[1].as<int>();
            int bot = inst.ptr[2].as<int>();
            int left = inst.ptr[3].as<int>();
            int right = inst.ptr[4].as<int>();
            int rows = inst.ptr[5].as<int>();
            int cols = inst.ptr[6].as<int>();
            if (cols)
                throw std::runtime_error("Column scrolling not expected");

            int start = 0;
            int stop = 0;
            int step = 0;

            --bot;
            if (rows > 0)
            {
                start = top;
                stop = bot - rows + 1;
                step = 1;
            }
            else if (rows < 0)
            {
                start = bot;
                stop = top - rows - 1;
                step = -1;
            }
            else
                throw std::runtime_error("Rows should not equal 0");

            // this is very inefficient, but there doesn't appear to be a curses function for extracting whole lines incl.
            // attributes. another alternative would be to keep our own copy of the screen buffer
            for (int r = start; r != stop; r += step)
            {
                for (int c = left; c < right; ++c)
                {
                    chtype ch = mvwinch(_wnd, r + rows, c);
                    mvwaddch(_wnd, r, c, ch);
                }
            }
        }
    }

    void _HlAttrDefine(const msgpack::object_array &event)
    {
        for (size_t j = 1; j < event.size; ++j)
        {
            const auto &inst = event.ptr[j].via.array;
            int hl_id = inst.ptr[0].as<int>();
            // rgb_attr = inst[1]
            const auto &cterm_attr = inst.ptr[2].via.map;
            int fg{-1};
            int bg{-1};
            for (size_t i = 0; i < cterm_attr.size; ++i)
            {
                std::string key{cterm_attr.ptr[i].key.as<std::string>()};
                if (key == "foreground")
                    fg = cterm_attr.ptr[i].val.as<int>();
                else if (key == "background")
                    bg = cterm_attr.ptr[i].val.as<int>();
            }
            // info = inst[3]
            ::init_pair(hl_id, fg, bg);
            int attr = COLOR_PAIR(hl_id);
            // nvim api docs state that boolean keys here are only sent if true
            for (size_t i = 0; i < cterm_attr.size; ++i)
            {
                std::string key{cterm_attr.ptr[i].key.as<std::string>()};
                if (key == "reverse")
                    attr |= A_REVERSE;
                else if (key == "bold")
                    attr |= A_BOLD;
            }
            _attributes[hl_id] = attr;
        }
    }
};
