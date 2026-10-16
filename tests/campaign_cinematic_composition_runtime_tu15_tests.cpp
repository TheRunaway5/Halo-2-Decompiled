#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    // Keep the actual placements; release the temporary native content before
    // original world construction. No live source object or resource is omitted.
    const auto placements=[&] {
        const auto content=cache.load_content(0,false);std::map<unsigned,ScenarioObject> out;
        const std::array names{22u,31u,32u,33u,34u,35u,36u,37u,38u,39u,40u,56u,58u,59u,61u};
        for(auto name:names) {
            const auto &n=content.object_names.at(name);
            const auto p=std::ranges::find_if(content.objects,[&](const auto &p){return p.type==n.type && p.placement==n.placement;});
            if(p==content.objects.end())throw FormatError("Authored scene01b placement unavailable");out.emplace(name,*p);
        }
        return out;
    }();
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-composition-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source composition storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    std::cout<<"PHASE source-construction\n"<<std::flush;
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);source_campaign_cinematic_control_tu15(w,true);
    source_campaign_camera_control_tu15(w,true);source_campaign_camera_field_of_view_tu15(w,60.f,0);
    source_campaign_camera_animation_tu15(w,0xe9d90864u,0x07001c9cu,null_datum,1);
    const auto create=[&](unsigned name) {
        const auto object=objects.create_scenario_object(placements.at(name),SourceScenarioBirth::runtime);
        if(object==null_datum)throw FormatError("Original scene01b birth returned NONE");source_campaign_cinematic_lod_tu15(w,object,true);return object;
    };
    for(auto [parent_name,child_name]:{std::pair{58u,61u},std::pair{56u,59u}}) {
        const auto parent=create(parent_name),child=create(child_name);source_campaign_cinematic_attach_tu15(w,parent,0,child,0x10001c4au);
    }
    const auto anchor=create(22);std::vector<std::uint32_t> units;std::vector<std::int16_t> first;
    std::vector<std::vector<Transform>> initial;
    for(unsigned name=31;name<=40;++name) {
        const auto object=create(name);units.push_back(object);
        if(!source_campaign_cinematic_animation_tu15(w,object,0xe9e3086eu,0x0d001ccbu+name-31u,anchor,true,false,false))
            throw FormatError("Original scene01b custom start failed");
        first.push_back(source_campaign_cinematic_animation_time_tu15(w,object,true));initial.push_back(objects.world_nodes(object));
    }
    unsigned compositions{};w.observe_physics_stores([&](auto pc,const auto &){if(pc==0x28c916u)++compositions;});
    std::cout<<"PHASE actual scene01b camera/prophets/ten relative units\n"<<std::flush;
    for(unsigned tick=0;tick<120;++tick)w.step_objects();w.observe_physics_stores({});
    unsigned moved{};
    for(unsigned n=0;n<units.size();++n) {
        if(first[n]-source_campaign_cinematic_animation_time_tu15(w,units[n],true)!=120)
            throw FormatError("Actual scene01b unit clock did not advance120 phases");
        const auto later=objects.world_nodes(units[n]);
        if(!std::ranges::equal(std::as_bytes(std::span(initial[n])),std::as_bytes(std::span(later))))++moved;
    }
    if(!compositions || moved!=10)throw FormatError("Actual scene01b did not exercise additive composition and all ten moving WORLD hierarchies");
    std::cout<<"PASS120 actual scene01b phases; additive compositions="<<compositions<<" moving WORLD hierarchies="<<moved<<'\n';
    std::cout<<"LIMIT no whole opening VM, GPU/codec differential or completed cinematic acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
