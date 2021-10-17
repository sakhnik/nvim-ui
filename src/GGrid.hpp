#pragma once

#include "Session.hpp"
#include "IWindow.hpp"
#include "Utils.hpp"
#include "GPointer.hpp"
#include <vector>
#include <gtk/gtk.h>

class GGrid
{
public:
    GGrid(GtkWidget *grid, Session::PtrT &session);

    GtkStyleProvider* GetStyle() const
    {
        return GTK_STYLE_PROVIDER(_css_provider.get());
    }

    void UpdateStyle();
    void MeasureCell();
    void Present();
    void Clear();

    struct Texture : IWindow::ITexture
    {
        // Non-owning
        GtkWidget *widget = nullptr;
    };

    double CalcX(int col) const
    {
        return 1.0 * col * _cell_width / PANGO_SCALE;
    }

    double CalcY(int row) const
    {
        return row * _cell_height;
    }

    GtkFixed* GetFixed() const { return GTK_FIXED(_grid); }

private:
    GtkWidget *_grid;
    Session::PtrT &_session;
    PtrT<GtkCssProvider> _css_provider = NullPtr<GtkCssProvider>([](auto *p) { g_object_unref(p); });

    int _cell_width = 0;
    int _cell_height = 0;
    std::vector<IWindow::ITexture::PtrT> _textures;

    GPointer _pointer;
};
