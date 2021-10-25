#pragma once

#include "IWindow.hpp"
#include <string>
#include <list>
#include <functional>

class GridLine
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

    template <void (BaseTexture::*inc_ref)(bool)>
    struct ChunkWrapper : Chunk
    {
        template <typename ChunkT>
        ChunkWrapper(const ChunkT &c)
            : Chunk{c}
        {
            (texture.get()->*inc_ref)(true);
        }

        ~ChunkWrapper()
        {
            (texture.get()->*inc_ref)(false);
        }

        template <typename ChunkT>
        ChunkWrapper& operator=(const ChunkT &o)
        {
            static_cast<Chunk &>(*this) = o;
            (texture.get()->*inc_ref)(true);
            return *this;
        }
    };

private:
    // Sorted list
    using _ChunkT = ChunkWrapper<&BaseTexture::IncRef>;
    using _ChunkListT = std::list<_ChunkT>;
    _ChunkListT _chunks;

public:
    void ForEach(std::function<void(const Chunk &)> action);

    // Copy the ready chunks to another container
    template <typename OtherT>
        void CopyTo(OtherT &other) const
        {
            other.assign(_chunks.begin(), _chunks.end());
        }

    // Scan the chunks left to right removing outdated, reusing the actual and
    // adding updated chunks.
    class Scanner
    {
        _ChunkListT &_chunks;
        using IterT = std::list<_ChunkT>::iterator;
        IterT _iter = _chunks.begin();
        bool _started = false;

    public:
        Scanner(_ChunkListT &chunks, uint32_t redraw_token)
            : _chunks{chunks}
            , _redraw_token{redraw_token}
        {
        }

        ~Scanner();

        Scanner(const Scanner &) = delete;
        Scanner& operator=(const Scanner &) = delete;

        // Access the current chunk
        Chunk& Get() const { return *_iter; }

        // Generator function to create chunk to be stored in the chunk list.
        using GeneratorT = std::function<BaseTexture::PtrT(const Chunk &)>;

        // Ensure the subsequent chunk is the given one.
        // If there is no suitable chunk in the list, the generator functor will be invoked
        // to create one and insert into the sequence.
        // The scanner cursor will point at either the newly created or the cached chunk.
        // Return true if the generator was invoked.
        bool EnsureNext(Chunk &&, GeneratorT);

    private:
        uint32_t _redraw_token;
    };

    Scanner GetScanner(uint32_t redraw_token)
    {
        return Scanner{_chunks, redraw_token};
    }

    void Clear();

    // Move chunks from the other grid line in the given column limits
    // (used when scrolling)
    void MoveFrom(GridLine &o, int left, int right, uint32_t redraw_token);
};
