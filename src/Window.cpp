#include "Window.hpp"
#include "Logger.hpp"
#include "Input.hpp"
#include "Renderer.hpp"

#include <sstream>
#include <spdlog/fmt/fmt.h>

Window::Window(GtkApplication *app, Session::PtrT &session)
    : _app{app}
    , _session{session}
{
    GtkIconTheme *icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    gtk_icon_theme_add_resource_path(icon_theme, "/org/nvim-ui/icons");

    _builder = gtk_builder_new_from_resource("/org/nvim-ui/gtk/main.ui");

    _window = GTK_WIDGET(gtk_builder_get_object(_builder, "main_window"));
    gtk_window_set_application(GTK_WINDOW(_window), app);
    if (_session)
        gtk_window_set_deletable(GTK_WINDOW(_window), false);

    _grid = GTK_WIDGET(gtk_builder_get_object(_builder, "grid"));
    gtk_widget_set_focusable(_grid, true);

    _css_provider.reset(gtk_css_provider_new());
    gtk_style_context_add_provider(
            gtk_widget_get_style_context(_grid),
            GTK_STYLE_PROVIDER(_css_provider.get()),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // GTK wouldn't allow shrinking the window if there are widgets
    // placed in the _grid. So the scroll view is required.
    _scroll = GTK_WIDGET(gtk_builder_get_object(_builder, "scrolled_window"));

    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(_builder, "status"));
    gtk_style_context_add_provider(
            gtk_widget_get_style_context(status_label),
            GTK_STYLE_PROVIDER(_css_provider.get()),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_widget_set_visible(status_label, false);

    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
    using OnKeyPressedT = gboolean (*)(GtkEventControllerKey *,
                                       guint                  keyval,
                                       guint                  keycode,
                                       GdkModifierType        state,
                                       gpointer               data);
    OnKeyPressedT onKeyPressed = [](auto *, guint keyval, guint keycode, GdkModifierType state, gpointer data) {
        return reinterpret_cast<Window *>(data)->_OnKeyPressed(keyval, keycode, state);
    };
    g_signal_connect(GTK_EVENT_CONTROLLER_KEY(controller), "key-pressed", G_CALLBACK(onKeyPressed), this);
    OnKeyPressedT onKeyReleased = [](auto *, guint keyval, guint keycode, GdkModifierType state, gpointer data) {
        return reinterpret_cast<Window *>(data)->_OnKeyReleased(keyval, keycode, state);
    };
    g_signal_connect(GTK_EVENT_CONTROLLER_KEY(controller), "key-released", G_CALLBACK(onKeyReleased), this);
    gtk_widget_add_controller(_grid, controller);

    gtk_widget_show(_window);

    using SizeChangedT = gboolean (*)(GObject *, GParamSpec *, gpointer data);
    SizeChangedT sizeChanged = [](auto *, auto *, gpointer data) {
        reinterpret_cast<Window *>(data)->_CheckSizeAsync();
        return FALSE;
    };

    g_signal_connect(_window, "notify::default-width", G_CALLBACK(sizeChanged), this);
    g_signal_connect(_window, "notify::default-height", G_CALLBACK(sizeChanged), this);

    using OnShowT = void (*)(GtkWidget *, gpointer);
    OnShowT onShow = [](auto *, gpointer data) {
        reinterpret_cast<Window *>(data)->_CheckSizeAsync();
    };
    g_signal_connect(_window, "show", G_CALLBACK(onShow), this);

    using OnCloseT = gboolean (*)(GtkWindow *, gpointer);
    OnCloseT on_close = [](auto *, gpointer data) {
        Logger().info("Window close request");
        Window *w = reinterpret_cast<Window *>(data);
        if (w->_session)
        {
            w->_session->SetWindow(nullptr);
        }
        else
        {
            g_application_quit(G_APPLICATION(w->_app));
        }
        gtk_application_remove_window(w->_app, GTK_WINDOW(w->_window));
        gtk_window_close(GTK_WINDOW(w->_window));
        return FALSE;
    };
    g_signal_connect(_window, "close-request", G_CALLBACK(on_close), this);

    if (_session)
    {
        // Initial style setup
        auto guard = _session->GetRenderer()->Lock();
        _UpdateStyle();
    }

    _MeasureCell();

    _cursor = GTK_WIDGET(gtk_builder_get_object(_builder, "cursor"));
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(_cursor), _cell_width / PANGO_SCALE);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(_cursor), _cell_height);

    auto drawCursor = [](GtkDrawingArea *da, cairo_t *cr, int width, int height, gpointer data) {
        reinterpret_cast<Window *>(data)->_DrawCursor(da, cr, width, height);
    };
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(_cursor), drawCursor, this, nullptr);

    // Adjust the grid size to the actual window size
    _CheckSizeAsync();
}

