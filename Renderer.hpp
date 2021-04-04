#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <memory>
#include <map>
#include <vector>


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

    struct _Chunk
    {
        unsigned hl_id = 0;
        std::string text;
        std::vector<int> offsets;
        PtrT<SDL_Texture> texture = NullPtr(SDL_DestroyTexture);
    };

    using _LineT = std::map<int, _Chunk>;
    std::vector<_LineT> _lines;

    std::vector<int> _CalcOffsets(const std::string &text);
};
