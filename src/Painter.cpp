#include "Painter.hpp"
#include "Utils.hpp"
#include "HlAttr.hpp"
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

    pango_font_description_set_weight(_font_desc.get(), PANGO_WEIGHT_NORMAL);
    pango_font_description_set_style(_font_desc.get(), PANGO_STYLE_NORMAL);

    auto layout = PtrT<PangoLayout>(pango_cairo_create_layout(cr.get()),
            [](PangoLayout *l) { g_object_unref(l); });
    pango_layout_set_font_description(layout.get(), _font_desc.get());
    const std::string_view RULER = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    pango_layout_set_text(layout.get(), RULER.data(), RULER.size());
    PangoRectangle rect;
    pango_layout_get_extents(layout.get(), nullptr, &rect);
    _cell_width = _scale_x * rect.width / (PANGO_SCALE * RULER.size());
    _cell_height = _scale_y * rect.height / PANGO_SCALE;
}

namespace {

void SetSource(cairo_t *cr, unsigned rgb)
{
    cairo_set_source_rgb(cr,
        static_cast<double>(rgb >> 16) / 255,
        static_cast<double>((rgb >> 8) & 0xff) / 255,
        static_cast<double>(rgb & 0xff) / 255);
}

PtrT<PangoAttrList> CreateAttrList(const HlAttr &attr)
{
    PtrT<PangoAttrList> al(nullptr, [](PangoAttrList *l) { pango_attr_list_unref(l); });

    auto add = [&](PangoAttribute *a) {
        if (!al)
            al.reset(pango_attr_list_new());
        pango_attr_list_insert(al.get(), a);
    };

    if ((attr.flags & HlAttr::F_UNDERLINE))
        add(pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
    if ((attr.flags & HlAttr::F_UNDERCURL))
    {
        add(pango_attr_underline_new(PANGO_UNDERLINE_ERROR));
        add(pango_attr_underline_color_new(65535, 0, 0));
    }

    return al;
}

} //namespace;

void Painter::Paint(SDL_Surface *surface, std::string_view text, const HlAttr &attr, const HlAttr &def_attr)
{
    auto cr_surface = PtrT<cairo_surface_t>(
        cairo_image_surface_create_for_data(static_cast<unsigned char *>(surface->pixels),
            CAIRO_FORMAT_RGB24, surface->w, surface->h, surface->pitch),
        cairo_surface_destroy);
    cairo_surface_set_device_scale(cr_surface.get(), _scale_x, _scale_y);
    auto cr = PtrT<cairo_t>(cairo_create(cr_surface.get()), cairo_destroy);

    unsigned bg = attr.bg.value_or(def_attr.bg.value());
    unsigned fg = attr.fg.value_or(def_attr.fg.value());
    if ((attr.flags & HlAttr::F_REVERSE))
        std::swap(bg, fg);

    // Print the foreground text
    SetSource(cr.get(), fg);
    cairo_move_to(cr.get(), 0, 0);
    pango_font_description_set_weight(_font_desc.get(),
            (attr.flags & HlAttr::F_BOLD) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
    pango_font_description_set_style(_font_desc.get(),
            (attr.flags & HlAttr::F_ITALIC) ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);

    auto layout = PtrT<PangoLayout>(pango_cairo_create_layout(cr.get()),
            [](PangoLayout *l) { g_object_unref(l); });
    pango_layout_set_font_description(layout.get(), _font_desc.get());
    pango_layout_set_text(layout.get(), text.data(), text.size());

    auto al = CreateAttrList(attr);
    if (al)
        pango_layout_set_attributes(layout.get(), al.get());

    pango_cairo_show_layout(cr.get(), layout.get());
}
