#include "Window.hpp"
#include "Logger.hpp"
#include <sstream>

void Window::Init()
{
    SDL_Init(SDL_INIT_VIDEO);

    const int WIN_W = 1024;
    const int WIN_H = 768;

    _window.reset(SDL_CreateWindow("nvim-ui",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIN_W, WIN_H,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));
    _renderer.reset(SDL_CreateRenderer(_window.get(), -1, SDL_RENDERER_ACCELERATED));

    SDL_RendererInfo info;
    SDL_GetRendererInfo(_renderer.get(), &info);
    std::ostringstream oss;
    if ((info.flags & SDL_RENDERER_SOFTWARE))
        oss << " software";
    if ((info.flags & SDL_RENDERER_ACCELERATED))
        oss << " accelerated";
    if ((info.flags & SDL_RENDERER_PRESENTVSYNC))
        oss << " vsync";
    if ((info.flags & SDL_RENDERER_TARGETTEXTURE))
        oss << " target_texture";
    Logger().info("Using renderer {}:{}", info.name, oss.str());

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

Window::RowsColsT
Window::GetRowsCols() const
{
    int wp{}, hp{};
    SDL_GetRendererOutputSize(_renderer.get(), &wp, &hp);

    int cols = std::max(1, wp / _painter->GetCellWidth());
    int rows = std::max(1, hp / _painter->GetCellHeight());
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

namespace {

struct Texture : Window::ITexture
{
    Texture(SDL_Texture *t)
        : texture{t, SDL_DestroyTexture}
    {
    }
    ::PtrT<SDL_Texture> texture;
};

} //namespace;

void Window::CopyTexture(int row, int col, ITexture *texture)
{
    SDL_Texture *t = static_cast<Texture *>(texture)->texture.get();

    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(t, NULL, NULL, &texW, &texH);
    SDL_Rect dstrect = { col * _painter->GetCellWidth(), _painter->GetCellHeight() * row, texW, texH };
    SDL_RenderCopy(_renderer.get(), t, NULL, &dstrect);
}

Window::ITexture::PtrT
Window::CreateTexture(int width, std::string_view text, const HlAttr &attr, const HlAttr &def_attr)
{
    auto surface = PtrT<SDL_Surface>(SDL_CreateRGBSurface(0,
        width * _painter->GetCellWidth(), _painter->GetCellHeight(),
        32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0),
        SDL_FreeSurface);
    _painter->Paint(surface.get(), text, attr, def_attr);

    // Create a possibly hardware accelerated texture from the surface
    return Texture::PtrT(new Texture(SDL_CreateTextureFromSurface(_renderer.get(), surface.get())));
}

void Window::Present()
{
    SDL_RenderPresent(_renderer.get());
}

void Window::DrawCursor(int row, int col, unsigned fg, std::string_view mode)
{
    int cell_width = _painter->GetCellWidth();
    int cell_height = _painter->GetCellHeight();

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
