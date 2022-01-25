#include "GGrid.hpp"
#include "Logger.hpp"
#include "Renderer.hpp"
#include "IWindowHandler.hpp"

#include "Gtk/StyleContext.hpp"
#include "Gtk/EventController.hpp"
#include "Gtk/EventControllerKey.hpp"
#include "Gtk/PropagationPhase.hpp"

#include <sstream>

namespace Gdk = gir::Gdk;

GGrid::GGrid(Gtk::Fixed grid, Session::PtrT &session, IWindowHandler *window_handler)
    : _grid{grid}
    , _session{session}
    , _window_handler{window_handler}
    , _css_provider{Gtk::CssProvider::new_()}
{
    _grid.set_focusable(true);

    _grid.get_style_context().add_provider(
        _css_provider,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );

    GtkWidget *cursor = gtk_drawing_area_new();
    _cursor.reset(new GCursor{cursor, this, _session});

    Gtk::EventControllerKey controller = Gtk::EventControllerKey::new_().g_obj();
    controller.set_propagation_phase(Gtk::PropagationPhase::capture);
    controller.on_key_pressed(controller, [this](Gtk::EventControllerKey, guint keyval, guint keycode, Gdk::ModifierType state) -> gboolean {
        return _OnKeyPressed(keyval, keycode, state);
    });
    controller.on_key_released(controller, [this](Gtk::EventControllerKey, guint keyval, guint keycode, Gdk::ModifierType state) {
        _OnKeyReleased(keyval, keycode, state);
    });
    grid.add_controller(controller);
}

void GGrid::MeasureCell()
{
    // Measure cell width and height
    const char *const RULER = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    Gtk::Widget ruler{Gtk::Label::new_(RULER)};
    ruler.get_style_context().add_provider(_css_provider.get(), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    int width, height;
    gtk_widget_measure(GTK_WIDGET(ruler.g_obj()),
            GTK_ORIENTATION_HORIZONTAL, -1, &width, nullptr, nullptr, nullptr);
    gtk_widget_measure(GTK_WIDGET(ruler.g_obj()),
            GTK_ORIENTATION_VERTICAL, -1, &height, nullptr, nullptr, nullptr);
    _cell_width = width * PANGO_SCALE / strlen(RULER);
    _cell_height = height;
    Logger().info("Measured cell: width={} height={}", static_cast<double>(_cell_width) / PANGO_SCALE, _cell_height);
    ruler.ref_sink();

    _cursor->UpdateSize();
}

void GGrid::UpdateStyle()
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
    assert(renderer);

    oss << "* {\n";
    oss << "font-family: Fira Code;\n";
    oss << "font-size: " << _font_size_pt << "pt;\n";
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

    std::string style = oss.str();
    Logger().debug("Updated CSS Style:\n{}", style);
    _css_provider.load_from_data(style.data(), -1);

    MeasureCell();
    _window_handler->CheckSizeAsync();
}

