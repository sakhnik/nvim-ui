#pragma once

#include <memory>
#include <vector>
#include <list>
#include <unordered_map>
#include <string_view>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class MsgPackRpc;

template <typename T>
using PtrT = std::unique_ptr<T, void(*)(T*)>;

template <typename T>
PtrT<T> NullPtr(void(*d)(T*))
{
    return PtrT<T>(nullptr, d);
}

class Renderer
{
public:
    Renderer(MsgPackRpc *);
    ~Renderer();

    void Flush();

    // Return the number of columns
    int GridLine(int row, int col, std::string_view text, unsigned hl_id);

    enum HlFlags
    {
        HF_BOLD = 1 << 0,
        HF_REVERSE = 1 << 1,
        HF_ITALIC = 1 << 2,
        HF_UNDERLINE = 1 << 3,
        HF_UNDERCURL = 1 << 4,
    };
    struct HlAttr
    {
        std::optional<uint32_t> fg, bg;
        unsigned flags = 0;
    };

    // Window was resized
    void OnResized();

    void HlAttrDefine(unsigned hl_id, HlAttr attr);
    void DefaultColorSet(unsigned fg, unsigned bg);
    void GridCursorGoto(int row, int col);
    void GridScroll(int top, int bot, int left, int right, int rows);
    void GridResize(int width, int height);

private:
    MsgPackRpc *_rpc;
    PtrT<SDL_Window> _window = NullPtr(SDL_DestroyWindow);
    PtrT<SDL_Renderer> _renderer = NullPtr(SDL_DestroyRenderer);
    PtrT<TTF_Font> _font = NullPtr(TTF_CloseFont);
    int _cell_width = 0;
    int _cell_height = 0;
    std::unordered_map<unsigned, HlAttr> _hl_attr;
    uint32_t _fg = 0xffffff; 
    uint32_t _bg = 0;
    int _cursor_row = 0;
    int _cursor_col = 0;

    struct _Texture
    {
        // Start column of the texture
        int col = 0;
        unsigned hl_id = 0;
        std::string text;
        PtrT<SDL_Texture> texture = NullPtr(SDL_DestroyTexture);
    };

    struct _Line
    {
        std::string text;
        std::vector<int> offsets;
        std::vector<unsigned> hl_id;
        // Remember the previously rendered textures, high chance they're reusable.
        std::list<_Texture> texture_cache;
        // Is it necessary to redraw this line carefully or can just draw from the texture cache?
        bool dirty = true;
    };

    std::vector<_Line> _lines;

    std::vector<int> _CalcOffsets(std::string_view text, size_t init = 0);
    void _InsertText(int row, int col, std::string_view text,
                     size_t size, const int *offsets, const unsigned *hl_id);
};
