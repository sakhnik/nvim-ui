#include "GWindow.hpp"
#include "Logger.hpp"
#include "Input.hpp"
#include "Renderer.hpp"
#include "SessionSpawn.hpp"

#include <sstream>
#include <spdlog/fmt/fmt.h>

GWindow::GWindow(GtkApplication *app, Session::PtrT &session)
    : _app{app}
    , _session{session}
{
    GtkIconTheme *icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    gtk_icon_theme_add_resource_path(icon_theme, "/org/nvim-ui/icons");

    _builder.reset(gtk_builder_new_from_resource("/org/nvim-ui/gtk/main.ui"));

    _SetupWindow();

    // Grid
    GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(_builder.get(), "grid"));
    _grid.reset(new GGrid(grid, _session, this));

    // GTK wouldn't allow shrinking the window if there are widgets
    // placed in the grid. So the scroll view is required.
    _scroll = GTK_WIDGET(gtk_builder_get_object(_builder.get(), "scrolled_window"));

    _SetupStatusLabel();

    gtk_widget_show(_window);

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
    gtk_window_destroy(GTK_WINDOW(_window));
}

void GWindow::_SetupWindow()
{
    _window = GTK_WIDGET(gtk_builder_get_object(_builder.get(), "main_window"));
    gtk_window_set_application(GTK_WINDOW(_window), _app);
    if (_session)
        gtk_window_set_deletable(GTK_WINDOW(_window), false);

    using ActionCbT = void (*)(GSimpleAction *, GVariant *, gpointer);
    ActionCbT action_cb = [](GSimpleAction *action, GVariant *, gpointer) {
        Logger().info("Action {}", g_action_get_name(G_ACTION(action)));
    };
    ActionCbT quit_cb = [](GSimpleAction *, GVariant *, gpointer data) {
        reinterpret_cast<GWindow*>(data)->_OnQuitAction();
    };
    ActionCbT spawn_cb = [](GSimpleAction *, GVariant *, gpointer data) {
        reinterpret_cast<GWindow*>(data)->_OnSpawnAction();
    };

    const GActionEntry actions[] = {
        { "spawn", spawn_cb, nullptr, nullptr, nullptr, {0, 0, 0} },
        { "connect", action_cb, nullptr, nullptr, nullptr, {0, 0, 0} },
        { "quit", quit_cb, nullptr, nullptr, nullptr, {0, 0, 0} },
    };
    g_action_map_add_action_entries(G_ACTION_MAP(_window), actions, G_N_ELEMENTS(actions), this);

    GtkEventController *controller = gtk_shortcut_controller_new();
    gtk_shortcut_controller_set_scope(GTK_SHORTCUT_CONTROLLER(controller), GTK_SHORTCUT_SCOPE_GLOBAL);
    gtk_widget_add_controller(_window, controller);
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
            gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_n, GDK_CONTROL_MASK),
                             gtk_named_action_new("win.spawn")));
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
            gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_t, GDK_CONTROL_MASK),
                             gtk_named_action_new("win.connect")));
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller),
            gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_q, GDK_CONTROL_MASK),
                             gtk_named_action_new("win.quit")));

    _UpdateActions();
}

