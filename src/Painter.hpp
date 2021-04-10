#pragma once

#include <string>
#include <SDL2/SDL_surface.h>

struct HlAttr;

class Painter
{
public:
    Painter(double scale_x, double scale_y);

    void Paint(SDL_Surface *, const std::string &text, const HlAttr &, const HlAttr &def);

private:
    double _scale_x = 1.0;
    double _scale_y = 1.0;
};
