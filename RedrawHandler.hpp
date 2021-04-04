#pragma once

#include <stdexcept>
#include <iostream>
#include <optional>
#include <functional>
#include <msgpack/object_fwd.hpp>


class MsgPackRpc;
class Renderer;


class RedrawHandler
{
public:
    RedrawHandler(MsgPackRpc *, Renderer *);

    void AttachUI();

private:
    MsgPackRpc *_rpc;
    Renderer *_renderer;
    unsigned _fg{0xffffff};
    unsigned _bg{0};
    std::unordered_map<unsigned, std::function<std::string(void)>> _attributes;


    void _OnNotification(std::string method, const msgpack::object &obj);

    //void _GridCursorGoto(const msgpack::object_array &event);
    void _GridLine(const msgpack::object_array &event);
    //void _GridScroll(const msgpack::object_array &event);
    //void _HlDefaultColorsSet(const msgpack::object_array &event);
    //void _HlAttrDefine(const msgpack::object_array &event);
    //void _AddHlAttr(unsigned hl_id, std::optional<unsigned> fg, std::optional<unsigned> bg, bool bold, bool reverse);
};
