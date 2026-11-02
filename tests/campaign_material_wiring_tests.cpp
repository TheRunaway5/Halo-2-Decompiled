#include "halo2/source_campaign_model.hpp"
#include "halo2/content.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/mcc_sound_class_reference.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <bit>
#include <iostream>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char**argv)try {
    if(argc!=3)throw FormatError("Supply prepared MCC maps and output report");
    if(!std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"01b_spacestation.map"))return 77;
    Json report={{"cases",Json::array()}};unsigned failures=0,checked=0,sound_namespaces=0;
    const std::set<std::string> leaves{"main_forward_view","scarab_leg_shell","scarab_big_turrets",
        "ch_flat_generic_graytech","texture_camera","forerunner_rusted_blue","holo_tex_cam","strip_b"};
    for(const auto*name:{"01b_spacestation","03a_oldmombasa","03b_newmombasa","04b_floodlab",
        "05a_deltaapproach","05b_deltatowers","06b_floodzone","07a_highcharity","07b_forerunnership"}) {
        Cache cache(std::filesystem::path(argv[1])/(std::string(name)+".map"));
        const auto*globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
        if(!globals)throw FormatError("Campaign material publication lost globals");
        const auto route=Bytes(cache.read_tag_bytes(globals->address+0xc0,8)).u32(4);
        const TagHandle legacy{Bytes(cache.read_tag_bytes(route,44)).u32(12)};
        const auto*classes=cache.find_tag(legacy,TagGroup{"sncl"});
        if(!classes)throw FormatError("Campaign material publication lost Legacy sound classes");
        const auto rows=Bytes(cache.read_tag_bytes(classes->address,8)).u32(4);
        const auto candidate=project_mcc_sound_class_records(cache.read_tag_bytes(rows,54*120));
        MccSoundRuntimeSource source;source.tags=cache.tags();
        source.read=[&](auto address,auto size){return cache.read_tag_bytes(address,size);};
        const auto sound=project_mcc_sound_class_reference(source,legacy,candidate);
        if(sound.status!=MccSoundClassReferenceStatus::Converted && sound.status!=MccSoundClassReferenceStatus::AlreadyEquivalent)
            throw FormatError(std::string(name)+": camera material publication changed the authenticated sound namespace");
        ++sound_namespaces;
        const auto content=cache.load_content(0,false);std::set<std::uint32_t> visited;
        for(const auto&object:content.objects) {
            if(object.definition==null_datum || !visited.insert(object.definition).second)continue;
            std::set<std::string> selected;
            try {
                const auto initial=cache.load_initial_model_definition(object.definition);
                if(initial.render_model==null_datum)continue;
                const Bytes model(cache.tag_payload({initial.render_model}));
                const auto count=model.u32(0x60);if(count>65536)throw FormatError("Excessive model materials");
                if(!count)continue;
                const Bytes rows(cache.read_tag_bytes(model.u32(0x64),count*32u));
                for(unsigned n=0;n<count;++n)if(const auto*t=cache.find_tag({rows.u32(n*32+12)})) {
                    if(leaves.contains(t->name.substr(t->name.find_last_of('\\')+1)))selected.insert(t->name);
                }
            }catch(const FormatError&){continue;}
            if(selected.empty())continue;
            Json row={{"map",name},{"definition",object.definition},{"shaders",selected}};++checked;
            try {
                SourceCampaignModel model(cache,object);Mesh resources;resources.materials.resize(3);model.prepare_materials(cache,resources);
                bool needs_prt=false;
                for(const auto&shader:selected) {
                    bool found=false;
                    for(const auto&m:resources.materials)if(m.name==shader) {
                        if(!m.forward_pass || !m.world_pass)throw FormatError("Authored material has no resident pass: "+shader);
                        if(m.template_name=="shaders\\shader_templates\\texture_camera\\texture_camera" && !m.texture_camera_texture)
                            throw FormatError("Camera material has no live target binding: "+shader);
                        if(m.lods.at(m.selected_shader_lod).available_layers&(1u<<5)) {
                            if(!m.environment_pass || !m.environment_pass->vertex_bank || !m.environment_pass->vertex_bank->prt_inputs)
                                throw FormatError("PRT material has no coefficient/stream declaration: "+shader);
                            if(!m.environment_pass->requires_owner_prt_bank)
                                throw FormatError("PRT declaration can be drawn without its actual owner upload");
                            needs_prt=true;
                        }
                        found=true;
                    }
                    if(!found)throw FormatError("Prepared model lost its authored shader: "+shader);
                }
                if(needs_prt) {
                    const auto initial=cache.load_initial_model_definition(object.definition);
                    std::set<unsigned> sections;for(const auto&origin:model.hierarchy().vertex_origins)sections.insert(origin.section);
                    unsigned banks=0,rejected=0;
                    for(auto lighting_class:{2u,3u})for(auto section:sections) {
                        SourceLiveModelBaseDrawTu15 draw;draw.object=0x12340001;draw.definition=object.definition;
                        draw.unit.words[1]=initial.render_model;draw.unit.words[3]=section<<9;
                        draw.lighting_class=lighting_class;draw.override_slot=255;draw.geometry.geometry.resident=true;
                        try{(void)model.prt_banks(draw.object,std::span(&draw,1));}
                        catch(const FormatError&){++rejected;}
                        auto bank=std::make_shared<VertexProgramBank>();bank->first_constant=68;bank->prt_inputs=true;
                        bank->constants.resize(2+std::min<std::size_t>(2,cache.source_prt_section_nodes(initial.render_model,section).size())*40);
                        bank->constants[0]={1,2,3,0};draw.prt_bank=bank;
                        for(const auto&binding:model.prt_banks(draw.object,std::span(&draw,1))) {
                            if(binding.section!=section || binding.bank!=bank || binding.material>=resources.materials.size() ||
                               !resources.materials[binding.material].environment_pass->requires_owner_prt_bank ||
                               resources.materials[binding.material].environment_pass->vertex_bank->constants.size()!=bank->constants.size())
                                throw FormatError("PRT section/owner/namespace binding differs from its resident declaration");
                            ++banks;
                        }
                    }
                    if(!banks || !rejected)throw FormatError("PRT positive/absent-owner controls were not exercised");
                    row["prt_owner_bindings"]=banks;row["missing_owner_controls"]=rejected;
                }
                // Paired MCC source tuple, independent of the original main
                // camera surface. Its panel UV parameters must survive the
                // projection; borrowing main_forward_view's constants fails.
                for(const auto&m:resources.materials)if(m.name.ends_with("\\miranda_viewscreen")) {
                    const auto*tag=cache.find_tag({m.source_shader_datum});
                    if(!tag || !cache.is_preserved_native_shader(*tag) || m.vertex_constants.size()!=1)
                        throw FormatError("MCC-only camera surface lost its own admitted parameter bank");
                    constexpr std::array<std::uint32_t,4> expected{0x41600000u,0x41000000u,0x3ee66666u,0x400ccccdu};
                    for(unsigned n=0;n<4;++n)if(std::bit_cast<std::uint32_t>(m.vertex_constants[0][n])!=expected[n])
                        throw FormatError("MCC-only camera projection changed an authored UV parameter");
                }
                row["passed"]=true;row["resident_materials"]=resources.materials.size();
            }catch(const std::exception&e){row["passed"]=false;row["error"]=e.what();++failures;}
            report["cases"].push_back(row);std::cout<<row.dump()<<'\n';
        }
    }
    report["sound_namespace_checks"]=sound_namespaces;
    report["checked"]=checked;report["expected"]=45;report["failures"]=failures;report["passed"]=checked==45 && !failures && sound_namespaces==9;
    std::ofstream(argv[2])<<report.dump(2)<<'\n';
    if(!report["passed"].get<bool>())throw FormatError("Campaign material preparation regression: "+std::to_string(failures)+" failures / "+std::to_string(checked)+" definitions");
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
