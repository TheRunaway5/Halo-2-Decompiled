#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/original_data.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include "halo2/player_command.hpp"
#include "halo2/posed_collision.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/multiplayer_world.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include "helpers/live_native_physics_frame_capture_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <unistd.h>

// Birth placement and delivered commands are the natural-route stimuli.
// Explicit diagnostic position inputs use the original pose owner. Survey
// geometry never resolves a live contact or supplies a body velocity.
namespace {
using namespace halo2;
using Json=nlohmann::json;
void require(bool yes,const char *why){if(!yes)throw FormatError(why);}
Json xyz(Vec3 v){return Json::array({v.x,v.y,v.z});}
Vec3 vec(const Json &j){return {j.at(0).get<float>(),j.at(1).get<float>(),j.at(2).get<float>()};}
struct Directory {
    std::filesystem::path path;
    Directory(){auto name=(std::filesystem::temp_directory_path()/"halo2-superbounce-XXXXXX").string();
        require(::mkdtemp(name.data()),"Cannot create owned superbounce storage");path=name;}
    ~Directory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t tick{};mutable std::atomic<std::uint64_t> counter{};
    std::uint32_t milliseconds()const noexcept override{return 1000u+tick*1000u/30u;}
    std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
void save(const std::filesystem::path &path,const Json &j){
    std::ofstream out(path);out<<j.dump(2)<<'\n';require(bool(out),"Cannot write superbounce report");
}
Json survey(Cache &cache){
    const auto content=cache.load_content(0,true);CollisionWorld geometry(content.collision);
    Json rows=Json::array(),clearances=Json::array();
    for(std::size_t i=0;i<content.collision.size();++i){
        const auto &t=content.collision[i];if(t.source_normal.z<.5f)continue;
        const auto center=(t.a+t.b+t.c)*(1.f/3.f);
        auto ceiling=geometry.raycast(center+Vec3{0,0,.05f},{0,0,1},content.player.height*2);
        rows.push_back({{"triangle",i},{"vertices",{xyz(t.a),xyz(t.b),xyz(t.c)}},
            {"normal",xyz(t.source_normal)},
            {"surface",{t.source.bsp,t.source.record,t.source.surface,t.source.instance,t.source.plane}},
            {"ceiling",ceiling?Json(ceiling->distance+.05f):Json(nullptr)}});
        if(t.source_normal.z>.98f)for(const auto &weights:std::array<std::array<float,3>,7>{{
            {1.f/3,1.f/3,1.f/3},{.8f,.1f,.1f},{.1f,.8f,.1f},{.1f,.1f,.8f},
            {.45f,.45f,.1f},{.45f,.1f,.45f},{.1f,.45f,.45f}}}){
            const auto point=t.a*weights[0]+t.b*weights[1]+t.c*weights[2];
            const auto roof=geometry.raycast(point+Vec3{0,0,.05f},{0,0,1},1.f);
            if(roof && roof->distance+.05f>.45f && roof->distance+.05f<.85f)
                clearances.push_back({{"point",xyz(point)},{"gap",roof->distance+.05f},
                    {"triangle",i},{"surface",{t.source.bsp,t.source.record,t.source.surface,t.source.instance}}});
        }
    }
    Json spawns=Json::array();for(const auto &spawn:cache.spawns())spawns.push_back(xyz(spawn.position));
    Json objects=Json::array();
    for(const auto &object:content.objects){
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto &t){return t.datum==object.definition;});
        if(tag!=cache.tags().end()){
            Json row={{"name",tag->name},{"type",object.type},
                {"placement",object.placement},{"definition",object.definition},{"position",xyz(object.position)},
                {"rotation",xyz(object.rotation)}};
            if(tag->group=="scen"&&tag->name.find("tent_medical_")!=std::string::npos){
                const auto definition=Bytes(cache.read_tag_bytes(tag->address,tag->size)).u32(0x38);
                const auto collision=cache.load_model_collision(definition);Json bsps=Json::array();
                for(const auto &region:collision.regions)for(const auto &permutation:region)for(const auto &bsp:permutation){
                    Json vertices=Json::array(),edges=Json::array();
                    for(const auto &point:bsp.vertices)vertices.push_back(xyz(point));
                    for(const auto &edge:bsp.edges)edges.push_back(edge.vertices);
                    bsps.push_back({{"node",bsp.node},{"vertices",vertices},{"edges",edges}});
                }
                row["survey_model_collision_bsps"]=std::move(bsps);
            }
            objects.push_back(std::move(row));
        }
    }
    return {{"survey_only",true},{"map",cache.path().string()},{"player_radius",content.player.radius},
        {"player_height",content.player.height},{"crouch_height",content.player.crouch_height},
        {"ordinary_jump_speed",content.player.jump_velocity},{"spawns",std::move(spawns)},
        {"scenario_objects",std::move(objects)},
        {"crouch_clearance_samples",std::move(clearances)},{"floor_triangles",std::move(rows)}};
}
Json replay(Cache &cache,const Json &spec,const std::filesystem::path &output,
    const std::optional<std::filesystem::path> &reference={}){
    Directory directory;auto clock=std::make_shared<Clock>();
    auto owner=reference?std::make_unique<PhysicsLiveWorldTu15>(cache,*reference,directory.path,0,clock):
        std::make_unique<PhysicsLiveWorldTu15>(cache,directory.path,0,clock);
    auto &world=*owner;
    world.initialize_map();auto variant=source_game_variant_default(0);source_game_variant_set(variant,0x59,5);
    world.configure_session(variant,1,30,0x12345678u);
    const auto content=cache.load_content(0,true);
    Json scene_births=Json::array();
    for(const auto &name:spec.value("scenario_definition_names",std::vector<std::string>{})){
        const auto definition=std::ranges::find_if(cache.tags(),[&](const auto &t){return t.name==name&&t.group=="scen";});
        require(definition!=cache.tags().end(),"Declared original scenery definition is absent");
        bool found=false;
        for(const auto &placement:content.objects)if(placement.definition==definition->datum){
            const auto born=world.create_scenario_object(placement);
            require(born!=0xffffffffu,"Original scenery birth rejected the authored placement");
            scene_births.push_back({{"name",name},{"placement",placement.placement},{"object",born}});found=true;
        }
        require(found,"Declared original scenery has no authored placement");
    }
    const auto definition=std::ranges::find_if(cache.tags(),[](const auto &t){
        return t.group=="bipd" && t.name=="objects\\characters\\masterchief\\masterchief_mp";});
    require(definition!=cache.tags().end(),"Original multiplayer biped is absent");
    DatumPool players(world.memory(),world.player_pool());const auto player=players.allocate_at(0);
    require(player!=DatumPool::none,"Original player allocation failed");
    // Explicit local test-machine identity is an API input, never a physics result.
    world.construct_player(player,0,source_default_player_configuration(),{2,0,0,0,0,1});
    const auto spawn=vec(spec.at("spawn")),facing=vec(spec.value("facing",Json::array({1,0,0})));
    const auto unit=world.create_biped(definition->datum,spawn,facing,player);
    require(unit!=0xffffffffu,"Original source biped creation rejected the declared start");
    world.bind_player(player,unit);world.initialize_player_equipment(player,unit);
    if(spec.value("local_view",true))world.set_local_player(player,0);
    const auto word=[&](std::uint32_t at){return Bytes(world.memory().view(at,4)).u32(0);};
    const auto scalar=[&](std::uint32_t at){return std::bit_cast<float>(word(at));};
    const auto vector=[&](std::uint32_t at){return Vec3{scalar(at),scalar(at+4),scalar(at+8)};};
    Json report={{"passed",false},{"runtime_cache_source",reference?"Xbox offline reference":"MCC"},
        {"runtime_acceptance",!reference.has_value()},{"local_view",spec.value("local_view",true)},
        {"map",cache.path().string()},{"source_scenery_births",std::move(scene_births)},
        {"stimulus",spec},{"ticks",Json::array()},{"supplied_contacts_or_velocity",false},
        {"original_superbounce_parity",false},{"large_artifacts_retained",Json::array()}};
    float previous_z_velocity=0,maximum_up=0,maximum_z=spawn.z;
    unsigned tick=0,native_queries=0,forced_crouch_ticks=0;
    std::optional<test::LivePhysicsEntryCaptureTu15> capture;
    std::optional<test::LiveNativePhysicsFrameCaptureTu15> native_frame_capture;
    std::optional<NativeTu15PhysicsFrameObserverBinding> native_frame_observer;
    if(spec.contains("capture_entry")||spec.contains("capture_native_frame_after_tick")){
        require(!(spec.contains("capture_entry")&&spec.contains("capture_native_frame_after_tick")),
            "Use separate replays for entry and native-frame comparisons");
        const auto name=std::filesystem::path(spec.at("capture_name").get<std::string>());
        require(!name.empty()&&name==name.filename()&&name!="."&&name!="..",
            "Entry capture requires a direct owned scratch name");
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch,"Entry capture requires tools/run_test.py");
        const auto directory=std::filesystem::path(scratch)/name;
        require(!std::filesystem::exists(directory),"Entry capture already exists");
        if(spec.contains("capture_entry")){
            capture.emplace(directory);report["owned_entry_capture"]=directory.string();
        }else{
            native_frame_capture.emplace(directory);
            native_frame_observer.emplace(world.memory(),[&](const auto &event){
                if(tick>=spec.at("capture_native_frame_after_tick").get<unsigned>())
                    native_frame_capture->observe(event,world.memory());
            });
            report["owned_native_frame_capture"]=directory.string();
        }
    }
    SourcePhysicsEntryObserverBinding entry_observer(world.memory(),[&](bool entering,std::uint32_t actual,const auto &abi){
        if(entering&&actual==0x2f5600u)++native_queries;
        if(capture&&actual==spec.at("capture_entry").get<std::uint32_t>()&&tick>=spec.value("capture_after_tick",0u))
            capture->observe(entering,abi,world.memory(),actual);
    });
    const auto ordinary_jump=content.player.jump_velocity;
    bool reversal=false;const auto threshold=spec.value("minimum_launch_speed",2.f*ordinary_jump);
    require(std::isfinite(threshold)&&threshold>1.5f*ordinary_jump && ordinary_jump>0,
        "Superbounce threshold must exceed the authored ordinary jump");
    const auto expected_launch=spec.value("expected_launch",true);
    const auto observe_after=spec.value("observe_after_tick",0u);
    try {
        for(const auto &segment:spec.at("segments")){
            if(segment.contains("position")){
                require(spec.value("diagnostic_position_inputs",false),
                    "Repositioning requires an explicitly declared diagnostic stimulus");
                world.set_object_position(unit,vec(segment.at("position")));
            }
            const auto count=segment.at("ticks").get<unsigned>();require(count<=600,"Input segment exceeds bounded replay");
            for(unsigned n=0;n<count;++n){
                require(tick<3600,"Input replay exceeds bounded tick budget");
                std::array<std::byte,0x5c> command;initialize_source_neutral_command(command);
                const auto put=[&](unsigned offset,std::uint32_t value){
                    for(unsigned b=0;b<4;++b)command[offset+b]=std::byte(value>>(8*b));};
                const bool jump=segment.value("jump",false),crouch=segment.value("crouch",false);
                put(0,unsigned(crouch)|(unsigned(jump)<<1));
                put(4,std::bit_cast<std::uint32_t>(segment.value("yaw",std::atan2(facing.y,facing.x))));
                put(8,std::bit_cast<std::uint32_t>(segment.value("pitch",0.f)));
                put(0xc,std::bit_cast<std::uint32_t>(segment.value("forward",0.f)));
                put(0x10,std::bit_cast<std::uint32_t>(-segment.value("right",0.f)));
                world.deliver_command(player,unit,command);++clock->tick;world.step_objects();++tick;
                const auto body=world.source_object_body(unit);const auto p=vector(body+0x64),v=vector(body+0x88);
                require(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&
                    std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"Native replay produced nonfinite motion");
                if(!crouch&&scalar(body+0x2c4)>=.99f)++forced_crouch_ticks;
                if(tick>=observe_after){maximum_up=std::max(maximum_up,v.z);maximum_z=std::max(maximum_z,p.z);
                    reversal|=!jump && previous_z_velocity<-.5f && v.z>=threshold;}
                report["ticks"].push_back({{"tick",tick},{"position",xyz(p)},{"velocity",xyz(v)},
                    {"crouch",scalar(body+0x2c4)},{"state",word(body+0x3dc)&255u},
                    {"biped_flags",word(body+0x348)&65535u},{"jump_input",jump},{"crouch_input",crouch}});
                previous_z_velocity=v.z;
            }
        }
        const auto clock_address=word(0x51df54u);
        require(scalar(clock_address+4)==1.f/30.f,"Source clock changed from OG30Hz");
        const auto activity=world.native_physics_frame_activity();
        report["native_physics_activity"]={{"frames",activity.frame_calls},
            {"residual_calls",activity.residual_calls},{"world_integrations",activity.world_integrations},
            {"native_force_calls",activity.native_force_calls},{"native_pose_calls",activity.native_pose_calls},
            {"world_residual_calls",activity.world_residual_calls}};
        report["full_native_physics_closure"]=false;
        report.update({{"maximum_upward_speed",maximum_up},{"maximum_height",maximum_z},
            {"ordinary_jump_speed",ordinary_jump},{"minimum_launch_speed",threshold},
            {"uncommanded_downward_to_upward_launch",reversal},{"tick_rate",30},
            {"native_scoped_contact_queries",native_queries},{"forced_crouch_ticks",forced_crouch_ticks}});
        require(forced_crouch_ticks>=spec.value("minimum_forced_crouch_ticks",0u),
            "Replay did not remain crouched after releasing its crouch input");
        require(native_queries>=spec.value("minimum_native_contact_queries",0u),
            "Replay did not exercise its required native collision kernel");
        if(spec.contains("expected_peak_upward_speed_bits"))
            require(std::bit_cast<std::uint32_t>(maximum_up)==spec.at("expected_peak_upward_speed_bits").get<std::uint32_t>(),
                "Native contact regression changed the qualified peak velocity bits");
        report["passed"]=reversal==expected_launch;save(output,report);
        require(reversal==expected_launch,expected_launch?
            "No uncommanded super-bounce launch at the declared location/input sequence":
            "Ordinary landing control unexpectedly produced a super-bounce launch");
    }catch(const std::exception &e){report["native_scoped_contact_queries"]=native_queries;report["error"]=e.what();save(output,report);throw;}
    return report;
}
Json match_replay(Cache &cache,const Json &spec,const std::filesystem::path &output){
    require(!spec.value("diagnostic_position_inputs",false),"Match replay accepts ordinary commands and birth placement only");
    Directory directory;auto clock=std::make_shared<Clock>();
    MultiplayerOptions options;options.map=cache.path();options.shared_maps=cache.path().parent_path();
    options.source_object_world=true;options.source_object_savegame_directory=directory.path;
    options.local_players=1;options.starting_weapon=5;options.session_random_seed=0x12345678u;
    options.variant.values.set(2,0);options.variant.values.set(10,0);
    const auto facing=vec(spec.value("facing",Json::array({1,0,0})));
    Spawn start{};start.position=vec(spec.at("spawn"));start.facing=std::atan2(facing.y,facing.x);
    options.native_spawn_inputs={start};
    MultiplayerPreparedWorld prepared(options,std::nullopt,clock);
    require(prepared.source_objects&&prepared.local_slots.size()==1,"Match replay lost its native source world");
    auto &match=prepared.match;auto &world=prepared.source_objects->tu15_world();
    const auto slot=prepared.local_slots.front();
    // An offline placement births its unit during Match construction. Finish
    // one genuine neutral update before binding that active unit's local FP
    // role, whose registry requires completed source temporal inputs.
    std::array<Controls,multiplayer_capacity> warmup{};warmup[slot].yaw=start.facing;
    ++clock->tick;match.step(warmup);
    prepared.bind_source_local_views(prepared.local_slots);
    const auto unit=match.player(slot).source_current_object;
    require(unit!=null_datum,"Match replay has no original current unit");
    const auto scalar=[&](std::uint32_t at){return Bytes(world.memory().view(at,4)).f32(0);};
    const auto vector=[&](std::uint32_t at){return Vec3{scalar(at),scalar(at+4),scalar(at+8)};};
    Json report={{"passed",false},{"runtime_cache_source","MCC"},{"runtime_acceptance",true},
        {"execution_path","MultiplayerPreparedWorld and MultiplayerMatch::step"},
        {"canonical_source_engine",match.source_engine_owned()},
        {"declared_offline_birth_placement",true},{"neutral_setup_ticks",1},{"rendered",false},
        {"map",cache.path().string()},{"stimulus",spec},{"ticks",Json::array()},
        {"supplied_contacts_or_velocity",false},{"original_superbounce_parity",false},
        {"full_native_physics_closure",false},{"large_artifacts_retained",Json::array()}};
    const auto ordinary=prepared.content.player.jump_velocity;
    const auto threshold=spec.value("minimum_launch_speed",2.f*ordinary);
    require(std::isfinite(threshold)&&threshold>1.5f*ordinary&&ordinary>0,"Invalid match launch threshold");
    float previous=0,maximum=0,height=start.position.z,flight_height=0;
    unsigned tick=0,forced=0,queries=0;bool launch=false;
    std::optional<Vec3> launch_position;std::optional<unsigned> launch_tick;
    std::optional<test::LivePhysicsEntryCaptureTu15> capture;
    std::optional<test::LiveNativePhysicsFrameCaptureTu15> native_frame_capture;
    std::optional<NativeTu15PhysicsFrameObserverBinding> native_frame_observer;
    if(spec.contains("capture_entry")||spec.contains("capture_native_frame_after_tick")){
        require(!(spec.contains("capture_entry")&&spec.contains("capture_native_frame_after_tick")),
            "Use separate match replays for entry and native-frame captures");
        const auto name=std::filesystem::path(spec.at("capture_name").get<std::string>());
        require(!name.empty()&&name==name.filename()&&name!="."&&name!="..","Invalid match capture name");
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch,"Match capture requires tools/run_test.py");
        const auto path=std::filesystem::path(scratch)/name;
        require(!std::filesystem::exists(path),"Match capture already exists");
        if(spec.contains("capture_entry")){
            capture.emplace(path);report["owned_entry_capture"]=path.string();
        }else{
            native_frame_capture.emplace(path);
            native_frame_observer.emplace(world.memory(),[&](const auto &event){
                if(tick>=spec.at("capture_native_frame_after_tick").get<unsigned>())
                    native_frame_capture->observe(event,world.memory());
            });
            report["owned_native_frame_capture"]=path.string();
        }
    }
    SourcePhysicsEntryObserverBinding entries(world.memory(),[&](bool entering,std::uint32_t actual,const auto &abi){
        if(entering&&actual==0x2f5600u)++queries;
        if(capture&&actual==spec.at("capture_entry").get<std::uint32_t>()&&tick>=spec.value("capture_after_tick",0u))
            capture->observe(entering,abi,world.memory(),actual);
    });
    try{
        for(const auto &segment:spec.at("segments")){
            require(!segment.contains("position"),"Match replay cannot reposition a live player");
            const auto count=segment.at("ticks").get<unsigned>();require(count<=600,"Match segment exceeds tick budget");
            for(unsigned n=0;n<count;++n){
                require(tick<3600,"Match replay exceeds tick budget");
                std::array<Controls,multiplayer_capacity> commands{};auto &input=commands[slot];
                input.yaw=segment.value("yaw",start.facing);input.pitch=segment.value("pitch",0.f);
                input.forward=segment.value("forward",0.f);input.right=segment.value("right",0.f);
                input.jump=segment.value("jump",false);input.crouch=segment.value("crouch",false);
                ++clock->tick;match.step(commands);++tick;
                require(match.player(slot).source_current_object==unit&&!match.unit(slot).vitality.dead,
                    "Match replay lost its original living unit");
                const auto body=world.source_object_body(unit);const auto p=vector(body+0x64),v=vector(body+0x88);
                require(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&
                    std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"Nonfinite match movement");
                const auto crouch=scalar(body+0x2c4);if(!input.crouch&&crouch>=.99f)++forced;
                if(tick>=spec.value("observe_after_tick",0u)){
                    maximum=std::max(maximum,v.z);height=std::max(height,p.z);
                    const bool reversal=!input.jump&&previous<-.5f&&v.z>=threshold;
                    if(reversal&&!launch_position){launch_position=p;launch_tick=tick;flight_height=p.z;}
                    launch|=reversal;
                }
                if(launch_position)flight_height=std::max(flight_height,p.z);
                report["ticks"].push_back({{"tick",tick},{"position",xyz(p)},{"velocity",xyz(v)},
                    {"crouch",crouch},{"jump_input",input.jump},{"crouch_input",input.crouch}});previous=v.z;
            }
        }
        const auto activity=world.native_physics_frame_activity();
        report["native_physics_activity"]={{"frames",activity.frame_calls},{"residual_calls",activity.residual_calls},
            {"world_integrations",activity.world_integrations},{"native_force_calls",activity.native_force_calls},
            {"native_pose_calls",activity.native_pose_calls},{"world_residual_calls",activity.world_residual_calls}};
        report.update({{"maximum_upward_speed",maximum},{"maximum_height",height},{"ordinary_jump_speed",ordinary},
            {"minimum_launch_speed",threshold},{"uncommanded_downward_to_upward_launch",launch},
            {"forced_crouch_ticks",forced},{"native_scoped_contact_queries",queries},{"tick_rate",30}});
        if(launch_position)report.update({{"launch_tick",*launch_tick},{"launch_position",xyz(*launch_position)},
            {"maximum_height_after_launch",flight_height},{"launch_height_gain",flight_height-launch_position->z}});
        const auto clock_pointer=Bytes(world.memory().view(0x51df54u,4)).u32(0);
        require(scalar(clock_pointer+4)==1.f/30.f,"Match changed from OG30Hz");
        require(forced>=spec.value("minimum_forced_crouch_ticks",0u),"Match did not retain its required forced crouch");
        require(queries>=spec.value("minimum_native_contact_queries",0u),"Match did not reach its required native collision kernel");
        const auto minimum_gain=spec.value("minimum_launch_height_gain",0.f);
        require(std::isfinite(minimum_gain)&&minimum_gain>=0,"Invalid launch flight-height gate");
        if(minimum_gain>0)require(launch_position&&flight_height-launch_position->z>=minimum_gain,
            "Match did not sustain the required post-launch flight");
        if(spec.contains("expected_peak_upward_speed_bits"))require(std::bit_cast<std::uint32_t>(maximum)==
            spec.at("expected_peak_upward_speed_bits").get<std::uint32_t>(),"Match peak differs from isolated-world control");
        require(launch==spec.value("expected_launch",true),"Match did not produce its required super-bounce result");
        report["passed"]=true;save(output,report);
    }catch(const std::exception &error){report["error"]=error.what();save(output,report);throw;}
    return report;
}
}
int main(int argc,char **argv)try{
    if(argc==6 && std::string_view(argv[1])=="--reference-replay"){
        // Original assets and executable remain confined to offline comparison.
        Cache cache(argv[3],{},OriginalData::load_xbe(argv[2]));
        require(!cache.is_mcc(),"Offline Xbox comparison requires its original cache");
        std::ifstream in(argv[4]);Json spec;in>>spec;replay(cache,spec,argv[5],std::filesystem::path(argv[2]));
        std::cout<<"PASS offline TU1.5 native-owner replay; physical Xbox parity remains separate\n";return 0;
    }
    require(argc==4||argc==5,"Use --survey MAP OUTPUT, --replay or --match-replay MAP INPUT OUTPUT, or --reference-replay XBE MAP INPUT OUTPUT");
    Cache cache(argv[2],{},OriginalData::native_tu15());require(cache.is_mcc(),"Runtime acceptance requires prepared MCC content");
    if(std::string_view(argv[1])=="--survey" && argc==4){save(argv[3],survey(cache));return 0;}
    require((std::string_view(argv[1])=="--replay"||std::string_view(argv[1])=="--match-replay") && argc==5,"Unknown superbounce validation mode");
    std::ifstream in(argv[3]);Json spec;in>>spec;
    const auto result=std::string_view(argv[1])=="--match-replay"?match_replay(cache,spec,argv[4]):replay(cache,spec,argv[4]);
    std::cout<<"PASS native TU1.5 "<<(spec.value("expected_launch",true)?"super-bounce launch":"ordinary landing control")
        <<": "<<result.at("maximum_upward_speed")<<" upward speed\n";return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
