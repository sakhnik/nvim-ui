#include "GGrid.hpp"
#include "Logger.hpp"
#include "Renderer.hpp"
#include "IWindowHandler.hpp"
#include "GFont.hpp"

#include "Gtk/DrawingArea.hpp"
#include "Gtk/EventController.hpp"
#include "Gtk/EventControllerKey.hpp"
#include "Gtk/Orientation.hpp"
#include "Gtk/PropagationPhase.hpp"
#include "Gtk/StyleContext.hpp"

#include <sstream>

#ifdef GIR_INLINE
#include "Gtk/ApplicationWindow.ipp"
#include "Gtk/CssProvider.ipp"
#include "Gtk/DrawingArea.ipp"
#include "Gtk/EventControllerKey.ipp"
#include "Gtk/Fixed.ipp"
#include "Gtk/Label.ipp"
#include "Gtk/StyleContext.ipp"
#endif


namespace Gdk = gir::Gdk;

GGrid::GGrid(Gtk::Fixed grid, GFont &font, Session::PtrT &session, IWindowHandler *window_handler)
    : _grid{grid}
    , _font{font}
    , _session{session}
    , _window_handler{window_handler}
    , _css_provider{Gtk::CssProvider::new_()}
{
    _grid.set_focusable(true);
    _grid.get_style_context().add_provider(_css_provider.get(), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    Gtk::DrawingArea cursor = Gtk::DrawingArea::new_().g_obj();
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

namespace {

std::string GetRuler()
{
    std::string str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 5; ++i)
        str += str;
    return str;
}

} // namespace;

void GGrid::MeasureCell()
{
    // Measure cell width and height
    static const std::string RULER = GetRuler();

    Gtk::Widget ruler{Gtk::Label::new_(RULER.c_str())};
    ruler.get_style_context().add_provider(_css_provider.get(), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    int width, height;
    ruler.measure(Gtk::Orientation::horizontal, -1, &width, nullptr, nullptr, nullptr);
    ruler.measure(Gtk::Orientation::vertical, -1, &height, nullptr, nullptr, nullptr);
    _cell_width = 1.0 * width / RULER.size();
    _cell_height = height;
    Logger().info("Measured cell: width={} height={}", _cell_width, _cell_height);
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

        if ((attr.flags & HlAttr::F_TEXT_DECORATION))
        {
            oss << "text-decoration-line: underline;\n";
            const char *style = "solid";
            if ((attr.flags & HlAttr::F_UNDERUNDERLINE))
                style = "double";
            else if ((attr.flags & HlAttr::F_UNDERCURL))
                style = "wavy";
            //else if ((attr.flags & HlAttr::F_UNDERDASH))
            //    style = "dashed";
            //else if ((attr.flags & HlAttr::F_UNDERDOT))
            //    style = "dotted";
            oss << "text-decoration-style: " << style << ";\n";
            if (attr.special.has_value())
                oss << fmt::format("text-decoration-color: #{:06x};\n", attr.special.value());
        }
        else if ((attr.flags & HlAttr::F_STRIKETHROUGH))
        {
            oss << "text-decoration-line: line-through;\n";
            oss << "text-decoration-style: " << "solid" << ";\n";
        }
    };

    auto renderer = _session->GetRenderer();
    assert(renderer);

    oss << "* {\n";
    oss << "font-family: " << _font.GetFamily() << ";\n";
    oss << "font-size: " << _font.GetSizePt() << "pt;\n";
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
    _tasks.push_back([=, this] { _Present(token); });
    _tasks.push_back([=, this] {
        auto renderer = _session->GetRenderer();
        if (!renderer)
            return;
        auto guard = renderer->Lock();
        _CheckSize(width, height);
    });

    _ExecuteTasks();
}

void GGrid::_ExecuteTasks()
{
    // Take the task from the front of the queue.
    if (_tasks.empty())
        return;
    _TaskT task = _tasks.front();
    _tasks.pop_front();

    // Execute the task
    task();

    // If there are more tasks, reschedule execution.
    if (!_tasks.empty())
    {
        auto on_timeout = [](gpointer data) {
            (reinterpret_cast<GGrid *>(data)->_ExecuteTasks)();
            return FALSE;
        };
        g_timeout_add(1, on_timeout, this);
    }
}

void GGrid::_Present(uint32_t token)
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
    _RemoveOutdated();

    // Then move the surviving labels to their new positions
    _MoveLabels(token);

    // Create and place new labels
    int last_row = _CreateLabelsInterrupted(0);
    if (-1 == last_row)
    {
        // The process of label creation wasn't interrupted, we can do
        // the sanity check. Count the active textures.
        _CheckConsistency();
    }

    _cursor->Move();
    _grid.set_cursor_from_name(renderer->IsBusy() ? "progress" : "default");
}

