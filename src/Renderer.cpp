#include "Renderer.hpp"
#include "MsgPackRpc.hpp"
#include "Timer.hpp"
#include "IWindow.hpp"
#include <iostream>


Renderer::Renderer(MsgPackRpc *rpc, IWindow *window, Timer *timer)
    : _rpc{rpc}
    , _window{window}
    , _timer{timer}
{
    // Default hightlight attributes
    _def_attr.fg = 0xffffff;
    _def_attr.bg = 0;

    // Create a window
    _window->Init();
    auto [rows, cols] = _window->GetRowsCols();

    // Prepare the initial cell grid to fill the whole window.
    // The NeoVim UI will be attached using these dimensions.
    GridResize(cols, rows);
}

Renderer::~Renderer()
{
    _window->Deinit();
}

void Renderer::Flush()
{
    // It's worth limiting flush rate, a user wouldn't necessarily need
    // to observe the intermediate screen states. And the CPU consumption
    // is improved dramatically when limiting the flush rate.

    // 40 ms => 25 FPS (PAL). Perhaps worth making it configurable.
    const int FLUSH_DURATION_MS = 40;

    auto now = ClockT::now();
    if (now - _last_flush_time > std::chrono::milliseconds(FLUSH_DURATION_MS))
    {
        // Do repaint the grid if enough time elapsed since last time.
        // This is useful when fast scrolling.
        _DoFlush();
    }
    else
    {
        // Make sure the final view will be presented if no more flush requests.
        _timer->Start(FLUSH_DURATION_MS, 0, [&] { _DoFlush(); });
    }
}

void Renderer::_AnticipateFlush()
{
    // The grid is being updated, cancel the last flush request. A new one is pending.
    _timer->Stop();
}

void Renderer::_DoFlush()
{
    _AnticipateFlush();
    std::cout << "Flush ";
    _last_flush_time = ClockT::now();

    _window->Clear(_def_attr.bg.value());

    for (int row = 0, rowN = _lines.size(); row < rowN; ++row)
    {
        auto &line = _lines[row];
        auto &tex_cache = line.texture_cache;

        auto copy_texture = [&](const TextureCache::Texture &tex) {
            _window->CopyTexture(row, tex.col, tex.texture.get());
        };

        // Check if it's possible to just copy the prepared textures first
        if (!line.dirty)
        {
            line.texture_cache.ForEach(copy_texture);
            continue;
        }
        // Mark the line clear as we're going to redraw the necessary parts
        // and update the texture cache.
        line.dirty = false;

        // Group text chunks by hl_id
        TextureCache::Texture texture;
        auto tit = tex_cache.Begin();

        auto print_group = [&]() {
            // Test whether the texture should be rendered again
            if (tex_cache.ClearUntil(tit, texture))
            {
                // Paint the text on the surface carefully
                auto hlit = _hl_attr.find(texture.hl_id);

                texture.texture = _window->CreateTexture(texture.width, texture.text,
                        hlit != _hl_attr.end() ? hlit->second : _def_attr,
                        _def_attr);
                tex_cache.Insert(tit, std::move(texture));
                std::cout << "+";
            }
            else
            {
                std::cout << ".";
            }

            // Copy the texture (cached or new) to the renderer
            copy_texture(*tit++);
        };

        // Split the cells into chunks by the same hl_id
        size_t chunks[line.hl_id.size() + 1];
        size_t n = _SplitChunks(line, chunks);

        // Print and cache the chunks individually
        for (size_t i = 1; i < n; ++i)
        {
            int col = chunks[i - 1];
            int end = chunks[i];
            texture = {
                .col = col,
                .hl_id = line.hl_id[col],
            };
            texture.width = end - col;
            while (col < end)
                texture.text += line.text[col++];
            print_group();
        }

        line.texture_cache.RemoveTheRest(tit);
    }

    _window->DrawCursor(_cursor_row, _cursor_col, _def_attr.fg.value(), _mode);
    _window->Present();

    auto end_time = ClockT::now();
    std::cout << " " << std::chrono::duration<double>(end_time - _last_flush_time).count() << std::endl;
}

