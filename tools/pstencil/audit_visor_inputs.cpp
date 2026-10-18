#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_shader_texture.hpp"
#include "halo2/world_material.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char** argv)try {
    if(argc!=3)throw FormatError("Supply current map and new evidence directory");
    const std::filesystem::path out=argv[2];
    if(std::filesystem::exists(out))throw FormatError("Evidence directory already exists");
    std::filesystem::create_directories(out);
    Cache cache(argv[1]);Json rows=Json::array();
    const auto tag_name=[&](std::uint32_t id) {
        for(const auto& tag:cache.tags())if(tag.datum==id)return tag.name;
        return std::string{};
    };
    const auto image=[](const Texture& t) {
        Json mips=Json::array();
        for(const auto& m:t.mips)mips.push_back({{"width",m.width},{"height",m.height},
            {"offset",m.offset},{"size",m.size},{"depth",m.depth}});
        return Json{{"width",t.width},{"height",t.height},{"layers",t.layers},
            {"format",t.format},{"depth",t.depth},{"bytes",t.pixels.size()},{"mips",mips}};
    };
    for(const auto& tag:cache.tags())if(tag.group=="shad" &&
        (tag.name=="objects\\characters\\masterchief\\shaders\\masterchief" ||
         tag.name=="objects\\characters\\masterchief\\shaders\\masterchief_visor")) {
        auto source=cache.load_material(tag.datum);
        Json row{{"shader",tag.name},{"datum",tag.datum},{"template",source.template_name},
            {"bitmaps",Json::array()},{"passes",Json::array()},{"prepared_lods",Json::array()}};
        for(const auto& bitmap:source.bitmaps)row["bitmaps"].push_back({{"datum",bitmap.datum},
            {"frame",bitmap.index},{"name",tag_name(bitmap.datum)}});
        for(const auto& pass:source.passes) {
            Json p{{"name",pass.name},{"implementation",pass.implementation},
                {"template_pass",pass.template_pass},{"vertex_shader",pass.vertex_shader},
                {"textures",Json::array()},{"bitmap_remaps",Json::array()}};
            for(const auto& t:pass.textures)p["textures"].push_back({{"stage",t.stage},
                {"parameter",t.parameter},{"external",t.external},{"flags",t.flags}});
            for(const auto& r:pass.bitmap_remaps)p["bitmap_remaps"].push_back(
                {r.destination,r.component,r.source});
            row["passes"].push_back(p);
        }
        const auto inspect=[&](auto&& self,const Material& m,Json& target)->void {
            if(m.forward_pass) {
                const auto& p=m.passes.at(m.forward_pass_index);
                target.push_back({{"pass",p.name},{"implementation",p.implementation},
                    {"vertex_shader",p.vertex_shader},{"vertex",m.forward_vertex},{"blend",m.forward_blend},
                    {"accumulation_texture",m.accumulation_texture},
                    {"textures",{image(m.diffuse),image(m.detail),image(m.bump),image(m.auxiliary)}}});
            }
            if(m.environment_pass)self(self,*m.environment_pass,target);
            for(const auto& c:m.world_overlays_before)self(self,*c,target);
            for(const auto& c:m.world_overlays_after)self(self,*c,target);
        };
        for(unsigned lod=0;lod<source.lods.size();++lod) {
            auto m=source;m.selected_shader_lod=lod;
            Json selected=Json::array();
            const auto accepted=prepare_dynamic_material(cache,m,{},true,0.f);
            if(accepted)inspect(inspect,m,selected);
            row["prepared_lods"].push_back({{"lod",lod},{"accepted",accepted},{"draws",selected}});
        }
        const auto normalization=source_shader_texture_reference(cache,tag.datum,1);
        const auto texture=cache.load_bitmap(normalization.bitmap,normalization.frame);
        row["normalization"]={{"datum",normalization.bitmap},{"frame",normalization.frame},
            {"name",tag_name(normalization.bitmap)},{"image",image(texture)}};
        std::ofstream file(out/"current-normalization.rgba",std::ios::binary);
        file.write(reinterpret_cast<const char*>(texture.pixels.data()),std::streamsize(texture.pixels.size()));
        if(!file)throw FormatError("Cannot retain current normalization pixels");
        rows.push_back(std::move(row));
    }
    if(rows.size()!=2)throw FormatError("Actual Chief armor/visor shaders absent");
    std::ofstream(out/"inputs.json")<<Json{{"map",argv[1]},{"scope","Read-only current-asset bindings and prepared LODs; no new renderer or original framebuffer claim"},{"materials",rows}}.dump(2)<<'\n';
    std::cout<<"Retained "<<rows.size()<<" actual Chief material banks\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
