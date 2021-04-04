#pragma once

#include <SDL2/SDL.h>
#include <memory>

template <typename T>
using PtrT = std::unique_ptr<T, void(*)(T*)>;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void Flush();
    void GridLine(int row, int col, const std::string &text, unsigned hl_id);

private:
    PtrT<SDL_Window> _window;
    PtrT<SDL_Renderer> _renderer;
};
