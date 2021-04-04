#include "Renderer.hpp"
#include <SDL2/SDL_ttf.h>
#include <iostream>


Renderer::Renderer()
    : _window(nullptr, SDL_DestroyWindow)
    , _renderer(nullptr, SDL_DestroyRenderer)
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    _window.reset(SDL_CreateWindow("NeoVim",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024,
        768, 0));
    _renderer.reset(SDL_CreateRenderer(_window.get(), -1, 0));
}

Renderer::~Renderer()
{
    TTF_Quit();
    SDL_Quit();
}

void Renderer::Flush()
{
    SDL_RenderPresent(_renderer.get());
}

void Renderer::GridLine(int row, int col, const std::string &text, unsigned hl_id)
{
    std::cerr << "GridLine " << row << ", " << col << " " << hl_id << " «" << text << "»" << std::endl;
}
