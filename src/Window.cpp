#include "Window.hpp"
#include "Logger.hpp"
#include "Input.hpp"


Window::Window(Input *&input)
    : _input{input}
{
    gtk_init();

    _window = gtk_window_new ();
    gtk_window_set_title (GTK_WINDOW (_window), "nvim-ui");
    gtk_window_set_default_size (GTK_WINDOW (_window), 1024, 768);

    _grid = gtk_drawing_area_new ();
    gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (_grid), 1024);
    gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (_grid), 768);
    gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (_grid), _OnDraw, this, nullptr);
    gtk_widget_set_can_focus(_grid, true);
    gtk_widget_set_focusable(_grid, true);

    gtk_window_set_child (GTK_WINDOW (_window), _grid);

    g_signal_connect (G_OBJECT (_grid), "resize", G_CALLBACK(_OnResize), this);

    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
    g_signal_connect(GTK_EVENT_CONTROLLER_KEY(controller), "key-pressed", G_CALLBACK(_OnKeyPressed), this);
    gtk_widget_add_controller(_grid, controller);

    gtk_widget_show (_window);

    int scale = gtk_widget_get_scale_factor (_window);
    _painter.reset(new Painter(scale, scale));
}

Window::~Window()
{
    gtk_window_destroy(GTK_WINDOW(_window));
}

void Window::_OnResize(GtkDrawingArea *, int width, int height, gpointer data)
{
    reinterpret_cast<Window *>(data)->_OnResize2(width, height);
}

void Window::_OnResize2(int width, int height)
{
    std::lock_guard<std::mutex> guard{_mut};
    GdkSurface *s = gtk_native_get_surface(gtk_widget_get_native(_grid));
    _surface.reset(gdk_surface_create_similar_surface(s, CAIRO_CONTENT_COLOR, width, height));
    _cairo.reset(cairo_create(_surface.get()));
}

Window::RowsColsT
Window::GetRowsCols() const
{
    GtkAllocation allocation;
    gtk_widget_get_allocation (_grid, &allocation);

    int cols = std::max(1, allocation.width / _painter->GetCellWidth());
    int rows = std::max(1, allocation.height / _painter->GetCellHeight());
    return {rows, cols};
}

void Window::_OnDraw(GtkDrawingArea *, cairo_t *cr, int width, int height, gpointer data)
{
    reinterpret_cast<Window*>(data)->_OnDraw2(cr, width, height);
}

