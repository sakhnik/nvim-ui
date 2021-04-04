#include "RedrawHandler.hpp"
#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include "Utils.hpp"


RedrawHandler::RedrawHandler(MsgPackRpc *rpc, Renderer *renderer)
    : _rpc{rpc}
    , _renderer{renderer}
{
    //_AddHlAttr(0, {}, {}, false, false);
}

void RedrawHandler::AttachUI()
{
    _rpc->OnNotifications(
        [this] (std::string method, const msgpack::object &obj) {
            _OnNotification(std::move(method), obj);
        }
    );

    _rpc->Request(
        [](auto &pk) {
            pk.pack(std::string{"nvim_ui_attach"});
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

void RedrawHandler::_OnNotification(std::string method, const msgpack::object &obj)
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
        std::string subtype = event.ptr[0].as<std::string>();
        if (subtype == "flush")
        {
            _renderer->Flush();
            continue;
        }
        //else if (subtype == "grid_cursor_goto")
        //{
        //    _GridCursorGoto(event);
        //}
        else if (subtype == "grid_line")
        {
            for_each_event(event, [this](const auto &e) { _GridLine(e); });
        }
        //else if (subtype == "grid_scroll")
        //{
        //    _GridScroll(event);
        //}
        //else if (subtype == "hl_attr_define")
        //{
        //    _HlAttrDefine(event);
        //}
        //else if (subtype == "default_colors_set")
        //{
        //    _HlDefaultColorsSet(event);
        //}
        else
        {
            std::cerr << "Ignoring redraw " << subtype << std::endl;
        }
    }
}

//void RedrawHandler::_GridCursorGoto(const msgpack::object_array &event)
//{
//    for (size_t j = 1; j < event.size; ++j)
//    {
//        const auto &inst = event.ptr[j].via.array;
//        int grid = inst.ptr[0].as<int>();
//        if (grid != 1)
//            throw std::runtime_error("Multigrid not supported");
//        //int row = inst.ptr[1].as<int>();
//        //int col = inst.ptr[2].as<int>();

//        //std::cout << "[" << (row+1) << ";" << (col+1) << "H";
//    }
//}

void RedrawHandler::_GridLine(const msgpack::object_array &event)
{
    int grid = event.ptr[0].as<int>();
    if (grid != 1)
        throw std::runtime_error("Multigrid not supported");
    int row = event.ptr[1].as<int>();
    int col = event.ptr[2].as<int>();
    const auto &cells = event.ptr[3].via.array;

    unsigned hl_id = 0;
    for (size_t c = 0; c < cells.size; ++c)
    {
        const auto &cell = cells.ptr[c].via.array;
        std::string text = cell.ptr[0].as<std::string>();
        if (cell.size > 1)
            hl_id = cell.ptr[1].as<unsigned>();
        int repeat = 1;
        // if repeat is greater than 1, we are guaranteed to send an hl_id
        // https://github.com/neovim/neovim/blob/master/src/nvim/api/ui.c#L483
        if (cell.size > 2)
            repeat = cell.ptr[2].as<int>();

        size_t count = Utf8Len(text.data()) * repeat;
        size_t len = text.size();
        while (--repeat)
        {
            text += std::string_view(text.data(), len);
        }

        _renderer->GridLine(row, col, text, hl_id);
        col += count;
    }
}

//void RedrawHandler::_GridScroll(const msgpack::object_array &event)
//{
//    for (size_t j = 1; j < event.size; ++j)
//    {
//        const auto &inst = event.ptr[j].via.array;
//        int grid = inst.ptr[0].as<int>();
//        if (grid != 1)
//            throw std::runtime_error("Multigrid not supported");
//        int top = inst.ptr[1].as<int>();
//        int bot = inst.ptr[2].as<int>();
//        int left = inst.ptr[3].as<int>();
//        int right = inst.ptr[4].as<int>();
//        int rows = inst.ptr[5].as<int>();
//        int cols = inst.ptr[6].as<int>();
//        if (cols)
//            throw std::runtime_error("Column scrolling not expected");

//        int start = 0;
//        int stop = 0;
//        int step = 0;

//        --bot;
//        if (rows > 0)
//        {
//            start = top;
//            stop = bot - rows + 1;
//            step = 1;
//        }
//        else if (rows < 0)
//        {
//            start = bot;
//            stop = top - rows - 1;
//            step = -1;
//        }
//        else
//            throw std::runtime_error("Rows should not equal 0");

//        // this is very inefficient, but there doesn't appear to be a curses function for extracting whole lines incl.
//        // attributes. another alternative would be to keep our own copy of the screen buffer
//        for (int r = start; r != stop; r += step)
//        {
//            //std::cout << "[" << (r+1) << ";" << (left+1) << "H";
//            size_t idx = r * _size.ws_col + left;
//            unsigned hl_id = _grid[idx].hl_id;
//            //std::cout << _attributes[hl_id]();
//            for (int c = left; c < right; ++c, ++idx)
//            {
//                if (hl_id != _grid[idx].hl_id)
//                {
//                    hl_id = _grid[idx].hl_id;
//                    //std::cout << _attributes[hl_id]();
//                }
//                //std::cout << _grid[idx].text;
//            }
//        }
//    }
//}

//void RedrawHandler::_HlDefaultColorsSet(const msgpack::object_array &event)
//{
//    const auto &inst = event.ptr[1].via.array;
//    _fg = inst.ptr[0].as<unsigned>();
//    _bg = inst.ptr[1].as<unsigned>();
//}

//void RedrawHandler::_HlAttrDefine(const msgpack::object_array &event)
//{
//    for (size_t j = 1; j < event.size; ++j)
//    {
//        const auto &inst = event.ptr[j].via.array;
//        unsigned hl_id = inst.ptr[0].as<unsigned>();
//        const auto &rgb_attr = inst.ptr[1].via.map;

//        std::optional<unsigned> fg, bg;
//        //const auto &cterm_attr = inst.ptr[2].via.map;
//        for (size_t i = 0; i < rgb_attr.size; ++i)
//        {
//            std::string key{rgb_attr.ptr[i].key.as<std::string>()};
//            if (key == "foreground")
//            {
//                fg = rgb_attr.ptr[i].val.as<unsigned>();
//            }
//            else if (key == "background")
//            {
//                bg = rgb_attr.ptr[i].val.as<unsigned>();
//            }
//        }
//        bool reverse{false};
//        bool bold{false};
//        // info = inst[3]
//        // nvim api docs state that boolean keys here are only sent if true
//        for (size_t i = 0; i < rgb_attr.size; ++i)
//        {
//            std::string key{rgb_attr.ptr[i].key.as<std::string>()};
//            if (key == "reverse")
//            {
//                reverse = true;
//            }
//            else if (key == "bold")
//            {
//                bold = true;
//            }
//        }
//        _AddHlAttr(hl_id, fg, bg, bold, reverse);
//    }
//}

//void RedrawHandler::_AddHlAttr(unsigned hl_id, std::optional<unsigned> fg, std::optional<unsigned> bg, bool bold, bool reverse)
//{
//    auto attr_to_string = [this, fg, bg, bold, reverse] {
//        std::ostringstream oss;
//        auto add_rgb = [&](unsigned rgb) {
//            oss << ";" << (rgb >> 16)
//                << ";" << ((rgb >> 8) & 0xff)
//                << ";" << (rgb & 0xff);
//        };
//        oss << "[38;2";
//        if (fg)
//            add_rgb(*fg);
//        else
//            add_rgb(_fg);
//        oss << ";48;2";
//        if (bg)
//            add_rgb(*bg);
//        else
//            add_rgb(_bg);
//        if (bold)
//            oss << ";1";
//        if (reverse)
//            oss << ";7";
//        oss << "m";
//        return oss.str();
//    };
//    _attributes[hl_id] = attr_to_string;
//}
