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

    _hl_attr[0] = {};

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    _window.reset(SDL_CreateWindow("NeoVim",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024,
        768, 0));
    _renderer.reset(SDL_CreateRenderer(_window.get(), -1, SDL_RENDERER_ACCELERATED));
    _font.reset(TTF_OpenFont("DejaVuSansMono.ttf", 20));
    TTF_GlyphMetrics(_font.get(), '@', nullptr /*minx*/, nullptr /*maxx*/,
            nullptr /*miny*/, nullptr /*maxy*/, &_advance);
}

Renderer::~Renderer()
{
    TTF_Quit();
    SDL_Quit();
}

void Renderer::Flush()
{
    SDL_RenderClear(_renderer.get());

    for (int row = 0, rowN = _lines.size(); row < rowN; ++row)
    {
        // Group text chunks by hl_id
        _Texture texture;

        auto &line = _lines[row];
        auto &tex_cache = line.texture_cache;
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
                || tit->text != texture.text)
            {
                SDL_Color fg = { 255, 255, 255 };
                SDL_Color bg = { 0, 0, 0 };

                auto hlit = _hl_attr.find(tit->hl_id);
                if (hlit == _hl_attr.end())
                {
                    std::cerr << "Undefined hl_id " << tit->hl_id << std::endl;
                }
                else
                {
                    const HlAttr &attr = hlit->second;

                    auto calcSdlColor = [](std::optional<uint32_t> color, uint32_t def_color) -> SDL_Color {
                        if (color.has_value())
                            def_color = color.value();
                        return SDL_Color{
                            Uint8(def_color >> 16),
                                Uint8((def_color >> 8) & 0xff),
                                Uint8(def_color & 0xff),
                        };
                    };
                    fg = calcSdlColor(attr.fg, _fg);
                    bg = calcSdlColor(attr.bg, _bg);
                    if ((attr.flags & HF_REVERSE))
                        std::swap(fg, bg);
                }

                auto surface = PtrT<SDL_Surface>(TTF_RenderUTF8_Shaded(_font.get(),
                            texture.text.c_str(), fg, bg), SDL_FreeSurface);
                texture.texture.reset(SDL_CreateTextureFromSurface(_renderer.get(), surface.get()));
                tit = tex_cache.insert(tit, std::move(texture));
            }

            // Copy the texture (cached or new) to the renderer
            int texW = 0;
            int texH = 0;
            SDL_QueryTexture(tit->texture.get(), NULL, NULL, &texW, &texH);
            SDL_Rect dstrect = { tit->col * _advance, texH * row, texW, texH };
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
            texture.text += std::string_view(line.text.data() + line.offsets[c], line.offsets[c+1] - line.offsets[c]);
        }
        print_group();

        // Remove the unused rest of the cache
        line.texture_cache.erase(tit, line.texture_cache.end());
    }

    SDL_RenderPresent(_renderer.get());
}

int Renderer::GridLine(int row, int col, const std::string &text, unsigned hl_id)
{
    _Line &line = _lines[row];

    auto offsets = _CalcOffsets(text);
    int cols = offsets.size();

    for (size_t i = 0; i < offsets.size(); ++i)
    {
        line.hl_id[i] = hl_id;
    }
    int start_offset = line.offsets[col];
    int replace_len = line.offsets[col + offsets.size()] - start_offset;
    line.text.replace(start_offset, replace_len, text);

    for (size_t i = 0; i < offsets.size(); ++i)
    {
        line.offsets[col + i] = offsets[i] + start_offset;
    }

    int doff = (int)text.size() - replace_len;
    for (size_t i = col + offsets.size(); i < line.offsets.size(); ++i)
        line.offsets[i] += doff;

    return cols;
}

// Scan through a UTF-8 string and remember the starts of cells when glyphs rendered
// (advance != 0).
std::vector<int> Renderer::_CalcOffsets(const std::string &utf8, size_t init)
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
}
