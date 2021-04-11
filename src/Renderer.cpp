#include "Renderer.hpp"
#include "MsgPackRpc.hpp"
#include "Painter.hpp"
#include <iostream>


Renderer::Renderer(MsgPackRpc *rpc)
    : _rpc{rpc}
{
    // Default hightlight attributes
    _def_attr.fg = 0xffffff;
    _def_attr.bg = 0;

    SDL_Init(SDL_INIT_VIDEO);

    const int WIN_W = 1024;
    const int WIN_H = 768;

    // Create a window
    _window.reset(SDL_CreateWindow("NeoVim",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIN_W, WIN_H,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));
    _renderer.reset(SDL_CreateRenderer(_window.get(), -1, SDL_RENDERER_ACCELERATED));

    // Get the window size in pixels to cope with HiDPI
    int wp{}, hp{};
    SDL_GetRendererOutputSize(_renderer.get(), &wp, &hp);
    _scale_x = static_cast<double>(wp) / WIN_W;
    _scale_y = static_cast<double>(hp) / WIN_H;
    _painter.reset(new Painter(_scale_x, _scale_y));

    // Prepare the initial cell grid to fill the whole window.
    // The NeoVim UI will be attached using these dimensions.
    int width = wp / (_painter->GetCellWidth() * _scale_x);
    if (width < 1)
        width = 1;
    int height = hp / (_painter->GetCellHeight() * _scale_y);
    if (height < 1)
        height = 1;
    GridResize(width, height);
}

Renderer::~Renderer()
{
    SDL_Quit();
}

namespace {

inline SDL_Color GetColor(uint32_t val)
{
    return SDL_Color{ Uint8(val >> 16), Uint8((val >> 8) & 0xff), Uint8(val & 0xff) };
}

} //namespace

void Renderer::Flush()
{
    const SDL_Color bg0 = GetColor(_def_attr.bg.value());
    SDL_SetRenderDrawColor(_renderer.get(), bg0.r, bg0.g, bg0.b, 255);
    SDL_RenderClear(_renderer.get());

    int cell_width = _painter->GetCellWidth() * _scale_x;
    int cell_height = _painter->GetCellHeight() * _scale_y;

    for (int row = 0, rowN = _lines.size(); row < rowN; ++row)
    {
        auto &line = _lines[row];
        auto &tex_cache = line.texture_cache;

        auto copy_texture = [&](const _Texture &tex) {
            int texW = 0;
            int texH = 0;
            SDL_QueryTexture(tex.texture.get(), NULL, NULL, &texW, &texH);
            SDL_Rect dstrect = { tex.col * cell_width, cell_height * row, texW, texH };
            SDL_RenderCopy(_renderer.get(), tex.texture.get(), NULL, &dstrect);
        };

        // Check if it's possible to just copy the prepared textures first
        if (!line.dirty)
        {
            for (const auto &tex : line.texture_cache)
                copy_texture(tex);
            continue;
        }
        // Mark the line clear as we're going to redraw the necessary parts
        // and update the texture cache.
        line.dirty = false;

        // Group text chunks by hl_id
        _Texture texture;

        auto tit = tex_cache.begin();

        auto print_group = [&]() {
            // Remove potentially outdated cached textures
            while (tit != tex_cache.end() && tit->col < texture.col)
                tit = tex_cache.erase(tit);
            while (tit != tex_cache.end() && tit->col == texture.col
                && (tit->hl_id != texture.hl_id || tit->text != texture.text))
                tit = tex_cache.erase(tit);

            // Test whether the texture should be rendered again
            if (tit == tex_cache.end()
                || tit->col != texture.col || tit->hl_id != texture.hl_id
                || tit->text != texture.text || !tit->texture)
            {
                auto surface = PtrT<SDL_Surface>(SDL_CreateRGBSurface(0,
                    texture.width * cell_width, cell_height,
                    32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0),
                    SDL_FreeSurface);

                // Paint the text on the surface carefully
                auto hlit = _hl_attr.find(texture.hl_id);
                _painter->Paint(surface.get(),
                        texture.text,
                        hlit != _hl_attr.end() ? hlit->second : _def_attr,
                        _def_attr);

                // Create a possibly hardware accelerated texture from the surface
                texture.texture.reset(SDL_CreateTextureFromSurface(_renderer.get(), surface.get()));
                tit = tex_cache.insert(tit, std::move(texture));
            }

            // Copy the texture (cached or new) to the renderer
            copy_texture(*tit++);
        };

        for (int c = 0, colN = line.hl_id.size(); c != colN; ++c)
        {
            if (texture.hl_id != line.hl_id[c])
            {
                if (c != texture.col)
                    print_group();
                texture.hl_id = line.hl_id[c];
                texture.col = c;
                texture.width = 0;
                texture.text.clear();
                texture.texture.reset();
            }
            texture.text += line.text[c];
            ++texture.width;
        }
        print_group();
        // Remove the unused rest of the cache
        line.texture_cache.erase(tit, line.texture_cache.end());
    }

    _DrawCursor();
    SDL_RenderPresent(_renderer.get());
}

