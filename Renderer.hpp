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

private:
    PtrT<SDL_Window> _window;
    PtrT<SDL_Renderer> _renderer;
};
