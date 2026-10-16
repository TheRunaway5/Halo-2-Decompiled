#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto content=cache.load_content(0,false);
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-camera-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source camera storage unavailable");
    struct Cleanup {std::string path;~Cleanup(){std::error_code error;std::filesystem::remove_all(path,error);}}cleanup{path};
    std::cout<<"PHASE source-construction\n"<<std::flush;
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &world=objects.tu15_world();
    world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678);
    source_campaign_cinematic_control_tu15(world,true);
    const auto word=[&](auto at){return Bytes(world.memory().view(at,4)).u32(0);};
    const auto same=[](const auto &a,const auto &b) {
        static_assert(sizeof(Transform)==52);
        return std::ranges::equal(std::as_bytes(std::span(a)),std::as_bytes(std::span(b)));
    };
    const auto create=[&](unsigned name) {
        const auto &named=content.object_names.at(name);
        const auto placed=std::ranges::find_if(content.objects,[&](const auto &p) {
            return p.type==named.type && p.placement==named.placement;
        });
        if(placed==content.objects.end())throw FormatError("Authored camera-test placement unavailable");
        const auto object=objects.create_scenario_object(*placed,SourceScenarioBirth::runtime);
        if(object==null_datum)throw FormatError("Original camera-test birth returned NONE");
        source_campaign_cinematic_lod_tu15(world,object,true);return object;
    };
    const auto anchor=create(23),pillar=create(25);
    std::cout<<"PHASE original scripted camera actions\n"<<std::flush;
    source_campaign_camera_control_tu15(world,true);
    source_campaign_camera_field_of_view_tu15(world,60.f,0);
    source_campaign_camera_animation_tu15(world,0xe9d90864u,0x06001c9eu,null_datum,2);
    if(!source_campaign_cinematic_animation_tu15(world,pillar,0xe9f1087cu,0x06001d05u,anchor,false,false,false))
        throw FormatError("Original camera-test Pillar start failed");
    const auto camera_state=word(0x4f5504u),director=word(0x51df6cu);
    if(!camera_state || world.memory().view(camera_state,1)[0]!=std::byte{1} ||
        Bytes(world.memory().view(director+2u,2)).u16(0)!=3u)
        throw FormatError("Original scripted camera did not admit its actual animation mode");
    volatile float expected_fov=60.f*std::bit_cast<float>(word(0x46a164u));
    if(word(director+0x10u)!=std::bit_cast<std::uint32_t>(float(expected_fov)))
        throw FormatError("Original scripted camera did not retain its requested FOV");
    const auto sample_camera=[&] {
        // Original16FD40 passes the output matrix and seconds as two stack
        // arguments to whole16FE00. Reserve caller scratch above this world's
        // native stack frame, where the callee's downward stack cannot alias it.
        const auto scratch=world.prepare_source_call().gp[4]+0x100u;
        const std::array<std::byte,56> clear{};
        world.write_source_call_input(scratch,clear);
        auto abi=world.prepare_source_call(std::array{scratch,scratch+52u});
        world.run_actual(0x16fe00u,abi);
        if(!(abi.gp[0]&255u))throw FormatError("Original animated camera sample unavailable");
        std::array<std::byte,56> result{};
        std::ranges::copy(world.memory().view(scratch,result.size()),result.begin());
        return result;
    };
    const auto cluster=world.call_actual(0x16fd40u);
    if((cluster&65535u)==65535u)throw FormatError("Actual animated camera has no source BSP cluster");
    const auto first=source_campaign_cinematic_animation_time_tu15(world,pillar,false);
    const auto camera_first=source_campaign_camera_time_tu15(world);
    const auto initial=objects.world_nodes(pillar);
    const auto initial_camera=sample_camera();
    const auto saved=objects.authority_checkpoint();
    const auto advance=[&] {
        for(unsigned tick=0;tick<60;++tick)world.step_objects();
        const auto later=source_campaign_cinematic_animation_time_tu15(world,pillar,false);
        const auto camera_later=source_campaign_camera_time_tu15(world);
        const auto nodes=objects.world_nodes(pillar);
        const auto camera=sample_camera();
        if(first-later!=60 || camera_first-camera_later!=60 || initial.empty() || same(initial,nodes))
            throw FormatError("Actual cinematic camera/PVS did not preserve 60 scenery and camera ticks");
        if(std::ranges::equal(std::span(camera).first(52),std::span(initial_camera).first(52)))
            throw FormatError("Original animated camera matrix did not move over 60 ticks");
        std::cout<<"ADVANCED pillar="<<first<<" -> "<<later<<" camera="<<camera_first<<" -> "<<camera_later
            <<" source-cluster="<<cluster<<'\n';
        return std::pair{nodes,camera};
    };
    const auto result=advance();
    if(!objects.commit_authority(objects.prepare_authority(saved)))throw FormatError("Source camera checkpoint did not publish");
    if(source_campaign_camera_time_tu15(world)!=camera_first || !same(objects.world_nodes(pillar),initial))
        throw FormatError("Source camera/scenery checkpoint lost its clock or WORLD state");
    const auto repeated=advance();
    if(!same(repeated.first,result.first) || repeated.second!=result.second)
        throw FormatError("Restored source camera/scenery WORLD progression differs");
    source_campaign_camera_control_tu15(world,false);
    if(world.memory().view(camera_state,1)[0]!=std::byte{})throw FormatError("Original camera stop retained scripted admission");
    std::cout<<"PASS actual authored camera/control/FOV/animation/PVS, 120 scenery/camera ticks, WORLD and camera matrix motion, source checkpoint repeat and stop\n";
    std::cout<<"LIMIT no complete opening VM, rendered camera comparison, GPU/audio or converted MCC acceptance\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
