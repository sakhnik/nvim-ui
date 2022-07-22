#include "GWindow.hpp"
#include "config.hpp"
#include "Logger.hpp"
#include "Input.hpp"
#include "Renderer.hpp"
#include "SessionSpawn.hpp"
#include "SessionTcp.hpp"
#include "GSettingsDlg.hpp"

#include "Gtk/ApplicationWindow.hpp"
#include "Gtk/Dialog.hpp"
#include "Gtk/Entry.hpp"
#include "Gtk/Fixed.hpp"
#include "Gtk/IconTheme.hpp"
#include "Gtk/KeyvalTrigger.hpp"
#include "Gtk/Label.hpp"
#include "Gtk/NamedAction.hpp"
#include "Gtk/ResponseType.hpp"
#include "Gtk/Shortcut.hpp"
#include "Gtk/ShortcutController.hpp"
#include "Gtk/ShortcutScope.hpp"
#include "Gtk/StyleContext.hpp"
#include "Gtk/StyleProvider.hpp"
#include "Gtk/Window.hpp"

#include <iterator>
#include <msgpack/v1/unpack.hpp>
#include <sstream>
#include <fmt/format.h>

#ifdef GIR_INLINE
#include "Gtk/Application.ipp"
#include "Gtk/ApplicationWindow.ipp"
#include "Gtk/Builder.ipp"
#include "Gtk/Entry.ipp"
#include "Gtk/IconTheme.ipp"
#include "Gtk/KeyvalTrigger.ipp"
#include "Gtk/Label.ipp"
#include "Gtk/NamedAction.ipp"
#include "Gtk/Shortcut.ipp"
#include "Gtk/ShortcutController.ipp"
#include "Gtk/StyleContext.ipp"
#endif


GWindow::GWindow(const Gtk::Application &app, Session::PtrT &session)
    : _app{app}
    , _session{session}
    , _builder{Gtk::Builder::new_from_resource("/org/sakhnik/nvim-ui/gtk/main.ui")}
{
    Gtk::IconTheme icon_theme{Gtk::IconTheme::get_for_display(gdk_display_get_default())};
    icon_theme.add_resource_path("/org/sakhnik/nvim-ui/icons");

    _SetupWindow();

    // Font handler
    _font.reset(new GFont{_window});
    _font->Subscribe([this] {
        // Update the style
        auto guard = _session->GetRenderer()->Lock();
        _grid->UpdateStyle();
    });

    // Grid
    Gtk::Fixed grid{_builder.get_object("grid").g_obj()};
    _grid.reset(new GGrid(grid, *_font.get(), _session, this));

    // GTK wouldn't allow shrinking the window if there are widgets
    // placed in the grid. So the scroll view is required.
    _scroll = Gtk::Widget{_builder.get_object("scrolled_window").g_obj()};

    _SetupStatusLabel();

    _window.show();

    _SetupWindowSignals();

    if (_session && _session->GetRenderer())
    {
        // Initial style setup
        auto guard = _session->GetRenderer()->Lock();
        _grid->UpdateStyle();
    }

    // Adjust the grid size to the actual window size
    CheckSizeAsync();
}

GWindow::~GWindow()
{
    _window.destroy();
}

