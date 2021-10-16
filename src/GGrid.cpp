#include "GGrid.hpp"
#include "Logger.hpp"
#include "Renderer.hpp"
#include <sstream>

GGrid::GGrid(GtkWidget *grid, Session::PtrT &session)
    : _grid{grid}
    , _session{session}
{
    gtk_widget_set_focusable(_grid, true);

    _css_provider.reset(gtk_css_provider_new());
    gtk_style_context_add_provider(
            gtk_widget_get_style_context(_grid),
            GTK_STYLE_PROVIDER(_css_provider.get()),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

}

void GGrid::MeasureCell()
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

    std::string style = oss.str();
    Logger().debug("Updated CSS Style:\n{}", style);
    gtk_css_provider_load_from_data(_css_provider.get(), style.data(), -1);

    MeasureCell();
}

void GGrid::Present()
{
    auto renderer = _session->GetRenderer();
    auto guard = renderer->Lock();

    if (renderer->IsAttrMapModified())
    {
        renderer->MarkAttrMapProcessed();
        UpdateStyle();
    }

    // First remove outdated textures
    auto removeOutdated = [&] {
        auto it = std::partition(_textures.begin(), _textures.end(), [](auto &t) { return !t->ToBeDestroyed(); });
        for (auto it2 = it; it2 != _textures.end(); ++it2)
        {
            Texture *texture = static_cast<Texture *>(it2->get());
            if (texture->widget)
                gtk_fixed_remove(GTK_FIXED(_grid), texture->widget);
        }
        _textures.erase(it, _textures.end());
    };
    removeOutdated();

    size_t texture_count = 0;

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
            if (!t->widget)
            {
                t->widget = gtk_label_new(texture.text.c_str());

                gtk_style_context_add_provider(
                        gtk_widget_get_style_context(t->widget),
                        GTK_STYLE_PROVIDER(_css_provider.get()),
                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                std::string class_name = fmt::format("hl{}", texture.hl_id);
                gtk_widget_add_css_class(t->widget, class_name.data());

                gtk_fixed_put(GTK_FIXED(_grid), t->widget, x, y);
                _textures.push_back(texture.texture);
            }

            if (t->ToBeRedrawn())
            {
                gtk_fixed_move(GTK_FIXED(_grid), t->widget, x, y);
                t->MarkToRedraw(false);
            }
        }
    }

    assert(texture_count == _textures.size() && "Texture count consistency");
}

void GGrid::Clear()
{
    for (auto &t : _textures)
    {
        Texture *texture = static_cast<Texture *>(t.get());
        if (texture->widget)
            gtk_fixed_remove(GTK_FIXED(_grid), texture->widget);
    }
    _textures.clear();
}
