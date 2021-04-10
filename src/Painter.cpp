#include "Painter.hpp"
#include "Utils.hpp"
#include <cairo/cairo.h>

Painter::Painter(double scale_x, double scale_y)
    : _scale_x{scale_x}
    , _scale_y{scale_y}
{
}

void Painter::Paint(SDL_Surface *surface)
{
    auto cr_surface = PtrT<cairo_surface_t>(
        cairo_image_surface_create_for_data(static_cast<unsigned char *>(surface->pixels),
            CAIRO_FORMAT_RGB24, surface->w, surface->h, surface->pitch),
        cairo_surface_destroy);
    cairo_surface_set_device_scale(cr_surface.get(), _scale_x, _scale_y);
    auto cr = PtrT<cairo_t>(cairo_create(cr_surface.get()), cairo_destroy);

}
