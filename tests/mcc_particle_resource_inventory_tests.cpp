#include "halo2/cache.hpp"
#include "halo2/particle_material.hpp"
#include "halo2/particle_material_validation.hpp"
#include "halo2/particle_model.hpp"
#include "halo2/particle_render_inputs.hpp"
#include "halo2/particle_resource_error.hpp"
#include "halo2/particle_draw_resource_validation.hpp"
#include "halo2/source_particle_sprite_context.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t none=0xffffffffu;
constexpr std::size_t report_budget=1024u*1024u;
void check(bool value,const char* message) {if(!value)throw FormatError(message);}
std::string failure_kind(std::string_view stage,std::string_view diagnostic) {
    if(diagnostic.find("compiled native fragment")!=std::string_view::npos)return "missing_compiled_fragment_variant";
    if(diagnostic.find("vertex shader is not original Xbox format")!=std::string_view::npos)
        return "missing_pc_shader_projection";
    if(diagnostic.find("program differs")!=std::string_view::npos ||
       diagnostic.find("vertex classification")!=std::string_view::npos)return "unsupported_vertex_contract";
    if(diagnostic.find("external")!=std::string_view::npos || diagnostic.find("framebuffer")!=std::string_view::npos)
        return "external_resource_contract";
    if(diagnostic.find("sampler")!=std::string_view::npos)return "unsupported_or_missing_sampler";
    if(diagnostic.find("bitmap")!=std::string_view::npos || diagnostic.find("texture")!=std::string_view::npos)
        return "unsupported_or_missing_bitmap_texture";
    return stage=="compiled_vulkan_contract"?"unsupported_render_contract":"unqualified_source_contract";
}
Bytes block(const Cache& cache,const Bytes& root,unsigned offset,unsigned stride) {
    const auto descriptor=read_tag_block_descriptor(root,offset);
    const auto extent=descriptor.extent(stride,65536);
    return Bytes(descriptor.count?cache.read_tag_bytes(descriptor.address,extent):std::span<const std::byte>{});
}
std::string template_name(const Cache& cache,const Tag& tag) {
    const auto payload=cache.load_source_tag_metadata(tag.datum);
    const Bytes root(payload.bytes_at(payload.source_address(),tag.group=="PRTM"?0xe0:0x94));
    std::uint32_t template_id{};
    if(tag.group=="PRTM") {
        const auto shader=cache.load_source_tag_metadata(root.u32(0x1c));
        const Bytes material(shader.bytes_at(shader.source_address(),0x28));
        if(!material.u32(0x20))return {};
        template_id=Bytes(cache.read_tag_bytes(material.u32(0x24),4)).u32(0);
    } else {
        if(!root.u32(0x8c))return {};
        template_id=Bytes(cache.read_tag_bytes(root.u32(0x90),4)).u32(0);
    }
    const auto* shader_template=cache.query_tag({.handle=TagHandle{template_id},
        .group=TagGroup::from_string("stem"),.require_address=true});
    return shader_template?shader_template->name:std::string{};
}
struct Inventory {
    Json report{{"schema","H2_MCC_PARTICLE_RESOURCE_INVENTORY_V1"},
        {"scope","Every resident sprite/model material and authored effect, breakable-surface and child-system reference; no simulation, current-frame feedback or framebuffer claim"},
        {"maps",Json::array()},{"resources",Json::array()},{"compiled_contracts",Json::array()},
        {"dependency_failures",Json::array()}};
    std::map<std::string,std::size_t> resource_keys,dependency_keys,catalog_keys;
    std::size_t rejected{},invalid_metadata{},dependency_failures{},resident{},unresident{},no_sprite{},feedback{},failed_maps{};
    void resource(Json value,unsigned map,std::uint32_t datum) {
        if(value.value("material_admitted",false)) {
            Json contract;
            for(const auto* key:{"group","shader_template","classification","program_hash",
                "fragment","source_vertex_tag_present","requires_scene_feedback"})
                if(value.contains(key))contract[key]=value.at(key);
            const auto identity=contract.dump();auto match=catalog_keys.find(identity);
            if(match==catalog_keys.end()) {
                const auto index=report["compiled_contracts"].size();contract["occurrences"]=0;
                report["compiled_contracts"].push_back(std::move(contract));
                match=catalog_keys.emplace(identity,index).first;
            }
            auto& count=report["compiled_contracts"][match->second]["occurrences"];
            count=count.get<unsigned>()+1;
        }
        const auto key=value.dump();auto found=resource_keys.find(key);
        if(found==resource_keys.end()) {
            const auto index=report["resources"].size();value["occurrences"]=Json::array();
            report["resources"].push_back(std::move(value));
            found=resource_keys.emplace(key,index).first;
        }
        report["resources"][found->second]["occurrences"].push_back({map,datum});
    }
    void dependency(const Tag& owner,unsigned map,std::string stage,std::string status,
        std::uint32_t target,std::string diagnostic) {
        ++dependency_failures;
        Json value{{"owner",owner.name},{"owner_group",std::string(owner.group)},
            {"stage",std::move(stage)},{"status",std::move(status)},
            {"diagnostic",std::move(diagnostic)}};
        const auto key=value.dump();auto found=dependency_keys.find(key);
        if(found==dependency_keys.end()) {
            const auto index=report["dependency_failures"].size();value["occurrences"]=Json::array();
            report["dependency_failures"].push_back(std::move(value));
            found=dependency_keys.emplace(key,index).first;
        }
        report["dependency_failures"][found->second]["occurrences"].push_back({map,owner.datum,target});
    }
    bool map(const std::filesystem::path& path) {
        Cache cache(path,path.parent_path());
        check(cache.is_mcc(),"Particle inventory requires actual prepared imported MCC content");
        const unsigned index=report["maps"].size();
        Json summary{{"map",cache.name()},{"file",path.filename().string()},
            {"resident_particles",0},{"authored_unresident_particles",0},
            {"admitted_resources",0},{"rejected_resources",0},{"invalid_metadata",0},
            {"no_sprite_data",0},{"requires_scene_feedback",0},
            {"effect_owners",0},{"particle_systems",0},{"effect_child_references",0},
            {"dependency_failures",0},{"authored_cycle_edges",0}};
        const auto bump=[&](const char* key){summary[key]=summary[key].get<unsigned>()+1;};
        bool invalid_handle_rejected=false;
        try {(void)cache.load_particle_material(none,15,0,0);}
        catch(const FormatError&) {invalid_handle_rejected=true;}
        check(invalid_handle_rejected,"Missing full particle handle was admitted by the resource loader");
        summary["missing_handle_negative_control"]=true;
        for(const auto& tag:cache.tags()) {
            if(tag.group!="prt3" && tag.group!="PRTM")continue;
            if(!tag.address) {++unresident;bump("authored_unresident_particles");continue;}
            ++resident;bump("resident_particles");
            Json value{{"name",tag.name},{"group",std::string(tag.group)},{"status","admitted"}};
            std::string stage="authored_owner";
            bool resource_rejected=false,metadata_invalid=false;
            const auto failure=[&](std::string status,const std::exception& error,bool invalid=false) {
                Json detail{{"status",status},{"stage",stage},{"diagnostic",error.what()},
                    {"failure_kind",invalid?"invalid_metadata":failure_kind(stage,error.what())}};
                if(!resource_rejected)for(const auto& [key,entry]:detail.items())value[key]=entry;
                value["stage_failures"].push_back(std::move(detail));
                resource_rejected=true;metadata_invalid|=invalid;
            };
            const auto attempt=[&](auto&& operation) {
                try {operation();}
                catch(const ParticleResourceError& error){failure("unsupported_or_missing_content",error);}
                catch(const FormatError& error){failure("source_contract_rejected",error);}
                catch(const std::out_of_range& error){failure("invalid_metadata",error,true);}
                catch(const std::invalid_argument& error){failure("invalid_metadata",error,true);}
            };
            attempt([&] {
                value["shader_template"]=template_name(cache,tag);
            });
            attempt([&] {
                if(tag.group=="PRTM") {
                    stage="model_resource";const auto model=load_particle_model(cache,tag.datum);
                    check(!model.vertices.empty() && !model.indices.empty(),"Particle model has no actual geometry");
                    stage="model_draw_contract";validate_particle_draw_resource(model);
                    value["model_vertices"]=model.vertices.size();value["model_indices"]=model.indices.size();
                } else {
                    stage="sprite_resource";const auto sprite=cache.load_particle_sprite(tag.datum);
                    stage="sprite_context";const auto context=source_particle_sprite_context(cache,tag.datum,sprite);
                    check(bool(context),"Particle sprite has no admitted bitmap data context");
                    stage="sprite_draw_contract";validate_particle_draw_resource(sprite,*context);
                    value["authored_bitmap"]=sprite.bitmap;
                    if(!context->has_bitmap) {++no_sprite;bump("no_sprite_data");value["emits_sprite"]=false;}
                    else value["emits_sprite"]=true;
                }
            });
            // A bad sprite/geometry contract must not hide a separate missing
            // compiled material family on the same imported resource.
            attempt([&] {
                stage="material_resource";
                const auto material=cache.load_particle_material(tag.datum,15,0,tag.group=="PRTM"?1:0);
                stage="compiled_vulkan_contract";const auto validation=validate_particle_material(material);
                value["classification"]=material.vertex.classification;
                value["program_hash"]=validation.program_hash;
                value["instructions"]=validation.instructions;
                value["fragment"]=validation.fragment_kernel;
                value["source_vertex_tag_present"]=material.vertex.vertex_datum!=none;
                value["requires_scene_feedback"]=validation.external_scene_texture;
                value["material_admitted"]=true;
                if(validation.external_scene_texture){++feedback;bump("requires_scene_feedback");}
            });
            if(resource_rejected){++rejected;bump("rejected_resources");}
            else bump("admitted_resources");
            if(metadata_invalid){++invalid_metadata;bump("invalid_metadata");}
            resource(std::move(value),index,tag.datum);
        }
        std::map<std::uint32_t,std::set<std::uint32_t>> graph;
        std::uint64_t graph_hash=14695981039346656037ull;
        const auto edge=[&](const Tag& owner,std::uint32_t target,std::string_view expected,std::string stage) {
            graph[owner.datum].insert(target);
            for(const auto word:{owner.datum,target})for(unsigned byte=0;byte<4;++byte)
                graph_hash=(graph_hash^((word>>(byte*8))&255u))*1099511628211ull;
            const auto* dependency_tag=cache.query_tag({.handle=TagHandle{target}});
            if(!dependency_tag || (!expected.empty() && dependency_tag->group!=expected) ||
               (expected.empty() && dependency_tag->group!="prt3" && dependency_tag->group!="PRTM")) {
                bump("dependency_failures");dependency(owner,index,std::move(stage),"missing_typed_dependency",target,
                    "Authored effect reference has no matching full-salt typed tag");
            } else if(!dependency_tag->address) {
                bump("dependency_failures");dependency(owner,index,std::move(stage),"authored_unresident_dependency",target,
                    "Authored effect references an unresident tag directory row");
            }
        };
        for(const auto& tag:cache.tags()) {
            if(!tag.address || (tag.group!="effe" && tag.group!="prt3" && tag.group!="PRTM" && tag.group!="bsdt"))continue;
            bump("effect_owners");
            std::string stage="effect_owner";
            try {
                const auto metadata=cache.load_source_tag_metadata(tag.datum);
                const Bytes root(metadata.bytes_at(metadata.source_address(),
                    tag.group=="prt3"?0x8c:tag.group=="PRTM"?0x80:0x1c));
                const auto systems=[&](unsigned event,const Bytes& rows) {
                    for(unsigned system=0;system<rows.size()/0x38;++system) {
                        bump("particle_systems");
                        stage="particle_system/"+(event==none?std::string("child"):std::to_string(event))+"/"+std::to_string(system);
                        const auto target=rows.u32(system*0x38+4);
                        if(target!=none)edge(tag,target,{},stage);
                        try {
                            const auto render=cache.load_particle_system_render_definition(tag.datum,event,system);
                            check(render.particle_datum==target,"Particle system renderer changed the authored full particle handle");
                            validate_particle_draw_resource(render);
                        } catch(const FormatError& error) {
                            bump("dependency_failures");dependency(tag,index,stage,"particle_system_contract_rejected",target,error.what());
                        } catch(const std::out_of_range& error) {
                            bump("dependency_failures");dependency(tag,index,stage,"invalid_particle_system_metadata",target,error.what());
                        } catch(const std::invalid_argument& error) {
                            bump("dependency_failures");dependency(tag,index,stage,"invalid_particle_system_metadata",target,error.what());
                        }
                    }
                };
                if(tag.group=="prt3")systems(none,block(cache,root,0x84,0x38));
                else if(tag.group=="PRTM")systems(none,block(cache,root,0x78,0x38));
                else if(tag.group=="bsdt")systems(none,block(cache,root,0x14,0x38));
                else {
                    const auto events=block(cache,root,0x14,0x38);
                    for(unsigned event=0;event<events.size()/0x38;++event) {
                        stage="effect_child_block/"+std::to_string(event);
                        try {
                            const auto parts=block(cache,events,event*0x38+0x18,0x38);
                            for(unsigned part=0;part<parts.size()/0x38;++part) {
                                if(parts.u32(part*0x38+8)!=tag_groups::effect.value)continue;
                                const auto child=parts.u32(part*0x38+0x10);if(child==none)continue;
                                bump("effect_child_references");
                                edge(tag,child,"effe","effect_child/"+std::to_string(event)+"/"+std::to_string(part));
                            }
                        } catch(const FormatError& error) {
                            bump("dependency_failures");dependency(tag,index,stage,"effect_child_contract_rejected",none,error.what());
                        }
                        stage="particle_system_block/"+std::to_string(event);
                        try {systems(event,block(cache,events,event*0x38+0x30,0x38));}
                        catch(const FormatError& error) {
                            bump("dependency_failures");dependency(tag,index,stage,"particle_system_block_rejected",none,error.what());
                        }
                    }
                }
            } catch(const FormatError& error) {
                bump("dependency_failures");dependency(tag,index,stage,"effect_contract_rejected",none,error.what());
            } catch(const std::out_of_range& error) {
                bump("dependency_failures");dependency(tag,index,stage,"invalid_effect_metadata",none,error.what());
            } catch(const std::invalid_argument& error) {
                bump("dependency_failures");dependency(tag,index,stage,"invalid_effect_metadata",none,error.what());
            }
        }
        // A cycle is a property of authored effect graphs, not proof of an
        // infinite source lifetime. Bound traversal to the full tag identities.
        std::map<std::uint32_t,unsigned> colors;
        for(const auto& [start,unused]:graph) {
            (void)unused;if(colors[start])continue;
            std::vector<std::pair<std::uint32_t,bool>> stack{{start,false}};
            while(!stack.empty()) {
                const auto [node,leave]=stack.back();stack.pop_back();
                if(leave){colors[node]=2;continue;}
                if(colors[node]==2)continue;
                if(colors[node]==1){bump("authored_cycle_edges");continue;}
                colors[node]=1;stack.emplace_back(node,true);
                if(const auto found=graph.find(node);found!=graph.end())
                    for(auto target:found->second) {
                        if(colors[target]==1)bump("authored_cycle_edges");
                        else if(!colors[target])stack.emplace_back(target,false);
                    }
            }
        }
        summary["dependency_graph_fnv64"]=graph_hash;
        report["maps"].push_back(std::move(summary));
        std::cout<<cache.name()<<": "<<report["maps"].back().dump()<<'\n'<<std::flush;
        return true;
    }
    bool save(const std::filesystem::path& path) {
        const bool passed=resident>0 && !rejected && !invalid_metadata && !dependency_failures && !failed_maps;
        report["inventory_complete"]=true;report["passed"]=passed;
        report["resident_resources"]=resident;report["authored_unresident_resources"]=unresident;
        report["rejected_resources"]=rejected;report["invalid_metadata"]=invalid_metadata;
        report["missing_or_unsupported_dependencies"]=dependency_failures;
        report["failed_map_initializations"]=failed_maps;
        report["authored_no_sprite_data"]=no_sprite;report["requires_current_scene_feedback"]=feedback;
        report["current_scene_feedback_validated"]=false;
        auto data=report.dump();
        if(data.size()>report_budget) {
            Json rejected_rows=Json::array();
            for(const auto& row:report["resources"])if(row.at("status")!="admitted")rejected_rows.push_back(row);
            report["admitted_resource_details_omitted_for_report_budget"]=report["resources"].size()-rejected_rows.size();
            report["resources"]=std::move(rejected_rows);data=report.dump();
        }
        check(data.size()<=report_budget,"Particle inventory exceeds the bounded retained report budget");
        std::ofstream output(path);check(bool(output),"Cannot save particle inventory report");
        output<<data<<'\n';output.close();check(bool(output),"Cannot finish particle inventory report");
        return passed;
    }
};
}
int main(int argc,char** argv)try {
    check(argc==3 || (argc==4 && std::string_view(argv[3])=="--inventory-only"),
        "Supply prepared MCC map or maps directory, owned JSON report, optional --inventory-only");
    check(std::getenv("HALO2_TEST_SCRATCH"),"Particle inventory requires its owned test scratch");
    const std::filesystem::path source=argv[1];
    if(source.empty() || !std::filesystem::exists(source))return 77;
    std::vector<std::filesystem::path> maps;
    if(std::filesystem::is_directory(source)) {
        for(const auto& entry:std::filesystem::directory_iterator(source)) {
            const auto name=entry.path().stem().string();
            if(entry.is_regular_file() && entry.path().extension()==".map" &&
               name!="mainmenu" && name!="shared" && name!="single_player_shared")maps.push_back(entry.path());
        }
        std::sort(maps.begin(),maps.end());
    } else maps.push_back(source);
    check(!maps.empty(),"Particle inventory has no actual MCC map input");
    Inventory inventory;
    for(const auto& map:maps)try {inventory.map(map);}
    catch(const FormatError& error) {
        ++inventory.failed_maps;
        inventory.report["maps"].push_back({{"file",map.filename().string()},
            {"status","map_initialization_rejected"},{"diagnostic",error.what()}});
        std::cerr<<map.filename()<<": "<<error.what()<<'\n';
    }
    const bool passed=inventory.save(argv[2]);
    std::cout<<(passed?"PASS":"CONTENT GAPS")<<" particle inventory: "<<maps.size()<<" maps, "
        <<inventory.resident<<" resident resources, "<<inventory.rejected<<" resource rejections, "
        <<inventory.dependency_failures<<" dependency failures\n";
    return passed || (argc==4 && !inventory.failed_maps)?0:1;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
