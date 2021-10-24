#include "BaseTexture.hpp"

bool BaseTexture::TakeRedrawToken(uint32_t token)
{
    auto it = std::remove(_redraw_tokens.begin(), _redraw_tokens.end(), token);
    if (it == _redraw_tokens.end())
        return false;
    _redraw_tokens.erase(it, _redraw_tokens.end());
    return true;
}
