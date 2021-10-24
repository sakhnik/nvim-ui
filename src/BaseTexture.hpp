#pragma once

#include <memory>
#include <vector>

class BaseTexture
{
public:
    using PtrT = std::shared_ptr<BaseTexture>;
    virtual ~BaseTexture() {}

    // Check whether this texture is to be redrawn with the given
    // redraw token. Remove the token if present.
    bool TakeRedrawToken(uint32_t token);

    // Mark this texture to be redrawn with a redraw token.
    void MarkToRedraw(uint32_t token) { _redraw_tokens.push_back(token); }

    bool ToBeDestroyed() const { return _destroy; }
    void MarkToDestroy(bool destroy = true) { _destroy = destroy; }

private:
    std::vector<uint32_t> _redraw_tokens;
    bool _destroy = false;
};
