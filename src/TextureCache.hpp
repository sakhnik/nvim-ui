#pragma once

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

private:
    using _TextureListT = std::list<Texture>;
    _TextureListT _cache;

public:
    void ForEach(std::function<void(const Texture &)> action);

    // Scan the cache left to right removing outdated, reusing the actual and
    // adding updated textures.
    class Scanner
    {
        _TextureListT &_cache;
        using IterT = std::list<Texture>::iterator;
        IterT _iter;

    public:
        Scanner(_TextureListT &cache)
            : _cache{cache}
            , _iter(_cache.begin())
        {
        }

        ~Scanner()
        {
            // The unscanned entries are considered outdated
            _cache.erase(_iter, _cache.end());
        }

        Scanner(const Scanner &) = delete;
        Scanner& operator=(const Scanner &) = delete;

        Texture& Get() const { return *_iter; }
        void Advance() { ++_iter; }

        // Ensure the subsequent texture is the given one.
        // True if there is no necessary texture in the cache.
        bool EnsureNext(const Texture &);

        void Insert(Texture &&texture)
        {
            _iter = _cache.insert(_iter, std::move(texture));
        }
    };

    Scanner GetScanner()
    {
        return Scanner{_cache};
    }

    void Clear();

    // Move textures from the other cache in the given column limits
    // (used when scrolling)
    void MoveFrom(TextureCache &o, int left, int right);
};
