#include "halo2/gameplay_font_source.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
unsigned checks{},negatives{};
void check(bool value,const char* message) {++checks;if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F function,const char* message) {
    bool rejected=false;try{function();}catch(const FormatError&){rejected=true;}
    check(rejected,message);++negatives;
}
struct Temp {
    std::filesystem::path path;
    Temp() {
        const auto now=std::chrono::steady_clock::now().time_since_epoch().count();
        for(unsigned i=0;;++i) {
            path=std::filesystem::temp_directory_path()/("h2-font-source-"+std::to_string(now)+"-"+std::to_string(i));
            if(std::filesystem::create_directory(path))break;
        }
    }
    ~Temp(){std::error_code error;std::filesystem::remove_all(path,error);}
};
std::vector<std::byte> read(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file || file.tellg()<0)throw std::runtime_error("Missing test resource");
    std::vector<std::byte> result(static_cast<std::size_t>(file.tellg()));file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(result.data()),result.size()))throw std::runtime_error("Truncated test resource");
    return result;
}
void write(const std::filesystem::path& path,std::span<const std::byte> bytes) {
    std::ofstream file(path,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
}
void table(const std::filesystem::path& directory,const std::vector<std::string>& names) {
    std::ofstream file(directory/"font_table.txt",std::ios::binary|std::ios::trunc);
    for(const auto& name:names)file<<name<<"\r\n";
}
std::vector<std::byte> minimal_font() {
    std::vector<std::byte> bytes(0x40411);
    const auto word=[&](unsigned p,unsigned value){bytes[p]=std::byte(value);bytes[p+1]=std::byte(value>>8);};
    const auto dword=[&](unsigned p,unsigned value){word(p,value);word(p+2,value>>16);};
    dword(0x200,0xf0000001);word(0x204,7);word(0x206,2);word(0x208,1);word(0x20a,0xfffd);
    word(0x40400,4);word(0x40402,1);word(0x40404,2);word(0x40406,3);word(0x40408,1);word(0x4040a,2);
    dword(0x4040c,0x40410);bytes[0x40410]=std::byte{6};return bytes;
}
bool same_glyph(const FontGlyph& a,const FontGlyph& b) {
    return a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height && a.advance==b.advance &&
        a.bearing_x==b.bearing_x && a.bearing_y==b.bearing_y;
}
void same_layout(const RetailFont& a,const RetailFont& b) {
    check(a.ascent==b.ascent && a.descent==b.descent && a.leading==b.leading,"Line metrics changed");
    check(a.kerning==b.kerning && a.glyph_record_indices==b.glyph_record_indices,"Kerning/lookup changed");
    check(a.glyphs.size()==b.glyphs.size(),"Glyph coverage changed");
    for(const auto& [c,g]:a.glyphs)check(same_glyph(g,b.glyphs.at(c)),"Logical glyph geometry changed");
}
void portable() {
    Temp temp;const auto bytes=minimal_font();write(temp.path/"custom",bytes);
    std::vector<std::string> names(11,"custom");table(temp.path,names);
    const auto source=load_gameplay_font_source(temp.path);
    check(source.domain==GameplayFontDomain::Original && source.directory==temp.path,"Custom original source changed");
    const auto font=load_gameplay_font(source,6);const auto raw=load_retail_font(temp.path/"custom");same_layout(font,raw);
    check(font.atlas.pixels==raw.atlas.pixels && font.sampled_glyphs.empty(),"Original raster changed");
    check(gameplay_font_horizontal_offset(source,6)==-3,"Original signed offset changed");
    rejects([&]{load_gameplay_font(source,11);},"Original selector overflow admitted");
    rejects([&]{load_gameplay_font_source(temp.path/"missing");},"Missing source admitted");
    for(const auto count:{10,12,13}){table(temp.path,std::vector<std::string>(count,"custom"));rejects([&]{load_gameplay_font_source(temp.path);},"Unknown table shape admitted");}
    names[3]="../custom";table(temp.path,names);rejects([&]{load_gameplay_font_source(temp.path);},"Escaping owner admitted");
    names[3]="custom";table(temp.path,names);
    auto forged=source;forged.domain=GameplayFontDomain::Mcc;rejects([&]{load_gameplay_font(forged,6);},"Forged domain admitted");
    forged=source;forged.names[6]="other";rejects([&]{load_gameplay_font(forged,6);},"Forged binding admitted");
    const auto mcc=temp.path/"halo2/h2_fonts";std::filesystem::create_directories(mcc);table(mcc,names);
    rejects([&]{load_gameplay_font_source(mcc);},"Truncated MCC table treated as original");
    check(read(temp.path/"custom")==bytes,"Reader mutated custom source");
}
struct Totals { unsigned selectors{},glyphs{},raster_differences{},fallback_extras{},baseline_mismatches{};std::size_t raster_bytes{}; };
Totals actual(const std::filesystem::path& mcc,const std::filesystem::path& original) {
    const auto source=load_gameplay_font_source(mcc),reference=load_gameplay_font_source(original);
    check(source.domain==GameplayFontDomain::Mcc && reference.domain==GameplayFontDomain::Original,"Actual typed domains wrong");
    const auto legacy=load_gameplay_font_source(mcc.parent_path()/"h2_maps_win64_dx11/fonts");
    check(legacy.domain==source.domain && legacy.directory==source.directory && legacy.names==source.names,"Legacy MCC path owner not resolved");
    Totals total;
    for(unsigned selector=0;selector<11;++selector) {
        const auto before=read(mcc/source.names[selector]);
        const auto raw=load_retail_font(mcc/source.names[selector]);
        const auto converted=load_gameplay_font(source,selector);
        const auto og=load_retail_font(original/reference.names[selector]);
        const bool baseline_mismatch=raw.ascent!=og.ascent || raw.descent!=og.descent || raw.leading!=og.leading || raw.kerning!=og.kerning;
        check(baseline_mismatch==(selector!=0 && selector!=8),"Reference fixture no longer exposes original MCC layout differences");
        total.baseline_mismatches+=baseline_mismatch;
        same_layout(converted,og);
        check(converted.file_fingerprint==raw.file_fingerprint,"Actual MCC source identity changed");
        check(converted.atlas.width==raw.atlas.width && converted.atlas.height==raw.atlas.height &&
            converted.atlas.format==raw.atlas.format && converted.atlas.pixels==raw.atlas.pixels,"MCC raster bytes changed");
        check(gameplay_font_horizontal_offset(source,selector)==gameplay_font_horizontal_offset(reference,selector),"Original horizontal offset not restored");
        const auto again=load_gameplay_font(source,selector);same_layout(converted,again);
        check(again.atlas.pixels==converted.atlas.pixels && again.sampled_glyphs.size()==converted.sampled_glyphs.size() &&
            again.file_fingerprint==raw.file_fingerprint,"Repeated projection changed output/source identity");
        std::map<std::uint32_t,unsigned> representative;
        for(unsigned c=32;c<65536;++c)representative.emplace(og.glyph_record_indices[c],c);
        for(const auto& [c,logical]:converted.glyphs) {
            const auto& physical=raw.glyphs.at(representative.at(og.glyph_record_indices[c]));
            if(logical.width && logical.height) {
                const auto uv=retail_font_sample_uvs(converted,c,0,0,int(logical.width),int(logical.height));
                check(uv[0]==float(physical.x)/raw.atlas.width && uv[1]==float(physical.y)/raw.atlas.height &&
                    uv[2]==float(physical.x+physical.width)/raw.atlas.width && uv[3]==float(physical.y+physical.height)/raw.atlas.height,
                    "Sample does not own actual MCC glyph rectangle");
            }
            if(og.glyph_record_indices[c]==og.glyph_record_indices[0] && raw.glyph_record_indices[c]!=raw.glyph_record_indices[0])++total.fallback_extras;
        }
        for(const auto& [record,c]:representative) {
            (void)record;const auto& a=og.glyphs.at(c);const auto& b=raw.glyphs.at(c);
            bool different=a.width!=b.width || a.height!=b.height;
            if(!different)for(unsigned y=0;y<a.height && !different;++y)for(unsigned x=0;x<a.width && !different;++x)
                for(unsigned channel=0;channel<4;++channel)
                    different|=og.atlas.pixels[((a.y+y)*og.atlas.width+a.x+x)*4+channel]!=raw.atlas.pixels[((b.y+y)*raw.atlas.width+b.x+x)*4+channel];
            total.raster_differences+=different;
        }
        require_mcc_gameplay_font_sample(source,selector,mcc/source.names[selector]);
        check(read(mcc/source.names[selector])==before,"Reader mutated MCC source");
        ++total.selectors;total.glyphs+=unsigned(converted.glyphs.size());total.raster_bytes+=raw.atlas.pixels.size();
    }
    Temp temp;const auto copied=temp.path/"halo2/h2_fonts";std::filesystem::create_directories(copied);
    std::filesystem::create_directories(temp.path/"halo2/h2_maps_win64_dx11");
    std::set<std::string> unique(source.names.begin(),source.names.end());
    for(const auto& name:unique)std::filesystem::copy_file(mcc/name,copied/name);
    table(copied,source.names);const auto selected=load_gameplay_font_source(copied);
    const auto owner=copied/source.names[6];const auto before=read(owner);
    auto mutation=before;mutation.back()^=std::byte{1};write(owner,mutation);
    rejects([&]{load_gameplay_font_source(copied);},"Custom pixel payload admitted");
    rejects([&]{load_gameplay_font(selected,6);},"Source mutation after binding admitted");
    write(owner,before);mutation=before;mutation[0x200]^=std::byte{1};write(owner,mutation);
    rejects([&]{load_gameplay_font_source(copied);},"Wrong font version admitted");
    write(owner,std::span(before).first(before.size()-1));rejects([&]{load_gameplay_font_source(copied);},"Partial font admitted");
    write(owner,before);auto names=source.names;names.pop_back();table(copied,names);
    rejects([&]{load_gameplay_font_source(copied);},"Truncated real MCC table treated as original");
    names=source.names;std::swap(names[4],names[5]);table(copied,names);
    rejects([&]{load_gameplay_font_source(copied);},"Wrong selector tuple admitted");
    table(copied,source.names);std::filesystem::create_directories(temp.path/"halo2/h2_maps_win64_dx11/fonts");
    rejects([&]{load_gameplay_font_source(temp.path/"halo2/h2_maps_win64_dx11/fonts");},"Existing incomplete legacy path silently bypassed");
    std::filesystem::copy_file(owner,temp.path/"sample");require_mcc_gameplay_font_sample(selected,6,temp.path/"sample");
    write(temp.path/"sample",mutation);rejects([&]{require_mcc_gameplay_font_sample(selected,6,temp.path/"sample");},"Custom redraw silently discarded");
    rejects([&]{load_gameplay_font(selected,11);},"MCC-only selector used as original");
    const auto arbitrary=temp.path/"arbitrary";std::filesystem::create_directory(arbitrary);table(arbitrary,source.names);
    rejects([&]{load_gameplay_font_source(arbitrary);},"Arbitrary12-row source admitted");
    return total;
}
} // namespace
int main(int argc,char** argv) {
    try {
        if(argc!=1 && argc!=3)return 2;
        if(argc==3 && (!*argv[1] || !*argv[2] || !std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"font_table.txt") ||
                      !std::filesystem::is_regular_file(std::filesystem::path(argv[2])/"font_table.txt")))return 77;
        portable();Totals totals;if(argc==3)totals=actual(argv[1],argv[2]);
        std::cout<<"{\"checks\":"<<checks<<",\"negative_cases\":"<<negatives<<",\"actual_selectors\":"<<totals.selectors
            <<",\"logical_glyphs\":"<<totals.glyphs<<",\"unchanged_mcc_atlas_bytes\":"<<totals.raster_bytes
            <<",\"original_fallback_aliases_with_extra_mcc_glyphs\":"<<totals.fallback_extras
            <<",\"baseline_layout_mismatches\":"<<totals.baseline_mismatches
            <<",\"distinct_source_rasters_or_dimensions_differ\":"<<totals.raster_differences<<"}\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
