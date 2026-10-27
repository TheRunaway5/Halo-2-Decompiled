#pragma once
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_campaign_lighting_tu15.hpp"
#include "halo2/source_texture_camera_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <map>
#include <set>
#include <nlohmann/json.hpp>

namespace halo2::texture_camera_test {
using Json=nlohmann::json;
inline void require(bool ok,const char *why){if(!ok)throw FormatError(why);}
struct AuthoredAction {
    std::string function;
    std::vector<std::string> names;
    std::vector<std::uint32_t> literals;
    std::vector<std::uint32_t> source_nodes;
    std::vector<unsigned> object_indices;
    Json semantic;
};
// Inspect the real loaded Cache's projected script namespace. Original and
// prepared inputs are compared by authored names/types/FOV, while each actual
// native call retains that Cache's own object index and marker literal.
inline std::vector<AuthoredAction> authored_setup(const Cache &cache,const LevelContent &content) {
    const auto &program=content.scripts;
    const auto definition=std::find_if(program.scripts.begin(),program.scripts.end(),[](const auto &s) {
        return s.name=="x06_texture_camera_02_01";
    });
    require(definition!=program.scripts.end(),"Actual camera setup script64 semantic owner is absent");
    const auto node=[&](std::uint32_t datum)->const ScriptNode & {
        require((datum&65535u)<program.nodes.size(),"Authored camera syntax escapes the actual Cache bank");
        const auto &n=program.nodes[datum&65535u];
        require(n.salt==(datum>>16),"Authored camera syntax lost its full salted identity");return n;
    };
    const auto text=[&](unsigned offset) {
        require(offset<program.strings.size(),"Authored camera string escapes its actual Cache bank");
        const auto end=program.strings.find('\0',offset);
        require(end!=std::string::npos,"Authored camera string is unterminated");return program.strings.substr(offset,end-offset);
    };
    const auto children=[&](std::uint32_t datum) {
        const auto &parent=node(datum);require(!(parent.flags&3u),"Camera setup expression is not a direct authored builtin");
        const auto &fn=node(parent.value);require((fn.flags&1u)!=0 && fn.type==2,"Camera setup function-name node differs");
        std::vector<std::uint32_t> out;std::set<std::uint32_t> seen;
        for(auto child=fn.next;child!=0xffffffffu;child=node(child).next) {
            require(seen.insert(child).second && seen.size()<=8,"Camera setup sibling list changed/cycled");out.push_back(child);
        }
        return out;
    };
    const auto function=[&](std::uint32_t datum){return text(node(node(datum).value).string_offset);};
    require(function(definition->root)=="begin","Camera setup root is not the authenticated begin owner");
    const auto expressions=children(definition->root);
    const std::array<std::string_view,7> expected{"print","object_create_anew","object_create_anew","object_create_anew",
        "object_cinematic_lod","object_uses_cinematic_lighting","texture_camera_set_object_marker"};
    require(expressions.size()==expected.size(),"Camera setup complete expression count changed");
    std::vector<AuthoredAction> out;
    for(unsigned i=0;i<expressions.size();++i) {
        const auto datum=expressions[i];AuthoredAction action;action.function=function(datum);
        require(action.function==expected[i],"Camera setup source expression order/owner changed");
        action.source_nodes.push_back(datum);const auto args=children(datum);
        const auto arity=i<4?1u:i==6?3u:2u;require(args.size()==arity,"Camera setup actual argument count changed");
        Json semantic=Json::array();
        for(unsigned a=0;a<args.size();++a) {
            const auto &arg=node(args[a]);require((arg.flags&1u)!=0,"Camera setup argument is not an authored literal");
            action.source_nodes.push_back(args[a]);action.literals.push_back(arg.value);
            if(i==0) {
                require(arg.type==9,"Camera setup print literal type changed");
                semantic.push_back({{"type","string"},{"value",text(arg.string_offset)}});continue;
            }
            if(a==0) {
                require(arg.type==50 || arg.type==56,"Camera setup lost its actual object-name literal type");
                const auto index=arg.value&65535u;require(index<content.object_names.size(),"Camera object name escapes actual Cache namespace");
                const auto &name=content.object_names[index];require(name.type==6 && name.placement>=0,"Camera setup object isn't authored scenery");
                action.names.push_back(name.name);action.object_indices.push_back(index);
                semantic.push_back({{"type","object_name"},{"value",name.name}});continue;
            }
            if(i==6 && a==1) {
                require(arg.type==11,"Camera marker lost its actual string-ID literal type");
                const auto marker=cache.string_id(arg.value);require(marker=="primary_trigger" && text(arg.string_offset)==marker,
                    "Camera marker does not resolve in this Cache's own authored namespace");
                action.names.push_back(marker);semantic.push_back({{"type","marker_name"},{"value",marker}});continue;
            }
            if(i==6) {
                require(arg.type==6 && arg.value==std::bit_cast<std::uint32_t>(40.f),"Camera setup authored FOV changed");
                semantic.push_back({{"type","real_bits"},{"value",arg.value}});
            } else {
                require(arg.type==5 && (arg.value&255u)==1u,"Camera setup authored true literal changed");
                semantic.push_back({{"type","boolean"},{"value",true}});
            }
        }
        action.semantic={{"function",action.function},{"arguments",semantic}};out.push_back(std::move(action));
    }
    require(out[1].names.at(0)=="texture_camera" && out[3].names.at(0)=="texture_halo" &&
        out[4].names.at(0)=="texture_halo" && out[5].names.at(0)=="texture_halo" && out[6].names.at(0)=="texture_camera",
        "Camera setup named producer/light owner changed");return out;
}
// Fresh-world control: prove the three names are absent in the actual native
// object bitmap/full-salt rows before using the real scenario birth. This
// does not replace create_anew's destructive branch on an existing owner.
inline unsigned require_setup_names_absent(MultiplayerObjectRuntime &objects,
    const std::vector<AuthoredAction> &actions) {
    const auto &memory=objects.memory();
    const auto word=[&](unsigned p){return Bytes(memory.view(p,4)).u32(0);};
    const auto pool=word(0x4ecbd0u);require(pool!=0,"Camera fresh-world control lost its actual object pool");
    const auto capacity=word(pool+0x20u),high=word(pool+0x38u),rows=word(pool+0x44u),bits=word(pool+0x48u);
    require(capacity>0 && capacity<=65536u && high<=capacity && word(pool+0x24u)==12u,
        "Camera fresh-world control lost the original object pool layout");
    memory.view(rows,std::size_t(capacity)*12u);memory.view(bits,((capacity+31u)/32u)*4u);
    std::set<unsigned> names;
    for(const auto &a:actions)if(a.function=="object_create_anew")
        require(names.insert(a.object_indices.at(0)).second,"Authored setup repeats a named creation");
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    unsigned checked=0;
    for(unsigned i=0;i<high;++i) {
        if(!(word(bits+(i/32u)*4u)&(1u<<(i%32u))))continue;
        const auto row=rows+i*12u;const auto salt=Bytes(memory.view(row,2)).u16(0);
        const auto identity=(unsigned(salt)<<16u)|i;
        require(salt && source_raw_datum_lookup(pool,identity,storage)==row,
            "Camera fresh-world control lost a full live object identity");
        const auto body=word(row+8u);require(body!=0,"Live object has no source-created body");
        const auto name=Bytes(memory.view(body+0xacu,2)).u16(0);
        require(!names.contains(name),"create_anew setup requires these actual names absent in its fresh source world");++checked;
    }
    return checked;
}
struct SetupResult {std::uint32_t camera{},marker{},fov_bits{};std::map<std::string,std::uint32_t> owners;Json actions;};
inline SetupResult execute_setup(MultiplayerObjectRuntime &objects,const LevelContent &content,
    const std::vector<AuthoredAction> &actual,const std::vector<AuthoredAction> &original) {
    require(actual.size()==original.size(),"Prepared camera setup differs from complete original source");
    SetupResult result;auto &world=objects.tu15_world();
    const auto initially_live=require_setup_names_absent(objects,actual);
    result.actions.push_back({{"fresh_world_live_objects_checked",initially_live},{"three_names_absent",true}});
    for(unsigned i=0;i<actual.size();++i) {
        const auto &a=actual[i];require(a.semantic==original[i].semantic,"Prepared camera setup semantic inputs differ from original source");
        result.actions.push_back({{"semantic",a.semantic},{"actual_nodes",a.source_nodes},{"actual_literals",a.literals}});
        if(a.function=="print")continue;
        const auto &name=a.names.at(0);
        if(a.function=="object_create_anew") {
            require(!result.owners.contains(name),"Camera setup has duplicate creation within its fresh source world");
            const auto &named=content.object_names.at(a.object_indices.at(0));
            const auto placed=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &p) {
                return p.type==named.type && p.placement==named.placement;
            });
            require(placed!=content.objects.end() && placed->name==int(a.object_indices.at(0)),"Camera setup actual named placement does not match its source factory");
            const auto handle=objects.create_scenario_object(*placed,SourceScenarioBirth::runtime);
            require(handle!=null_datum,"Camera setup actual native scenario constructor returned NONE");
            result.owners.emplace(name,handle);continue;
        }
        require(result.owners.contains(name),"Camera setup uses an object before its actual source creation");
        const auto owner=result.owners.at(name);
        if(a.function=="object_cinematic_lod")source_campaign_cinematic_lod_tu15(world,owner,true);
        else if(a.function=="object_uses_cinematic_lighting")source_campaign_object_lighting_tu15(world,owner,true);
        else if(a.function=="texture_camera_set_object_marker") {
            result.camera=owner;result.marker=a.literals.at(1);result.fov_bits=a.literals.at(2);
            auto call=world.prepare_source_call(std::array{result.marker,result.fov_bits});call.gp[6]=owner;
            world.run_actual(0x255e0u,call);
            const auto published=source_texture_camera_inputs_tu15(world.memory());
            require(published.camera_words[0]==owner && published.camera_words[9]==result.marker &&
                published.camera_words[10]==result.fov_bits && published.copy_complete==0,
                "Native255E0 did not publish the actual created camera/marker/FOV/reset bank");
            const auto &memory=world.memory();const auto body=objects.object_body(owner);
            const auto name=Bytes(memory.view(body+0xacu,2)).u16(0);
            require(name==a.object_indices.at(0),"Created camera body lost its actual scenario name binding");
            const auto scenario=Bytes(memory.view(0x4ecc20u,4)).u32(0);
            const auto names=Bytes(memory.view(scenario+0x4cu,4)).u32(0);
            require(std::ranges::equal(memory.view(0x4c6194u,32u),memory.view(names+name*36u,32u)),
                "Native255E0 did not copy its exact current32-byte scenario name owner");
        } else throw FormatError("Unknown source camera setup action");
    }
    require(result.camera!=0 && result.camera!=null_datum,"Authored source camera setup did not create its real camera");return result;
}
}
