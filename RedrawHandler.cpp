#include "RedrawHandler.hpp"
#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include "Utils.hpp"
#include <iostream>


RedrawHandler::RedrawHandler(MsgPackRpc *rpc, Renderer *renderer)
    : _rpc{rpc}
    , _renderer{renderer}
{
}

void RedrawHandler::AttachUI()
{
    _rpc->OnNotifications(
        [this] (std::string_view method, const msgpack::object &obj) {
            _OnNotification(method, obj);
        }
    );

    _rpc->Request(
        [](auto &pk) {
            pk.pack("nvim_ui_attach");
            pk.pack_array(3);
            pk.pack(80);
            pk.pack(25);
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

void RedrawHandler::_OnNotification(std::string_view method, const msgpack::object &obj)
{
    if (method != "redraw")
    {
        std::cerr << "Unexpected notification " << method << std::endl;
        return;
    }

    auto for_each_event = [](const msgpack::object_array &event, const auto &handler) {
        for (size_t j = 1; j < event.size; ++j)
        {
            const auto &inst = event.ptr[j].via.array;
            handler(inst);
        }
    };

    const auto &arr = obj.via.array;
    for (size_t i = 0; i < arr.size; ++i)
    {
        const auto &event = arr.ptr[i].via.array;
        std::string_view subtype = event.ptr[0].as<std::string_view>();
        if (subtype == "flush")
        {
            _renderer->Flush();
        }
        else if (subtype == "grid_cursor_goto")
        {
            for_each_event(event, [this](const auto &e) { _GridCursorGoto(e); });
        }
        else if (subtype == "grid_line")
        {
            for_each_event(event, [this](const auto &e) { _GridLine(e); });
        }
        else if (subtype == "grid_scroll")
        {
            for_each_event(event, [this](const auto &e) { _GridScroll(e); });
        }
        else if (subtype == "hl_attr_define")
        {
            for_each_event(event, [this](const auto &e) { _HlAttrDefine(e); });
        }
        else if (subtype == "win_viewport")
        {
            // Just informational, skip
        }
        else if (subtype == "default_colors_set")
        {
            const auto &inst = event.ptr[1].via.array;
            unsigned fg = inst.ptr[0].as<unsigned>();
            unsigned bg = inst.ptr[1].as<unsigned>();
            _renderer->DefaultColorSet(fg, bg);
        }
        else
        {
            std::cerr << "Ignoring redraw " << subtype << std::endl;
        }
    }
}

void RedrawHandler::_GridCursorGoto(const msgpack::object_array &event)
{
    int grid = event.ptr[0].as<int>();
    if (grid != 1)
        throw std::runtime_error("Multigrid not supported");
    int row = event.ptr[1].as<int>();
    int col = event.ptr[2].as<int>();
    _renderer->GridCursorGoto(row, col);
}

void RedrawHandler::_GridLine(const msgpack::object_array &event)
{
    int grid = event.ptr[0].as<int>();
    if (grid != 1)
        throw std::runtime_error("Multigrid not supported");
    int row = event.ptr[1].as<int>();
    int col = event.ptr[2].as<int>();
    const auto &cells = event.ptr[3].via.array;
    // Group text chunks by hl_id
    std::string text;

    unsigned hl_id = 0;
    for (size_t c = 0; c < cells.size; ++c)
    {
        const auto &cell = cells.ptr[c].via.array;
        std::string_view chars = cell.ptr[0].as<std::string_view>();
        if (cell.size > 1)
        {
            unsigned new_hl_id = cell.ptr[1].as<unsigned>();
            if (new_hl_id != hl_id && !text.empty())
            {
                col += _renderer->GridLine(row, col, text, hl_id);
                text.clear();
            }
            hl_id = new_hl_id;
        }
        // if repeat is greater than 1, we are guaranteed to send an hl_id
        // https://github.com/neovim/neovim/blob/master/src/nvim/api/ui.c#L483
        int repeat = 1;
        if (cell.size > 2)
            repeat = cell.ptr[2].as<int>();

        while (repeat--)
            text += chars;
    }

    col += _renderer->GridLine(row, col, text, hl_id);
}

void RedrawHandler::_GridScroll(const msgpack::object_array &event)
{
    int grid = event.ptr[0].as<int>();
    if (grid != 1)
        throw std::runtime_error("Multigrid not supported");
    int top = event.ptr[1].as<int>();
    int bot = event.ptr[2].as<int>();
    int left = event.ptr[3].as<int>();
    int right = event.ptr[4].as<int>();
    int rows = event.ptr[5].as<int>();
    int cols = event.ptr[6].as<int>();
    if (cols)
        throw std::runtime_error("Column scrolling not expected");

    _renderer->GridScroll(top, bot, left, right, rows);
}

void RedrawHandler::_HlAttrDefine(const msgpack::object_array &event)
{
    unsigned hl_id = event.ptr[0].as<unsigned>();
    const auto &rgb_attr = event.ptr[1].via.map;

    Renderer::HlAttr attr;
    for (size_t i = 0; i < rgb_attr.size; ++i)
    {
        std::string_view key{rgb_attr.ptr[i].key.as<std::string_view>()};
        if (key == "foreground")
            attr.fg = rgb_attr.ptr[i].val.as<unsigned>();
        else if (key == "background")
            attr.bg = rgb_attr.ptr[i].val.as<unsigned>();
        // nvim api docs state that boolean keys here are only sent if true
        else if (key == "reverse")
            attr.flags |= Renderer::HF_REVERSE;
        //else if (key == "bold")
        //    attr.flags |= Renderer::HF_BOLD;
        //else if (key == "italic")
        //    attr.flags |= Renderer::HF_ITALIC;
        //else if (key == "underline")
        //    attr.flags |= Renderer::HF_UNDERLINE;
        //else if (key == "undercurl")
        //    attr.flags |= Renderer::HF_UNDERCURL;
        else
            std::cerr << "Unknown rgb attribute: " << key << std::endl;
    }
    // info = inst[3]
    _renderer->HlAttrDefine(hl_id, attr);
}
