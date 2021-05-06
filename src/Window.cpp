#include "Window.hpp"
#include "Logger.hpp"
#include "Input.hpp"
#include "Renderer.hpp"


Window::Window(Renderer *renderer, Input *input)
    : _renderer{renderer}
    , _input{input}
{
    gtk_init();

    _window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(_window), "nvim-ui");
    gtk_window_set_default_size(GTK_WINDOW(_window), 1024, 768);

    _grid = gtk_fixed_new();
    gtk_widget_set_can_focus(_grid, true);
    gtk_widget_set_focusable(_grid, true);

    gtk_window_set_child(GTK_WINDOW(_window), _grid);

    g_signal_connect(G_OBJECT(_grid), "resize", G_CALLBACK(_OnResize), this);

    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
    g_signal_connect(GTK_EVENT_CONTROLLER_KEY(controller), "key-pressed", G_CALLBACK(_OnKeyPressed), this);
    gtk_widget_add_controller(_grid, controller);

    gtk_widget_show(_window);

    int scale = gtk_widget_get_scale_factor(_window);
    _painter.reset(new Painter(scale, scale));

    _renderer->AttachWindow(this);
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
    Logger().info("OnResize {} {}", width, height);
    int cols = std::max(1, width / _painter->GetCellWidth());
    int rows = std::max(1, height / _painter->GetCellHeight());
    _renderer->OnResized(rows, cols);
}

namespace {

struct Texture : IWindow::ITexture
{
    // Non-owning
    GtkWidget *widget = nullptr;
    HlAttr hl_attr;

    Texture(const HlAttr &attr, const HlAttr &def_attr)
        : hl_attr{attr}
    {
        if (!hl_attr.fg.has_value())
            hl_attr.fg = def_attr.fg;
        if (!hl_attr.bg.has_value())
            hl_attr.bg = def_attr.bg;
    }
};

} //namespace;

//void Window::_OnDraw(GtkDrawingArea *, cairo_t *cr, int width, int height, gpointer data)
//{
//    reinterpret_cast<Window*>(data)->_OnDraw2(cr, width, height);
//}
//
//void Window::_OnDraw2(cairo_t *cr, int width, int height)
//{
//    auto guard = _renderer->Lock();
//
//    cairo_save(cr);
//    Painter::SetSource(cr, _renderer->GetBg());
//    cairo_paint(cr);
//    cairo_restore(cr);
//
//    for (int row = 0, rowN = _renderer->GetGridLines().size();
//         row < rowN; ++row)
//    {
//        const auto &line = _renderer->GetGridLines()[row];
//        for (const auto &texture : line)
//        {
//            Texture *t = static_cast<Texture *>(texture.texture.get());
//            auto col = texture.col;
//
//            cairo_save(cr);
//            cairo_translate(cr, col * _painter->GetCellWidth(), row * _painter->GetCellHeight());
//            cairo_set_source_surface(cr, t->texture.get(), 0, 0);
//            cairo_rectangle(cr, 0, 0, t->width, t->height);
//            cairo_fill(cr);
//            cairo_restore(cr);
//        }
//    }
//
//    if (!_renderer->IsBusy())
//    {
//        DrawCursor(cr, _renderer->GetCursorRow(), _renderer->GetCursorCol(), _renderer->GetFg(), _renderer->GetMode());
//    }
//}

IWindow::ITexture::PtrT
Window::CreateTexture(int width, std::string_view text, const HlAttr &attr, const HlAttr &def_attr)
{
    // not starts with "  "
    //bool has_text = 0 != text.rfind("  ", 0);
    //int pixel_width = (width + (has_text ? 1 : 0)) * _painter->GetCellWidth();
    //int pixel_height = _painter->GetCellHeight();

    return Texture::PtrT(new Texture(attr, def_attr));


    //// Allocate a surface slightly wider than necessary
    //GdkSurface *s = gtk_native_get_surface(gtk_widget_get_native(_grid));
    //auto surface = PtrT<cairo_surface_t>(
    //    gdk_surface_create_similar_surface(s, CAIRO_CONTENT_COLOR, pixel_width, pixel_height),
    //    cairo_surface_destroy);

    //// Paint the background color
    //unsigned fg = attr.fg.value_or(def_attr.fg.value());
    //unsigned bg = attr.bg.value_or(def_attr.bg.value());
    //if ((attr.flags & HlAttr::F_REVERSE))
    //    std::swap(bg, fg);

    //{
    //    auto cr = PtrT<cairo_t>(cairo_create(surface.get()), cairo_destroy);
    //    Painter::SetSource(cr.get(), bg);
    //    cairo_paint(cr.get());
    //}

    //if (has_text)
    //{
    //    pixel_width = _painter->Paint(surface.get(), text, attr, def_attr);
    //}
}

