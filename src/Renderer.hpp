#pragma once

#include "HlAttr.hpp"
#include "TextureCache.hpp"

#include <vector>
#include <unordered_map>
#include <string_view>
#include <string>
#include <chrono>

class MsgPackRpc;
struct IWindow;
class Timer;

class Renderer
{
public:
    Renderer(MsgPackRpc *, IWindow *, Timer *);
    ~Renderer();

    // Get current grid cell dimensions
    int GetHeight() const { return _lines.size(); }
    int GetWidth() const { return _lines[0].hl_id.size(); }

    void Flush();

    // Window was resized
    void OnResized();

    void GridLine(int row, int col, std::string_view chunk, unsigned hl_id, int repeat);
    void GridCursorGoto(int row, int col);
    void GridScroll(int top, int bot, int left, int right, int rows);
    void GridResize(int width, int height);
    void GridClear();
    void HlAttrDefine(unsigned hl_id, HlAttr attr);
    void DefaultColorSet(unsigned fg, unsigned bg);
    void ModeChange(std::string_view mode);
    void SetBusy(bool is_busy);

private:
    MsgPackRpc *_rpc;
    IWindow *_window;
    Timer *_timer;

    std::unordered_map<unsigned, HlAttr> _hl_attr;
    HlAttr _def_attr;
    int _cursor_row = 0;
    int _cursor_col = 0;
    std::string _mode;

    struct _Line
    {
        std::vector<std::string> text;
        std::vector<unsigned> hl_id;
        // Remember the previously rendered textures, high chance they're reusable.
        TextureCache texture_cache;
        // Is it necessary to redraw this line carefully or can just draw from the texture cache?
        bool dirty = true;
    };

    std::vector<_Line> _lines;

    static size_t _SplitChunks(const _Line &, size_t chunks[]);

    // Make sure flush requests are executed not too frequently,
    // but cleanly.
    using ClockT = std::chrono::high_resolution_clock;
    ClockT::time_point _last_flush_time;

    void _DoFlush();
    void _AnticipateFlush();
};
