#include <boost/ut.hpp>
#include "TextureCache.hpp"

namespace {

using namespace boost::ut;
using namespace std::string_literals;

suite s = [] {
    "TextureCache"_test = [] {
        "foreach"_test = [] {
            TextureCache c;
            auto s = c.GetScanner();
            s.Insert(TextureCache::Texture{
                .col = 0,
                .width = 2,
                .text = "He",
            });
            s.Advance();
            s.Insert(TextureCache::Texture{
                .col = 2,
                .width = 3,
                .text = "llo",
            });
            s.Advance();
            s.Insert(TextureCache::Texture{
                .col = 3,
                .width = 2,
                .text = " world",
            });

            std::string buf;
            auto action = [&](const TextureCache::Texture &t) {
                buf += t.text;
            };
            c.ForEach(action);

            expect(eq("Hello world"s, buf));
        };
        "scan"_test = [] {
            struct Tex : IWindow::ITexture
            {
                static PtrT n() { return PtrT{new Tex}; }
            };

            TextureCache c;
            {
                auto s = c.GetScanner();
                s.Insert(TextureCache::Texture{.col = 0, .text = "He", .texture = Tex::n()});
                s.Advance();
                s.Insert(TextureCache::Texture{.col = 2, .text = "llo", .texture = Tex::n()});
                s.Advance();
                s.Insert(TextureCache::Texture{.col = 3, .text = " world", .texture = Tex::n()});
            }

            auto s = c.GetScanner();
            bool missing = s.EnsureNext(TextureCache::Texture{.col = 0, .text = "He"});
            expect(!missing);
            s.Advance();
            missing = s.EnsureNext(TextureCache::Texture{.col = 2, .text = "llo "});
            expect(missing);
            s.Insert(TextureCache::Texture{.col = 2, .text = "llo "});
            s.Advance();
            missing = s.EnsureNext(TextureCache::Texture{.col = 3, .text = "again"});
            expect(missing);
            s.Insert(TextureCache::Texture{.col = 3, .text = "again"});
            s.Advance();

            std::string buf;
            auto action = [&](const TextureCache::Texture &t) {
                buf += t.text;
            };
            c.ForEach(action);

            expect(eq("Hello again"s, buf));
        };
    };
};

} //namespace;
