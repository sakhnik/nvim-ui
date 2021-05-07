#include "Window.hpp"
#include "Logger.hpp"
#include "Input.hpp"
#include "Renderer.hpp"

#include <sstream>
#include <fmt/format.h>


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

    _css_provider.reset(gtk_css_provider_new());
    gtk_style_context_add_provider(
            gtk_widget_get_style_context(_grid),
            GTK_STYLE_PROVIDER(_css_provider.get()),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

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

IWindow::ITexture::PtrT
Window::CreateTexture(int width, std::string_view text, const HlAttr &attr, const HlAttr &def_attr)
{
    return Texture::PtrT(new Texture(attr, def_attr));
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

void Window::_Present()
{
    auto guard = _renderer->Lock();

    if (_style.empty())
    {
        std::ostringstream oss;
        auto mapAttr = [&](const HlAttr &attr, const HlAttr &def_attr) {
            if ((attr.flags & HlAttr::F_REVERSE))
            {
                auto fg = attr.fg.value_or(def_attr.fg.value());
                auto bg = attr.bg.value_or(def_attr.bg.value());
                oss << fmt::format("background-color: #{:06x};\n", fg);
                oss << fmt::format("color: #{:06x};\n", bg);
            }
            else
            {
                if (attr.bg.has_value())
                    oss << fmt::format("background-color: #{:06x};\n", attr.bg.value());
                if (attr.fg.has_value())
                    oss << fmt::format("color: #{:06x};\n", attr.fg.value());
            }
            if ((attr.flags & HlAttr::F_ITALIC))
                oss << "font-style: italic;\n";
            if ((attr.flags & HlAttr::F_BOLD))
                oss << "font-weight: bold;\n";
        };

        oss << "* {\n";
        oss << "font-family: Fira Code;\n";
        oss << "font-size: 14pt;\n";
        mapAttr(_renderer->GetDefAttr(), _renderer->GetDefAttr());
        oss << "}\n";

        for (const auto &id_attr : _renderer->GetAttrMap())
        {
            int id = id_attr.first;
            const auto &attr = id_attr.second;

            oss << "\n";
            oss << "label.hl" << id << " {\n";
            mapAttr(attr, _renderer->GetDefAttr());
            oss << "}\n";
        }

        _style = oss.str();
        Logger().info("{}", _style);
        gtk_css_provider_load_from_data(_css_provider.get(), _style.data(), -1);
    }

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

                gtk_style_context_add_provider(
                        gtk_widget_get_style_context(t->widget),
                        GTK_STYLE_PROVIDER(_css_provider.get()),
                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                std::string class_name = fmt::format("hl{}", texture.hl_id);
                gtk_widget_add_css_class(t->widget, class_name.data());
            }
            if (t->widget)
            {
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
    auto input = MkPtr(g_string_append_unichar(g_string_new(nullptr), uc),
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