size_t Renderer::_SplitChunks(const _Line &line, size_t chunks[])
{
    // Split the line into the chunks with contiguous highlighting.
    // However, contiguous spaces should form their own chunk to avoid unnecessary text rerendering.
    const auto &hl = line.hl_id;
    const auto &text = line.text;

    size_t n = 0;
    chunks[n++] = 0;
    chunks[n++] = 1;
    bool is_space = false;
    while (chunks[n - 1] < hl.size())
    {
        size_t &back = chunks[n - 1];
        if (hl[back] != hl[chunks[n - 2]])
        {
            chunks[n++] = back + 1;
            is_space = false;
        }
        else if (!is_space && back > 1 && text[back] == " " && text[back - 1] == " ")
        {
            // Make sure contiguous spaces form their own chunk
            is_space = true;
            // If the line started with spaces, no need to form a new chunk. Otherwise:
            if (back > 2)
            {
                chunks[n - 1] = back - 1;
                chunks[n++] = back + 1;
            }
        }
        else if (is_space && text[back] != " ")
        {
            // End of space chunk
            is_space = false;
            chunks[n++] = back + 1;
        }
        else
            ++back;
    }
    return n;
}

void Renderer::GridLine(int row, int col, std::string_view chunk, unsigned hl_id, int repeat)
{
    _AnticipateFlush();

    _Line &line = _lines[row];
    line.dirty = true;

    for (int i = 0; i < repeat; ++i)
    {
        line.text[col + i] = chunk;
        line.hl_id[col + i] = hl_id;
    }
}

void Renderer::GridCursorGoto(int row, int col)
{
    _AnticipateFlush();
    _cursor_row = row;
    _cursor_col = col;
}

void Renderer::GridScroll(int top, int bot, int left, int right, int rows)
{
    _AnticipateFlush();
    auto copy = [&](int row, int row_from) {
        auto &line_from = _lines[row_from];
        auto &line_to = _lines[row];
        line_to.dirty = true;
        for (int col = left; col < right; ++col)
        {
            line_to.text[col] = std::move(line_from.text[col]);
            line_to.hl_id[col] = std::move(line_from.hl_id[col]);
        }
        line_to.texture_cache.MoveFrom(line_from.texture_cache, left, right);
    };

    if (rows > 0)
    {
        for (int row = top; row < bot - rows; ++row)
            copy(row, row + rows);
    }
    else if (rows < 0)
    {
        for (int row = bot - 1; row > top - rows - 1; --row)
            copy(row, row + rows);
    }
    else
    {
        throw std::runtime_error("Rows should not equal 0");
    }
}

void Renderer::GridClear()
{
    _AnticipateFlush();
    for (auto &line : _lines)
    {
        line.dirty = true;
        for (auto &t : line.text)
            t = ' ';
        for (auto &hl : line.hl_id)
            hl = 0;
    }
}

void Renderer::HlAttrDefine(unsigned hl_id, HlAttr attr)
{
    _hl_attr[hl_id] = attr;
}

void Renderer::DefaultColorSet(unsigned fg, unsigned bg)
{
    _def_attr.fg = fg;
    _def_attr.bg = bg;

    for (auto &line : _lines)
    {
        line.dirty = true;
        line.texture_cache.Clear();
    }
}

void Renderer::OnResized()
{
    auto rows_cols = _window->GetRowsCols();
    auto rows = rows_cols.first;
    auto cols = rows_cols.second;

    if (rows != static_cast<int>(_lines.size()) ||
        cols != static_cast<int>(_lines[0].text.size()))
    {
        _rpc->Request(
            [rows, cols](auto &pk) {
                pk.pack("nvim_ui_try_resize");
                pk.pack_array(2);
                pk.pack(cols);
                pk.pack(rows);
            },
            [](const msgpack::object &err, const auto &resp) {
                if (!err.is_nil())
                {
                    std::ostringstream oss;
                    oss << "Failed to resize UI " << err << std::endl;
                    throw std::runtime_error(oss.str());
                }
            }
        );
    }
}

void Renderer::GridResize(int width, int height)
{
    _lines.resize(height);
    for (auto &line : _lines)
    {
        line.hl_id.resize(width, 0);
        line.text.resize(width, " ");
    }
}

void Renderer::ModeChange(std::string_view mode)
{
    _mode = mode;
}

void Renderer::SetBusy(bool is_busy)
{
    _window->SetBusy(is_busy);
}
