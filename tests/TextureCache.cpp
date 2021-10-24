#include <boost/ut.hpp>
#include "TextureCache.hpp"

namespace {

using namespace boost::ut;
using namespace std::string_literals;

struct Tex : BaseTexture
{
    std::string s;
    Tex(const std::string &s): s{s} {}
    static PtrT n(const std::string &s) { return PtrT{new Tex(s)}; }
};

suite s = [] {
    auto generator = [](const auto &t) { return Tex::n(t.text); };
    auto no_generator = [](const auto &) { return Tex::n("*"); };

    auto dump = [](TextureCache &tc) {
        std::string buf;
        auto action = [&](const TextureCache::Chunk &t) {
            buf += t.text;
        };
        tc.ForEach(action);
        return buf;
    };

    auto dump2 = [](TextureCache &tc) {
        std::string buf;
        auto action = [&](const TextureCache::Chunk &t) {
            buf += static_cast<Tex*>(t.texture.get())->s;
        };
        tc.ForEach(action);
        return buf;
    };

    "TextureCache"_test = [&] {
        "foreach"_test = [&] {
            TextureCache c;
            auto s = c.GetScanner(0);

            expect(s.EnsureNext(TextureCache::Chunk(0, 1, 0, "He"), generator));
            expect(s.EnsureNext(TextureCache::Chunk(2, 1, 0, "llo"), generator));
            expect(s.EnsureNext(TextureCache::Chunk(3, 1, 0, " world"), generator));

            expect(eq("Hello world"s, dump(c)));
            expect(eq("Hello world"s, dump2(c)));
        };

        "scan"_test = [&] {
            TextureCache c;
            {
                auto s = c.GetScanner(0);
                expect(s.EnsureNext(TextureCache::Chunk(0, 1, 0, "He"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(2, 1, 0, "llo"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(3, 1, 0, " world"), generator));
            }

            {
                auto s = c.GetScanner(0);
                expect(!s.EnsureNext(TextureCache::Chunk(0, 1, 0, "He"), no_generator));
                expect(s.EnsureNext(TextureCache::Chunk(2, 1, 0, "llo "), generator));
                expect(s.EnsureNext(TextureCache::Chunk(3, 1, 0, "again"), generator));
            }
            expect(eq("Hello again"s, dump(c)));
            expect(eq("Hello again"s, dump2(c)));
        };

        "update"_test = [&] {
            TextureCache c;
            {
                auto s = c.GetScanner(0);
                expect(s.EnsureNext(TextureCache::Chunk(0, 1, 0, "T"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(1, 1, 0, "e"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(2, 1, 0, "s"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(3, 1, 0, "t"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(4, 1, 0, "2"), generator));
            }
            {
                auto s = c.GetScanner(0);
                expect(s.EnsureNext(TextureCache::Chunk(0, 1, 0, "B"), generator));
                expect(!s.EnsureNext(TextureCache::Chunk(1, 1, 0, "e"), no_generator));
                expect(!s.EnsureNext(TextureCache::Chunk(3, 1, 0, "t"), no_generator));
            }
            expect(eq("Bet"s, dump(c)));
            expect(eq("Bet"s, dump2(c)));
        };

        "move"_test = [&] {
            TextureCache c1;
            {
                auto s = c1.GetScanner(0);
                expect(s.EnsureNext(TextureCache::Chunk(0, 1, 0, "T"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(2, 1, 0, "e"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(4, 1, 0, "s"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(6, 1, 0, "t"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(8, 1, 0, "2"), generator));
            }
            TextureCache c2;
            {
                auto s = c2.GetScanner(0);
                expect(s.EnsureNext(TextureCache::Chunk(1, 1, 0, "Q"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(3, 1, 0, "W"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(5, 1, 0, "E"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(7, 1, 0, "R"), generator));
            }

            c1.MoveFrom(c2, 2, 7, 0xffffffff);
            expect(eq("TeWsEt2"s, dump(c1)));
            expect(eq("TeWsEt2"s, dump2(c1)));
            expect(eq("QR"s, dump(c2)));
            expect(eq("QR"s, dump2(c2)));
        };

        "shift"_test = [&] {
            TextureCache c;
            {
                auto s = c.GetScanner(0);
                expect(s.EnsureNext(TextureCache::Chunk(0, 2, 0, "He"), generator));
                expect(s.EnsureNext(TextureCache::Chunk(2, 1, 0, " "), generator));
                expect(s.EnsureNext(TextureCache::Chunk(3, 3, 0, "llo"), generator));
            }
            expect(eq("He llo"s, dump(c)));
            expect(eq("He llo"s, dump2(c)));

            {
                auto s = c.GetScanner(0);
                expect(!s.EnsureNext(TextureCache::Chunk(0, 2, 0, "He"), no_generator));
                expect(!s.EnsureNext(TextureCache::Chunk(2, 3, 0, "llo"), no_generator));
            }
            expect(eq("Hello"s, dump(c)));
            expect(eq("Hello"s, dump2(c)));
        };
    };
};

} //namespace;
