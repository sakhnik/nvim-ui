#include "Painter.hpp"
#include "Utils.hpp"
#include <cairo/cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <iostream>

Painter::Painter(double scale_x, double scale_y)
    : _scale_x{scale_x}
    , _scale_y{scale_y}
    , _font_desc(pango_font_description_new(), pango_font_description_free)
{
    pango_font_description_set_family(_font_desc.get(), "monospace");
    pango_font_description_set_absolute_size(_font_desc.get(), 20 * PANGO_SCALE);

    CalcCellInfo();
}

void Painter::CalcCellInfo()
{
    unsigned char pixel;
    auto cr_surface = PtrT<cairo_surface_t>(
        cairo_image_surface_create_for_data(&pixel,
            CAIRO_FORMAT_RGB24, 9999, 9999, 9999 * 3),
        cairo_surface_destroy);
    cairo_surface_set_device_scale(cr_surface.get(), _scale_x, _scale_y);
    auto cr = PtrT<cairo_t>(cairo_create(cr_surface.get()), cairo_destroy);

    auto layout = PtrT<PangoLayout>(pango_cairo_create_layout(cr.get()),
            [](PangoLayout *l) { g_object_unref(l); });
    pango_layout_set_font_description(layout.get(), _font_desc.get());
    const std::string_view RULER = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    pango_layout_set_text(layout.get(), RULER.data(), RULER.size());
    PangoRectangle rect;
    pango_layout_get_extents(layout.get(), nullptr, &rect);
    _cell_width = 1.0 * rect.width / (PANGO_SCALE * RULER.size());
    _cell_height = 1.0 * rect.height / PANGO_SCALE;
}

void Painter::Paint(SDL_Surface *surface, const std::string &text, const HlAttr &attr, const HlAttr &def_attr)
{
    auto cr_surface = PtrT<cairo_surface_t>(
        cairo_image_surface_create_for_data(static_cast<unsigned char *>(surface->pixels),
            CAIRO_FORMAT_RGB24, surface->w, surface->h, surface->pitch),
        cairo_surface_destroy);
    cairo_surface_set_device_scale(cr_surface.get(), _scale_x, _scale_y);
    auto cr = PtrT<cairo_t>(cairo_create(cr_surface.get()), cairo_destroy);

    auto layout = PtrT<PangoLayout>(pango_cairo_create_layout(cr.get()),
            [](PangoLayout *l) { g_object_unref(l); });
    pango_layout_set_font_description(layout.get(), _font_desc.get());
    pango_layout_set_text(layout.get(), text.data(), text.size());

    cairo_set_source_rgb(cr.get(), 1, 0, 0);
    cairo_move_to(cr.get(), 0, 0);
    pango_cairo_show_layout(cr.get(), layout.get());
}