Window::~Window()
{
    gtk_window_destroy(GTK_WINDOW(_window));
    g_object_unref(_builder);
}

void Window::_CheckSizeAsync()
{
    g_timeout_add(0, [](gpointer data) {
            reinterpret_cast<Window *>(data)->_CheckSize();
            return FALSE;
        }, this);
}

void Window::_CheckSize()
{
    if (!_session)
        return;

    int width = gtk_widget_get_allocated_width(_scroll);
    int height = gtk_widget_get_allocated_height(_scroll);

    int cols = std::max(1, width * PANGO_SCALE / _cell_width);
    int rows = std::max(1, height / _cell_height);
    // Dejitter to request resizing only once
    if (cols == _last_cols && rows == _last_rows)
        return;
    _last_cols = cols;
    _last_rows = rows;
    auto renderer = _session->GetRenderer();
    if (cols != renderer->GetWidth() || rows != renderer->GetHeight())
    {
        Logger().info("Grid size change detected rows={} cols={}", rows, cols);
        renderer->OnResized(rows, cols);
    }
}

void Window::_MeasureCell()
{
    // Measure cell width and height
    const char *const RULER = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    GtkWidget *ruler = gtk_label_new(RULER);
    gtk_style_context_add_provider(
            gtk_widget_get_style_context(ruler),
            GTK_STYLE_PROVIDER(_css_provider.get()),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    int width, height;
    gtk_widget_measure(ruler, GTK_ORIENTATION_HORIZONTAL, -1, &width, nullptr, nullptr, nullptr);
    gtk_widget_measure(ruler, GTK_ORIENTATION_VERTICAL, -1, &height, nullptr, nullptr, nullptr);
    _cell_width = width * PANGO_SCALE / strlen(RULER);
    _cell_height = height;
    Logger().info("Measured cell: width={} height={}", static_cast<double>(_cell_width) / PANGO_SCALE, _cell_height);
    g_object_ref_sink(ruler);
}

namespace {

struct Texture : IWindow::ITexture
{
    // Non-owning
    GtkWidget *widget = nullptr;
};

} //namespace;

IWindow::ITexture::PtrT
Window::CreateTexture(int /*width*/, std::string_view /*text*/,
                      const HlAttr &/*attr*/, const HlAttr &/*def_attr*/)
{
    return Texture::PtrT(new Texture);
}

void Window::Present()
{
    auto present = [](gpointer data) {
        reinterpret_cast<Window *>(data)->_Present();
        return FALSE;
    };
    g_timeout_add(0, present, this);
}

void Window::SessionEnd()
{
    auto session_end = [](gpointer data) {
        reinterpret_cast<Window *>(data)->_SessionEnd();
        return FALSE;
    };
    g_timeout_add(0, session_end, this);
}

void Window::_SessionEnd()
{
    Logger().info("Session end");
    for (auto *w : _widgets)
    {
        gtk_fixed_remove(GTK_FIXED(_grid), w);
    }
    _widgets.clear();
    gtk_widget_hide(_cursor);

    gtk_window_set_deletable(GTK_WINDOW(_window), true);

    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(_builder, "status"));
    if (_session && !_session->GetOutput().empty())
    {
        gtk_widget_set_visible(status_label, true);
        gtk_label_set_text(GTK_LABEL(status_label), _session->GetOutput().c_str());
    }
    _session.reset();

    // TODO: change the style for some fancy background
}

void Window::_UpdateStyle()
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

    auto renderer = _session->GetRenderer();

    oss << "* {\n";
    oss << "font-family: Fira Code;\n";
    oss << "font-size: 14pt;\n";
    mapAttr(renderer->GetDefAttr(), renderer->GetDefAttr());
    oss << "}\n";

    oss << "label.status {\n";
    oss << "color: #cccccc;";
    oss << "}\n";

    for (const auto &id_attr : renderer->GetAttrMap())
    {
        int id = id_attr.first;
        const auto &attr = id_attr.second;

        oss << "\n";
        oss << "label.hl" << id << " {\n";
        mapAttr(attr, renderer->GetDefAttr());
        oss << "}\n";
    }

    _style = oss.str();
    Logger().debug("Updated CSS Style:\n{}", _style);
    gtk_css_provider_load_from_data(_css_provider.get(), _style.data(), -1);

    _MeasureCell();
}

