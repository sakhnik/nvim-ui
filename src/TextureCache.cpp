#include "TextureCache.hpp"
#include "Logger.hpp"

void TextureCache::ForEach(std::function<void(const Chunk &)> action)
{
    for (auto &texture : _cache)
        action(texture);
}

TextureCache::Scanner::~Scanner()
{
    if (_started)
        ++_iter;
    // The unscanned entries are considered outdated
    for (auto it = _iter; it != _cache.end(); ++it)
    {
        it->texture->MarkToDestroy();
    }
    _cache.erase(_iter, _cache.end());
}

bool TextureCache::Scanner::EnsureNext(Chunk &&chunk, GeneratorT generator)
{
    if (_started)
        ++_iter;
    else
        _started = true;

    // Remove potentially outdated cached chunks
    while (_iter != _cache.end() && _iter->col < chunk.col)
    {
        _iter->texture->MarkToDestroy();
        _iter = _cache.erase(_iter);
    }
    while (_iter != _cache.end() && _iter->col == chunk.col
        && (_iter->hl_id != chunk.hl_id || _iter->text != chunk.text))
    {
        _iter->texture->MarkToDestroy();
        _iter = _cache.erase(_iter);
    }

    if (_iter == _cache.end()
        || _iter->hl_id != chunk.hl_id
        || _iter->text != chunk.text)
    {
        chunk.texture = generator(chunk);
        _iter = _cache.insert(_iter, std::move(chunk));
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

void TextureCache::Clear()
{
    for (auto &t : _cache)
    {
        t.texture->MarkToDestroy();
    }
    _cache.clear();
}

void TextureCache::MoveFrom(TextureCache &o, int left, int right, uint32_t redraw_token)
{
    // Find the beginning of the source cache
    auto it_from = o._cache.begin();
    while (it_from != o._cache.end() && it_from->col < left)
        ++it_from;

    auto it = _cache.begin();
    while (it_from != o._cache.end() && it_from->col < right)
    {
        // Find a suitable place to insert to still maintaining the sorted order
        while (it != _cache.end() && it->col < it_from->col)
            ++it;
        it_from->texture->MarkToRedraw(redraw_token);
        _cache.splice(it++, o._cache, it_from++);
    }
}
