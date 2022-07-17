#pragma once

#include "IWindow.hpp"
#include <compare>
#include <string>
#include <list>
#include <functional>
#include <cassert>

class GridLine
{
public:
    struct Word
    {
        unsigned hl_id = 0;
        std::string text;

        auto operator<=>(const Word &) const = default;

        bool IsSpace() const
        {
            return text.rfind("  ", 0) == 0;
        }
    };

    struct Chunk
    {
        // Start column of the chunk
        // TODO: remove col as it's always 0
        int col = 0;
        int width = 0; // count of cells

        using WordsT = std::vector<Word>;
        WordsT words;

        BaseTexture::PtrT texture;

        Chunk(int col, int width, WordsT &&words)
            : col{col}
            , width{width}
            , words{std::forward<WordsT>(words)}
        {
        }
    };

    template <void (BaseTexture::*inc_ref)(bool)>
    struct ChunkWrapper : Chunk
    {
        ChunkWrapper(const ChunkWrapper &c)
            : Chunk{c}
        {
            if (texture)
                (texture.get()->*inc_ref)(true);
        }

        template <typename ChunkT>
        ChunkWrapper(const ChunkT &c)
            : Chunk{c}
        {
            if (texture)
                (texture.get()->*inc_ref)(true);
        }

        ChunkWrapper& operator=(const ChunkWrapper &o)
        {
            if (texture)
                (texture.get()->*inc_ref)(false);
            static_cast<Chunk &>(*this) = o;
            if (texture)
                (texture.get()->*inc_ref)(true);
            return *this;
        }

        ~ChunkWrapper()
        {
            if (texture)
                (texture.get()->*inc_ref)(false);
        }
    };

private:
    // Sorted list
    using _ChunkT = ChunkWrapper<&BaseTexture::IncRef>;
    using _ChunkListT = std::list<_ChunkT>;
    _ChunkListT _chunks;

public:
    void ForEach(std::function<void(const Chunk &)> action);

    void Clear();

    // Move chunks from the other grid line in the given column limits
    // (used when scrolling)
    void MoveFrom(GridLine &o, int left, int right);
};
