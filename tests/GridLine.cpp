#include <boost/ut.hpp>
#include "GridLine.hpp"

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

    auto dump = [](GridLine &tc) {
        std::string buf;
        auto action = [&](const GridLine::Chunk &t) {
            buf += t.text;
        };
        tc.ForEach(action);
        return buf;
    };

    auto dump2 = [](GridLine &tc) {
        std::string buf;
        auto action = [&](const GridLine::Chunk &t) {
            buf += static_cast<Tex*>(t.texture.get())->s;
        };
        tc.ForEach(action);
        return buf;
    };

    "GridLine"_test = [&] {
        "foreach"_test = [&] {
            GridLine c;
            auto s = c.GetScanner(0);

            expect(s.EnsureNext(GridLine::Chunk(0, 1, 0, "He"), generator));
            expect(s.EnsureNext(GridLine::Chunk(2, 1, 0, "llo"), generator));
            expect(s.EnsureNext(GridLine::Chunk(3, 1, 0, " world"), generator));

            expect(eq("Hello world"s, dump(c)));
            expect(eq("Hello world"s, dump2(c)));
        };

        "scan"_test = [&] {
            GridLine c;
            {
                auto s = c.GetScanner(0);
                expect(s.EnsureNext(GridLine::Chunk(0, 1, 0, "He"), generator));
                expect(s.EnsureNext(GridLine::Chunk(2, 1, 0, "llo"), generator));
                expect(s.EnsureNext(GridLine::Chunk(3, 1, 0, " world"), generator));
            }

            {
                auto s = c.GetScanner(0);
                expect(!s.EnsureNext(GridLine::Chunk(0, 1, 0, "He"), no_generator));
                expect(s.EnsureNext(GridLine::Chunk(2, 1, 0, "llo "), generator));
                expect(s.EnsureNext(GridLine::Chunk(3, 1, 0, "again"), generator));
            }
            expect(eq("Hello again"s, dump(c)));
            expect(eq("Hello again"s, dump2(c)));
        };

        "update"_test = [&] {
            GridLine c;
            {
                auto s = c.GetScanner(0);
                expect(s.EnsureNext(GridLine::Chunk(0, 1, 0, "T"), generator));
                expect(s.EnsureNext(GridLine::Chunk(1, 1, 0, "e"), generator));
                expect(s.EnsureNext(GridLine::Chunk(2, 1, 0, "s"), generator));
                expect(s.EnsureNext(GridLine::Chunk(3, 1, 0, "t"), generator));
                expect(s.EnsureNext(GridLine::Chunk(4, 1, 0, "2"), generator));
            }
            {
                auto s = c.GetScanner(0);
                expect(s.EnsureNext(GridLine::Chunk(0, 1, 0, "B"), generator));
                expect(!s.EnsureNext(GridLine::Chunk(1, 1, 0, "e"), no_generator));
                expect(!s.EnsureNext(GridLine::Chunk(3, 1, 0, "t"), no_generator));
            }
            expect(eq("Bet"s, dump(c)));
            expect(eq("Bet"s, dump2(c)));
        };

        "move"_test = [&] {
            GridLine c1;
            {
                auto s = c1.GetScanner(0);
                expect(s.EnsureNext(GridLine::Chunk(0, 1, 0, "T"), generator));
                expect(s.EnsureNext(GridLine::Chunk(2, 1, 0, "e"), generator));
                expect(s.EnsureNext(GridLine::Chunk(4, 1, 0, "s"), generator));
                expect(s.EnsureNext(GridLine::Chunk(6, 1, 0, "t"), generator));
                expect(s.EnsureNext(GridLine::Chunk(8, 1, 0, "2"), generator));
            }
            GridLine c2;
            {
                auto s = c2.GetScanner(0);
                expect(s.EnsureNext(GridLine::Chunk(1, 1, 0, "Q"), generator));
                expect(s.EnsureNext(GridLine::Chunk(3, 1, 0, "W"), generator));
                expect(s.EnsureNext(GridLine::Chunk(5, 1, 0, "E"), generator));
                expect(s.EnsureNext(GridLine::Chunk(7, 1, 0, "R"), generator));
            }

            c1.MoveFrom(c2, 2, 7, 0xffffffff);
            expect(eq("TeWsEt2"s, dump(c1)));
            expect(eq("TeWsEt2"s, dump2(c1)));
            expect(eq("QR"s, dump(c2)));
            expect(eq("QR"s, dump2(c2)));
        };

        "shift"_test = [&] {
            GridLine c;
            {
                auto s = c.GetScanner(0);
                expect(s.EnsureNext(GridLine::Chunk(0, 2, 0, "He"), generator));
                expect(s.EnsureNext(GridLine::Chunk(2, 1, 0, " "), generator));
                expect(s.EnsureNext(GridLine::Chunk(3, 3, 0, "llo"), generator));
            }
            expect(eq("He llo"s, dump(c)));
            expect(eq("He llo"s, dump2(c)));

            {
                auto s = c.GetScanner(0);
                expect(!s.EnsureNext(GridLine::Chunk(0, 2, 0, "He"), no_generator));
                expect(!s.EnsureNext(GridLine::Chunk(2, 3, 0, "llo"), no_generator));
            }
            expect(eq("Hello"s, dump(c)));
            expect(eq("Hello"s, dump2(c)));
        };
    };

    "GridLine::ChunkWrapper"_test = [&] {
        using WrapperT = GridLine::ChunkWrapper<&BaseTexture::IncRef>;
        GridLine::Chunk c{0, 1, 0, "test"};
        c.texture = generator(c);
        expect(!c.texture->IsAlive());
        expect(!c.texture->IsVisible());

        "wrapper"_test = [&] {
            {
                WrapperT w{c};
                expect(c.texture->IsAlive());
                expect(!c.texture->IsVisible());
            }
            expect(!c.texture->IsAlive());
            expect(!c.texture->IsVisible());
        };

        "wrapper-copy"_test = [&] {
            {
                WrapperT w{c};
                {
                    WrapperT w2{w};
                    expect(c.texture->IsAlive());
                    expect(!c.texture->IsVisible());
                }
                expect(c.texture->IsAlive());
                expect(!c.texture->IsVisible());
            }

            expect(!c.texture->IsAlive());
            expect(!c.texture->IsVisible());
        };

        "wrapper-assign"_test = [&] {
            {
                WrapperT w{c};
                {
                    WrapperT w2{w};
                    w = w2;
                    expect(c.texture->IsAlive());
                    expect(!c.texture->IsVisible());
                }
                expect(c.texture->IsAlive());
                expect(!c.texture->IsVisible());
            }

            expect(!c.texture->IsAlive());
            expect(!c.texture->IsVisible());
        };

        using ShowT = GridLine::ChunkWrapper<&BaseTexture::SetVisible>;

        "show-copy"_test = [&] {
            {
                WrapperT w{c};
                {
                    ShowT w2{w};
                    expect(c.texture->IsAlive());
                    expect(c.texture->IsVisible());
                }
                expect(c.texture->IsAlive());
                expect(!c.texture->IsVisible());
            }

            expect(!c.texture->IsAlive());
            expect(!c.texture->IsVisible());
        };
    };
};

} //namespace;