void Window::_Present()
{
    auto renderer = _session->GetRenderer();
    auto guard = renderer->Lock();

    if (renderer->IsAttrMapModified())
    {
        renderer->MarkAttrMapProcessed();
        _UpdateStyle();
    }

    decltype(_widgets) widgets;

    for (int row = 0, rowN = renderer->GetGridLines().size();
         row < rowN; ++row)
    {
        const auto &line = renderer->GetGridLines()[row];
        for (const auto &texture : line)
        {
            Texture *t = reinterpret_cast<Texture *>(texture.texture.get());
            int x = texture.col * _cell_width / PANGO_SCALE;
            int y = row * _cell_height;
            if (!t->widget)
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

    // Move the cursor
    if (gtk_widget_get_parent(_cursor))
    {
        g_object_ref(_cursor);
        gtk_fixed_remove(GTK_FIXED(_grid), _cursor);
    }
    if (!renderer->IsBusy())
    {
        gtk_fixed_put(GTK_FIXED(_grid), _cursor,
                1.0 * renderer->GetCursorCol() * _cell_width / PANGO_SCALE,
                renderer->GetCursorRow() * _cell_height);
    }

    if (renderer->IsBusy())
    {
        if (!_busy_cursor)
            _busy_cursor.reset(gdk_cursor_new_from_name("progress", nullptr));
        gtk_widget_set_cursor(_grid, _busy_cursor.get());
    }
    else
    {
        if (!_active_cursor)
            _active_cursor.reset(gdk_cursor_new_from_name("default", nullptr));
        gtk_widget_set_cursor(_grid, _active_cursor.get());
    }

    _CheckSize();
}

void Window::_DrawCursor(GtkDrawingArea *, cairo_t *cr, int /*width*/, int /*height*/)
{
    if (!_session)
        return;

    auto renderer = _session->GetRenderer();
    auto lock = renderer->Lock();
    double cell_width = 1.0 * _cell_width / PANGO_SCALE;

    cairo_save(cr);
    unsigned fg = renderer->GetFg();
    cairo_set_source_rgba(cr,
        static_cast<double>(fg >> 16) / 255,
        static_cast<double>((fg >> 8) & 0xff) / 255,
        static_cast<double>(fg & 0xff) / 255,
        0.5);

    const auto &mode = renderer->GetMode();
    if (mode == "insert")
    {
        cairo_rectangle(cr, 0, 0, 0.2 * cell_width, _cell_height);
    }
    else if (mode == "replace" || mode == "operator")
    {
        cairo_rectangle(cr, 0, 0.75 * _cell_height, cell_width, 0.25 * _cell_height);
    }
    else
    {
        cairo_rectangle(cr, 0, 0, cell_width, _cell_height);
    }
    cairo_fill(cr);
    cairo_restore(cr);
}

gboolean Window::_OnKeyPressed(guint keyval, guint /*keycode*/, GdkModifierType state)
{
    //string key = Gdk.keyval_name (keyval);
    //print ("* key pressed %u (%s) %u\n", keyval, key, keycode);

    if (keyval == GDK_KEY_Alt_L)
    {
        _alt_pending = true;
        return true;
    }
    _alt_pending = false;
    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(_window), false);

    if (!_session)
        return true;

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

    _session->GetInput()->Accept(input->str);
    return true;
}

gboolean Window::_OnKeyReleased(guint keyval, guint /*keycode*/, GdkModifierType /*state*/)
{
    if (_alt_pending && keyval == GDK_KEY_Alt_L)
    {
        // Toggle the menubar
        gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(_window),
            !gtk_application_window_get_show_menubar(GTK_APPLICATION_WINDOW(_window)));
    }
    return true;
}

void Window::SetError(const char *error)
{
    std::string title = "nvim-ui";
    if (error)
    {
        title += " - ";
        title += error;
    }
    gtk_window_set_title(GTK_WINDOW(_window), title.c_str());
}
