#include <boost/ut.hpp>
#define private public
#include "../src/Renderer.hpp"
#undef private
#include <string>

namespace {

using namespace boost::ut;
using namespace std::string_literals;

suite s = [] {
    "SplitChunks"_test = [] {
        "empty"_test = [] {
            Renderer::_Line line;
            auto chunks = Renderer::_SplitChunks(line);
            expect(2_u == chunks.size());
            expect(0_u == chunks[0]);
            expect(0_u == chunks[0]);
        };

        "contiguous"_test = [] {
            Renderer::_Line line{
                .text = {"H"s, "e"s, "l"s, "l"s, "o"s},
                .hl_id = {0, 0, 0, 0, 0},
                .texture_cache = {},
            };
            auto chunks = Renderer::_SplitChunks(line);
            expect(2_u == chunks.size());
            expect(0_u == chunks[0]);
            expect(5_u == chunks[1]);
        };

        "two_chunks"_test = [] {
            Renderer::_Line line{
                .text = {"a"s, "b"s, "c"s, "d"s},
                .hl_id = {0, 0, 1, 1},
                .texture_cache = {},
            };
            auto chunks = Renderer::_SplitChunks(line);
            expect(3_u == chunks.size());
            expect(0_u == chunks[0]);
            expect(2_u == chunks[1]);
            expect(4_u == chunks[2]);
        };

        "space"_test = [] {
            Renderer::_Line line{
                .text = {"a"s, "b"s, " "s, " "s, "c"s},
                .hl_id = {0, 0, 0, 0, 0},
                .texture_cache = {},
            };
            auto chunks = Renderer::_SplitChunks(line);
            expect(4_u == chunks.size());
            expect(0_u == chunks[0]);
            expect(2_u == chunks[1]);
            expect(4_u == chunks[2]);
            expect(5_u == chunks[3]);
        };

        "space_start"_test = [] {
            Renderer::_Line line{
                .text = {" "s, " "s, " "s, "a"s, "b"s},
                .hl_id = {0, 0, 0, 0, 0},
                .texture_cache = {},
            };
            auto chunks = Renderer::_SplitChunks(line);
            expect(3_u == chunks.size());
            expect(0_u == chunks[0]);
            expect(3_u == chunks[1]);
            expect(5_u == chunks[2]);
        };

        "two_space_start"_test = [] {
            Renderer::_Line line{
                .text = {" "s, " "s, "a"s, "b"s},
                .hl_id = {0, 0, 0, 0},
                .texture_cache = {},
            };
            auto chunks = Renderer::_SplitChunks(line);
            expect(3_u == chunks.size());
            expect(0_u == chunks[0]);
            expect(2_u == chunks[1]);
            expect(4_u == chunks[2]);
        };

        "two_space_end"_test = [] {
            Renderer::_Line line{
                .text = {"~"s, " "s, " "s, " "s},
                .hl_id = {0, 0, 0, 0},
                .texture_cache = {},
            };
            auto chunks = Renderer::_SplitChunks(line);
            expect(3_u == chunks.size());
            expect(0_u == chunks[0]);
            expect(1_u == chunks[1]);
            expect(4_u == chunks[2]);
        };
    };
};

} //namespace;
