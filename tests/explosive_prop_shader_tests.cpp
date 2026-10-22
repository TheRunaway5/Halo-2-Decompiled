#include "halo2/cache.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/world_material.hpp"
#include <array>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>

using namespace halo2;
namespace {
void check(bool value,const std::string &why){if(!value)throw FormatError(why);}
bool explosive(std::string_view name) {
    return name.find("power_module")!=name.npos || name.find("fusion_coil")!=name.npos ||
        name.find("gas_container")!=name.npos;
}
std::vector<std::uint32_t> shaders(const Cache &cache,std::uint32_t model) {
    const Bytes root(cache.tag_payload({model}));
    const auto count=root.u32(0x60);check(count<256,"Excessive prop material table");
    const Bytes rows(cache.read_tag_bytes(root.u32(0x64),count*32));
    std::vector<std::uint32_t> out;
    for(unsigned n=0;n<count;++n)out.push_back(rows.u32(n*32+12));
    return out;
}
void bound_passes(const Material &material,std::set<unsigned> &passes) {
    check(material.forward_pass,"Explosive prop lost a bound forward pass");
    check(passes.insert(material.forward_pass_index).second,"Explosive prop duplicated a shader pass");
    if(material.environment_pass)bound_passes(*material.environment_pass,passes);
    for(const auto &child:material.world_overlays_before)bound_passes(*child,passes);
    for(const auto &child:material.world_overlays_after)bound_passes(*child,passes);
    for(const auto &child:material.world_bloom_passes)bound_passes(*child,passes);
}
void material(const Cache &source,const Cache &cache,std::uint32_t datum,const Material &reference,unsigned &lods) {
    const auto raw=cache.load_material(datum);
    check(raw.name==reference.name,"Missing prop shader: "+reference.name);
    check(raw.template_name==reference.template_name && raw.pixel_constants==reference.pixel_constants &&
        raw.vertex_constants==reference.vertex_constants && raw.bitmaps.size()==reference.bitmaps.size(),
        "Prop shader changed its authored parameters: "+raw.name);
    for(unsigned n=0;n<raw.bitmaps.size();++n) {
        const auto &a=raw.bitmaps[n],&b=reference.bitmaps[n];
        const auto *actual=cache.find_tag({a.datum}),*expected=source.find_tag({b.datum});
        check(a.index==b.index && a.log_dimension==b.log_dimension &&
            ((a.datum==0xffffffffu && b.datum==0xffffffffu) ||
             (actual && expected && actual->group=="bitm" && actual->name==expected->name)),
            "Prop shader changed an authored bitmap/frame: "+raw.name);
    }
    check(raw.lods.size()==reference.lods.size() && raw.passes.size()==reference.passes.size(),
        "Prop shader lost original template passes: "+raw.name);
    for(unsigned n=0;n<raw.passes.size();++n) {
        const auto &a=raw.passes[n],&b=reference.passes[n];
        check(a.name==b.name && a.vertex_shader==b.vertex_shader && a.vertex_code==b.vertex_code &&
            a.program.words==b.program.words && a.program.fragment_offsets==b.program.fragment_offsets,
            "Prop shader differs from original Xbox programs: "+raw.name);
    }
    for(unsigned lod=0;lod<raw.lods.size();++lod) {
        check(raw.lods[lod].available_layers==reference.lods[lod].available_layers &&
            raw.lods[lod].layers.packed==reference.lods[lod].layers.packed,
            "Prop shader lost original LOD layers: "+raw.name);
        auto prepared=raw;prepared.selected_shader_lod=lod;
        check(prepare_dynamic_material(cache,prepared,{},true),
            "Prop shader cannot bind LOD "+std::to_string(lod)+": "+raw.name);
        check(prepared.rendering_gaps.empty(),"Prop shader has unresolved bound passes: "+raw.name);
        check(prepared.forward_pass && prepared.world_pass && prepared.source_shader_datum==datum &&
            prepared.selected_shader_lod==lod,"Prop shader substituted its authored material identity: "+raw.name);
        std::set<unsigned> bound,expected;bound_passes(prepared,bound);
        // These are the independently scheduled material layers used by the
        // original prop templates. Shadow, fog and light passes have separate
        // scene owners and are not part of dynamic material preparation.
        for(const auto layer:{1u,2u,3u,7u,8u,15u,17u}) {
            const auto &definition=raw.lods[lod];
            if(!(definition.available_layers&(1u<<layer)))continue;
            const auto range=raw.layers.at(definition.layers.first()+layer);
            for(unsigned n=0;n<raw.passes.size();++n) {
                const auto &pass=raw.passes[n];
                if(pass.template_pass>=range.first() && pass.template_pass-range.first()<range.count() &&
                    pass.implementation==(layer==3?2u:0u))expected.insert(n);
            }
        }
        check(bound==expected,"Prop shader omitted an authored body/emissive/transparent pass: "+raw.name);
        ++lods;
    }
}
void map(const std::filesystem::path &original,const std::filesystem::path &imported,
    unsigned &models,unsigned &materials,unsigned &lods) {
    Cache source(original);std::map<std::string,std::uint32_t> definitions;
    for(const auto &tag:source.tags())if(tag.group=="bloc" && explosive(tag.name))
        definitions.emplace(tag.name,source.load_initial_model_definition(tag.datum).render_model);
    check(!definitions.empty(),"Explosive prop reference map has no explosive models");
    std::optional<Cache> target;
    if(!imported.empty()){target.emplace(imported);check(target->is_mcc(),"Expected prepared MCC cache");}
    const Cache &actual=target?*target:source;
    std::set<std::uint32_t> checked_models,checked_shaders;
    for(const auto &[name,model]:definitions) {
        if(!checked_models.insert(model).second)continue;
        const auto expected=shaders(source,model);
        const auto *object=actual.query_tag({.group=TagGroup{"bloc"},.name=name,.require_address=true});
        check(object,"Imported explosive object missing: "+name);
        const auto actual_model=actual.load_initial_model_definition(object->datum).render_model;
        const auto bound=shaders(actual,actual_model);
        check(bound.size()==expected.size(),"Explosive model changed its authored material slots: "+name);
        for(unsigned slot=0;slot<expected.size();++slot) {
            const auto reference=source.load_material(expected[slot]);
            const auto *tag=actual.find_tag({bound[slot]});
            check(tag && tag->group=="shad" && tag->name==reference.name,
                "Explosive model bound another shader at slot "+std::to_string(slot)+": "+name);
            if(checked_shaders.insert(bound[slot]).second){material(source,actual,bound[slot],reference,lods);++materials;}
        }
        ++models;
    }
    std::cout<<original.stem().string()<<": original material slots and every shader LOD verified"<<std::endl;
}
}
int main(int argc,char **argv)try {
    check(argc==2 || argc==3,"Supply Xbox map directory and optional prepared MCC map directory");
    if(argc==3 && std::string_view(argv[2]).empty())return 77;
    unsigned models=0,materials=0,lods=0;
    for(const auto *name:{"lockout","foundation","colossus","cyclotron","headlong","waterworks",
        "zanzibar","highplains2","01b_spacestation","04a_gasgiant","04b_floodlab","06b_floodzone",
        "08a_deltacliffs","08b_deltacontrol","containment"}) {
        const auto file=std::string(name)+".map";
        auto original=std::filesystem::path(argv[1])/file;
        if(!std::filesystem::exists(original))original=std::filesystem::path(argv[1]).parent_path().parent_path()/"map-pack/maps"/file;
        // The Xbox disc calls Tombstone highplains2; MCC calls it highplains.
        const auto imported=std::string_view(name)=="highplains2"?"highplains.map":file;
        map(original,argc==3?std::filesystem::path(argv[2])/imported:std::filesystem::path{},models,materials,lods);
    }
    check(models>=7 && materials>=9 && lods>=materials,"Explosive prop shader coverage is incomplete");
    std::cout<<"PASS "<<models<<" intact/debris models, "<<materials<<" shaders, "<<lods<<" shader LODs\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
