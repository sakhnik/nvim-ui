#pragma once

#include "IWindow.hpp"
#include "Utils.hpp"
#include "Painter.hpp"
#include "TextureCache.hpp"
#include <SDL2/SDL.h>

class Window
    : public IWindow
{
public:
    void Init() override;
    void Deinit() override;

    RowsColsT GetRowsCols() const override;

    void Clear(unsigned bg) override;
    void CopyTexture(int row, int col, ITexture *) override;
    ITexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) override;
    void Present() override;
    void DrawCursor(int row, int col, unsigned fg, std::string_view mode) override;
    void SetBusy(bool is_busy) override;

private:
    PtrT<SDL_Window> _window = NullPtr(SDL_DestroyWindow);
    PtrT<SDL_Renderer> _renderer = NullPtr(SDL_DestroyRenderer);
    std::unique_ptr<Painter> _painter;
    // Mouse pointers for the active/busy state
    PtrT<SDL_Cursor> _active_cursor = NullPtr(SDL_FreeCursor);
    PtrT<SDL_Cursor> _busy_cursor = NullPtr(SDL_FreeCursor);

    double _scale_x = 1.0;
    double _scale_y = 1.0;

    void _DumpSurface(SDL_Surface *, const char *fname);
    void _DumpTexture(SDL_Texture *, const char *fname);
};
