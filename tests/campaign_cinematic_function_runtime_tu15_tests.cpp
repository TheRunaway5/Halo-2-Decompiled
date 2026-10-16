#include "halo2/source_object_functions_tu15.hpp"
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
    const auto content=[&] {
        const auto all=cache.load_content(0,false);const auto &n=all.object_names.at(31);
        const auto p=std::ranges::find_if(all.objects,[&](const auto &p){return p.type==n.type && p.placement==n.placement;});
        if(p==all.objects.end())throw FormatError("Authored opening Banshee placement unavailable");
        LevelContent out;out.objects.push_back(*p);out.object_names=all.object_names;return out;
    }();
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-functions-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source function storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    std::cout<<"PHASE source-construction\n"<<std::flush;
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();auto &m=w.memory();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);source_campaign_cinematic_control_tu15(w,true);
    NativeObjectLifetimes lifetimes(m,objects.source_object_pool());
    SceneObjects scene(content,0,SourceSceneGameContext{1,0},{},&lifetimes,
        {[&](const auto &p,auto birth){return objects.create_scenario_object(p,birth);},[&](auto object){objects.destroy_object(object);}});
    const auto object=scene.create(31);source_campaign_cinematic_lod_tu15(w,object,true);
    if(!source_object_function_samples_tu15(w).empty())throw FormatError("Actual function bank was not reset by source map initialization");
    constexpr std::uint32_t name=0x0c001b89u;
    source_object_function_set_tu15(w,object,name,0.f,0.f);source_object_function_set_tu15(w,object,name,1.f,30.f);
    const auto publish=[&] {scene.publish_source_function_samples(source_object_function_samples_tu15(w));};
    const auto value=[&](float expected) {
        publish();const auto v=scene.function_variable(object,name,999999);
        if(!v || *v!=expected || source_object_function_value_tu15(w,object,name)!=v)
            throw FormatError("Borrowed renderer function differs from its actual source clock/value");
    };
    value(0.f);for(unsigned tick=0;tick<15;++tick)w.step_objects();value(.5f);
    if((std::to_integer<unsigned>(m.view(objects.source_object_body(object)+0xc1u,1)[0])&2u)==0)
        throw FormatError("Original setter did not publish its actual object function flag");
    const auto saved=objects.authority_checkpoint();const auto saved_lifetimes=lifetimes.source_authority_checkpoint();
    for(unsigned tick=0;tick<15;++tick)w.step_objects();value(1.f);
    auto restored=objects.prepare_authority(saved);auto pool=lifetimes.prepare_source_authority(saved_lifetimes,restored.replacement_memory());
    if(!objects.commit_authority(std::move(restored)) || !lifetimes.commit_source_authority(std::move(pool)))
        throw FormatError("Source function checkpoint did not publish its coupled pool/world");
    value(.5f);source_object_function_clear_tu15(w,object,name);publish();
    if(scene.function_variable(object,name,0))throw FormatError("Original source clear did not remove its borrowed renderer value");
    for(unsigned n=0;n<33;++n)source_object_function_set_tu15(w,object,0x04001000u+n,float(n),0.f);
    const auto samples=source_object_function_samples_tu15(w);
    if(samples.size()!=32 || samples.contains({object,0x04001000u}) || !samples.contains({object,0x04001020u}))
        throw FormatError("Actual original32-slot tie/replacement policy differs");
    scene.publish_source_function_samples(samples);
    if(scene.function_variable(object,0x04001000u,0) || scene.function_variable(object,0x04001020u,0)!=32.f)
        throw FormatError("Borrowed renderer retained an evicted source slot");
    std::cout<<"PASS original Banshee function interpolation/flag, renderer samples, coupled checkpoint, clear and32-slot replacement\n";
    std::cout<<"LIMIT no rendered shader-pixel, whole opening or converted MCC acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
