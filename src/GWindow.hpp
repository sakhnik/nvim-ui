#pragma once

#include "IWindow.hpp"
#include "Utils.hpp"
#include "Session.hpp"
#include "GGrid.hpp"
#include "IWindowHandler.hpp"
#include <string>
#include <memory>
#include <gtk/gtk.h>


class GWindow
    : public IWindow
    , public IWindowHandler
{
public:
    GWindow(GtkApplication *, Session::PtrT &session);
    ~GWindow();

    BaseTexture::PtrT CreateTexture(int width, std::string_view text, const HlAttr &, const HlAttr &def_attr) override;
    void Present(uint32_t token) override;
    void DrawCursor(cairo_t *, int row, int col, unsigned fg, std::string_view mode);
    void SessionEnd() override;

    void SetError(const char *error) override;

private:
    GtkApplication *_app;
    Session::PtrT &_session;

    PtrT<GtkBuilder> _builder = NullPtr<GtkBuilder>([](auto *b) { g_object_unref(b); });
    GtkWidget *_window;
    GtkWidget *_scroll;
    std::unique_ptr<GGrid> _grid;

    void _SetupWindow();
    void _SetupWindowSignals();
    void _SetupStatusLabel();

    void CheckSizeAsync() override;
    void _CheckSize();

    void _Present(uint32_t token);
    void _SessionEnd();

    void MenuBarToggle() override;
    void MenuBarHide() override;

    void _UpdateActions();
    void _EnableAction(const char *name, bool enable);
    void _OnQuitAction();
    void _OnSpawnAction();
    void _OnConnectAction();

    // A generic async pass to the Gtk thread.
    template <void (GWindow::*func)()>
    void _GtkTimer0(int ms)
    {
        auto on_timeout = [](gpointer data) {
            (reinterpret_cast<GWindow *>(data)->*func)();
            return FALSE;
        };
        g_timeout_add(ms, on_timeout, this);
    }

    static void _GtkTimer(int ms, std::function<void(void)> func)
    {
        using FuncT = decltype(func);
        FuncT *boxed_func = new FuncT{func};
        auto on_timeout = [](gpointer data) {
            std::unique_ptr<FuncT> f{reinterpret_cast<FuncT*>(data)};
            (*f.get())();
            return FALSE;
        };
        g_timeout_add(ms, on_timeout, boxed_func);
    }
};
