#pragma once

#include <string_view>
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

    void _OnNotification(std::string_view method, const msgpack::object &obj);

    void _GridCursorGoto(const msgpack::object_array &event);
    void _GridLine(const msgpack::object_array &event);
    void _GridScroll(const msgpack::object_array &event);
    void _GridClear(const msgpack::object_array &event);
    void _HlAttrDefine(const msgpack::object_array &event);
    void _GridResize(const msgpack::object_array &event);
    void _ModeChange(const msgpack::object_array &event);

    void _SetGuifont(std::string_view);
};
