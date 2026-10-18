#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_scene_physics.hpp"
#include "halo2/source_scripted_sound_point_tu15.hpp"
#include "halo2/cache.hpp"
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto placements=[&] {
        const auto c=cache.load_content(0,false);std::map<unsigned,ScenarioObject> out;
        for(auto name:{1u,22u,23u,52u,53u,57u}) {
            const auto &n=c.object_names.at(name);
            const auto p=std::ranges::find_if(c.objects,[&](const auto &p){return p.type==n.type && p.placement==n.placement;});
            if(p==c.objects.end())throw FormatError("Actual opening placement unavailable");out.emplace(name,*p);
        }
        return out;
    }();
    auto path=(std::filesystem::temp_directory_path()/"halo2-sound-point-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Sound-point source storage unavailable");
    struct Cleanup {std::string p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{path};
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);
    source_campaign_cinematic_control_tu15(w,true);source_campaign_camera_control_tu15(w,true);
    source_campaign_camera_animation_tu15(w,0xea2208adu,0x0b001d2bu,null_datum,2);
    const auto create=[&](unsigned name) {
        const auto id=objects.create_scenario_object(placements.at(name),SourceScenarioBirth::runtime);
        if(id==null_datum)throw FormatError("Actual speaker constructor returned NONE");
        source_campaign_cinematic_lod_tu15(w,id,true);return id;
    };
    const auto anchor=create(23),chief=create(52),armorer=create(53);
    if(!source_campaign_cinematic_animation_tu15(w,chief,0xea2e08b9u,0x08001d45u,anchor,true,false,false) ||
       !source_campaign_cinematic_animation_tu15(w,armorer,0xea2f08bau,0x10001d46u,anchor,true,false,false))
        throw FormatError("Actual speaker animation did not start");
    const auto covenant_anchor=create(22),tartarus=create(1),truth=create(57);
    if(!source_campaign_cinematic_animation_tu15(w,tartarus,0xe9ed0878u,0x0b001cfbu,covenant_anchor,true,false,false) ||
       !source_campaign_cinematic_animation_tu15(w,truth,0xe9ec0877u,0x08001cf5u,covenant_anchor,true,false,false))
        throw FormatError("Actual Tartarus/Truth animation did not start");
    for(unsigned tick=0;tick<2;++tick)w.step_objects();
    unsigned checks{};
    std::map<std::uint32_t,Vec3> initial_points;
    for(const auto object:{chief,armorer,tartarus,truth}) {
        const auto scratch=w.prepare_source_call().gp[4]+0x100u;
        auto a=w.prepare_source_call(std::array{object,0x04000095u,1u});a.gp[3]=scratch;a.gp[1]&=~255u;
        w.run_actual(0xbad60u,a);
        const Bytes marker(w.memory().view(scratch,0x70));
        const auto root=source_scene_physics_transform(objects.memory(),objects.source_object_pool(),object);
        const auto nodes=objects.world_nodes(object);
        const auto print=[](Vec3 p){std::cout<<p.x<<','<<p.y<<','<<p.z;};
        std::cout<<"object="<<std::hex<<object<<std::dec<<" root=";print(root.position);
        std::cout<<" node0=";print(nodes.at(0).position);
        std::cout<<" marker_local=";print({marker.f32(0x2c),marker.f32(0x30),marker.f32(0x34)});
        std::cout<<" marker_world=";print({marker.f32(0x60),marker.f32(0x64),marker.f32(0x68)});
        std::cout<<" count="<<(a.gp[0]&65535u)<<'\n';
        const Vec3 expected{marker.f32(0x60),marker.f32(0x64),marker.f32(0x68)};
        const auto got=source_scripted_sound_point_tu15(w,object);
        initial_points.emplace(object,got);
        if(!std::ranges::equal(std::as_bytes(std::span(&expected,1)),std::as_bytes(std::span(&got,1))))
            throw FormatError("Scripted sound did not consume the actual original WORLD marker point");
        if(length(got-root.position)<1.f)throw FormatError("Actual animated speaker fixture did not distinguish placement from WORLD audio");
        ++checks;
    }
    if(checks!=4)throw FormatError("Incomplete authored speaker coverage");
    for(unsigned tick=0;tick<12;++tick)w.step_objects();
    unsigned moved{};
    for(const auto &[object,point]:initial_points) {
        const auto current=source_scripted_sound_point_tu15(w,object);
        if(length(current-point)>.0001f)++moved;
    }
    if(!moved)throw FormatError("Animated speaker fixture did not exercise changing source WORLD points");
    std::cout<<"PASS original marker path for Chief, Armorer, Tartarus and Truth; completed WORLD points distinguish all four placement origins\n";
    std::cout<<"Animated WORLD points changed for "<<moved<<" actual speakers across12 source ticks\n";
    std::cout<<"LIMIT no Xbox DSP/HRTF, complete emitter/listener lifecycle or full recorded-audio acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
