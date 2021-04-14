#include "TextureCache.hpp"

void TextureCache::ForEach(std::function<void(const Texture &)> action)
{
    for (auto &texture : _cache)
        action(texture);
}

bool TextureCache::ClearUntil(IterT &tit, const Texture &texture)
{
    // Remove potentially outdated cached textures
    while (tit != _cache.end() && tit->col < texture.col)
        tit = _cache.erase(tit);
    while (tit != _cache.end() && tit->col == texture.col
        && (tit->hl_id != texture.hl_id || tit->text != texture.text))
        tit = _cache.erase(tit);

    return tit == _cache.end()
        || tit->col != texture.col || tit->hl_id != texture.hl_id
        || tit->text != texture.text || !tit->texture;
}

void TextureCache::Insert(IterT &tit, Texture &&texture)
{
    tit = _cache.insert(tit, std::move(texture));
}

void TextureCache::RemoveTheRest(IterT tit)
{
    _cache.erase(tit, _cache.end());
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
