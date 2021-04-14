#pragma once

#include "Utils.hpp"
#include <string>
#include <list>
#include <functional>
#include <SDL2/SDL_render.h>

class TextureCache
{
public:
    struct Texture
    {
        // Start column of the texture
        int col = 0;
        int width = 0; // count of cells
        unsigned hl_id = 0;
        std::string text;
        PtrT<SDL_Texture> texture = NullPtr(SDL_DestroyTexture);

        bool Matches(const Texture &o) const
        {
            return col == o.col && hl_id == o.hl_id && text == o.text;
        }
    };

    void ForEach(std::function<void(const Texture &)> action);

    using IterT = std::list<Texture>::iterator;
    IterT Begin() { return _cache.begin(); }
    // True if there is no necessary texture
    bool AdvanceFor(IterT &, const Texture &);
    void Insert(IterT &, Texture &&);
    void RemoveTheRest(IterT);
    void Clear();

private:
    std::list<Texture> _cache;
};
