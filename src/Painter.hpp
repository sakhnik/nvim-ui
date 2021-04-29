#pragma once

#include "Utils.hpp"
#include <string>
#include <SDL2/SDL_surface.h>
#include <pango/pango-font.h>

struct HlAttr;

class Painter
{
public:
    Painter(double scale_x, double scale_y);

    void CalcCellInfo();
    int GetCellWidth() const { return _cell_width; }
    int GetCellHeight() const { return _cell_height; }

    // Returns actual width
    int Paint(SDL_Surface *, std::string_view text, const HlAttr &, const HlAttr &def);

private:
    double _scale_x = 1.0;
    double _scale_y = 1.0;
    int _cell_width = 0.0;
    int _cell_height = 0.0;
    PtrT<PangoFontDescription> _font_desc;
};