void GWindow::_SetupWindowSignals()
{
    using SizeChangedT = gboolean (*)(GObject *, GParamSpec *, gpointer data);
    SizeChangedT sizeChanged = [](auto *, auto *, gpointer data) {
        reinterpret_cast<GWindow *>(data)->CheckSizeAsync();
        return FALSE;
    };

    g_signal_connect(_window, "notify::default-width", G_CALLBACK(sizeChanged), this);
    g_signal_connect(_window, "notify::default-height", G_CALLBACK(sizeChanged), this);

    using OnShowT = void (*)(GtkWidget *, gpointer);
    OnShowT onShow = [](auto *, gpointer data) {
        reinterpret_cast<GWindow *>(data)->CheckSizeAsync();
    };
    g_signal_connect(_window, "show", G_CALLBACK(onShow), this);

    using OnCloseT = gboolean (*)(GWindow *, gpointer);
    OnCloseT on_close = [](auto *, gpointer data) {
        Logger().info("GWindow close request");
        GWindow *w = reinterpret_cast<GWindow *>(data);
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
}

void GWindow::_SetupStatusLabel()
{
    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(_builder.get(), "status"));
    gtk_style_context_add_provider(
            gtk_widget_get_style_context(status_label),
            GTK_STYLE_PROVIDER(_grid->GetStyle()),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_widget_set_visible(status_label, false);
}

void GWindow::CheckSizeAsync()
{
    _GtkTimer0<&GWindow::_CheckSize>(0);
}

void GWindow::_CheckSize()
{
    int width = gtk_widget_get_allocated_width(_scroll);
    int height = gtk_widget_get_allocated_height(_scroll);
    _grid->CheckSize(width, height);
}

BaseTexture::PtrT
GWindow::CreateTexture(int /*width*/, std::string_view /*text*/,
                       const HlAttr &/*attr*/, const HlAttr &/*def_attr*/)
{
    return BaseTexture::PtrT(new GGrid::Texture);
}

void GWindow::Present(uint32_t token)
{
    _GtkTimer(0, [this, token]() { _Present(token); });
}

void GWindow::_Present(uint32_t token)
{
    int width = gtk_widget_get_allocated_width(_scroll);
    int height = gtk_widget_get_allocated_height(_scroll);
    _grid->Present(width, height, token);
}

void GWindow::SessionEnd()
{
    _GtkTimer0<&GWindow::_SessionEnd>(0);
}

void GWindow::_SessionEnd()
{
    Logger().info("Session end");
    _grid->Clear();

    gtk_window_set_deletable(GTK_WINDOW(_window), true);

    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(_builder.get(), "status"));
    if (_session && !_session->GetOutput().empty())
    {
        gtk_widget_set_visible(status_label, true);
        gtk_label_set_text(GTK_LABEL(status_label), _session->GetOutput().c_str());
    }
    _session.reset();

    _UpdateActions();

    // TODO: change the style for some fancy background
}

void GWindow::SetError(const char *error)
{
    std::string title = "nvim-ui";
    if (error)
    {
        title += " - ";
        title += error;
    }
    gtk_window_set_title(GTK_WINDOW(_window), title.c_str());
}

void GWindow::MenuBarToggle()
{
    // Toggle the menubar
    bool is_menu_visible = gtk_application_window_get_show_menubar(GTK_APPLICATION_WINDOW(_window));
    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(_window), !is_menu_visible);
}

void GWindow::MenuBarHide()
{
    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(_window), false);
}

void GWindow::_UpdateActions()
{
    bool session_active = _session.get() != nullptr;
    _EnableAction("spawn", !session_active);
    _EnableAction("connect", !session_active);
    _EnableAction("quit", !session_active);
}

void GWindow::_EnableAction(const char *name, bool enable)
{
    GAction *a = g_action_map_lookup_action(G_ACTION_MAP(_window), name);
    g_simple_action_set_enabled(G_SIMPLE_ACTION(a), enable);
}

void GWindow::_OnQuitAction()
{
    Logger().info("Bye!");
    GtkApplication *app = gtk_window_get_application(GTK_WINDOW(_window));
    gtk_window_destroy(GTK_WINDOW(_window));
    g_application_quit(G_APPLICATION(app));
}

void GWindow::_OnSpawnAction()
{
    Logger().info("Spawn new");
    std::string error;
    try
    {
        SetError(nullptr);
        _session.reset(new SessionSpawn(0, nullptr));
        _session->SetWindow(this);
        _session->RunAsync();
    }
    catch (std::exception &ex)
    {
        Logger().error("Failed to spawn: {}", ex.what());
        SetError(ex.what());
    }
}
