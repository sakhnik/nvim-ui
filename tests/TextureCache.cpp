#include <boost/ut.hpp>
#include "TextureCache.hpp"

namespace {

using namespace boost::ut;
using namespace std::string_literals;

struct Tex : IWindow::ITexture
{
    std::string s;
    Tex(const std::string &s): s{s} {}
    static PtrT n(const std::string &s) { return PtrT{new Tex(s)}; }
};

suite s = [] {
    auto generator = [](const auto &t) { return Tex::n(t.text); };

    auto dump = [](TextureCache &tc) {
        std::string buf;
        auto action = [&](const TextureCache::Texture &t) {
            buf += t.text;
        };
        tc.ForEach(action);
        return buf;
    };

    auto dump2 = [](TextureCache &tc) {
        std::string buf;
        auto action = [&](const TextureCache::Texture &t) {
            buf += static_cast<Tex*>(t.texture.get())->s;
        };
        tc.ForEach(action);
        return buf;
    };

    "TextureCache"_test = [&] {
        "foreach"_test = [&] {
            TextureCache c;
            auto s = c.GetScanner();

            expect(s.EnsureNext(TextureCache::Texture{.col = 0, .text = "He"}, generator));
            expect(s.EnsureNext(TextureCache::Texture{.col = 2, .text = "llo"}, generator));
            expect(s.EnsureNext(TextureCache::Texture{.col = 3, .text = " world"}, generator));

            expect(eq("Hello world"s, dump(c)));
            expect(eq("Hello world"s, dump2(c)));
        };

        "scan"_test = [&] {
            TextureCache c;
            {
                auto s = c.GetScanner();
                expect(s.EnsureNext(TextureCache::Texture{.col = 0, .text = "He"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 2, .text = "llo"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 3, .text = " world"}, generator));
            }

            {
                auto s = c.GetScanner();
                expect(!s.EnsureNext(TextureCache::Texture{.col = 0, .text = "He"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 2, .text = "llo "}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 3, .text = "again"}, generator));
            }
            expect(eq("Hello again"s, dump(c)));
            expect(eq("Hello again"s, dump2(c)));
        };

        "update"_test = [&] {
            TextureCache c;
            {
                auto s = c.GetScanner();
                expect(s.EnsureNext(TextureCache::Texture{.col = 0, .text = "T"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 1, .text = "e"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 2, .text = "s"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 3, .text = "t"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 4, .text = "2"}, generator));
            }
            {
                auto s = c.GetScanner();
                expect(s.EnsureNext(TextureCache::Texture{.col = 0, .text = "B"}, generator));
                expect(!s.EnsureNext(TextureCache::Texture{.col = 1, .text = "e"}, generator));
                expect(!s.EnsureNext(TextureCache::Texture{.col = 3, .text = "t"}, generator));
            }
            expect(eq("Bet"s, dump(c)));
            expect(eq("Bet"s, dump2(c)));
        };

        "move"_test = [&] {
            TextureCache c1;
            {
                auto s = c1.GetScanner();
                expect(s.EnsureNext(TextureCache::Texture{.col = 0, .text = "T"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 2, .text = "e"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 4, .text = "s"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 6, .text = "t"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 8, .text = "2"}, generator));
            }
            TextureCache c2;
            {
                auto s = c2.GetScanner();
                expect(s.EnsureNext(TextureCache::Texture{.col = 1, .text = "Q"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 3, .text = "W"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 5, .text = "E"}, generator));
                expect(s.EnsureNext(TextureCache::Texture{.col = 7, .text = "R"}, generator));
            }

            c1.MoveFrom(c2, 2, 7);
            expect(eq("TeWsEt2"s, dump(c1)));
            expect(eq("TeWsEt2"s, dump2(c1)));
            expect(eq("QR"s, dump(c2)));
            expect(eq("QR"s, dump2(c2)));
        };
    };
};

} //namespace;
