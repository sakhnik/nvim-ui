#include "GridLine.hpp"
#include "Logger.hpp"

void GridLine::ForEach(std::function<void(const Chunk &)> action)
{
    for (auto &chunk : _chunks)
        action(chunk);
}

GridLine::Scanner::~Scanner()
{
    if (_started)
        ++_iter;
    // The unscanned entries are considered outdated
    _chunks.erase(_iter, _chunks.end());
}

bool GridLine::Scanner::EnsureNext(Chunk &&chunk, GeneratorT generator)
{
    if (_started)
        ++_iter;
    else
        _started = true;

    // Remove potentially outdated cached chunks
    while (_iter != _chunks.end() && _iter->col < chunk.col)
    {
        _iter = _chunks.erase(_iter);
    }
    while (_iter != _chunks.end() && _iter->col == chunk.col
        && _iter->words != chunk.words)
    {
        _iter = _chunks.erase(_iter);
    }

    if (_iter == _chunks.end() || _iter->words != chunk.words)
    {
        chunk.texture = generator(chunk);
        _iter = _chunks.insert(_iter, std::move(chunk));
        return true;
    }

    if (_iter->col != chunk.col)
    {
        // The chunk has just shifted
        _iter->col = chunk.col;
        _iter->texture->MarkToRedraw(_redraw_token);
    }

    return false;
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
