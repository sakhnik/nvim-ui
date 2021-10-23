#include "TextureCache.hpp"
#include "Logger.hpp"

void TextureCache::ForEach(std::function<void(const Texture &)> action)
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

bool TextureCache::Scanner::EnsureNext(Texture &&texture, GeneratorT generator)
{
    if (_started)
        ++_iter;
    else
        _started = true;

    // Remove potentially outdated cached textures
    while (_iter != _cache.end() && _iter->col < texture.col)
    {
        _iter->texture->MarkToDestroy();
        _iter = _cache.erase(_iter);
    }
    while (_iter != _cache.end() && _iter->col == texture.col
        && (_iter->hl_id != texture.hl_id || _iter->text != texture.text))
    {
        _iter->texture->MarkToDestroy();
        _iter = _cache.erase(_iter);
    }

    if (_iter == _cache.end()
        || _iter->hl_id != texture.hl_id
        || _iter->text != texture.text)
    {
        texture.texture = generator(texture);
        _iter = _cache.insert(_iter, std::move(texture));
        return true;
    }

    if (_iter->col != texture.col)
    {
        // The texture has just shifted
        _iter->col = texture.col;
        _iter->texture->MarkToRedraw(true);
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

void TextureCache::MoveFrom(TextureCache &o, int left, int right)
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
        it_from->texture->MarkToRedraw(true);
        _cache.splice(it++, o._cache, it_from++);
    }
}
