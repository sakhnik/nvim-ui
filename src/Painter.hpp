#pragma once

#include <SDL2/SDL_surface.h>

class Painter
{
public:
    Painter(double scale_x, double scale_y);

    void Paint(SDL_Surface *);

private:
    double _scale_x = 1.0;
    double _scale_y = 1.0;
};