void GGrid::Clear()
{
    for (auto &t : _textures)
    {
        Texture *texture = static_cast<Texture *>(t.get());
        // TODO: Implement relation operators
        if (texture->label)
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
        double font_size_pt = _font.GetSizePt();
        if (keyval == GDK_KEY_equal)
            font_size_pt *= 1.1;
        else if (keyval == GDK_KEY_minus)
            font_size_pt /= 1.1;
        else if (keyval == GDK_KEY_0)
            font_size_pt = 14;
        if (font_size_pt != _font.GetSizePt())
        {
            auto guard = renderer->Lock();
            _font.SetSizePt(font_size_pt);
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

void GGrid::_RemoveOutdated()
{
    auto start_time = ClockT::now();

    // Count how many labels are going to be removed from the grid
    int labels_removed{};
    // Count how many textures are going to be destroyed
    int textures_deleted{};

    auto it_visible_end = std::partition(_textures.begin(), _textures.end(), [](auto &t) { return t->IsVisible(); });
    auto it_alive_end = std::partition(it_visible_end, _textures.end(), [](auto &t) { return t->IsAlive(); });

    for (auto it = it_visible_end; it != it_alive_end; ++it)
    {
        Texture *texture = static_cast<Texture *>(it->get());
        if (texture->label && texture->label.get_parent() == _grid)
        {
            texture->label.ref();
            _grid.remove(texture->label);
            ++labels_removed;
        }
    }

    for (auto it = it_alive_end; it != _textures.end(); ++it)
    {
        Texture *texture = static_cast<Texture *>(it->get());
        if (texture->label)
        {
            if (texture->label.get_parent() == _grid)
            {
                _grid.remove(texture->label);
                ++labels_removed;
            }
            else
                texture->label.unref();
            texture->label = nullptr;
        }
    }

    textures_deleted = _textures.end() - it_alive_end;
    _textures.erase(it_alive_end, _textures.end());

    auto finish_time = ClockT::now();
    auto duration = ToMs(finish_time - start_time).count();
    Logger().debug("GGrid::_RemoveOutdated labels_removed={} textures_deleted={} in {} ms",
            labels_removed, textures_deleted, duration);
}

void GGrid::_MoveLabels(uint32_t token)
{
    auto start_time = ClockT::now();

    // Count how many textures are going to be moved
    int labels_moved{};

    auto renderer = _session->GetRenderer();
    // Move the existing labels to their locations
    for (int row = 0, rowN = renderer->GetGridLines().size();
         row < rowN; ++row)
    {
        const auto &line = renderer->GetGridLines()[row];
        for (const auto &texture : line)
        {
            Texture *t = reinterpret_cast<Texture *>(texture.texture.get());
            int x = std::round(CalcX(texture.col));
            int y = row * _cell_height;

            if (t->TakeRedrawToken(token) && t->IsVisible())
            {
                _grid.move(t->label, x, y);
                ++labels_moved;
            }
        }
    }

    auto finish_time = ClockT::now();
    auto duration = ToMs(finish_time - start_time).count();
    Logger().debug("GGrid::_MoveLabels labels_moved={} in {} ms", labels_moved, duration);
}

int GGrid::_CreateLabels(int start_row)
{
    auto start_time = ClockT::now();

    // Count how many new textures are going to be created
    int labels_created{};

    // Create the newly appearing labels
    auto renderer = _session->GetRenderer();
    for (int row = start_row, rowN = renderer->GetGridLines().size();
         row < rowN; ++row)
    {
        // If creating new labels takes too much time (consider :digraphs, next page),
        // interrupt earlier. The rest of the labels will be created asynchronously
        // in _CreateLabelsInterrupted().
        // TODO: Make the number a configuration parameter.
        auto cur_time = ClockT::now();
        if (cur_time - start_time > std::chrono::milliseconds(100))
        {
            auto duration = ToMs(cur_time - start_time).count();
            Logger().debug("GGrid::_CreateLabels interrupted labels_created={} in {} ms", labels_created, duration);
            return row;
        }

        const auto &line = renderer->GetGridLines()[row];
        for (const auto &texture : line)
        {
            Texture *t = reinterpret_cast<Texture *>(texture.texture.get());
            int x = std::round(CalcX(texture.col));
            int y = row * _cell_height;
            if (!t->label)
            {
                t->label = Gtk::Label::new_(texture.text.c_str()).g_obj();
                t->label.set_sensitive(false);
                t->label.set_can_focus(false);
                t->label.set_focus_on_click(false);
                // Specify the width of the widget manually to make sure it occupies
                // the whole extent and isn't dependent on the font micro typing features.
                t->label.set_size_request(std::round(CalcX(texture.width)), 0);

                t->label.get_style_context().add_provider(_css_provider.get(), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                std::string class_name = fmt::format("hl{}", texture.hl_id);
                t->label.add_css_class(class_name.data());

                _grid.put(t->label, x, y);
                _textures.push_back(texture.texture);
                ++labels_created;
            }
        }
    }

    auto finish_time = ClockT::now();
    auto duration = ToMs(finish_time - start_time).count();
    Logger().debug("GGrid::_CreateLabels labels_created={} in {} ms", labels_created, duration);
    return -1;
}

int GGrid::_CreateLabelsInterrupted(int start_row)
{
    int last_row = _CreateLabels(start_row);
    if (last_row != -1)
    {
        // Haven't finished all the labels, reschedule continuation.
        _tasks.push_front([=, this] {
            auto renderer = _session->GetRenderer();
            if (!renderer)
                return;
            auto guard = renderer->Lock();

            _CreateLabelsInterrupted(last_row);

            _cursor->Move();
            _grid.set_cursor_from_name(renderer->IsBusy() ? "progress" : "default");
        });
    }
    return last_row;
}

void GGrid::_CheckConsistency()
{
    ssize_t texture_count = 0;
    auto renderer = _session->GetRenderer();
    for (int row = 0, rowN = renderer->GetGridLines().size();
         row < rowN; ++row)
    {
        const auto &line = renderer->GetGridLines()[row];
        texture_count += line.size();
    }
    auto visible_texture_count = std::count_if(_textures.begin(), _textures.end(), [](auto &t) { return t->IsVisible(); });
    assert(texture_count == visible_texture_count && "Texture count consistency");
}
