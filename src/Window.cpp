#include "Window.hpp"
#include <iostream>

void Window::Init()
{
    SDL_Init(SDL_INIT_VIDEO);

    const int WIN_W = 1024;
    const int WIN_H = 768;

    _window.reset(SDL_CreateWindow("NeoVim",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIN_W, WIN_H,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));
    _renderer.reset(SDL_CreateRenderer(_window.get(), -1, SDL_RENDERER_ACCELERATED));

    SDL_RendererInfo info;
    SDL_GetRendererInfo(_renderer.get(), &info);
    std::cout << "Using renderer " << info.name << ":";
    if ((info.flags & SDL_RENDERER_SOFTWARE))
        std::cout << " software";
    if ((info.flags & SDL_RENDERER_ACCELERATED))
        std::cout << " accelerated";
    if ((info.flags & SDL_RENDERER_PRESENTVSYNC))
        std::cout << " vsync";
    if ((info.flags & SDL_RENDERER_TARGETTEXTURE))
        std::cout << " target_texture";
    std::cout << std::endl;

    // Get the window size in pixels to cope with HiDPI
    int wp{}, hp{};
    SDL_GetRendererOutputSize(_renderer.get(), &wp, &hp);
    _scale_x = static_cast<double>(wp) / WIN_W;
    _scale_y = static_cast<double>(hp) / WIN_H;
    _painter.reset(new Painter(_scale_x, _scale_y));
}

void Window::Deinit()
{
    SDL_Quit();
}

int Window::GetCellWidth() const
{
    return _painter->GetCellWidth() * _scale_x;
}

int Window::GetCellHeight() const
{
    return _painter->GetCellHeight() * _scale_y;
}

Window::RowsColsT
Window::GetRowsCols() const
{
    int wp{}, hp{};
    SDL_GetRendererOutputSize(_renderer.get(), &wp, &hp);

    int cols = std::max(1, wp / GetCellWidth());
    int rows = std::max(1, hp / GetCellHeight());
    return {rows, cols};
}

namespace {

inline SDL_Color GetColor(uint32_t val)
{
    return SDL_Color{ Uint8(val >> 16), Uint8((val >> 8) & 0xff), Uint8(val & 0xff) };
}

} //namespace

void Window::Clear(unsigned bg)
{
    const SDL_Color bg0 = GetColor(bg);
    SDL_SetRenderDrawColor(_renderer.get(), bg0.r, bg0.g, bg0.b, 255);
    SDL_RenderClear(_renderer.get());
}

void Window::CopyTexture(int row, int col, SDL_Texture *texture)
{
    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
    SDL_Rect dstrect = { col * GetCellWidth(), GetCellHeight() * row, texW, texH };
    SDL_RenderCopy(_renderer.get(), texture, NULL, &dstrect);
}

PtrT<SDL_Texture> Window::CreateTexture(int width, std::string_view text, const HlAttr &attr, const HlAttr &def_attr)
{
    auto surface = PtrT<SDL_Surface>(SDL_CreateRGBSurface(0,
        width * GetCellWidth(), GetCellHeight(),
        32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0),
        SDL_FreeSurface);
    _painter->Paint(surface.get(), text, attr, def_attr);

    // Create a possibly hardware accelerated texture from the surface
    return PtrT<SDL_Texture>(SDL_CreateTextureFromSurface(_renderer.get(), surface.get()), SDL_DestroyTexture);
}

void Window::Present()
{
    SDL_RenderPresent(_renderer.get());
}

void Window::DrawCursor(int row, int col, unsigned fg, std::string_view mode)
{
    int cell_width = GetCellWidth();
    int cell_height = GetCellHeight();

    SDL_SetRenderDrawBlendMode(_renderer.get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(_renderer.get(), fg >> 16, (fg >> 8) & 0xff, fg & 0xff, 127);
    SDL_Rect rect;
    if (mode == "insert")
    {
        rect = {
            col * cell_width,
            cell_height * row,
            cell_width / 4,
            cell_height
        };
    }
    else if (mode == "replace" || mode == "operator")
    {
        rect = {
            col * cell_width,
            cell_height * row + cell_height * 3 / 4,
            cell_width,
            cell_height / 4
        };
    }
    else
    {
        rect = {
            col * cell_width,
            cell_height * row,
            cell_width,
            cell_height
        };
    }
    SDL_RenderFillRect(_renderer.get(), &rect);
}

void Window::SetBusy(bool is_busy)
{
    if (is_busy)
    {
        if (!_busy_cursor)
            _busy_cursor.reset(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW));
        SDL_SetCursor(_busy_cursor.get());
    }
    else
    {
        if (!_active_cursor)
            _active_cursor.reset(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
        SDL_SetCursor(_active_cursor.get());
    }
}