void Window::_OnDraw2(cairo_t *cr, int width, int height)
{
    std::lock_guard<std::mutex> guard{_mut};
    cairo_set_source_surface(cr, _surface.get(), 0, 0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
}

void Window::Clear(unsigned bg)
{
    std::lock_guard<std::mutex> guard{_mut};
    if (_cairo)
    {
        Painter::SetSource(_cairo.get(), bg);
        cairo_paint(_cairo.get());
    }
}

namespace {

struct Texture : IWindow::ITexture
{
    Texture(cairo_surface_t *t, int width, int height)
        : texture{t, cairo_surface_destroy}
        , width{width}
        , height{height}
    {
    }
    ::PtrT<cairo_surface_t> texture;
    int width;
    int height;
};

} //namespace;

void Window::CopyTexture(int row, int col, ITexture *texture)
{
    std::lock_guard<std::mutex> guard{_mut};

    if (!_cairo)
        return;

    Texture *t = static_cast<Texture *>(texture);

    cairo_save(_cairo.get());
    cairo_translate(_cairo.get(), col * _painter->GetCellWidth(), row * _painter->GetCellHeight());
    cairo_set_source_surface(_cairo.get(), t->texture.get(), 0, 0);
    cairo_rectangle(_cairo.get(), 0, 0, t->width, t->height);
    cairo_paint(_cairo.get());
    cairo_restore(_cairo.get());
}

IWindow::ITexture::PtrT
Window::CreateTexture(int width, std::string_view text, const HlAttr &attr, const HlAttr &def_attr)
{
    // not starts with "  "
    bool has_text = 0 != text.rfind("  ", 0);
    int pixel_width = (width + (has_text ? 1 : 0)) * _painter->GetCellWidth();
    int pixel_height = _painter->GetCellHeight();

    // Allocate a surface slightly wider than necessary
    GdkSurface *s = gtk_native_get_surface(gtk_widget_get_native(_grid));
    auto surface = PtrT<cairo_surface_t>(
        gdk_surface_create_similar_surface(s, CAIRO_CONTENT_COLOR, pixel_width, pixel_height),
        cairo_surface_destroy);

    // Paint the background color
    unsigned fg = attr.fg.value_or(def_attr.fg.value());
    unsigned bg = attr.bg.value_or(def_attr.bg.value());
    if ((attr.flags & HlAttr::F_REVERSE))
        std::swap(bg, fg);

    {
        auto cr = PtrT<cairo_t>(cairo_create(surface.get()), cairo_destroy);
        Painter::SetSource(cr.get(), bg);
        cairo_paint(cr.get());
    }

    if (has_text)
    {
        pixel_width = _painter->Paint(surface.get(), text, attr, def_attr);
    }

    std::unique_ptr<Texture> texture{new Texture(surface.release(), pixel_width, pixel_height)};
    return Texture::PtrT(texture.release());
}

void Window::Present()
{
    gtk_widget_queue_draw(_grid);
}

void Window::DrawCursor(int row, int col, unsigned fg, std::string_view mode)
{
    int cell_width = _painter->GetCellWidth();
    int cell_height = _painter->GetCellHeight();

    std::lock_guard<std::mutex> guard{_mut};

    PtrT<cairo_t> cr(cairo_create(_surface.get()), cairo_destroy);

    cairo_save(cr.get());
    cairo_translate(cr.get(), col * cell_width, row * cell_height);
    cairo_set_source_rgba(cr.get(),
        static_cast<double>(fg >> 16) / 255,
        static_cast<double>((fg >> 8) & 0xff) / 255,
        static_cast<double>(fg & 0xff) / 255,
        0.5);

    if (mode == "insert")
    {
        cairo_rectangle(cr.get(), 0, 0, 0.2 * cell_width, cell_height);
    }
    else if (mode == "replace" || mode == "operator")
    {
        cairo_rectangle(cr.get(), 0, 0.75 * cell_height, cell_width, 0.25 * cell_height);
    }
    else
    {
        cairo_rectangle(cr.get(), 0, 0, cell_width, cell_height);
    }
    cairo_fill(cr.get());
    cairo_restore(cr.get());
}

void Window::SetBusy(bool is_busy)
{
    //if (is_busy)
    //{
    //    if (!_busy_cursor)
    //        _busy_cursor.reset(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW));
    //    SDL_SetCursor(_busy_cursor.get());
    //}
    //else
    //{
    //    if (!_active_cursor)
    //        _active_cursor.reset(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
    //    SDL_SetCursor(_active_cursor.get());
    //}
}

gboolean Window::_OnKeyPressed(GtkEventControllerKey *,
                               guint                  keyval,
                               guint                  keycode,
                               GdkModifierType        state,
                               gpointer               data)
{
    return reinterpret_cast<Window *>(data)->_OnKeyPressed2(keyval, keycode, state);
}

gboolean Window::_OnKeyPressed2(guint keyval, guint keycode, GdkModifierType state)
{
    //string key = Gdk.keyval_name (keyval);
    //print ("* key pressed %u (%s) %u\n", keyval, key, keycode);

    gunichar uc = gdk_keyval_to_unicode(keyval);
    auto input = MkPtrT(g_string_append_unichar(g_string_new(nullptr), uc),
                                                [](GString *s) { g_string_free(s, true); });
    auto start_length = input->len;

    // TODO: functional keys, shift etc
    switch (keyval)
    {
    case GDK_KEY_Escape:
        input.reset(g_string_new("esc"));
        break;
    case GDK_KEY_Return:
        input.reset(g_string_new("cr"));
        break;
    case GDK_KEY_BackSpace:
        input.reset(g_string_new("bs"));
        break;
    case GDK_KEY_Tab:
        input.reset(g_string_new("tab"));
        break;
    case GDK_KEY_less:
        input.reset(g_string_new("lt"));
        break;
    }

    if (0 != (GDK_CONTROL_MASK & state))
    {
        input.reset(g_string_prepend(input.release(), "c-"));
    }
    if (0 != (GDK_META_MASK & state) || 0 != (GDK_ALT_MASK & state))
    {
        input.reset(g_string_prepend(input.release(), "m-"));
    }
    if (0 != (GDK_SUPER_MASK & state))
    {
        input.reset(g_string_prepend(input.release(), "d-"));
    }

    if (input->len != start_length)
    {
        std::string raw{input->str};
        g_string_printf(input.get(), "<%s>", raw.c_str());
    }

    _input->Accept(input->str);
    return true;
}