void GGrid::Present(int width, int height, uint32_t token)
{
    auto renderer = _session->GetRenderer();
    assert(renderer);
    auto guard = renderer->Lock();

    if (renderer->IsAttrMapModified())
    {
        renderer->MarkAttrMapProcessed();
        UpdateStyle();
    }

    // First remove outdated textures
    auto removeOutdated = [&] {
        auto it_visible_end = std::partition(_textures.begin(), _textures.end(), [](auto &t) { return t->IsVisible(); });
        auto it_alive_end = std::partition(it_visible_end, _textures.end(), [](auto &t) { return t->IsAlive(); });

        for (auto it = it_visible_end; it != it_alive_end; ++it)
        {
            Texture *texture = static_cast<Texture *>(it->get());
            if (texture->label.g_obj() && texture->label.get_parent().g_obj() == _grid.g_obj())
            {
                texture->label.ref();
                _grid.remove(texture->label);
            }
        }

        for (auto it = it_alive_end; it != _textures.end(); ++it)
        {
            Texture *texture = static_cast<Texture *>(it->get());
            if (texture->label.g_obj())
            {
                if (texture->label.get_parent().g_obj() == _grid.g_obj())
                    _grid.remove(texture->label);
                else
                    texture->label.unref();
                texture->label = nullptr;
            }
        }

        _textures.erase(it_alive_end, _textures.end());
    };
    removeOutdated();

    ssize_t texture_count = 0;

    for (int row = 0, rowN = renderer->GetGridLines().size();
         row < rowN; ++row)
    {
        const auto &line = renderer->GetGridLines()[row];
        texture_count += line.size();
        for (const auto &texture : line)
        {
            Texture *t = reinterpret_cast<Texture *>(texture.texture.get());
            int x = texture.col * _cell_width / PANGO_SCALE;
            int y = row * _cell_height;
            if (!t->label.g_obj())
            {
                t->label = Gtk::Label::new_(texture.text.c_str()).g_obj();

                t->label.get_style_context().add_provider(_css_provider.get(), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                std::string class_name = fmt::format("hl{}", texture.hl_id);
                t->label.add_css_class(class_name.data());

                _grid.put(t->label, x, y);
                _textures.push_back(texture.texture);
            }

            if (t->TakeRedrawToken(token) && t->IsVisible())
            {
                _grid.move(t->label, x, y);
            }
        }
    }

    auto visible_texture_count = std::count_if(_textures.begin(), _textures.end(), [](auto &t) { return t->IsVisible(); });
    assert(texture_count == visible_texture_count && "Texture count consistency");

    _cursor->Move();
    _pointer.Update(renderer->IsBusy(), GTK_WIDGET(_grid.g_obj()));
    _CheckSize(width, height);
}

void GGrid::Clear()
{
    for (auto &t : _textures)
    {
        Texture *texture = static_cast<Texture *>(t.get());
        // TODO: Implement relation operators
        if (texture->label.g_obj())
            _grid.remove(texture->label);
    }
    _textures.clear();
    _cursor->Hide();
}

gboolean GGrid::_OnKeyPressed(guint keyval, guint /*keycode*/, GdkModifierType state)
{
    //string key = Gdk.keyval_name (keyval);
    //print ("* key pressed %u (%s) %u\n", keyval, key, keycode);

    if (keyval == GDK_KEY_Alt_L)
    {
        _alt_pending = true;
        return true;
    }
    _alt_pending = false;
    _window_handler->MenuBarHide();

    if (!_session)
        return true;
    auto renderer = _session->GetRenderer();
    if (!renderer)
        return false;

    if (0 != (GDK_CONTROL_MASK & state))
    {
        double font_size_pt = _font_size_pt;
        if (keyval == GDK_KEY_equal)
            font_size_pt *= 1.1;
        else if (keyval == GDK_KEY_minus)
            font_size_pt /= 1.1;
        else if (keyval == GDK_KEY_0)
            font_size_pt = 14;
        if (font_size_pt != _font_size_pt)
        {
            auto guard = renderer->Lock();
            _font_size_pt = font_size_pt;
            UpdateStyle();
            return true;
        }
    }

    gunichar uc = gdk_keyval_to_unicode(keyval);
    auto input = MkPtr(g_string_append_unichar(g_string_new(nullptr), uc),
                                               [](GString *s) { g_string_free(s, true); });
    auto start_length = input->len;

    // TODO: functional keys, shift etc
    switch (keyval)
    {
    case GDK_KEY_Escape:        input.reset(g_string_new("esc"));   break;
    case GDK_KEY_Return:        input.reset(g_string_new("cr"));    break;
    case GDK_KEY_BackSpace:     input.reset(g_string_new("bs"));    break;
    case GDK_KEY_Tab:           input.reset(g_string_new("tab"));   break;
    case GDK_KEY_less:          input.reset(g_string_new("lt"));    break;
    case GDK_KEY_Left:          input.reset(g_string_new("Left"));  break;
    case GDK_KEY_Right:         input.reset(g_string_new("Right")); break;
    case GDK_KEY_Up:            input.reset(g_string_new("Up"));    break;
    case GDK_KEY_Down:          input.reset(g_string_new("Down"));  break;
    }

    auto remapForMeta = [](decltype(input) &in) -> auto& {
        if (in->len != 1)
            return in;
        char &ch = in->str[0];
        const char SHIFTS[] = ")!@#$%^&*(";
        if (ch >= '0' && ch <= '9')
            ch = SHIFTS[ch - '0'];
        return in;
    };

    if (0 != (GDK_CONTROL_MASK & state))
    {
        input.reset(g_string_prepend(remapForMeta(input).release(), "c-"));
    }
    if (0 != (GDK_META_MASK & state) || 0 != (GDK_ALT_MASK & state))
    {
        input.reset(g_string_prepend(remapForMeta(input).release(), "m-"));
    }
    if (0 != (GDK_SUPER_MASK & state))
    {
        input.reset(g_string_prepend(remapForMeta(input).release(), "d-"));
    }

    if (input->len != start_length)
    {
        std::string raw{input->str};
        g_string_printf(input.get(), "<%s>", raw.c_str());
    }

    _session->GetInput()->Accept(input->str);
    return true;
}

void GGrid::_OnKeyReleased(guint keyval, guint /*keycode*/, GdkModifierType /*state*/)
{
    if (_alt_pending && keyval == GDK_KEY_Alt_L)
    {
        _window_handler->MenuBarToggle();
    }
}

void GGrid::_CheckSize(int width, int height)
{
    if (!_session)
        return;

    int cols = std::max(1, static_cast<int>(width / CalcX(1)));
    int rows = std::max(1, static_cast<int>(height / CalcY(1)));
    // Dejitter to request resizing only once
    if (cols == _last_cols && rows == _last_rows)
        return;
    _last_cols = cols;
    _last_rows = rows;
    auto renderer = _session->GetRenderer();
    if (renderer && (cols != renderer->GetWidth() || rows != renderer->GetHeight()))
    {
        Logger().info("Grid size change detected rows={} cols={}", rows, cols);
        renderer->OnResized(rows, cols);
    }
}

void GGrid::CheckSize(int width, int height)
{
    if (!_session)
        return;

    auto renderer = _session->GetRenderer();
    if (!renderer)
        return;

    auto guard = renderer->Lock();
    _CheckSize(width, height);
}