void GWindow::_SetupWindow()
{
    _window = Gtk::Window{_builder.get_object("main_window").g_obj()};
    _window.set_application(_app);

    const GActionEntry actions[] = {
        { "spawn", MakeCallback<&GWindow::_OnSpawnAction>(), nullptr, nullptr, nullptr, {0, 0, 0} },
        { "connect", MakeCallback<&GWindow::_OnConnectAction>(), nullptr, nullptr, nullptr, {0, 0, 0} },
        { "settings", MakeCallback<&GWindow::_OnSettingsAction>(), nullptr, nullptr, nullptr, {0, 0, 0} },
        { "quit", MakeCallback<&GWindow::_OnQuitAction>(), nullptr, nullptr, nullptr, {0, 0, 0} },
        { "inspect", MakeCallback<&GWindow::_OnInspectAction>(), nullptr, nullptr, nullptr, {0, 0, 0} },
        { "about", MakeCallback<&GWindow::_OnAboutAction>(), nullptr, nullptr, nullptr, {0, 0, 0} },
    };
    g_action_map_add_action_entries(G_ACTION_MAP(_window.g_obj()), actions, G_N_ELEMENTS(actions), this);

    auto controller = Gtk::ShortcutController::new_();
    auto &shortcut_controller = static_cast<Gtk::ShortcutController &>(controller);
    shortcut_controller.set_scope(Gtk::ShortcutScope::global);
    _window.add_controller(shortcut_controller);

    shortcut_controller.add_shortcut(Gtk::Shortcut::new_(
            Gtk::KeyvalTrigger::new_(GDK_KEY_n, GDK_CONTROL_MASK),
            Gtk::NamedAction::new_("win.spawn")));
    shortcut_controller.add_shortcut(Gtk::Shortcut::new_(
            Gtk::KeyvalTrigger::new_(GDK_KEY_t, GDK_CONTROL_MASK),
            Gtk::NamedAction::new_("win.connect")));
    shortcut_controller.add_shortcut(Gtk::Shortcut::new_(
            Gtk::KeyvalTrigger::new_(GDK_KEY_q, GDK_CONTROL_MASK),
            Gtk::NamedAction::new_("win.quit")));

    _UpdateActions();
}

void GWindow::_SetupWindowSignals()
{
    using SizeChangedT = gboolean (*)(GObject *, GParamSpec *, gpointer data);
    SizeChangedT sizeChanged = [](auto *, auto *, gpointer data) {
        reinterpret_cast<GWindow *>(data)->CheckSizeAsync();
        return FALSE;
    };

    g_signal_connect(_window.g_obj(), "notify::default-width", G_CALLBACK(sizeChanged), this);
    g_signal_connect(_window.g_obj(), "notify::default-height", G_CALLBACK(sizeChanged), this);

    _window.on_show(_window, [this](auto) { CheckSizeAsync(); });

    _window.on_close_request(_window, [this](auto) -> gboolean {
        Logger().info("GWindow close request");
        if (_session)
            _session->SetWindow(nullptr);
        else
            _app.quit();
        _app.remove_window(_window);
        _window.close();
        return false;
    });
}