void Window::Present()
{
    g_main_context_invoke(nullptr, _Present, this);
}

gboolean Window::_Present(gpointer data)
{
    reinterpret_cast<Window *>(data)->_Present();
    return false;
}

namespace {

class FontMgr
{
public:
    FontMgr()
        : _font_desc(pango_font_description_new(), pango_font_description_free)
    {
        pango_font_description_set_family(_font_desc.get(), "Fira Code");
        pango_font_description_set_absolute_size(_font_desc.get(), 20 * PANGO_SCALE);
    }

    PtrT<PangoAttrList> CreateAttrList(const std::string &text, const HlAttr &attr)
    {
        pango_font_description_set_weight(_font_desc.get(),
                (attr.flags & HlAttr::F_BOLD) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
        pango_font_description_set_style(_font_desc.get(),
                (attr.flags & HlAttr::F_ITALIC) ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);

        PtrT<PangoAttrList> al(pango_attr_list_new(), [](PangoAttrList *l) { pango_attr_list_unref(l); });

        pango_attr_list_insert(al.get(), pango_attr_font_desc_new(_font_desc.get()));

        if ((attr.flags & HlAttr::F_UNDERLINE))
            pango_attr_list_insert(al.get(), pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
        if ((attr.flags & HlAttr::F_UNDERCURL))
        {
            pango_attr_list_insert(al.get(), pango_attr_underline_new(PANGO_UNDERLINE_ERROR));
            pango_attr_list_insert(al.get(), pango_attr_underline_color_new(65535, 0, 0));
        }

        return al;
    }

private:
    PtrT<PangoFontDescription> _font_desc;
};

} //namespace;

void Window::_Present()
{
    auto guard = _renderer->Lock();

    decltype(_widgets) widgets;

    for (int row = 0, rowN = _renderer->GetGridLines().size();
         row < rowN; ++row)
    {
        const auto &line = _renderer->GetGridLines()[row];
        for (const auto &texture : line)
        {
            Texture *t = reinterpret_cast<Texture *>(texture.texture.get());
            int x = texture.col * _painter->GetCellWidth();
            int y = row * _painter->GetCellHeight();
            if (!t->widget && !texture.IsSpace())
            {
                t->widget = gtk_label_new(texture.text.c_str());

                FontMgr font_mgr;
                auto al = font_mgr.CreateAttrList(texture.text, t->hl_attr);
                gtk_label_set_attributes(GTK_LABEL(t->widget), al.get());
            }
            if (gtk_widget_get_parent(t->widget))
            {
                gtk_fixed_move(GTK_FIXED(_grid), t->widget, x, y);
            }
            else
            {
                gtk_fixed_put(GTK_FIXED(_grid), t->widget, x, y);
            }
            widgets.insert(t->widget);
            _widgets.erase(t->widget);
        }
    }

    for (auto *w : _widgets)
    {
        gtk_fixed_remove(GTK_FIXED(_grid), w);
    }
    _widgets.swap(widgets);
}

void Window::DrawCursor(cairo_t *cr, int row, int col, unsigned fg, std::string_view mode)
{
    int cell_width = _painter->GetCellWidth();
    int cell_height = _painter->GetCellHeight();

    cairo_save(cr);
    cairo_translate(cr, col * cell_width, row * cell_height);
    cairo_set_source_rgba(cr,
        static_cast<double>(fg >> 16) / 255,
        static_cast<double>((fg >> 8) & 0xff) / 255,
        static_cast<double>(fg & 0xff) / 255,
        0.5);

    if (mode == "insert")
    {
        cairo_rectangle(cr, 0, 0, 0.2 * cell_width, cell_height);
    }
    else if (mode == "replace" || mode == "operator")
    {
        cairo_rectangle(cr, 0, 0.75 * cell_height, cell_width, 0.25 * cell_height);
    }
    else
    {
        cairo_rectangle(cr, 0, 0, cell_width, cell_height);
    }
    cairo_fill(cr);
    cairo_restore(cr);
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
