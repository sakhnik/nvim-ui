#pragma once

#include "Utils.hpp"
#include "IWindow.hpp"
#include <string>
#include <list>
#include <functional>

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
        IWindow::ITexture::PtrT texture;
    };

    void ForEach(std::function<void(const Texture &)> action);

    using IterT = std::list<Texture>::iterator;
    IterT Begin() { return _cache.begin(); }
    // True if there is no necessary texture
    bool ClearUntil(IterT &, const Texture &);
    void Insert(IterT &, Texture &&);
    void RemoveTheRest(IterT);
    void Clear();

    // Move textures from the other cache in the given column limits
    // (used when scrolling)
    void MoveFrom(TextureCache &o, int left, int right);

private:
    std::list<Texture> _cache;
};
