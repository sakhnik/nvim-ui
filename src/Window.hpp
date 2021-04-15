#pragma once

#include "Utils.hpp"
#include "Painter.hpp"
#include "TextureCache.hpp"
#include <SDL2/SDL.h>

class Window
{
public:
    void Init();
    void Deinit();

    int GetCellWidth() const;
    int GetCellHeight() const;

    using RowsColsT = std::pair<int, int>;
    RowsColsT GetRowsCols() const;

    void Clear(unsigned bg);
    void CopyTexture(int row, int col, SDL_Texture *);
    PtrT<SDL_Texture> CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr);
    void Present();
    void DrawCursor(int row, int col, unsigned fg, std::string_view mode);
    void SetBusy(bool is_busy);

private:
    PtrT<SDL_Window> _window = NullPtr(SDL_DestroyWindow);
    PtrT<SDL_Renderer> _renderer = NullPtr(SDL_DestroyRenderer);
    std::unique_ptr<Painter> _painter;
    // Mouse pointers for the active/busy state
    PtrT<SDL_Cursor> _active_cursor = NullPtr(SDL_FreeCursor);
    PtrT<SDL_Cursor> _busy_cursor = NullPtr(SDL_FreeCursor);

    double _scale_x = 1.0;
    double _scale_y = 1.0;
};
