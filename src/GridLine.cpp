#include "GridLine.hpp"
#include "Logger.hpp"

void GridLine::ForEach(std::function<void(const Chunk &)> action)
{
    for (auto &chunk : _chunks)
        action(chunk);
}

void GridLine::Clear()
{
    _chunks.clear();
}

void GridLine::MoveFrom(GridLine &o, int left, int right, uint32_t redraw_token)
{
    // Find the beginning of the source cache
    auto it_from = o._chunks.begin();
    while (it_from != o._chunks.end() && it_from->col < left)
        ++it_from;

    auto it = _chunks.begin();
    while (it_from != o._chunks.end() && it_from->col < right)
    {
        // Find a suitable place to insert to still maintaining the sorted order
        while (it != _chunks.end() && it->col < it_from->col)
            ++it;
        it_from->texture->MarkToRedraw(redraw_token);
        _chunks.splice(it++, o._chunks, it_from++);
    }
}
