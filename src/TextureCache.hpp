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

        Texture(int col, int width, unsigned hl_id, std::string_view text)
            : col{col}
            , width{width}
            , hl_id{hl_id}
            , text{text}
        {
        }

        bool IsSpace() const
        {
            return text.rfind("  ", 0) == 0;
        }
    };

private:
    // Sorted list
    using _TextureListT = std::list<Texture>;
    _TextureListT _cache;

public:
    void ForEach(std::function<void(const Texture &)> action);

    // Copy the ready textures to another container
    template <typename OtherT>
        void CopyTo(OtherT &other) const
        {
            other.assign(_cache.begin(), _cache.end());
        }

    // Scan the cache left to right removing outdated, reusing the actual and
    // adding updated textures.
    class Scanner
    {
        _TextureListT &_cache;
        using IterT = std::list<Texture>::iterator;
        IterT _iter = _cache.begin();
        bool _started = false;

    public:
        Scanner(_TextureListT &cache)
            : _cache{cache}
        {
        }

        ~Scanner();

        Scanner(const Scanner &) = delete;
        Scanner& operator=(const Scanner &) = delete;

        // Access the current texture
        Texture& Get() const { return *_iter; }

        // Generator function to create texture to be stored in the cache.
        using GeneratorT = std::function<IWindow::ITexture::PtrT(const Texture &)>;

        // Ensure the subsequent texture is the given one.
        // If there is no suitable texture in the cache, the generator functor will be invoked
        // to create one and insert into the cache.
        // The scanner cursor will point at either the newly created or the cached texture.
        // Return true if the generator was invoked.
        bool EnsureNext(Texture &&, GeneratorT);
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
