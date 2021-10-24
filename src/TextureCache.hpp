#pragma once

#include "IWindow.hpp"
#include <string>
#include <list>
#include <functional>

class TextureCache
{
public:
    struct Chunk
    {
        // Start column of the chunk
        int col = 0;
        int width = 0; // count of cells
        unsigned hl_id = 0;
        std::string text;
        BaseTexture::PtrT texture;

        Chunk(int col, int width, unsigned hl_id, std::string_view text)
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
    using _ChunkListT = std::list<Chunk>;
    _ChunkListT _cache;

public:
    void ForEach(std::function<void(const Chunk &)> action);

    // Copy the ready chunks to another container
    template <typename OtherT>
        void CopyTo(OtherT &other) const
        {
            other.assign(_cache.begin(), _cache.end());
        }

    // Scan the cache left to right removing outdated, reusing the actual and
    // adding updated chunks.
    class Scanner
    {
        _ChunkListT &_cache;
        using IterT = std::list<Chunk>::iterator;
        IterT _iter = _cache.begin();
        bool _started = false;

    public:
        Scanner(_ChunkListT &cache, uint32_t redraw_token)
            : _cache{cache}
            , _redraw_token{redraw_token}
        {
        }

        ~Scanner();

        Scanner(const Scanner &) = delete;
        Scanner& operator=(const Scanner &) = delete;

        // Access the current chunk
        Chunk& Get() const { return *_iter; }

        // Generator function to create chunk to be stored in the cache.
        using GeneratorT = std::function<BaseTexture::PtrT(const Chunk &)>;

        // Ensure the subsequent chunk is the given one.
        // If there is no suitable chunk in the cache, the generator functor will be invoked
        // to create one and insert into the cache.
        // The scanner cursor will point at either the newly created or the cached chunk.
        // Return true if the generator was invoked.
        bool EnsureNext(Chunk &&, GeneratorT);

    private:
        uint32_t _redraw_token;
    };

    Scanner GetScanner(uint32_t redraw_token)
    {
        return Scanner{_cache, redraw_token};
    }

    void Clear();

    // Move chunks from the other cache in the given column limits
    // (used when scrolling)
    void MoveFrom(TextureCache &o, int left, int right, uint32_t redraw_token);
};
