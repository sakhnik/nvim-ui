#pragma once

#include "HlAttr.hpp"
#include "GridLine.hpp"
#include "AsyncExec.hpp"
#include "Timer.hpp"
#include "Utils.hpp"

#include <vector>
#include <string_view>
#include <string>
#include <mutex>

class MsgPackRpc;
struct IWindow;

class Renderer
{
public:
    Renderer(uv_loop_t *, MsgPackRpc *);
    ~Renderer();

    void SetWindow(IWindow *);

    // Get current grid cell dimensions
    int GetHeight() const { return _lines.size(); }
    int GetWidth() const { return _lines[0].hl_id.size(); }

    void Flush();

    // Window was resized
    void OnResized(int rows, int cols);

    void GridLine(int row, int col, std::string_view chunk, unsigned hl_id, int repeat);
    void GridCursorGoto(int row, int col);
    void GridScroll(int top, int bot, int left, int right, int rows);
    void GridResize(int width, int height);
    void GridClear();
    void HlAttrDefine(unsigned hl_id, HlAttr attr);
    void DefaultColorSet(unsigned fg, unsigned bg);
    void ModeChange(std::string_view mode);
    void SetBusy(bool is_busy);
    void SetGuiFont(std::string_view);

    // The snapshot of last consistent grid state
    using ChunkT = GridLine::Chunk::PtrT;
    using GridLinesT = std::vector<ChunkT>;
    const GridLinesT& GetGridLines() const { return _grid_lines; }

    std::lock_guard<std::mutex> Lock()
    {
        return std::lock_guard<std::mutex>{_mutex};
    }

    const HlAttr::MapT& GetAttrMap() const { return _hl_attr_map; }
    bool IsAttrMapModified() const { return _hl_attr_modified; }
    void MarkAttrMapProcessed() { _hl_attr_modified = false; }
    unsigned GetBg() const { return _def_attr.bg.value(); }
    unsigned GetFg() const { return _def_attr.fg.value(); }
    const HlAttr& GetDefAttr() const { return _def_attr; }

    bool IsBusy() const { return _is_busy; }
    int GetCursorRow() const { return _cursor_row; }
    int GetCursorCol() const { return _cursor_col; }
    const std::string& GetMode() const { return _mode; }

private:
    MsgPackRpc *_rpc;
    Timer _timer;
    AsyncExec _async_exec;
    IWindow *_window = nullptr;

    HlAttr::MapT _hl_attr_map;
    bool _hl_attr_modified = false;
    HlAttr _def_attr;
    int _cursor_row = 0;
    int _cursor_col = 0;
    std::string _mode;
    bool _is_busy = false;

    struct _Line
    {
        std::vector<std::string> text;
        std::vector<unsigned> hl_id;
        // Is it necessary to redraw this line carefully or can just draw from the texture cache?
        bool dirty = true;
    };

    // The volatile state of the grid, the changes are collected here first
    // before going to _grid_lines;
    std::vector<_Line> _lines;

    GridLinesT _grid_lines;
    std::mutex _mutex;

    static std::vector<size_t> _SplitChunks(const _Line &);

    // Make sure flush requests are executed not too frequently,
    // but cleanly.
    ClockT::time_point _last_flush_time;
    // Rendering is done asyncrhonously and concurrently, make sure only clean
    // state after Flush() is displayed.
    bool _is_clean = true;

    void _DoFlush();
    void _AnticipateFlush();
};