void Renderer::_DrawCursor()
{
    int cell_width = _painter->GetCellWidth() * _scale_x;
    int cell_height = _painter->GetCellHeight() * _scale_y;

    SDL_SetRenderDrawBlendMode(_renderer.get(), SDL_BLENDMODE_BLEND);
    auto fg = _def_attr.fg.value();
    SDL_SetRenderDrawColor(_renderer.get(), fg >> 16, (fg >> 8) & 0xff, fg & 0xff, 127);
    SDL_Rect rect;
    if (_mode == "insert")
    {
        rect = {
            _cursor_col * cell_width,
            cell_height * _cursor_row,
            cell_width / 4,
            cell_height
        };
    }
    else if (_mode == "replace" || _mode == "operator")
    {
        rect = {
            _cursor_col * cell_width,
            cell_height * _cursor_row + cell_height * 3 / 4,
            cell_width,
            cell_height / 4
        };
    }
    else
    {
        rect = {
            _cursor_col * cell_width,
            cell_height * _cursor_row,
            cell_width,
            cell_height
        };
    }
    SDL_RenderFillRect(_renderer.get(), &rect);
}

void Renderer::GridLine(int row, int col, std::string_view chunk, unsigned hl_id, int repeat)
{
    _Line &line = _lines[row];
    line.dirty = true;

    for (int i = 0; i < repeat; ++i)
    {
        line.text[col + i] = chunk;
        line.hl_id[col + i] = hl_id;
    }
}

void Renderer::HlAttrDefine(unsigned hl_id, HlAttr attr)
{
    _hl_attr[hl_id] = attr;
}

void Renderer::DefaultColorSet(unsigned fg, unsigned bg)
{
    _def_attr.fg = fg;
    _def_attr.bg = bg;

    for (auto &line : _lines)
    {
        line.dirty = true;
        line.texture_cache.clear();
    }
}

void Renderer::GridCursorGoto(int row, int col)
{
    _cursor_row = row;
    _cursor_col = col;
}

void Renderer::GridScroll(int top, int bot, int left, int right, int rows)
{
    auto copy = [&](int row, int row_from) {
        const auto &line_from = _lines[row_from];
        auto &line_to = _lines[row];
        line_to.dirty = true;
        for (int col = left; col < right; ++col)
        {
            line_to.text[col] = line_from.text[col];
            line_to.hl_id[col] = line_from.hl_id[col];
        }
    };

    if (rows > 0)
    {
        for (int row = top; row < bot - rows; ++row)
            copy(row, row + rows);
    }
    else if (rows < 0)
    {
        for (int row = bot - 1; row > top - rows - 1; --row)
            copy(row, row + rows);
    }
    else
    {
        throw std::runtime_error("Rows should not equal 0");
    }
}

void Renderer::OnResized()
{
    int wp{}, hp{};
    SDL_GetRendererOutputSize(_renderer.get(), &wp, &hp);

    int cell_width = _painter->GetCellWidth() * _scale_x;
    int cell_height = _painter->GetCellHeight() * _scale_y;
    int new_width = std::max(1, wp / cell_width);
    int new_height = std::max(1, hp / cell_height);

    if (new_height != static_cast<int>(_lines.size()) ||
        new_width != static_cast<int>(_lines[0].text.size()))
    {
        _rpc->Request(
            [&](auto &pk) {
                pk.pack("nvim_ui_try_resize");
                pk.pack_array(2);
                pk.pack(new_width);
                pk.pack(new_height);
            },
            [](const msgpack::object &err, const auto &resp) {
                if (!err.is_nil())
                {
                    std::ostringstream oss;
                    oss << "Failed to resize UI " << err << std::endl;
                    throw std::runtime_error(oss.str());
                }
            }
        );
    }
}

void Renderer::GridResize(int width, int height)
{
    _lines.resize(height);
    for (auto &line : _lines)
    {
        line.hl_id.resize(width, 0);
        line.text.resize(width, " ");
    }
}

void Renderer::ModeChange(std::string_view mode)
{
    _mode = mode;
}