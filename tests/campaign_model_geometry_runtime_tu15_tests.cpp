#include "halo2/source_campaign_model.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply TU15_XBE OPENING_MAP SCENE08_OBJECT_INDEX");
    const auto requested=std::stoul(argv[3]);if(requested>=3)throw FormatError("Scene08 object index must be 0..2");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto placements=[&] {
        const auto c=cache.load_content(0,false);std::map<unsigned,ScenarioObject> out;
        for(unsigned name:{0u,1u,2u,22u}) {
            const auto &n=c.object_names.at(name);const auto p=std::ranges::find_if(c.objects,[&](const auto &p){return p.type==n.type && p.placement==n.placement;});
            if(p==c.objects.end())throw FormatError("Authored scene08 placement unavailable");out.emplace(name,*p);
        }
        return out;
    }();
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-model-geometry-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Campaign source model storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();auto &m=w.memory();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);source_campaign_cinematic_control_tu15(w,true);
    source_campaign_camera_control_tu15(w,true);source_campaign_camera_field_of_view_tu15(w,47.f,0);
    source_campaign_camera_animation_tu15(w,0xe9d90864u,0x06001ca3u,null_datum,1);
    const auto create=[&](unsigned name) {
        const auto owner=objects.create_scenario_object(placements.at(name),SourceScenarioBirth::runtime);
        if(owner==null_datum)throw FormatError("Original scene08 birth returned NONE");source_campaign_cinematic_lod_tu15(w,owner,true);return owner;
    };
    const auto anchor=create(22);unsigned random_models{},vertices{};
    const std::array graphs{0xe9ea0875u,0xe9ed0878u,0xe9ec0877u},clips{0x0a001cddu,0x0b001cfbu,0x0c001ce7u};
    const auto word=[&](auto at){return Bytes(m.view(at,4)).u32(0);};
    for(unsigned name=unsigned(requested);name<=requested;++name) {
        std::cout<<"PHASE source scene08 object="<<name<<'\n'<<std::flush;
        const auto owner=create(name);const auto &placement=placements.at(name);
        if(!source_campaign_cinematic_animation_tu15(w,owner,graphs[name],clips[name],anchor,true,false,false))
            throw FormatError("Actual scene08 custom start failed");
        w.step_objects();
        const auto random=word(0x4f3cd8u),before=word(random);
        const auto active=objects.object_model_regions(owner).selection;
        if(!active.resolved())throw FormatError("Actual source object did not publish its region bank");
        const auto preview=cache.load_initial_model_selection(placement.definition,placement.variant);
        random_models+=!preview.resolved();
        std::cout<<"PHASE resource union\n"<<std::flush;SourceCampaignModel geometry(cache,placement);
        if(word(random)!=before)throw FormatError("Campaign resource preparation consumed source RNG");
        w.begin_model_frame();w.begin_model_view();
        // Declared test metrics, rather than a guessed production camera.
        std::cout<<"PHASE original model builder\n"<<std::flush;
        const auto collector=objects.build_world_model(owner,0,1,false,1.f,1.f);
        const Bytes c(collector.bytes);if(c.u16(0x166)!=1 || c.u16(0x168)>=6)
            throw FormatError("Actual scene08 collector did not retain its model/LOD");
        const auto nodes=objects.rendered_world_nodes(owner);
        if(!nodes)throw FormatError("Actual scene08 builder did not publish its rendered pose");
        const auto shown=geometry.vertices(owner,collector,*nodes);
        const auto selected=cache.load_model(placement.definition,false,placement.variant,active,c.u16(0x168));
        const auto expected=skin_model_world_nodes(selected,*nodes);
        if(shown.empty() || shown.size()!=expected.size() ||
            std::memcmp(shown.data(),expected.data(),shown.size()*sizeof(Vertex)))
            throw FormatError("Actual source section order/geometry differs from its resolved live region bank");
        if(word(random)!=before)throw FormatError("Campaign render readback consumed source RNG");
        vertices+=unsigned(shown.size());std::cout<<"PASS scene08 object="<<name<<" vertices="<<shown.size()<<" unseeded_preview_unresolved="<<!preview.resolved()<<'\n'<<std::flush;
        objects.destroy_object(owner);
    }
    if(requested==1 && !random_models)throw FormatError("Scene08 check did not exercise its unresolved random model");
    std::cout<<"PASS actual source selections/resource union/rendered poses; random_models="<<random_models<<" vertices="<<vertices<<'\n';
    std::cout<<"LIMIT declared camera metrics; no shader pixels, full scene VM or converted MCC acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
