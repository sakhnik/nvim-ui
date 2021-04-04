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

void Renderer::GridLine(int row, int col, const std::string &text, unsigned hl_id)
{
    std::cerr << "GridLine " << row << ", " << col << " " << hl_id << " «" << text << "»" << std::endl;
    SDL_Color fg = { 255, 255, 255 };
    SDL_Color bg = { 0, 0, 0 };

    _Chunk chunk;
    chunk.text = text;

    auto surface = PtrT<SDL_Surface>(TTF_RenderUTF8_Shaded(_font.get(),
        text.c_str(), fg, bg), SDL_FreeSurface);
    chunk.texture.reset(SDL_CreateTextureFromSurface(_renderer.get(), surface.get()));
    _lines[row][col] = std::move(chunk);
}
