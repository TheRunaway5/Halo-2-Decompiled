#include "halo2/source_shield_model_shaders.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/content.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
template<class F>void rejects(F &&f){bool caught=false;try{f();}catch(const FormatError&){caught=true;}check(caught,"Unproved override bank admitted");}
void original(const std::filesystem::path &path) {
    std::ifstream file(path);check(bool(file),"Original override fixture missing");unsigned count=0;
    for(std::string line;std::getline(file,line);) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;unsigned type,shader,selected,original_count,override_count,expected;
        check(bool(row>>type>>shader>>selected>>original_count>>override_count),"Truncated override input");
        check(original_count<32&&override_count>0&&override_count<32,"Invalid override fixture bank");
        std::vector<std::uint32_t> original(original_count),overridden(override_count);
        for(auto &mask:original)check(bool(row>>mask),"Truncated original shader masks");
        for(auto &mask:overridden)check(bool(row>>mask),"Truncated overridden shader masks");
        check(bool(row>>expected),"Truncated original admission output");std::string extra;check(!(row>>extra),"Trailing override fixture input");
        check(source_model_shader_override_part(std::uint16_t(type),shader,std::int32_t(selected),original,overridden)==bool(expected),
              "Original41040 override admission differs");++count;
    }
    check(count>1440,"Actual override part contexts missing");
    const std::array<std::uint32_t,1> mask{0x8000};
    rejects([&]{source_model_shader_override_part(1,2,-1,mask,mask);});
    rejects([&]{source_model_shader_override_part(1,2,1,mask,mask);});
    rejects([&]{source_model_shader_override_part(1,2,0,{},mask);});
    rejects([&]{source_model_shader_override_part(1,2,0,mask,{});});
    std::cout<<count<<" unchanged-original override part admission cases\n";
}
void models(const Cache &cache) {
    const auto content=cache.load_content(0,true);unsigned models=0,parts=0,vertices=0,views=0,geometry_banks=0;
    for(unsigned representation:{2u,3u})for(bool first_person:{false,true}) {
        const auto &r=content.player_representations.at(representation);
        const auto header=cache.load_source_tag_metadata(r.biped.model);
        const auto mode=first_person?r.hands:Bytes(header.bytes_at(header.source_address(),8)).u32(4);
        auto model=cache.load_model(mode,false);const auto original_materials=model.mesh.materials.size();
        std::vector<Vertex> posed;
        if(first_person) {posed=pose_model(model);++geometry_banks;}
        else {
            const auto definition=cache.load_initial_model_definition(r.biped.datum);
            model.mesh.vertices.clear();model.vertex_origins.clear();
            for(unsigned lod=0;lod<6;++lod) {
                std::vector<unsigned> sections;
                for(const auto& region:definition.render_sections)for(const auto& permutation:region)
                    if(permutation[lod]>=0 && std::find(sections.begin(),sections.end(),unsigned(permutation[lod]))==sections.end())
                        sections.push_back(unsigned(permutation[lod]));
                const auto bank=cache.load_model_sections(r.biped.datum,false,0,lod,sections);
                model.mesh.vertices.insert(model.mesh.vertices.end(),bank.mesh.vertices.begin(),bank.mesh.vertices.end());
                model.vertex_origins.insert(model.vertex_origins.end(),bank.vertex_origins.begin(),bank.vertex_origins.end());
                const auto actual=pose_model(bank);posed.insert(posed.end(),actual.begin(),actual.end());++geometry_banks;
            }
        }
        const auto initial=model.mesh.vertices;
        const auto shader=first_person?r.biped.vitality.shield_damaged_first_person_shader:r.biped.vitality.shield_damaged_shader;
        SourceShieldModelShaders resident(cache,model,mode,shader);
        check(model.mesh.vertices.size()==initial.size()&&std::memcmp(model.mesh.vertices.data(),initial.data(),initial.size()*sizeof(Vertex))==0,
              "Shield resident bank changed source geometry");
        check(model.mesh.materials.size()>original_materials,"Shield override allocated no resident banks");
        std::vector<unsigned> sections;
        for(const auto &origin:model.vertex_origins)
            if(std::find(sections.begin(),sections.end(),origin.section)==sections.end())sections.push_back(origin.section);
        const auto authored=cache.load_source_transparent_model_parts(mode,sections);
        for(unsigned players:{1u,2u,4u})for(unsigned byte:{0u,1u,127u,255u}) {
            SourceModelShaderView view;view.shader=source_shader_lod_view(0,480,false,false);
            view.part_shader_multiplier=source_shader_lod_player_multiplier(players);
            const SourceModelShaderCollector collector{10000,std::uint8_t(byte)};
            auto selected=posed;resident.select(selected,collector,view);unsigned expected_vertices=0;
            for(const auto &section:authored.sections)for(const auto &part:section.parts) {
                const auto begin=std::find_if(posed.begin(),posed.end(),[&](const auto &v){return v.source_section==section.section&&v.source_part==part.ordinal;});
                if(begin==posed.end())continue;
                const bool admitted=part.type!=0&&part.type!=5&&part.shader!=0xffffffffu;
                const auto binding=resident.binding(section.section,part.ordinal,collector,view);
                check(bool(binding)==admitted,"Shield override changed actual part admission");
                if(binding) {
                    const auto &material=model.mesh.materials.at(binding->material);
                    check(binding->shader==shader&&binding->selected_lod==0&&binding->selected_template_layers==0x8000&&
                          binding->shader_priority_flags==0&&binding->shader_command_flags==cache.load_source_transparent_shader(shader).shader_command_flags,
                          "Shield override lost chosen shader metadata");
                    check(binding->material>=original_materials&&material.source_shader_datum==shader&&material.forward_pass_index==0&&material.forward_pass&&
                          material.lods.size()==1&&material.lods.front().available_layers==0x8000,
                          "Shield part did not select its resident plasma material");
                    expected_vertices+=unsigned(std::count_if(posed.begin(),posed.end(),[&](const auto &v){return v.source_section==section.section&&v.source_part==part.ordinal;}));
                }
                ++parts;
            }
            check(selected.size()==expected_vertices,"Shield override retained rejected geometry");
            unsigned cursor=0;
            for(const auto &v:posed) {
                const auto binding=resident.binding(v.source_section,v.source_part,collector,view);
                if(!binding)continue;
                auto expected=v;expected.material=binding->material;
                check(cursor<selected.size()&&std::memcmp(&expected,&selected[cursor],sizeof(Vertex))==0,
                      "Shield override changed posed geometry, ordering or local noise position");++cursor;++vertices;
            }
            ++views;
        }
        ++models;
    }
    check(models==4&&geometry_banks==14&&vertices>1000&&parts>0,"Incomplete actual shield resident coverage");
    std::cout<<models<<" actual body/arms models across "<<geometry_banks<<" authored geometry banks, "<<views<<" independent view selections, "<<parts<<" part checks and "<<vertices<<" unchanged posed vertices\n";
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply original override fixture");
        const auto map=std::filesystem::path(HALO2_SOURCE_DIR)/"assets/disc/maps/lockout.map";
        if(!std::filesystem::exists(map)||!std::filesystem::exists(argv[1]))return 77;
        original(argv[1]);Cache cache(map);models(cache);return 0;
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
