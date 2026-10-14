#include "capture_live_model_common.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/content.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
using Json=nlohmann::json;
namespace fs=std::filesystem;
namespace {
void require(bool b,const char* message){if(!b)throw FormatError(message);}
void dump(PhysicsLiveWorldTu15& world,const fs::path& path) {
    std::ofstream out(path,std::ios::binary);
    world.memory().visit_committed([&](auto address,auto bytes) {
        const auto count=std::uint32_t(bytes.size());
        out.write(reinterpret_cast<const char*>(&address),4);
        out.write(reinterpret_cast<const char*>(&count),4);
        out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    });
    require(bool(out),"live model cull bank export failed");
}
Json state(const SourceDrawUnitState& units) {
    Json result;
    for(const auto& row:units.units)result["units"].push_back(row.words);
    result["arena"]=units.arena.words;result["arena_used"]=units.arena.used;
    return result;
}
}
int main(int argc,char**argv)try {
    require(argc==5,"Supply TU15_XBE MAP MODEL_NAME_FRAGMENT NEW_OUTPUT");
    const fs::path out=argv[4];require(!fs::exists(out),"cull output already exists");fs::create_directories(out);
    Cache cache(argv[2]);
    const auto model=std::ranges::find_if(cache.tags(),[&](const auto& tag) {
        return tag.group=="mode" && tag.name.find(argv[3])!=std::string::npos;
    });
    require(model!=cache.tags().end(),"requested authored model absent");
    PhysicsLiveWorldTu15 world(cache,argv[1],out/"savegame");
    world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
    world.preload_render_model(model->datum);world.initialize_map();
    const auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
    const auto selected=original_model_placement(world,model->datum);
    const auto object=world.create_scenario_object(selected);
    require(object!=null_datum,"original scenario constructor declined the selected placement");
    (void)world.object_body(object);
    const auto body=word(word(word(0x4ecbd0u)+0x44u)+(object&65535u)*12u+8u);
    const Bytes b(world.memory().view(body+0x30u,16));const Vec3 center{b.f32(0),b.f32(4),b.f32(8)};
    const float radius=b.f32(12);require(radius>0,"created owner has no positive sphere");
    SourcePortalVisibilitySet visibility;
    const auto cluster_count=cache.load_source_bsp_admission(0).clusters.size();std::vector<std::uint8_t> fog(cluster_count);
    for(unsigned n=0;n<cluster_count;++n){SourceClusterFrustumRanges r;r.source_cluster=std::int16_t(n);visibility.clusters.push_back(r);}
    SourceLiveModelDrawContextTu15 view;view.entry.mode=view.draw.draw.mode=1;view.entry.distance_scale=1;
    view.entry.coverage_multiplier=source_geometry_lod_policy(1).coverage_multiplier;
    view.entry.eye={center.x-radius*3,center.y,center.z};view.forward={1,0,0};
    SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;
    projection.outer=projection.safe={0,0,480,640};
    view.draw.rendering=source_view_projection({view.entry.eye,view.forward,{0,0,1},.015625f,1024.f,{},source_frustum_projection(projection).projection_program});
    view.entry.coverage=source_shader_coverage_camera(view.draw.rendering);view.draw.eye=view.entry.eye;
    view.draw.viewport=projection.outer;view.draw.draw.mask_arena_address=0x71001000u;
    world.begin_model_frame();unsigned pose_reads=0;Json results=Json::array();
    std::map<std::pair<std::uint32_t,unsigned>,SourceBspRangeGeometry> ranges;
    for(unsigned ordinal=0;ordinal<6;++ordinal) {
        SourceObjectAdmissionInput input;input.mode=input.context_2a78=1;input.containing_fog=fog;
        input.sphere_center=center;input.sphere_radius=radius*std::array{.02f,.2f,2.f}[ordinal%3];
        auto admission=world.admit_render_objects(visibility,input);SourceDrawUnitState units;
        source_live_model_draws_tu15(world,admission,units,view);
        std::array<SourceCullDrawBanks,850> banks;
        for(unsigned n=units.main_prefix;n<units.draw_count;++n) {
            auto& unit=units.units[n];if(ordinal<3 && ((unit.words[0]&1u)||!(unit.words[0]&0x40u)))continue;
            const auto key=std::pair{unit.words[1],unit.words[3]>>9&511u};
            auto at=ranges.find(key);if(at==ranges.end())at=ranges.emplace(key,load_source_model_range_geometry(cache,key.first,key.second)).first;
            if(ordinal>=3) {
                const auto mask=units.arena.allocate(at->second.subpart_count);if(!mask)continue;
                unit.words[0]=(unit.words[0]&~1u)|0x40u;unit.words[5]=view.draw.draw.mask_arena_address+*mask*4u;
            }
            banks[n]=source_live_model_cull_geometry_tu15(world,unit,at->second);
            if(banks[n].geometry && banks[n].geometry->resolve_owner_pose) {
                auto resolve=banks[n].geometry->resolve_owner_pose;
                banks[n].geometry->resolve_owner_pose=[&,resolve]{++pose_reads;return resolve();};
            }
        }
        Json result;result["before"]=state(units);result["declared_subpart_request"]=ordinal>=3;
        result["prefix"]=units.main_prefix;result["total"]=units.draw_count;
        result["center"]={std::bit_cast<std::uint32_t>(input.sphere_center.x),std::bit_cast<std::uint32_t>(input.sphere_center.y),std::bit_cast<std::uint32_t>(input.sphere_center.z)};
        result["radius"]=std::bit_cast<std::uint32_t>(input.sphere_radius);result["arena_address"]=view.draw.draw.mask_arena_address;
        const auto stem="cull-"+std::to_string(ordinal);dump(world,out/(stem+"-inputs.bin"));
        const auto before_reads=pose_reads;
        const auto calls=source_draw_unit_sphere_cull(units.units,banks,units.arena,view.draw.draw.mask_arena_address,
            {units.main_prefix,units.draw_count,1,1,input.sphere_center,input.sphere_radius});
        result["after"]=state(units);result["calls"]=Json::array();
        for(const auto& call:calls)result["calls"].push_back({call.unit,std::bit_cast<std::uint32_t>(call.center.x),std::bit_cast<std::uint32_t>(call.center.y),std::bit_cast<std::uint32_t>(call.center.z),std::bit_cast<std::uint32_t>(call.radius)});
        std::ofstream(out/(stem+".json"))<<result.dump(2)<<'\n';
        results.push_back({{"case",ordinal},{"units",units.draw_count},{"calls",calls.size()},{"pose_resolutions",pose_reads-before_reads}});
    }
    std::ofstream(out/"native.json")<<Json{{"scope","Fresh native original-placement owner; declared camera/spheres, three explicit subpart controls; CPU evidence only"},{"model",model->name},{"object",object},{"pose_resolutions",pose_reads},{"cases",results}}.dump(2)<<'\n';
    require(pose_reads>0,"selected original asset never consumed a posed bound");
    std::cout<<"Live original-placement cull: "<<model->name<<" / "<<pose_reads<<" pose resolutions\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