void GWindow::_SetupStatusLabel()
{
    Gtk::Label status_label{_builder.get_object("status").g_obj()};
    status_label.get_style_context().add_provider(_grid->GetStyle(), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    status_label.set_visible(false);
}

void GWindow::CheckSizeAsync()
{
    _GtkTimer0<&GWindow::_CheckSize>(1);
}

void GWindow::_CheckSize()
{
    int width = _scroll.get_allocated_width();
    int height = _scroll.get_allocated_height();
    _grid->CheckSize(width, height);
}

void GWindow::Present()
{
    _GtkTimer(1, [this]() { _Present(); });
}

void GWindow::_Present()
{
    int width = _scroll.get_allocated_width();
    int height = _scroll.get_allocated_height();
    _grid->Present(width, height);
}

void GWindow::SessionEnd()
{
    _GtkTimer0<&GWindow::_SessionEnd>(1);
}

void GWindow::_SessionEnd()
{
    Logger().info("Session end");
    _grid->Clear();

    Gtk::Label status_label{_builder.get_object("status").g_obj()};
    if (_session && !_session->GetOutput().empty())
    {
        status_label.set_text(_session->GetOutput().c_str());
        status_label.set_visible(true);
    }
    _session.reset();

    _UpdateActions();

    // TODO: change the style for some fancy background
}

void GWindow::SetError(const char *error)
{
    _title = "nvim-ui";
    if (_session)
    {
        _title += " " + _session->GetDescription();
    }
    if (error)
    {
        _title += " (";
        _title += error;
        _title += ")";
    }
    _UpdateTitle();
}

void GWindow::_UpdateTitle()
{
    auto &app_window = static_cast<Gtk::ApplicationWindow &>(_window);
    bool is_menu_visible = app_window.get_show_menubar();
    if (!is_menu_visible)
    {
        std::string title{_title};
        title += " (";
        title += _("Press ALT to reach the menu");
        title += ")";
        _window.set_title(title.c_str());
    }
    else
    {
        _window.set_title(_title.c_str());
    }
}

void GWindow::MenuBarToggle()
{
    // Toggle the menubar
    auto &app_window = static_cast<Gtk::ApplicationWindow &>(_window);
    bool is_menu_visible = app_window.get_show_menubar();
    app_window.set_show_menubar(!is_menu_visible);
    _UpdateTitle();
}

void GWindow::MenuBarHide()
{
    auto &app_window = static_cast<Gtk::ApplicationWindow &>(_window);
    app_window.set_show_menubar(false);
    _UpdateTitle();
}

void GWindow::_UpdateActions()
{
    bool session_active = _session.get() != nullptr;
    _EnableAction("spawn", !session_active);
    _EnableAction("connect", !session_active);
    _EnableAction("quit", !session_active);
    _window.set_deletable(!session_active);
}

void GWindow::_EnableAction(const char *name, bool enable)
{
    GAction *a = g_action_map_lookup_action(G_ACTION_MAP(_window.g_obj()), name);
    g_simple_action_set_enabled(G_SIMPLE_ACTION(a), enable);
}

void GWindow::_OnQuitAction(GSimpleAction *, GVariant *)
{
    Logger().info("Bye!");
    auto app = _window.get_application();
    _window.destroy();
    app.quit();
}

void GWindow::_OnInspectAction(GSimpleAction *, GVariant *)
{
    Gtk::Window::set_interactive_debugging(true);
}

void GWindow::_OnAboutAction(GSimpleAction *, GVariant *)
{
    const char *authors[] = {
        _("Anatolii Sakhnik https://sakhnik.com"),
        nullptr
    };

    gtk_show_about_dialog(nullptr,
            "program-name", "nvim-ui",
            "title", _("About nvim-ui"),
            "logo-icon-name", "nvim-ui",
            "authors", authors,
            "license-type", GTK_LICENSE_MIT_X11,
            "website", "https://github.com/sakhnik/nvim-ui",
            "website-label", _("GitHub: sakhnik/nvim-ui"),
            "version", VERSION,
            NULL);
}

void GWindow::_OnSpawnAction(GSimpleAction *, GVariant *)
{
    Logger().info("Spawn new");
    try
    {
        _session.reset(new SessionSpawn(0, nullptr));
        SetError(nullptr);
        _session->SetWindow(this);
        _session->RunAsync();
    }
    catch (std::exception &ex)
    {
        Logger().error("Failed to spawn: {}", ex.what());
        SetError(ex.what());
    }
    _UpdateActions();
}

void GWindow::_OnConnectAction(GSimpleAction *, GVariant *)
{
    auto builder = Gtk::Builder::new_from_resource("/org/sakhnik/nvim-ui/gtk/connect-dlg.ui");
    Gtk::Dialog dlg{builder.get_object("connect_dlg").g_obj()};
    dlg.set_transient_for(_window);

    dlg.on_response(dlg, [this, builder](Gtk::Dialog dlg, gint response) {
        _OnConnectDlgResponse(dlg, response, builder);
    });
    dlg.on_destroy(dlg, [builder](Gtk::Dialog) {
        builder.unref();
    });
    dlg.show();
}

void GWindow::_OnConnectDlgResponse(Gtk::Dialog &dlg, gint response, Gtk::Builder builder)
{
    dlg.destroy();
    if (Gtk::ResponseType::ok != response)
        return;
    Gtk::Entry address_entry{builder.get_object("address_entry").g_obj()};
    Gtk::Entry port_entry{builder.get_object("port_entry").g_obj()};

    const char *address = address_entry.get_text();
    int port = std::stoi(port_entry.get_text());
    Logger().info("Connect to {}:{}", address, port);

    try
    {
        _session.reset(new SessionTcp(address, port));
        SetError(nullptr);
        _session->SetWindow(this);
        _session->RunAsync();
    }
    catch (std::exception &ex)
    {
        Logger().error("Failed to spawn: {}", ex.what());
        SetError(ex.what());
    }
    _UpdateActions();
}

void GWindow::_OnSettingsAction(GSimpleAction *, GVariant *)
{
    GSettingsDlg dlg{_window, *_font.get()};
}

void GWindow::SetGuiFont(const std::string &value)
{
    _GtkTimer(1, [this, value]() {
        _font->SetGuiFont(value);
    });
}
