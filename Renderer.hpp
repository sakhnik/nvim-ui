#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <memory>
#include <vector>
#include <list>


template <typename T>
using PtrT = std::unique_ptr<T, void(*)(T*)>;

template <typename T>
PtrT<T> NullPtr(void(*d)(T*))
{
    return PtrT<T>(nullptr, d);
}

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void Flush();

    // Return the number of columns
    int GridLine(int row, int col, const std::string &text, unsigned hl_id);

private:
    PtrT<SDL_Window> _window = NullPtr(SDL_DestroyWindow);
    PtrT<SDL_Renderer> _renderer = NullPtr(SDL_DestroyRenderer);
    PtrT<TTF_Font> _font = NullPtr(TTF_CloseFont);
    int _advance = 0;

    struct _Texture
    {
        // Start column of the texture
        int col = 0;
        unsigned hl_id = 0;
        std::string text;
        PtrT<SDL_Texture> texture = NullPtr(SDL_DestroyTexture);
    };

    struct _Line
    {
        std::string text;
        std::vector<int> offsets;
        std::vector<unsigned> hl_id;
        // Remember the previously rendered textures, high chance they're reusable.
        std::list<_Texture> texture_cache;
    };

    std::vector<_Line> _lines;

    std::vector<int> _CalcOffsets(const std::string &text, size_t init = 0);
};
