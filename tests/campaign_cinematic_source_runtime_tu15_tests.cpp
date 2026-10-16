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
    const auto content=cache.load_content(0,false);
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-cinematic-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Cinematic source test storage unavailable");
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
        if(placed==content.objects.end())throw FormatError("Authored cinematic placement unavailable");
        const auto object=objects.create_scenario_object(*placed,SourceScenarioBirth::runtime);
        if(object==null_datum)throw FormatError("Original cinematic birth returned NONE");
        source_campaign_cinematic_lod_tu15(world,object,true);return object;
    };
    std::cout<<"PHASE Pillar source custom controller\n"<<std::flush;
    const auto anchor=create(23),pillar=create(25);
    if(!source_campaign_cinematic_animation_tu15(world,pillar,0xe9f1087cu,0x06001d05u,anchor,false,false,false))
        throw FormatError("Original Pillar relative start failed");
    const auto first=source_campaign_cinematic_animation_time_tu15(world,pillar,false);
    const auto initial=objects.world_nodes(pillar);
    if(first<=0 || initial.empty() || word(objects.source_object_body(pillar)+0x138u)!=anchor)
        throw FormatError("Original Pillar channel/query/anchor admission differs");
    for(unsigned tick=0;tick<60;++tick)world.step_objects();
    const auto later=source_campaign_cinematic_animation_time_tu15(world,pillar,false);
    const auto advanced=objects.world_nodes(pillar);
    if(later>=first || same(initial,advanced))throw FormatError("Original Pillar controller did not advance actual WORLD poses");
    std::cout<<"PILLAR remaining="<<first<<" -> "<<later<<" nodes="<<advanced.size()<<'\n';
    std::cout<<"PHASE authored prophet/throne attachment\n"<<std::flush;
    const auto parent=create(58),child=create(61);
    source_campaign_cinematic_attach_tu15(world,parent,0,child,0x10001c4au);
    if(objects.object_parent(child)!=parent)throw FormatError("Original throne did not retain its prophet parent");
    const auto before_repeat=objects.world_nodes(child);
    source_campaign_cinematic_attach_tu15(world,parent,0,child,0x10001c4au);
    if(!same(objects.world_nodes(child),before_repeat))throw FormatError("Original already-attached guard changed its child pose");
    for(unsigned tick=0;tick<120;++tick)world.step_objects();
    if(objects.object_parent(child)!=parent || objects.world_nodes(child).empty())
        throw FormatError("Original throne attachment did not survive source updates");
    objects.destroy_object(parent);
    if(std::ranges::any_of(objects.live_objects(),[&](const auto &o){return o.object==child;}))
        throw FormatError("Original prophet retirement retained its attached child");
    std::cout<<"PHASE authored relative unit controller\n"<<std::flush;
    const auto unit_anchor=create(22),unit=create(31);
    if(!source_campaign_cinematic_animation_tu15(world,unit,0xe9e3086eu,0x0d001ccbu,unit_anchor,true,false,false))
        throw FormatError("Original relative unit start failed");
    const auto unit_first=source_campaign_cinematic_animation_time_tu15(world,unit,true);
    const auto unit_initial=objects.world_nodes(unit);
    for(unsigned tick=0;tick<60;++tick)world.step_objects();
    const auto unit_later=source_campaign_cinematic_animation_time_tu15(world,unit,true);
    if(unit_first-unit_later!=60 || unit_initial.empty() || same(unit_initial,objects.world_nodes(unit)))
        throw FormatError("Original relative unit controller did not advance its clock and actual WORLD poses");
    std::cout<<"UNIT remaining="<<unit_first<<" -> "<<unit_later<<'\n';
    std::cout<<"PASS actual source relative scenery/unit controllers, WORLD motion, authored attachment guard/update/retirement\n";
    std::cout<<"LIMIT Pillar advanced three frames in 60 phases; no complete camera/PVS delivery, full scene VM, GPU/audio, full cinematic or converted MCC acceptance\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
