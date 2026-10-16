#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/scene.hpp"
#include <iostream>
#include <unistd.h>
using namespace halo2;

int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    auto content=cache.load_content(0,false);
    // Declared subset: the actual Regret/throne pair from x01_01b_setup
    // and x01_10_cleanup. Preserve their authored placement fields.
    std::erase_if(content.objects,[](const auto &p){return p.name!=56 && p.name!=59;});
    if(content.objects.size()!=2 || std::ranges::any_of(content.objects,[](const auto &p){return !(p.flags&1u);}))
        throw FormatError("Authored initially absent Regret/throne pair unavailable");
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-scene-retirement-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source scene retirement storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);source_campaign_cinematic_control_tu15(w,true);
    NativeObjectLifetimes lifetimes(objects.memory(),objects.object_pool());unsigned deletions{};
    SceneObjects scene(content,0,SourceSceneGameContext{1,0},{},&lifetimes,{
        [&](const ScenarioObject &p,SourceScenarioBirth birth){return objects.create_scenario_object(p,birth);},
        [&](std::uint32_t owner){++deletions;objects.destroy_object(owner);}});
    for(unsigned repeat=0;repeat<2;++repeat) {
        const auto parent=scene.create(56,false),child=scene.create(59,false);
        source_campaign_cinematic_attach_tu15(w,parent,0,child,0x10001c4au);
        if(objects.object_parent(child)!=parent)throw FormatError("Original throne attachment was not established");
        std::cout<<"PHASE original parent/cascaded-child retirement\n"<<std::flush;
        scene.destroy(parent);
        if(!lifetimes.header(parent).empty() || !lifetimes.header(child).empty())
            throw FormatError("Original parent retirement did not retire its child");
        if(scene.resolve(56)!=null_datum || scene.resolve(59)!=null_datum)
            throw FormatError("Actual retired child retained its scenario name in the same script phase");
        // The next authored containing-name cleanup sees NONE immediately,
        // before another source game tick or pose-publication sweep.
        scene.destroy(scene.resolve(59));
        if(deletions!=repeat+1 || lifetimes.contains(parent) || lifetimes.contains(child))
            throw FormatError("Presentation repeated source deletion or retained the old identities");
    }
    std::cout<<"PASS actual Regret/throne attachment, cascade, immediate names and two fresh generations\n";
    std::cout<<"LIMIT declared authored pair; no full cleanup VM, scene pixels or whole opening acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
