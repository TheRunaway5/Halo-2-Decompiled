#include "halo2/source_campaign_model.hpp"
#include "halo2/content.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/mcc_sound_class_reference.hpp"
#include "halo2/world_material.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {
Json armory_prop_cases(const std::filesystem::path &maps,unsigned &failures) {
    Cache cache(maps/"01a_tutorial.map");
    const auto content=cache.load_content(0,false);
    Json cases=Json::array();
    std::optional<std::uint32_t> scenery_cart_model,prop_cart_model;
    for(const auto &[type,name]:std::array<std::pair<int,const char*>,4>{{
        {11,"objects\\gear\\human\\military\\case\\case"},
        {11,"objects\\gear\\human\\military\\drum_12gal\\drum_12gal"},
        {11,"objects\\cinematics\\human\\equipment_cart\\equipment_cart"},
        {6,"objects\\cinematics\\human\\equipment_cart\\equipment_cart"}}}) {
        Json row={{"name",name},{"type",type},{"passed",false}};
        try {
            const auto tag=cache.query_tag({.group=TagGroup{type==11?"bloc":"scen"},.name=name,.require_address=true});
            if(!tag)throw FormatError("Actual Armory prop definition missing");
            const auto placement=std::ranges::find_if(content.objects,[&](const auto &p){return p.type==type && p.definition==tag->datum;});
            if(placement==content.objects.end())throw FormatError("Actual Armory prop placement missing");
            const auto initial=cache.load_initial_model_definition(tag->datum);
            row["definition"]=tag->datum;row["render_model"]=initial.render_model;
            row["placement"]=placement->placement;
            if(std::string_view(name).ends_with("equipment_cart"))
                (type==6?scenery_cart_model:prop_cart_model)=initial.render_model;
            auto preview=cache.load_model(tag->datum,true,placement->variant);
            row["caller_admitted"]=source_campaign_lit_scenery(preview,*placement);
            unsigned lit_materials=0,independent_refusals=0;
            for(auto material:preview.mesh.materials) {
                if(material.lods.empty())continue; // Reserved/non-shader material slot.
                const auto layers=material.lods.at(material.selected_shader_lod).available_layers;
                if(layers&((1u<<3)|(1u<<5))) {
                    ++lit_materials;
                    if(!prepare_independent_object_material(cache,material))++independent_refusals;
                }
            }
            row["authored_lit_materials"]=lit_materials;
            row["independent_fallback_refusals"]=independent_refusals;
            SourceCampaignModel model(cache,*placement);Mesh resources;
            // Only scenery has authored per-instance baked color streams.
            model.prepare_materials(cache,resources,type==6);
            unsigned dynamic_passes=0;
            row["prepared_shaders"]=Json::array();
            for(const auto &material:resources.materials) {
                bool dynamic=false;
                for(const Material *pass:std::array<const Material*,2>{&material,material.environment_pass.get()})if(pass && pass->forward_pass && pass->forward_pass_index<pass->passes.size())
                    dynamic|=pass->passes[pass->forward_pass_index].implementation==2;
                if(dynamic)++dynamic_passes;
                row["prepared_shaders"].push_back({{"name",material.name},{"dynamic_implementation2",dynamic}});
            }
            row["dynamic_implementation2_materials"]=dynamic_passes;
            if(!lit_materials || independent_refusals!=lit_materials || !dynamic_passes)
                throw FormatError("Actual Armory model no longer proves authored dynamic lighting versus independent preview");
            if(!row["caller_admitted"].get<bool>())
                throw FormatError("Actual lit Armory BLOC is routed to independent preview instead of its native model lighting units");
            row["passed"]=true;
        } catch(const std::exception &e) {row["error"]=e.what();++failures;}
        std::cout<<row.dump()<<'\n';cases.push_back(std::move(row));
    }
    if(!scenery_cart_model || scenery_cart_model!=prop_cart_model)
        throw FormatError("SCEN/BLOC equipment-cart positive control no longer shares the same authored model");
    return cases;
}
}
int main(int argc,char**argv)try {
    if(argc<3 || argc>4 || (argc==4 && std::string_view(argv[3])!="--armory-props-only"))
        throw FormatError("Supply prepared MCC maps, output report and optional --armory-props-only");
    const bool armory_only=argc==4;
    if(armory_only) {
        const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
        if(!scratch || !std::filesystem::path(scratch).is_absolute())
            throw FormatError("Armory material regression requires run_test.py scratch");
#if defined(_WIN32)
        if(_putenv_s("XDG_CACHE_HOME",scratch))throw FormatError("Could not bind owned cache snapshots");
#else
        if(setenv("XDG_CACHE_HOME",scratch,1))throw FormatError("Could not bind owned cache snapshots");
#endif
        unsigned failures=0;
        Json report={{"scope","Actual MCC Armory resource/caller admission; live native class, lighting banks and visible pixels require separate proof"}};
        report["cases"]=armory_prop_cases(argv[1],failures);
        report["failures"]=failures;report["passed"]=failures==0;
        std::ofstream(argv[2])<<report.dump(2)<<'\n';
        return failures?1:0;
    }
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
