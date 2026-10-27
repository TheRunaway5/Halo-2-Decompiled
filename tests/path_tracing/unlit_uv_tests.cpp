#include "halo2/path_tracing/unlit_uv.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include "halo2/world_material.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <set>

using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
unsigned checks{},materials{},samples{},flattened_differences{};
std::set<std::string> families;
std::map<std::string,std::uint32_t> color_output_masks,attribute10_output_masks;
std::map<std::string,std::set<unsigned>> color_registers,attribute10_externs;
void require(bool value,const char* message){++checks;if(!value)throw FormatError(message);}
void verify(Material& material,std::mt19937& random) {
    const auto admitted=pt::qualified_unlit_uv(material);if(!admitted)return;
    ++materials;
    const auto& pass=material.passes[material.forward_pass_index];families.insert(pass.vertex_shader);
    require(pt::qualified_unlit_white_tint(material),"Actual native source material lost its radiance tint policy");
    if(material.vertex_declaration)
        for(unsigned reg=0;reg<material.vertex_declaration->semantics.size();++reg)
            if(material.vertex_declaration->semantics[reg]==14)color_registers[pass.vertex_shader].insert(reg);
    for(const auto& remap:pass.vertex_externs)
        if(((remap.destination>>4)&1)==0 && (remap.destination&15)==10)
            attribute10_externs[pass.vertex_shader].insert(remap.source);
    const auto program=decode_vertex_program(material.vertex_program);
    const auto transforms=material.forward_transforms;const auto constants=material.program_constants[1];
    for(unsigned c=0;c<8;++c)require(transforms[c]==constants[c],
        "Actual source material UV constants differ between retained native interfaces");
    auto changed=material.vertex_program.back();material.vertex_program.back()^=std::byte{1};
    require(!pt::qualified_unlit_uv(material),"Changed native UV program bypassed exact qualification");
    material.vertex_program.back()=changed;
    const auto declaration=material.vertex_declaration;
    material.vertex_declaration=SourceVertexDeclaration{0,{0,4}};
    require(!pt::qualified_unlit_uv(material),"A normal stream was accepted as raw unlit UV");
    material.vertex_declaration=declaration;
    auto mapped_color=declaration.value_or(SourceVertexDeclaration{0,{0,3}});
    mapped_color.semantics.resize(std::max<std::size_t>(11,mapped_color.semantics.size()),255);
    mapped_color.semantics[10]=14;material.vertex_declaration=mapped_color;
    require(!pt::qualified_unlit_white_tint(material),"Explicit color-stream override was whitened as model lighting");
    material.vertex_declaration=declaration;
    material.forward_pass=false;
    require(!pt::qualified_unlit_white_tint(material),"Procedural colored fixture acquired source radiance policy");
    material.forward_pass=true;
    material.forward_transforms[0][0]=NAN;
    require(!pt::qualified_unlit_uv(material),"Nonfinite native UV transform was accepted");
    material.forward_transforms=transforms;
    std::uniform_real_distribution<float> value(-4,4),uv(-3,3);
    for(unsigned profile=0;profile<17;++profile) {
        material.forward_transforms=transforms;material.program_constants[1]=constants;
        if(profile)for(unsigned c=0;c<3;++c)for(unsigned lane=0;lane<4;++lane)
            material.forward_transforms[c][lane]=value(random);
        for(unsigned c=0;c<8;++c)material.program_constants[1][c]=material.forward_transforms[c];
        const auto model=pt::qualified_unlit_uv(material);require(bool(model),"Finite UV update lost native qualification");
        for(unsigned sample=0;sample<32;++sample) {
            Vertex vertex;vertex.position={.2f,-.3f,1};vertex.normal={0,0,1};
            vertex.tangent={1,0,0};vertex.binormal={0,1,0};vertex.incident={0,0,1};
            vertex.color={.3f,.5f,.7f};vertex.uv={uv(random),uv(random)};
            vertex.lightmap_uv={17.f+uv(random),23.f+uv(random)};
            VertexProgramState state;state.attributes=material.program_constants[0];
            if(material.vertex_declaration)state.attributes=source_world_vertex_inputs(
                *material.vertex_declaration,vertex,state.attributes,material.world_vertex_input_transform);
            else {
                state.attributes[0]={vertex.position.x,vertex.position.y,vertex.position.z,1};
                state.attributes[1]={vertex.uv.x,vertex.uv.y,0,1};state.attributes[2]={0,0,1,1};
                state.attributes[3]={1,0,0,1};state.attributes[4]={0,1,0,1};state.attributes[5]={0,0,1,1};
                state.attributes[6]=material.per_vertex_lighting?ShaderVector{.3f,.5f,.7f,1}:
                    ShaderVector{vertex.lightmap_uv.x,vertex.lightmap_uv.y,0,1};
            }
            for(unsigned c=0;c<4;++c)state.constants[c][c]=1;
            state.constants[4]={1,0,0,1};state.constants[5]={0,1,0,.5f};
            state.constants[6]={0,0,-1,2};state.constants[7]={0,0,1,767.8125f};
            state.constants[10]={32,-32,16777215,0};state.constants[11]={32.5f,32.5f,0,0};
            state.constants[172]={1,1,0,0};
            for(unsigned c=0;c<16;++c)state.constants[18+c]=material.program_constants[1][c];
            execute_vertex_program(program,state);
            if(profile==0 && sample==0) {
                auto tinted=state;auto color_vertex=vertex;color_vertex.color={.07f,.11f,.19f};
                if(material.vertex_declaration)tinted.attributes=source_world_vertex_inputs(*material.vertex_declaration,
                    color_vertex,material.program_constants[0],material.world_vertex_input_transform);
                else if(material.per_vertex_lighting)tinted.attributes[6]={.07f,.11f,.19f,1};
                auto attribute=state;attribute.attributes[10]={.13f,.29f,.61f,.83f};
                execute_vertex_program(program,tinted);execute_vertex_program(program,attribute);
                for(unsigned output=0;output<16;++output) {
                    if(tinted.outputs[output]!=state.outputs[output])color_output_masks[pass.vertex_shader]|=1u<<output;
                    if(attribute.outputs[output]!=state.outputs[output])attribute10_output_masks[pass.vertex_shader]|=1u<<output;
                }
                const unsigned stages=admitted->family==pt::UnlitUvFamily::Texture?4:3;
                for(unsigned stage=0;stage<stages;++stage)
                    require(tinted.outputs[9+stage]==state.outputs[9+stage] && attribute.outputs[9+stage]==state.outputs[9+stage],
                        "Source radiance or uniform attribute10 changed qualified unlit UV");
            }
            const auto expected=pt::unlit_uv(*model,vertex.uv);
            const unsigned stages=model->family==pt::UnlitUvFamily::Texture?4:3;
            for(unsigned stage=0;stage<stages;++stage) {
                const auto& actual=state.outputs[9+stage];
                require(actual[3]==1 && actual[0]==expected[stage].x && actual[1]==expected[stage].y,
                    "Unlit UV helper differs from exact native source program output");
            }
            if(model->family==pt::UnlitUvFamily::LightmapDetail && !material.per_vertex_lighting) {
                require(state.outputs[12][0]==vertex.lightmap_uv.x && state.outputs[12][1]==vertex.lightmap_uv.y,
                    "Source lightmap stream was accidentally interpreted as raw unlit UV");
            }
            const auto& a=model->base;const auto& b=model->stages[1];
            const float flattened_x=vertex.uv.x*(a[0]*b[0])+(a[2]*b[0]+b[2]);
            const float flattened_y=vertex.uv.y*(a[1]*b[1])+(a[3]*b[1]+b[3]);
            flattened_differences+=flattened_x!=expected[1].x || flattened_y!=expected[1].y;
            ++samples;
        }
    }
    material.forward_transforms=transforms;material.program_constants[1]=constants;
}
}
int main(int argc,char** argv)try {
    require(argc==5,"Expected original Lockout, prepared Lockout/Coagulation maps and report path");
    for(unsigned i=1;i<=3;++i)if(!std::filesystem::is_regular_file(argv[i]))return 77;
    std::mt19937 random(0x15c18u);nlohmann::json report{{"cohorts",nlohmann::json::array()}};
    for(unsigned i=1;i<=3;++i) {
        Cache cache(argv[i]);auto mesh=cache.load_bsp(0);const auto before=materials;
        for(auto& material:mesh.materials)verify(material,random);
        for(const auto& tag:cache.tags())if(tag.group=="shad" &&
            (tag.name.find("tree")!=std::string::npos || tag.name.find("pine")!=std::string::npos))
            for(bool per_vertex:{false,true}) {
                auto material=cache.load_material(tag.datum);
                if(prepare_world_material(cache,material,per_vertex))verify(material,random);
            }
        report["cohorts"].push_back({{"map",argv[i]},{"mcc",cache.is_mcc()},{"qualified_materials",materials-before}});
    }
    require(families.size()==3 && materials>0 && samples>0 && flattened_differences>0,
        "UV oracle did not cover all three native bodies and chained-transform negative control");
    const std::string texture="rasterizer\\vertex_shaders_xbox\\stencil_texture";
    const std::string pixel="rasterizer\\vertex_shaders_xbox\\lightmap_perpixel_tex_detail";
    const std::string vertex="rasterizer\\vertex_shaders_xbox\\lightmap_pervertex_tex_detail";
    require(color_output_masks[texture]==0 && color_output_masks[pixel]==0 &&
        color_output_masks[vertex]==(1u<<4) && color_registers[vertex]==std::set<unsigned>{6},
        "Source model radiance did not remain isolated to the per-vertex lighting output");
    require(attribute10_output_masks[texture]==(1u<<3) &&
        attribute10_output_masks[pixel]==0 && attribute10_output_masks[vertex]==0,
        "Source uniform attribute10 was confused with decoded model radiance");
    report.update({{"passed",true},{"checks",checks},{"qualified_materials",materials},{"vertex_samples",samples},
        {"flattened_affine_differences",flattened_differences},{"families",families},
        {"vertex_color_output_masks",color_output_masks},{"uniform_attribute10_output_masks",attribute10_output_masks},
        {"source_radiance_registers",color_registers},{"uniform_attribute10_extern_sources",attribute10_externs},
        {"scope","Exact native UV arithmetic only; no GPU or full pixel/material equivalence claim"}});
    std::ofstream(argv[4])<<report.dump(2)<<'\n';
    std::cout<<"PASS native unlit UV: materials="<<materials<<" samples="<<samples<<" checks="<<checks<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
