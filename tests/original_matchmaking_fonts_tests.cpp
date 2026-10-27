#include "halo2/original_matchmaking_fonts.hpp"
#include "halo2/frontend.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;
    if(!value)throw std::runtime_error(message);
}
bool same_glyph(const FontGlyph& a,const FontGlyph& b) {
    return a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height &&
        a.advance==b.advance && a.bearing_x==b.bearing_x && a.bearing_y==b.bearing_y;
}
void exact_original(unsigned selector,const std::filesystem::path& path) {
    const auto selected=load_original_matchmaking_font(selector);
    const auto original=load_retail_font(path);
    check(selected.file_fingerprint==original.file_fingerprint,"Original font source payload changed");
    check(selected.ascent==original.ascent && selected.descent==original.descent &&
          selected.leading==original.leading,"Original line metrics changed");
    check(selected.kerning==original.kerning,"Original effective kerning changed");
    check(selected.glyph_record_indices==original.glyph_record_indices,"Original 65536-entry lookup changed");
    check(selected.glyphs.size()==original.glyphs.size(),"Original glyph coverage changed");
    for(const auto& [c,glyph]:original.glyphs)
        check(same_glyph(selected.glyphs.at(c),glyph),"Original packed glyph metrics changed");
    check(selected.sampled_glyphs.empty(),"Original font selected a foreign sampled raster");
    check(selected.atlas.width==original.atlas.width && selected.atlas.height==original.atlas.height &&
          selected.atlas.format==original.atlas.format,"Original atlas extent or format changed");
    check(selected.atlas.pixels==original.atlas.pixels,"Original decoded A4R4G4B4 atlas pixels changed");
}
} // namespace

int main(int argc,char** argv) {
    try {
        const auto base=load_original_matchmaking_font(1);
        check(base.ascent==15 && base.descent==4,"Conduit12 retained MCC line metrics");
        const auto a=base.glyphs.at('A');
        check(a.width==8 && a.height==20 && a.advance==7,"Conduit12 retained MCC glyph geometry");
        const auto white=base.glyphs.at(0xe105);
        check(white.width==16 && white.height==18 && white.advance==16,
              "Original white-button glyph was replaced by a PC keyboard raster");
        check(base.sampled_glyphs.empty(),"Finite original bank retained sampled MCC glyphs");
        const std::array supported{1u,2u,4u,6u,9u};
        for(unsigned selector=0;selector<13;++selector) {
            bool expected{};
            for(const auto value:supported)expected|=selector==value;
            check(original_matchmaking_font_selector(selector)==expected,"Finite font selector scope changed");
            if(!expected) {
                bool rejected{};
                try { load_original_matchmaking_font(selector); }
                catch(const FormatError&) { rejected=true; }
                check(rejected,"Unqualified original font selector admitted");
            }
        }
        const auto four=load_original_matchmaking_font(4),six=load_original_matchmaking_font(6);
        check(four.file_fingerprint==six.file_fingerprint && four.atlas.pixels==six.atlas.pixels &&
              four.glyph_record_indices==six.glyph_record_indices,
              "Scoreboard selector6 stopped owning the original Conduit13 source");
        if(argc!=2 || !std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"conduit-12")) {
            std::cout<<checks<<" embedded original-font controls passed; DVD comparison unavailable\n";
            return 77;
        }
        const std::filesystem::path root=argv[1];
        exact_original(1,root/"conduit-12");
        exact_original(2,root/"handel_gothic-11");
        exact_original(4,root/"conduit-13");
        exact_original(9,root/"conduit-16");
        exact_original(6,root/"conduit-13");
        std::cout<<checks<<" exact original-font lookup/metric/atlas controls passed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
