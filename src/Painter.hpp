#pragma once

#include "Utils.hpp"
#include <string>
#include <cairo/cairo.h>
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
    int Paint(cairo_surface_t *, std::string_view text, const HlAttr &, const HlAttr &def);

    static void SetSource(cairo_t *, unsigned rgb);

private:
    double _scale_x = 1.0;
    double _scale_y = 1.0;
    int _cell_width = 0.0;
    int _cell_height = 0.0;
    PtrT<PangoFontDescription> _font_desc;
};
