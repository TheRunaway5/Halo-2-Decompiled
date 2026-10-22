#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_render_policy.hpp"
#include "halo2/world_material.hpp"
#include "../src/rendering/renderer/material_fog_phase.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <type_traits>

namespace {
using namespace halo2;
void require(bool value,const char *why) {if(!value)throw FormatError(why);}
template<class F>void rejects(F &&f) {
    bool rejected=false;try{f();}catch(const FormatError &){rejected=true;}
    require(rejected,"Malformed selected pass was admitted");
}
// Same live input before/after a rejected bind, including existing preview
// state, every owned bank/image and child. Host representation bytes detect
// replacement; they are never persisted or compared with original-game data.
struct MaterialInputSnapshot {
    std::vector<std::byte> bytes;
    std::vector<const Material *> seen;
    void add(std::span<const std::byte> value) {
        require(value.size()<=64u*1024*1024-bytes.size(),"Material input snapshot exceeds64MiB");
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    template<class T>void object(const T &value) {add(std::as_bytes(std::span(&value,1)));}
    template<class T>void bank(const std::vector<T> &value) {
        static_assert(std::is_trivially_copyable_v<T>);
        object(value.size());add(std::as_bytes(std::span(value)));
    }
    void text(const std::string &value) {object(value.size());add(std::as_bytes(std::span(value.data(),value.size())));}
    void declaration(const std::optional<SourceVertexDeclaration> &value) {if(value)bank(value->semantics);}
    void texture(const Texture &value) {
        object(value);object(value.pixels.size());add(std::span<const std::byte>(value.pixels));bank(value.mips);
    }
    void material(const Material &value) {
        if(std::ranges::find(seen,&value)!=seen.end())return;
        require(seen.size()<64,"Material input child graph exceeds64owners");seen.push_back(&value);
        object(value);text(value.name);text(value.template_name);
        texture(value.diffuse);texture(value.detail);texture(value.bump);texture(value.auxiliary);
        bank(value.bitmaps);bank(value.pixel_constants);bank(value.vertex_constants);bank(value.lods);
        bank(value.layers);bank(value.pass_implementations);bank(value.vertex_program);declaration(value.vertex_declaration);
        object(value.passes.size());
        for(const auto &pass:value.passes) {
            object(pass);text(pass.name);text(pass.vertex_shader);
            bank(pass.textures);bank(pass.states);bank(pass.texture_states);bank(pass.bitmap_remaps);
            bank(pass.pixel_remaps);bank(pass.vertex_remaps);bank(pass.pixel_externs);
            bank(pass.vertex_externs);bank(pass.state_externs);bank(pass.fragments);
            bank(pass.permutations);bank(pass.combiners);bank(pass.vertex_code);declaration(pass.vertex_declaration);
        }
        const auto &a=value.animation;
        bank(a.lods);bank(a.layers);bank(a.passes);bank(a.parameters);bank(a.implementations);
        bank(a.references);bank(a.mappings);object(a.overlays.size());
        for(const auto &overlay:a.overlays){object(overlay);bank(overlay.curve);}
        if(a.tables)object(*a.tables);
        if(value.vertex_bank){object(*value.vertex_bank);bank(value.vertex_bank->constants);}
        for(const auto &stage:value.animated_textures){object(stage.size());for(const auto &image:stage)texture(image);}
        for(const auto &gap:value.rendering_gaps){object(gap);text(gap);}
        const auto child=[&](const auto &pass){if(pass)material(*pass);};
        child(value.environment_pass);child(value.planar_fog_pass);
        for(const auto &pass:value.world_overlays_before)child(pass);
        for(const auto &pass:value.world_overlays_after)child(pass);
        for(const auto &pass:value.world_bloom_passes)child(pass);
        for(const auto &pass:value.world_planar_passes)child(pass);
    }
    static std::vector<std::byte> capture(const Material &value) {
        MaterialInputSnapshot result;result.material(value);return std::move(result.bytes);
    }
};
void phase_controls() {
    Material procedural;
    require(!detail::material_after_fog(procedural),"Opaque procedural fallback changed");
    procedural.forward_blend=3;
    require(detail::material_after_fog(procedural),"Blended procedural fallback changed");
    Material m;m.forward_pass=true;m.forward_pass_index=0;m.forward_blend=3;
    m.passes.emplace_back();m.passes.back().template_pass=0;
    m.lods.push_back({{25u<<9},1u<<7,0});m.layers.resize(25);m.layers[7]={1u<<9};
    require(!detail::material_after_fog(m),"Blended layer7 was moved after fog");
    m.lods[0].available_layers=1u<<15;m.layers[15]={1u<<9};m.forward_blend=0;
    require(detail::material_after_fog(m),"Unblended layer15 lost its authored phase");
    rejects([&]{auto bad=m;bad.selected_shader_lod=1;(void)detail::material_after_fog(bad);});
    rejects([&]{auto bad=m;bad.forward_pass_index=1;(void)detail::material_after_fog(bad);});
    rejects([&]{auto bad=m;bad.layers.resize(3);(void)detail::material_after_fog(bad);});
    rejects([&]{auto bad=m;bad.lods[0].available_layers|=1u<<7;(void)detail::material_after_fog(bad);});
}
Material load(const Cache &cache,const std::string &name) {
    const auto *tag=cache.query_tag({.group=tag_groups::shader,.name=name,.require_address=true});
    require(tag,"Actual MCC character shader is missing");return cache.load_material(tag->datum);
}
void retained_pass(const Material &pass,const Material &authored,unsigned index,unsigned layer) {
    require(pass.source_shader_datum==authored.source_shader_datum && pass.selected_shader_lod==0 &&
        pass.forward_pass && pass.forward_pass_index==index && source_material_layer(pass)==layer,
        "Prepared child lost its selected source shader/LOD/pass/layer");
    const auto &raw=authored.passes.at(index);
    require(pass.vertex_program==raw.vertex_code && pass.vertex_declaration==raw.vertex_declaration &&
        raw.texture_states.size()<=pass.texture_states.size() &&
        std::equal(raw.texture_states.begin(),raw.texture_states.end(),pass.texture_states.begin()) &&
        std::all_of(pass.texture_states.begin()+raw.texture_states.size(),pass.texture_states.end(),
            [](const auto &state){return state==ShaderTextureState{};}) &&
        pass.forward_program.fragment_offsets==raw.program.fragment_offsets &&
        pass.forward_program.words[54]==raw.program.words[54] &&
        pass.forward_program.words[59]==raw.program.words[59],
        "Prepared child changed its compiled input/sampler/texture-addressing contract");
    require(detail::material_after_fog(pass)==(layer==15),"Prepared child has the wrong fog phase");
}
void cortana(const Cache &cache) {
    const std::array<std::string_view,5> names{
        "shaders\\shader_passes\\z_prepass\\z_prepass",
        "shaders\\shader_passes\\transparent\\cortana_default_transparent_one_sided",
        "shaders\\shader_passes\\plasma\\plasma_alpha",
        "shaders\\shader_passes\\transparent\\bumped_environment_additive",
        "shaders\\shader_passes\\bloom\\transparent_bloom"};
    for(const auto body:{"hair","body","arms","head","eyes","teeth"}) {
        const auto authored=load(cache,std::string("objects\\characters\\cortana\\shaders\\cortana_")+body);
        require(authored.lods.size()==1 && authored.lods[0].available_layers==0x28080 &&
            authored.layers.at(7).packed==0x0200 && authored.layers.at(15).packed==0x0601 &&
            authored.layers.at(17).packed==0x0204 && authored.passes.size()==5,
            "Actual Cortana template differs from its authenticated original five-pass fixture");
        for(unsigned i=0;i<names.size();++i)
            require(authored.passes[i].name==names[i] && authored.passes[i].template_pass==i &&
                authored.passes[i].implementation==0,"Cortana authored pass order changed");
        require(authored.passes[0].texture_states.empty(),
            "The authenticated textureless z-prepass acquired sampler states");
        if(std::string_view(body)=="hair") {
            const auto &pass=authored.passes[1];
            const auto stage=std::ranges::find_if(pass.textures,[](const auto &row){return row.stage==1;});
            require(stage!=pass.textures.end() && stage->external==0 && stage->parameter==1,
                "Actual Cortana hair late pass1/stage1 binding changed");
            const auto remap=std::ranges::find_if(pass.bitmap_remaps,[&](const auto &row){return (row.destination&255)==stage->parameter;});
            require(remap!=pass.bitmap_remaps.end() && remap->source==1 &&
                authored.bitmaps.at(remap->source).datum==0xffffffffu,
                "Actual Cortana hair pass1/stage1 lost its authored NONE bitmap");
            std::cout<<"Actual MCC Cortana hair: pass1/stage1/parameter1/bitmap1=FFFFFFFF; TU1.5 16FC8 default owner\n";
        }
        auto prepared=authored;
        require(prepare_dynamic_material(cache,prepared,{},true,0.f),"Cortana source pass binding failed");
        retained_pass(prepared,authored,0,7);
        require(prepared.world_overlays_after.size()==3,"Cortana lost its three authored layer15 passes");
        for(unsigned i=0;i<3;++i)retained_pass(*prepared.world_overlays_after[i],authored,i+1,15);
        if(std::string_view(body)=="hair") {
            const auto expected=cache.load_scope_default_bitmap();
            const auto same_texture=[&](const Texture &actual) {
                return actual.width==expected.width && actual.height==expected.height &&
                    actual.format==expected.format && actual.layers==expected.layers &&
                    actual.depth==expected.depth && std::ranges::equal(actual.pixels,expected.pixels) &&
                    std::ranges::equal(actual.mips,expected.mips,[](const auto &a,const auto &b){
                        return a.width==b.width && a.height==b.height && a.offset==b.offset &&
                            a.size==b.size && a.depth==b.depth;});
            };
            require(same_texture(prepared.world_overlays_after[0]->detail),
                "Authored NONE did not bind the actual rasterizer default image");
            auto reset_frame=authored;reset_frame.bitmaps[1].index=7;
            require(prepare_dynamic_material(cache,reset_frame,{},true,0.f) &&
                same_texture(reset_frame.world_overlays_after.at(0)->detail),
                "Authored NONE did not reset its source frame to0");
            auto wrong_group=authored;wrong_group.bitmaps[1].datum=authored.source_shader_datum;
            const auto wrong_before=MaterialInputSnapshot::capture(wrong_group);
            rejects([&]{(void)prepare_dynamic_material(cache,wrong_group,{},true,0.f);});
            require(MaterialInputSnapshot::capture(wrong_group)==wrong_before,
                "Wrong-typed bitmap exception changed its actual material input");
        }
        require(prepared.world_overlays_after[0]->forward_blend==2 &&
            prepared.world_overlays_after[1]->forward_blend==3 &&
            prepared.world_overlays_after[2]->forward_blend==3,
            "Cortana original premultiplied/additive pass factors changed");
        require(prepared.world_bloom_passes.size()==1 && prepared.world_bloom_passes[0]->world_bloom_pass,
            "Cortana lost its separately scheduled alpha bloom mask");
        retained_pass(*prepared.world_bloom_passes[0],authored,4,17);
        // Phase is authored metadata, independent of whether the view has
        // atmosphere. Actual command recording remains a separate GPU gate.
        std::vector<unsigned> early,late;
        for(const auto &child:prepared.world_overlays_after)
            (detail::material_after_fog(*child)?late:early).push_back(child->forward_pass_index);
        require(early.empty() && late==std::vector<unsigned>{1,2,3},
            "Cortana late child order/classification changed");
        auto invalid=authored;
        invalid.passes[2].pixel_externs.push_back({0,0,255});
        const auto before=MaterialInputSnapshot::capture(invalid);
        const bool prior_forward=invalid.forward_pass;
        const auto prior_pass=invalid.forward_pass_index;
        require(!prepare_dynamic_material(cache,invalid,{},true,0.f),
            "Consumed unsupported late pixel extern255 was admitted");
        require(MaterialInputSnapshot::capture(invalid)==before,
            "Rejected late pass changed its actual complete material input");
        std::cout<<"Rejected late extern255: "<<body<<" prior_forward="<<prior_forward
            <<" prior_pass="<<prior_pass<<" unchanged_input_bytes="<<before.size()<<'\n';
    }
}
}
int main(int argc,char **argv)try {
    phase_controls();
    if(argc==2 && std::string_view(argv[1])=="--phase-only") {
        std::cout<<"PASS material fog phase: procedural/full-default, layer7/layer15 and malformed inputs\n";return 0;
    }
    const char *donor=argc>1 && *argv[1]?argv[1]:std::getenv("HALO2_MCC_SOURCE_TAG_DONOR");
    if(!donor || !std::filesystem::is_regular_file(donor))return 77;
    const auto maps=std::filesystem::path(donor).parent_path();
    if(!std::filesystem::is_regular_file(maps/"01b_spacestation.map"))return 77;
    Cache cache(maps/"01b_spacestation.map",maps);
    require(cache.is_mcc() && !cache.is_vista(),"Character pass regression requires MCC-only inputs");
    cortana(cache);
    auto ordinary=load(cache,"objects\\characters\\masterchief\\shaders\\masterchief");
    require(prepare_dynamic_material(cache,ordinary,{},true,0.f),"Ordinary lit control failed");
    require(!detail::material_after_fog(ordinary) && std::ranges::none_of(ordinary.world_overlays_after,
        [](const auto &p){return detail::material_after_fog(*p);}),"Ordinary lit passes moved after fog");
    auto simple=load(cache,"objects\\characters\\jackal\\shaders\\jackal_shield");
    require(prepare_dynamic_material(cache,simple,{},true,0.f) &&
        detail::material_after_fog(simple) && simple.world_overlays_after.empty(),
        "Simple layer15 material was duplicated or moved before fog");
    std::cout<<"PASS actual MCC Cortana six shaders: layer7/pass0; layer15/passes1,2,3; separate layer17/pass4; ordinary lit unchanged\n";
    std::cout<<"LIMIT CPU binding/phase classification only; no World/GPU or matched original frame\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
