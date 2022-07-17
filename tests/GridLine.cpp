#include <boost/ut.hpp>
#include "GridLine.hpp"

namespace {

using namespace boost::ut;
using namespace std::string_literals;

struct Tex : BaseTexture
{
    std::string s;
    Tex(const auto &words)
    {
        for (const auto &w : words)
            s += w.text;
    }
    static PtrT n(const GridLine::Chunk::WordsT &words) { return PtrT{new Tex(words)}; }
};

suite s = [] {
    auto generator = [](const auto &t) { return Tex::n(t.words); };

    "GridLine::ChunkWrapper"_test = [&] {
        using WrapperT = GridLine::ChunkWrapper<&BaseTexture::IncRef>;
        GridLine::Chunk c{0, 1, {{0u, "test"}}};
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
