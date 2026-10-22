#pragma once
#include "halo2/action_prompt_bindings.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/hud.hpp"
#include "halo2/controller_prompt_artwork.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>

// Authenticated MCC font/CPU HUD owner qualification, not a native gameplay,
// hardware input, GPU pixel or full ONLINE visual acceptance claim. The tiny
// action state below is an explicit stimulus for the actual public HUD API;
// every expected glyph/image/advance comes from the actual source font.
namespace mcc_native_prompt_test {
inline void require(bool value,const char *message){if(!value)throw halo2::FormatError(message);}
struct Temp {
    std::filesystem::path path;
    Temp(){
        const auto *root=std::getenv("HALO2_TEST_SCRATCH");require(root&&*root,"MCC prompt fixture requires managed scratch");
        for(unsigned n=0;;++n){path=std::filesystem::path(root)/("native-prompt-fonts-"+std::to_string(n));
            if(std::filesystem::create_directory(path))break;}
    }
    ~Temp(){std::error_code error;std::filesystem::remove_all(path,error);}
};
inline std::string utf8(unsigned code){return std::string{char(0xe0|(code>>12)),char(0x80|((code>>6)&63)),char(0x80|(code&63))};}
inline bool near(float a,float b){return std::abs(a-b)<=.0001f;}
inline void verify_native_mcc_prompt_hud(const std::filesystem::path &font_root){
    using namespace halo2;
    const auto source=load_gameplay_font_source(font_root);
    require(source.domain==GameplayFontDomain::Mcc&&source.names.size()==12,"Prompt fixture lacks genuine MCC font ownership");
    Temp temporary;const auto fonts=temporary.path/"halo2/h2_fonts";
    std::filesystem::create_directories(fonts);
    for(const auto &name:std::set<std::string>(source.names.begin(),source.names.end()))
        std::filesystem::copy_file(source.directory/name,fonts/name);
    std::filesystem::copy_file(source.directory/"font_table.txt",fonts/"font_table.txt");
    const auto private_source=load_gameplay_font_source(fonts);
    unsigned bindings{};
    for(unsigned selector=0;selector<source.names.size();++selector){
        const auto raw=load_retail_font(fonts/source.names[selector]);
        std::map<std::uint32_t,unsigned> counts;for(auto record:raw.glyph_record_indices)++counts[record];
        unsigned present{};
        for(unsigned code=0xe100;code<=0xe13c;++code){
            // These native reserved keycap rows have blank pixels and are
            // outside the331 actually bound controller/keycap prompts.
            if((code>0xe111&&code<0xe12c)||(code>=0xe136&&code<=0xe13b))continue;
            const auto record=raw.glyph_record_indices.at(code);const auto found=raw.glyphs.find(code);
            const bool own=record!=0xffffffffu&&counts.at(record)==1&&found!=raw.glyphs.end()&&found->second.width&&found->second.height;
            if(code<=0xe111)require(own,"Native MCC control glyph aliases a fallback or is missing");
            if(!own)continue;
            const auto &g=found->second;bool alpha{};
            for(unsigned y=0;y<g.height&&!alpha;++y)for(unsigned x=0;x<g.width&&!alpha;++x)
                alpha=raw.atlas.pixels[(std::size_t(g.y+y)*raw.atlas.width+g.x+x)*4+3]!=std::byte{};
            require(alpha,"Native MCC control glyph has no decoded alpha");++present;
        }
        const auto &name=source.names[selector];
        const unsigned expected=name=="fixedsys-9"?18u:name=="MSLCD-14"?24u:name=="handel_gothic-24"?28u:29u;
        require(present==expected,"Native MCC selector/codepoint census changed");bindings+=present;
    }
    require(bindings==331,"Native MCC prompt owner census is incomplete");
    HudContent content;content.action_messages.enabled=true;content.action_messages.color=0xff12ab34u;
    content.strings.emplace(1,"\xee\x91\x88\xee\x91\x89"); // Existing primary/secondary action tokens.
    Mesh mesh;MultiplayerHud hud(content,fonts,mesh,4);
    // A retained foreign/authored bank is deliberately invalid. Native MCC
    // selection must not open it, repair it, or use it as sampled artwork.
    const auto old=temporary.path/"presentation/prompts";std::filesystem::create_directories(old);
    const std::string marker="foreign retained prompt bank\n",manifest="not native MCC pixels\n";
    {std::ofstream file(old/"prompt-source.tsv");file<<marker;}
    {std::ofstream file(old/"manifest.json");file<<manifest;}
    require(font_prompt_asset_directory(fonts,old).empty(),"Native MCC route selected retained authored artwork");
    Mesh old_mesh;MultiplayerHud with_old(content,fonts,old_mesh,4);
    const std::array<RetailFont,2> native{load_retail_font(fonts/source.names[6]),load_retail_font(fonts/source.names[5])};
    std::array<unsigned,2> materials{};
    for(unsigned n=0;n<2;++n){
        const auto label=std::string("action-message/mcc-buttons/font")+std::to_string(n?5:6);
        const auto found=std::find_if(mesh.materials.begin(),mesh.materials.end(),[&](const auto &m){return m.name==label;});
        require(found!=mesh.materials.end(),"Actual HUD did not construct native colored MCC button fonts");
        materials[n]=unsigned(found-mesh.materials.begin());const auto &actual=found->diffuse,&expected=native[n].atlas;
        require(actual.width==expected.width&&actual.height==expected.height&&actual.format==expected.format&&
            actual.layers==expected.layers&&actual.pixels==expected.pixels,
            "Actual HUD button material does not own the unchanged native MCC atlas");
        require(old_mesh.materials.size()==mesh.materials.size()&&old_mesh.materials[materials[n]].name==label&&
            old_mesh.materials[materials[n]].diffuse.pixels==actual.pixels,
            "Retained authored bank changed the actual native HUD material owner");
    }
    MultiplayerState state;
    for(auto &player:state.players){player.connected=true;player.health=player.shield=1;
        player.action_prompt.message=1;player.action_prompt.target.object=1;}
    ControllerPromptArtwork artwork(mesh,fonts);artwork.prepare(5);artwork.prepare(6);
    unsigned draws{};
    for(auto device:{PromptDevice::Xbox,PromptDevice::PlayStation})
    for(unsigned players:{1u,2u,4u})for(unsigned view=0;view<players;++view){
        hud.prompt_device(view,device);with_old.prompt_device(view,device);
        for(unsigned code=0xe100;code<=0xe10f;++code){
            hud.prompt_gamepad_buttons(view,{code,code});with_old.prompt_gamepad_buttons(view,{code,code});
            require(hud.action_text(state,view,view)==utf8(code)+utf8(code),"Native colored action glyph fell back to bracketed text");
            const auto n=players>1?1u:0u;const auto &font=native[n];const auto &g=font.glyphs.at(code);
            for(const auto viewport:{std::array{640u,480u},std::array{1280u,720u}}){
                const auto vertices=hud.draw(state,view,view,players,viewport[0],viewport[1],25,14400);
                const auto retained=with_old.draw(state,view,view,players,viewport[0],viewport[1],25,14400);
                require(vertices.size()==12,"Actual HUD action stimulus did not draw both controller glyphs");
                require(retained.size()==vertices.size()&&std::equal(vertices.begin(),vertices.end(),retained.begin(),[](const auto &a,const auto &b){
                    return a.position.x==b.position.x&&a.position.y==b.position.y&&a.position.z==b.position.z&&a.material==b.material&&
                        a.color.x==b.color.x&&a.color.y==b.color.y&&a.color.z==b.color.z&&a.uv.x==b.uv.x&&a.uv.y==b.uv.y;
                }),"Retained authored bank changed the actual native HUD draw");
                const auto sample=artwork.sample(n?5:6,code,device,g,0,0,g.width,g.height);
                require(bool(sample),"Local controller artwork was not selected");
                for(const auto &v:vertices)require(v.material==sample->material&&v.color.x==1&&v.color.y==1&&v.color.z==1,
                    "Controller control was tinted as ordinary HUD text or selected another family's material");
                const auto dx=(vertices[1].position.x-vertices[0].position.x)*float(viewport[0])/640,
                    dy=(vertices[2].position.y-vertices[0].position.y)*float(viewport[1])/480;
                require(dx>0&&dy>0&&near(dx/dy,float(g.width)/g.height),"Native MCC glyph aspect changed in HUD projection");
                const auto advance=(vertices[6].position.x-vertices[0].position.x)*float(viewport[0])/640;
                require(near(advance/dx,float(g.advance+g.bearing_x)/g.width),"Native MCC glyph advance changed in HUD projection");
                require(near(vertices[0].uv.x,sample->uv[0])&&near(vertices[0].uv.y,sample->uv[1])&&
                    near(vertices[1].uv.x,sample->uv[2])&&near(vertices[2].uv.y,sample->uv[3]),
                    "HUD glyph UVs do not select their player's actual control artwork");++draws;
            }
        }
    }
    // A foreign pixel owner must fail the genuine source guard, even if a
    // retained authored overlay is present. Modify only our private copy.
    const auto changed=fonts/source.names[6];const auto timestamp=std::filesystem::last_write_time(changed);
    {std::fstream file(changed,std::ios::binary|std::ios::in|std::ios::out);file.seekg(-1,std::ios::end);char value{};file.get(value);
        file.seekp(-1,std::ios::end);file.put(char(value^1));}
    std::filesystem::last_write_time(changed,timestamp);bool rejected{};
    try{Mesh foreign;MultiplayerHud invalid(content,fonts,foreign,1);}catch(const FormatError&){rejected=true;}
    require(rejected,"HUD admitted changed native MCC prompt pixels with unchanged file time");
    {std::ifstream file(old/"prompt-source.tsv");require(std::string(std::istreambuf_iterator<char>(file),{})==marker,
        "HUD mutated a retained prompt binding");}
    {std::ifstream file(old/"manifest.json");require(std::string(std::istreambuf_iterator<char>(file),{})==manifest,
        "HUD mutated retained authored prompt metadata");}
    std::cout<<"PASS native MCC prompt HUD ownership: selectors=12 bindings="<<bindings<<" colored_draws="<<draws
        <<" source_geometry=unchanged controller_families=2 historical_bank=ignored foreign_owner=rejected CPU_only=1\n";
}
} // namespace mcc_native_prompt_test
