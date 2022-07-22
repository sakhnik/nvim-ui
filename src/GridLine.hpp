#pragma once

#include <compare>
#include <string>
#include <vector>
#include <memory>

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
        using PtrT = std::shared_ptr<Chunk>;

        int width = 0; // count of cells

        using WordsT = std::vector<Word>;
        WordsT words;

        Chunk(int width, WordsT &&words)
            : width{width}
            , words{std::forward<WordsT>(words)}
        {
        }
    };
};
