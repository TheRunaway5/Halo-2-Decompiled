#include "halo2/source_unit_cinematic_tu15.hpp"
#include "halo2/source_campaign_model.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto placements=[&] {
        const auto c=cache.load_content(0,false);std::map<unsigned,ScenarioObject> out;
        for(auto name:{22u,56u,58u}) {
            const auto &n=c.object_names.at(name);const auto p=std::ranges::find_if(c.objects,[&](const auto &p){return p.type==n.type && p.placement==n.placement;});
            if(p==c.objects.end())throw FormatError("Authored prophet placement unavailable");out.emplace(name,*p);
        }
        return out;
    }();
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-unit-controls-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source unit-control storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    std::cout<<"PHASE source-construction\n"<<std::flush;
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();auto &m=w.memory();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);source_campaign_cinematic_control_tu15(w,true);
    source_campaign_camera_control_tu15(w,true);source_campaign_camera_field_of_view_tu15(w,47.f,0);
    source_campaign_camera_animation_tu15(w,0xe9d90864u,0x06001ca0u,null_datum,1);
    const auto create=[&](unsigned name) {
        const auto object=objects.create_scenario_object(placements.at(name),SourceScenarioBirth::runtime);
        if(object==null_datum)throw FormatError("Original prophet birth returned NONE");source_campaign_cinematic_lod_tu15(w,object,true);return object;
    };
    const auto anchor=create(22),regret=create(56),mercy=create(58);unsigned resets{};
    w.observe_physics_stores([&](auto pc,const auto &){if(pc==0x11196fu)++resets;});
    std::cout<<"PHASE real prophet graph switches\n"<<std::flush;
    for(unsigned repeat=0;repeat<2;++repeat) {
        if(!source_campaign_cinematic_animation_tu15(w,mercy,0xe9ec0877u,0x08001ce1u,anchor,true,false,false) ||
           !source_campaign_cinematic_animation_tu15(w,regret,0xe9ec0877u,0x09001cefu,anchor,true,false,false))
            throw FormatError("Original prophet custom graph start failed");
    }
    w.observe_physics_stores({});if(resets<2)throw FormatError("Actual prophet starts did not exercise original graph reset");
    const auto body=objects.source_object_body(regret);
    const auto record=body+std::bit_cast<std::int16_t>(Bytes(m.view(body+0x33eu,2)).u16(0));
    const auto word=[&](auto at){return Bytes(m.view(at,4)).u32(0);};
    if((word(record+0x5cu)>>16)==0xffffu)throw FormatError("Actual prophet graph did not bind its facial channel");
    const auto saved=objects.authority_checkpoint();const auto first_time=source_campaign_cinematic_animation_time_tu15(w,regret,true);
    const auto advance=[&](float weight) {
        source_unit_cinematic_emotion_tu15(w,regret,0x080001abu,weight,90);
        if(Bytes(m.view(record+0x68u,4)).f32(0)!=weight)throw FormatError("Actual emotional owner did not retain its authored weight");
        for(unsigned tick=0;tick<30;++tick)w.step_objects();
        if(first_time-source_campaign_cinematic_animation_time_tu15(w,regret,true)!=30)
            throw FormatError("Original prophet custom time did not advance30 phases");
        // Whole1111D0 increments this elapsed-tick counter by1, then
        // multiplies by the separate reciprocal duration at+60.
        const auto elapsed_ticks=Bytes(m.view(record+0x64u,4)).f32(0);
        if(elapsed_ticks!=30.f)throw FormatError("Actual facial transition did not advance30 source ticks");
        // Facial/finger layers are render-timeBF640 inputs, distinct from
        // the collision WORLD hierarchy. Use the actual model builder with
        // declared view0/solo/nonmodified, zero distance metrics; cinematic
        // LOD admission remains the original owner set above.
        w.begin_model_frame();w.begin_model_view();
        const auto collector=objects.build_world_model(regret,0,1,false,0.f,0.f);
        const auto rendered=objects.rendered_world_nodes(regret);
        if(!rendered)throw FormatError("Actual prophet model builder did not publish its render pose");
        // Resource preparation does not own source ticks. Keep this bounded
        // test's CPU union within the vertex-consumption phase; the graphics
        // runtime retains its resource bank across submitted frames.
        std::cout<<"PHASE source-selected emotional vertices\n"<<std::flush;
        SourceCampaignModel geometry(cache,placements.at(56));
        return geometry.vertices(regret,collector,*rendered);
    };
    std::cout<<"PHASE authored emotional layer and actual WORLD differential\n"<<std::flush;
    const auto emotional=advance(.5f);
    if(!objects.commit_authority(objects.prepare_authority(saved)))throw FormatError("Prophet source checkpoint did not publish");
    const auto neutral=advance(0.f);
    if(emotional.empty() || std::ranges::equal(std::as_bytes(std::span(emotional)),std::as_bytes(std::span(neutral))))
        throw FormatError("Actual emotional layer did not change the prophet rendered vertices");
    std::cout<<"PASS actual prophet graph reset count="<<resets<<", facial binding/progress,30-phase clocks, checkpoint and emotional vertex differential\n";
    std::cout<<"LIMIT no full scene VM, rendered facial pixels, all emotions or converted MCC acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
