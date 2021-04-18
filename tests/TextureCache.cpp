#include <boost/ut.hpp>
#include "TextureCache.hpp"

namespace {

using namespace boost::ut;
using namespace std::string_literals;

suite s = [] {
    "TextureCache"_test = [] {
        "foreach"_test = [] {
            TextureCache c;
            auto it = c.Begin();
            c.Insert(it, TextureCache::Texture{
                .col = 0,
                .width = 2,
                .text = "He",
            });
            c.Insert(++it, TextureCache::Texture{
                .col = 2,
                .width = 3,
                .text = "llo",
            });
            c.Insert(++it, TextureCache::Texture{
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
    };
};

} //namespace;
