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
    GameplayFontBank bank(temp.path);
    const auto retained_font=bank.font(6);
    check(retained_font==bank.font(6) && retained_font==bank.raw(6),"Original bank rebuilt its immutable font");
    check(bank.matches_directory(temp.path) && !bank.matches_directory(temp.path/"other"),"Font bank source ownership changed");
    same_layout(*retained_font,font);
    check(retained_font->atlas.pixels==font.atlas.pixels,"Bank raster changed");
    auto private_font=*retained_font;private_font.glyphs.at(32).advance+=1;
    private_font.atlas.pixels[0]^=std::byte{1};
    check(retained_font->glyphs.at(32).advance==font.glyphs.at(32).advance &&
          retained_font->atlas.pixels==font.atlas.pixels,"Private redraw mutated the shared font bank");
    rejects([&]{bank.font(11);},"Font bank selector overflow admitted");
    rejects([&]{bank.raw(12);},"Raw font bank selector overflow admitted");
    const auto shared=raw;
    check(shared.atlas.pixels.data()==raw.atlas.pixels.data(),"Completed font atlas was copied again");
    auto writable=raw;auto* escaped=writable.atlas.pixels.data();const auto protected_copy=writable;
    const auto retained=protected_copy.atlas.pixels[0];escaped[0]^=std::byte{1};
    check(protected_copy.atlas.pixels[0]==retained && raw.atlas.pixels[0]==retained,
          "Writable font atlas view changed a retained font");
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
    check(font_prompt_asset_directory(temp.path,temp.path/"reference-pack")==temp.path/"reference-pack",
          "Explicit non-MCC reference prompt route changed");
}
struct Totals { unsigned selectors{},glyphs{},raster_differences{},fallback_extras{},baseline_mismatches{};std::size_t raster_bytes{}; };
unsigned native_prompts(const std::filesystem::path &mcc,Totals &total) {
    const auto source=load_gameplay_font_source(mcc);
    check(source.domain==GameplayFontDomain::Mcc&&source.names.size()==12,"Native prompt case lacks its authenticated MCC owner");
    Temp temp;const auto copied=temp.path/"halo2/h2_fonts";
    std::filesystem::create_directories(copied);
    for(const auto &name:std::set<std::string>(source.names.begin(),source.names.end()))
        std::filesystem::copy_file(source.directory/name,copied/name);
    table(copied,source.names);const auto private_source=load_gameplay_font_source(copied);
    GameplayFontBank bank(copied);
    const auto legacy=temp.path/"presentation/prompts";
    std::filesystem::create_directories(legacy);
    {std::ofstream file(legacy/"prompt-source.tsv");file<<"foreign authored prompt binding\n";}
    {std::ofstream file(legacy/"manifest.json");file<<"not a native MCC font\n";}
    const auto marker=read(legacy/"prompt-source.tsv"),manifest=read(legacy/"manifest.json");
    unsigned bindings{};
    for(unsigned selector=0;selector<source.names.size();++selector){
        const auto raw=load_retail_font(private_source.directory/private_source.names[selector]);
        auto selected=raw;
        const auto pixels=raw.atlas.pixels;
        const auto prompt=font_prompt_asset_directory(private_source.directory,legacy);
        check(prompt.empty(),"MCC native prompt route selected an old authored or foreign overlay");
        check(apply_vista_prompt_sampling(selected,prompt,selector)==0,"MCC native prompt route appended authored pixels");
        same_layout(raw,selected);
        check(selected.file_fingerprint==raw.file_fingerprint&&selected.atlas.width==raw.atlas.width&&
            selected.atlas.height==raw.atlas.height&&selected.atlas.format==raw.atlas.format&&
            selected.atlas.pixels==pixels&&selected.sampled_glyphs.empty()&&raw.sampled_glyphs.empty(),
            "MCC native prompt owner changed its atlas, UVs or source fingerprint");
        std::map<std::uint32_t,unsigned> counts;
        for(const auto record:raw.glyph_record_indices)++counts[record];
        unsigned admitted{};
        for(unsigned code=0xe100;code<=0xe13c;++code){
            // E136..E13B are native blank reserved keycap rows, not any of
            // the331 actual prompt bindings in the authenticated inventory.
            if((code>0xe111&&code<0xe12c)||(code>=0xe136&&code<=0xe13b))continue;
            const auto record=raw.glyph_record_indices.at(code);const auto glyph=raw.glyphs.find(code);
            const bool own=record!=0xffffffffu&&counts.at(record)==1&&glyph!=raw.glyphs.end()&&
                glyph->second.width&&glyph->second.height;
            if(code<=0xe111)check(own,"Actual MCC controller code aliases a fallback or is missing");
            if(!own)continue;
            const auto &g=glyph->second;bool visible{};
            for(unsigned y=0;y<g.height&&!visible;++y)for(unsigned x=0;x<g.width&&!visible;++x)
                visible=raw.atlas.pixels[(std::size_t(g.y+y)*raw.atlas.width+g.x+x)*4+3]!=std::byte{};
            check(visible,"Actual MCC control glyph has no decoded alpha");++admitted;
        }
        const auto &name=source.names[selector];
        const unsigned expected=name=="fixedsys-9"?18u:name=="MSLCD-14"?24u:name=="handel_gothic-24"?28u:29u;
        check(admitted==expected,"Actual MCC private-control lookup census changed");
        bindings+=admitted;++total.selectors;total.raster_bytes+=raw.atlas.pixels.size();
    }
    check(bindings==331,"Actual MCC native prompt selector/codepoint census is incomplete");
    check(read(legacy/"prompt-source.tsv")==marker&&read(legacy/"manifest.json")==manifest,
          "MCC native selection modified a retained authored overlay");
    const auto owner=copied/source.names[6];const auto original=read(owner);
    auto damaged=original;damaged.back()^=std::byte{1};write(owner,damaged);
    rejects([&]{load_gameplay_font_source(copied);},"Foreign MCC prompt pixel owner admitted");
    rejects([&]{GameplayFontBank changed(copied);},"New font bank admitted a changed MCC source");
    const auto retained=bank.raw(6);
    const auto before_change=load_retail_font(source.directory/source.names[6]);
    same_layout(*retained,before_change);
    check(retained->atlas.pixels==before_change.atlas.pixels,"Bound font bank reread a replaced source");
    const auto projected=bank.font(6);
    const auto expected=load_gameplay_font(source,6);
    same_layout(*projected,expected);
    check(projected->atlas.pixels==expected.atlas.pixels,"Bound font projection changed its authenticated pixels");
    write(owner,original);auto names=source.names;std::swap(names[4],names[5]);table(copied,names);
    rejects([&]{load_gameplay_font_source(copied);},"Foreign MCC prompt selector binding admitted");
    table(copied,source.names);
    check(load_gameplay_font_source(copied).domain==GameplayFontDomain::Mcc,"Restored genuine MCC prompt owner rejected");
    return bindings;
}
Totals actual(const std::filesystem::path& mcc,const std::filesystem::path& original) {
    const auto source=load_gameplay_font_source(mcc),reference=load_gameplay_font_source(original);
    GameplayFontBank bank(mcc);
    check(source.domain==GameplayFontDomain::Mcc && reference.domain==GameplayFontDomain::Original,"Actual typed domains wrong");
    const auto legacy=load_gameplay_font_source(mcc.parent_path()/"h2_maps_win64_dx11/fonts");
    check(legacy.domain==source.domain && legacy.directory==source.directory && legacy.names==source.names,"Legacy MCC path owner not resolved");
    Totals total;
    for(unsigned selector=0;selector<11;++selector) {
        const auto before=read(mcc/source.names[selector]);
        const auto raw=load_retail_font(mcc/source.names[selector]);
        const auto converted=load_gameplay_font(source,selector);
        const auto retained=bank.font(selector),retained_raw=bank.raw(selector);
        check(retained==bank.font(selector) && retained_raw==bank.raw(selector),"MCC bank rebuilt its glyph maps");
        same_layout(*retained,converted);same_layout(*retained_raw,raw);
        check(retained->atlas.pixels==converted.atlas.pixels && retained_raw->atlas.pixels==raw.atlas.pixels,
              "MCC bank changed its decoded raster");
        check(retained->sampled_glyphs.size()==converted.sampled_glyphs.size(),"MCC bank changed sample coverage");
        for(const auto& [c,sample]:converted.sampled_glyphs) {
            const auto& shared=retained->sampled_glyphs.at(c);
            check(shared.x==sample.x && shared.y==sample.y && shared.width==sample.width && shared.height==sample.height,
                  "MCC bank changed a physical glyph sample");
        }
        const auto og=load_retail_font(original/reference.names[selector]);
        if(const auto metrics=gameplay_font_reference_line_metrics(selector))
            check((*metrics)[0]==og.ascent && (*metrics)[1]==og.descent && (*metrics)[2]==og.leading,
                  "Native waypoint font metrics differ from the actual original font header");
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
    const auto unused=copied/source.names[0];const auto unused_before=read(unused);
    const auto old_time=std::filesystem::last_write_time(unused);
    auto unused_mutation=unused_before;unused_mutation.back()^=std::byte{1};write(unused,unused_mutation);
    std::filesystem::last_write_time(unused,old_time);
    rejects([&]{gameplay_font_horizontal_offset(selected,6);},"Changed unused font with restored date escaped cached authentication");
    write(unused,unused_before);std::filesystem::last_write_time(unused,old_time);
    check(gameplay_font_horizontal_offset(selected,6)==gameplay_font_horizontal_offset(source,6),
          "Restored authenticated font bytes were not admitted");
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
        const bool native=argc==3&&std::string_view(argv[1])=="--mcc-native";
        if(argc==3 && (!*argv[1] || !*argv[2] || (!native&&!std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"font_table.txt")) ||
                      !std::filesystem::is_regular_file(std::filesystem::path(argv[2])/"font_table.txt")))return 77;
        portable();Totals totals;unsigned prompt_bindings{};
        if(native)prompt_bindings=native_prompts(argv[2],totals);
        else if(argc==3)totals=actual(argv[1],argv[2]);
        std::cout<<"{\"checks\":"<<checks<<",\"negative_cases\":"<<negatives<<",\"actual_selectors\":"<<totals.selectors
            <<",\"logical_glyphs\":"<<totals.glyphs<<",\"unchanged_mcc_atlas_bytes\":"<<totals.raster_bytes
            <<",\"original_fallback_aliases_with_extra_mcc_glyphs\":"<<totals.fallback_extras
            <<",\"baseline_layout_mismatches\":"<<totals.baseline_mismatches
            <<",\"distinct_source_rasters_or_dimensions_differ\":"<<totals.raster_differences<<"}\n";
        if(native)std::cout<<"PASS native MCC prompt font ownership: selectors="<<totals.selectors<<" bindings="<<prompt_bindings
            <<" unchanged_native_atlas_bytes="<<totals.raster_bytes<<" authored_dependency=0\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
