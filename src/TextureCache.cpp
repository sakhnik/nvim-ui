#include "TextureCache.hpp"

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
        _iter = _cache.erase(_iter);
    while (_iter != _cache.end() && _iter->col == texture.col
        && (_iter->hl_id != texture.hl_id || _iter->text != texture.text))
        _iter = _cache.erase(_iter);

    if (_iter == _cache.end()
        || _iter->col != texture.col || _iter->hl_id != texture.hl_id
        || _iter->text != texture.text || !_iter->texture)
    {
        texture.texture = generator(texture);
        _iter = _cache.insert(_iter, std::move(texture));
        return true;
    }

    return false;
}

void TextureCache::Clear()
{
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
        _cache.splice(it++, o._cache, it_from++);
    }
}
