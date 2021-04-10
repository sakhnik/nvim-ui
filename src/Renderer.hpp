#pragma once

#include "Utils.hpp"
#include "Painter.hpp"
#include "HlAttr.hpp"

#include <vector>
#include <list>
#include <unordered_map>
#include <string_view>
#include <string>
#include <SDL2/SDL.h>

class MsgPackRpc;

class Renderer
{
public:
    Renderer(MsgPackRpc *);
    ~Renderer();

    // Get current grid cell dimensions
    int GetHeight() const { return _lines.size(); }
    int GetWidth() const { return _lines[0].hl_id.size(); }

    void Flush();

    // Window was resized
    void OnResized();

    void GridLine(int row, int col, std::string_view chunk, unsigned hl_id, int repeat);
    void HlAttrDefine(unsigned hl_id, HlAttr attr);
    void DefaultColorSet(unsigned fg, unsigned bg);
    void GridCursorGoto(int row, int col);
    void GridScroll(int top, int bot, int left, int right, int rows);
    void GridResize(int width, int height);
    void ModeChange(std::string_view mode);

private:
    MsgPackRpc *_rpc;
    PtrT<SDL_Window> _window = NullPtr(SDL_DestroyWindow);
    PtrT<SDL_Renderer> _renderer = NullPtr(SDL_DestroyRenderer);
    std::unique_ptr<Painter> _painter;

    double _scale_x = 1.0;
    double _scale_y = 1.0;
    std::unordered_map<unsigned, HlAttr> _hl_attr;
    HlAttr _def_attr;
    int _cursor_row = 0;
    int _cursor_col = 0;
    std::string _mode;

    struct _Texture
    {
        // Start column of the texture
        int col = 0;
        int width = 0; // count of cells
        unsigned hl_id = 0;
        std::string text;
        PtrT<SDL_Texture> texture = NullPtr(SDL_DestroyTexture);
    };

    struct _Line
    {
        std::vector<std::string> text;
        std::vector<unsigned> hl_id;
        // Remember the previously rendered textures, high chance they're reusable.
        std::list<_Texture> texture_cache;
        // Is it necessary to redraw this line carefully or can just draw from the texture cache?
        bool dirty = true;
    };

    std::vector<_Line> _lines;

    void _DrawCursor();
};
