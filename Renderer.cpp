#include "Renderer.hpp"
#include <SDL2/SDL_ttf.h>
#include <iostream>


Renderer::Renderer()
    : _lines(25)
{
    for (auto &line : _lines)
    {
        for (int col = 0; col < 80; ++col)
        {
            line.text.push_back(' ');
            line.hl_id.push_back(0);
            line.offsets.push_back(col);
        }
        line.offsets.push_back(line.text.size());
    }

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    _window.reset(SDL_CreateWindow("NeoVim",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024,
        768, 0));
    _renderer.reset(SDL_CreateRenderer(_window.get(), -1, SDL_RENDERER_ACCELERATED));
    _font.reset(TTF_OpenFont("DejaVuSansMono.ttf", 20));

    // Check font metrics
    TTF_GlyphMetrics(_font.get(), '@', nullptr /*minx*/, nullptr /*maxx*/,
            nullptr /*miny*/, nullptr /*maxy*/, &_cell_width);
    _cell_height = TTF_FontHeight(_font.get());
}

Renderer::~Renderer()
{
    TTF_Quit();
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
    const SDL_Color bg0 = GetColor(_bg);
    const SDL_Color fg0 = GetColor(_fg);
    SDL_SetRenderDrawColor(_renderer.get(), bg0.r, bg0.g, bg0.b, 255);
    SDL_RenderClear(_renderer.get());

    for (int row = 0, rowN = _lines.size(); row < rowN; ++row)
    {
        // Group text chunks by hl_id
        _Texture texture;
        std::string_view cur_text;

        auto &line = _lines[row];
        auto &tex_cache = line.texture_cache;
        auto tit = tex_cache.begin();

        auto print_group = [&]() {
            // Remove potentially outdated cached textures
            while (tit != tex_cache.end() && tit->col < texture.col)
                tit = tex_cache.erase(tit);
            while (tit != tex_cache.end() && tit->col == texture.col
                && (tit->hl_id != texture.hl_id || tit->text != cur_text))
                tit = tex_cache.erase(tit);

            // Test whether the texture should be rendered again
            if (tit == tex_cache.end()
                || tit->col != texture.col || tit->hl_id != texture.hl_id
                || tit->text != cur_text || !tit->texture)
            {
                auto hlit = _hl_attr.find(texture.hl_id);

                SDL_Color fg = fg0;
                SDL_Color bg = bg0;
                if (hlit != _hl_attr.end())
                {
                    const HlAttr &attr = hlit->second;

                    if (attr.fg.has_value())
                        fg = GetColor(attr.fg.value());
                    if (attr.bg.has_value())
                        bg = GetColor(attr.bg.value());
                    if ((attr.flags & HF_REVERSE))
                        std::swap(fg, bg);
                }

                texture.text = cur_text;
                auto surface = PtrT<SDL_Surface>(TTF_RenderUTF8_Shaded(_font.get(),
                            texture.text.c_str(), fg, bg), SDL_FreeSurface);
                texture.texture.reset(SDL_CreateTextureFromSurface(_renderer.get(), surface.get()));
                tit = tex_cache.insert(tit, std::move(texture));
            }

            // Copy the texture (cached or new) to the renderer
            int texW = 0;
            int texH = 0;
            SDL_QueryTexture(tit->texture.get(), NULL, NULL, &texW, &texH);
            SDL_Rect dstrect = { tit->col * _cell_width, _cell_height * row, texW, texH };
            SDL_RenderCopy(_renderer.get(), tit->texture.get(), NULL, &dstrect);
            ++tit;
        };

        for (int c = 0, colN = line.hl_id.size(); c != colN; ++c)
        {
            if (texture.hl_id != line.hl_id[c])
            {
                if (c != texture.col)
                    print_group();
                texture.hl_id = line.hl_id[c];
                texture.col = c;
                texture.text.clear();
                texture.texture.reset();
            }
            cur_text = std::string_view(line.text.data() + line.offsets[texture.col],
                    line.offsets[c+1] - line.offsets[texture.col]);
        }
        print_group();
        // Remove the unused rest of the cache
        line.texture_cache.erase(tit, line.texture_cache.end());
    }

    // Draw the cursor
    SDL_SetRenderDrawBlendMode(_renderer.get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(_renderer.get(), _fg >> 16, (_fg >> 8) & 0xff, _fg & 0xff, 127);
    SDL_Rect rect = { _cursor_col * _cell_width, _cell_height * _cursor_row, _cell_width, _cell_height };
    SDL_RenderFillRect(_renderer.get(), &rect);

    SDL_RenderPresent(_renderer.get());
}

void Renderer::_InsertText(int row, int col, std::string_view text,
        size_t size, const int *offsets, const unsigned *hl_id)
{
    _Line &line = _lines[row];

    if (hl_id)
    {
        for (size_t i = 0; i < size; ++i)
            line.hl_id[col + i] = hl_id[i];
    }

    int start_offset = line.offsets[col];
    int replace_len = line.offsets[col + size] - start_offset;
    line.text.replace(start_offset, replace_len, text);

    for (size_t i = 0; i < size; ++i)
        line.offsets[col + i] = offsets[i] - offsets[0] + start_offset;

    int doff = (int)text.size() - replace_len;
    for (size_t i = col + size; i < line.offsets.size(); ++i)
        line.offsets[i] += doff;
}

int Renderer::GridLine(int row, int col, std::string_view text, unsigned hl_id)
{
    _Line &line = _lines[row];

    auto offsets = _CalcOffsets(text);
    int cols = offsets.size();

    _InsertText(row, col, text, offsets.size(), offsets.data(), nullptr);

    for (size_t i = 0; i < offsets.size(); ++i)
        line.hl_id[i + col] = hl_id;

    return cols;
}

// Scan through a UTF-8 string and remember the starts of cells when glyphs rendered
// (advance != 0).
std::vector<int> Renderer::_CalcOffsets(std::string_view utf8, size_t init)
{
    std::vector<int> offsets;

    size_t i = init;
    while (i < utf8.size())
    {
        size_t utf8_start = i;

        uint16_t uni;
        size_t todo;
        unsigned char ch = utf8[i++];
        if (ch <= 0x7F)
        {
            uni = ch;
            todo = 0;
        }
        else if (ch <= 0xBF)
        {
            // TODO: fall back to one glyph one cell
            throw std::logic_error("not a UTF-8 string");
        }
        else if (ch <= 0xDF)
        {
            uni = ch & 0x1F;
            todo = 1;
        }
        else if (ch <= 0xEF)
        {
            uni = ch & 0x0F;
            todo = 2;
        }
        else if (ch <= 0xF7)
        {
            uni = ch & 0x07;
            todo = 3;
        }
        else
        {
            throw std::logic_error("not a UTF-8 string");
        }

        for (size_t j = 0; j < todo; ++j)
        {
            if (i == utf8.size())
                throw std::logic_error("not a UTF-8 string");
            unsigned char ch = utf8[i++];
            if (ch < 0x80 || ch > 0xBF)
                throw std::logic_error("not a UTF-8 string");
            uni <<= 6;
            uni += ch & 0x3F;
        }

        // If advance is zero, the glyph doesn't start a new cell.
        int advance(0);
        TTF_GlyphMetrics(_font.get(), uni, nullptr, nullptr, nullptr, nullptr, &advance);
        if (advance)
        {
            offsets.push_back(utf8_start);
        }
    }
    return offsets;
}

void Renderer::HlAttrDefine(unsigned hl_id, HlAttr attr)
{
    _hl_attr[hl_id] = attr;
}

void Renderer::DefaultColorSet(unsigned fg, unsigned bg)
{
    _fg = fg;
    _bg = bg;

    for (auto &line : _lines)
        line.texture_cache.clear();
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
        size_t size = right - left;
        const auto *offsets = line_from.offsets.data() + left;
        const auto *hl_id = line_from.hl_id.data() + left;
        std::string_view text_from(line_from.text.data() + offsets[0], offsets[size] - offsets[0]);
        _InsertText(row, left, text_from, size, offsets, hl_id);
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
