#include "Renderer.hpp"
#include <SDL2/SDL_ttf.h>
#include <iostream>


Renderer::Renderer()
    : _lines(25)
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    _window.reset(SDL_CreateWindow("NeoVim",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024,
        768, 0));
    _renderer.reset(SDL_CreateRenderer(_window.get(), -1, 0));
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
        for (const auto &col_chunk : _lines[row])
        {
            int col = col_chunk.first;
            const auto &chunk = col_chunk.second;

            int texW = 0;
            int texH = 0;
            SDL_QueryTexture(chunk.texture.get(), NULL, NULL, &texW, &texH);
            SDL_Rect dstrect = { col * _advance, texH * row, texW, texH };
            SDL_RenderCopy(_renderer.get(), chunk.texture.get(), NULL, &dstrect);
            SDL_RenderPresent(_renderer.get());
        }
    }

}

int Renderer::GridLine(int row, int col, const std::string &text, unsigned hl_id)
{
    std::cerr << "GridLine " << row << ", " << col << " " << hl_id << " «" << text << "»" << std::endl;
    SDL_Color fg = { 255, 255, 255 };
    SDL_Color bg = { 0, 0, 0 };

    _Chunk chunk;
    chunk.text = text;
    chunk.offsets = _CalcOffsets(text);
    int cols = chunk.offsets.size();

    auto surface = PtrT<SDL_Surface>(TTF_RenderUTF8_Shaded(_font.get(),
        text.c_str(), fg, bg), SDL_FreeSurface);
    chunk.texture.reset(SDL_CreateTextureFromSurface(_renderer.get(), surface.get()));
    int texW = 0;
    SDL_QueryTexture(chunk.texture.get(), nullptr, nullptr, &texW, nullptr);

    _LineT &line = _lines[row];
    auto it = line.lower_bound(col);
    if (it == line.end())
    {
        // Append, try to merge with the last chunk
    }
    else if (it->first == col)
    {
        // Start together, overwrite and check the tails
    }
    else
    {
        // Check the previous chunk
    }
    line[col] = std::move(chunk);
    return cols;
}

// Scan through a UTF-8 string and remember the starts of cells when glyphs rendered
// (advance != 0).
std::vector<int> Renderer::_CalcOffsets(const std::string &utf8)
{
    std::vector<int> offsets;

    size_t i = 0;
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
