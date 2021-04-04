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

    unsigned hl_id = 0;
    int col = 0;
    std::string text;

    for (int row = 0, rowN = _lines.size(); row < rowN; ++row)
    {
        auto print = [&]() {
            SDL_Color fg = { 255, 255, 255 };
            SDL_Color bg = { 0, 0, 0 };
            auto surface = PtrT<SDL_Surface>(TTF_RenderUTF8_Shaded(_font.get(),
                        text.c_str(), fg, bg), SDL_FreeSurface);
            auto texture = PtrT<SDL_Texture>(SDL_CreateTextureFromSurface(_renderer.get(), surface.get()), SDL_DestroyTexture);

            int texW = 0;
            int texH = 0;
            SDL_QueryTexture(texture.get(), NULL, NULL, &texW, &texH);
            SDL_Rect dstrect = { col * _advance, texH * row, texW, texH };
            SDL_RenderCopy(_renderer.get(), texture.get(), NULL, &dstrect);
        };

        auto &line = _lines[row];
        for (int c = 0, colN = line.hl_id.size(); c != colN; ++c)
        {
            if (hl_id != line.hl_id[c])
            {
                if (c != col)
                    print();
                hl_id = line.hl_id[c];
                col = c;
                text.clear();
            }
            text += std::string_view(line.text.data() + line.offsets[c], line.offsets[c+1] - line.offsets[c]);
        }
        print();
    }
    SDL_RenderPresent(_renderer.get());

}

int Renderer::GridLine(int row, int col, const std::string &text, unsigned hl_id)
{
    std::cerr << "GridLine " << row << ", " << col << " " << hl_id << " «" << text << "»" << std::endl;

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
