#include "halo2/effect_light_view.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>

using namespace halo2;
int main(int argc,char** argv) {try {
    if(argc!=2)throw FormatError("Supply original effect light view fixture");
    std::ifstream file(argv[1],std::ios::binary);
    const auto read=[&](void* p,std::size_t n) {file.read(static_cast<char*>(p),n);if(!file)throw FormatError("Truncated original light view fixture");};
    const auto word=[&]() {std::uint32_t v;read(&v,4);return v;};
    const auto scalar=[&]() {return std::bit_cast<float>(word());};
    const auto vector=[&]() {return Vec3{scalar(),scalar(),scalar()};};
    if(word()!=0x31564c45)throw FormatError("Invalid original effect light view fixture");
    const auto cases=word();SourceClusterFrustumSet frustums;frustums.group_count=std::int16_t(word());
    const auto cluster_count=word(),frustum_count=word();
    for(unsigned i=0;i<cluster_count;++i) {
        SourceClusterFrustumRanges r;r.source_cluster=std::int16_t(word());
        for(auto& x:r.counts)x=std::int16_t(word());for(auto& x:r.first_indices)x=std::int16_t(word());frustums.clusters.push_back(r);
    }
    for(unsigned i=0;i<frustum_count;++i) {
        SourceSphereFrustum f;for(auto& x:f.bounds)x=scalar();for(auto& p:f.planes)for(auto& x:p)x=scalar();frustums.frustums.push_back(f);
    }
    std::map<std::uint32_t,EffectLightDefinition> definitions;
    for(unsigned i=0;i<2;++i) {const auto tag=word();std::array<std::byte,228> bytes;read(bytes.data(),bytes.size());definitions.emplace(tag,source_effect_light_definition(bytes));}
    const SourceEffectLightDefinitions definition=[&](auto id)->const EffectLightDefinition& {return definitions.at(id);};
    unsigned checkpoints=0,appends=0,selected=0,failures=0;std::uint64_t compared=0;
    for(unsigned n=0;n<cases;++n) {
        SourceEffectLightQueryState state;state.epoch=word();
        for(auto& r:state.main_queue)r.association=0; //Declared cold original queue columns.
        for(auto& r:state.light_queue)r.association=0;
        auto pool=std::make_unique<SourceEffectLightDatumPool>();
        for(auto& v:pool->occupied)v=word();read(pool->records.data(),sizeof(pool->records));
        pool->highwater=96;pool->next_generation=0xe981;
        for(const auto bits:pool->occupied)pool->active_count+=std::popcount(bits);
        auto spatial=std::make_unique<SourceEffectLightSpatialPool>(8);unsigned link=0;
        for(auto& head:spatial->heads) {
            const auto count=word();if(count>96)throw FormatError("Excessive original light view spatial list");
            head=count?0xec630000u|link:0xffffffffu;
            for(unsigned j=0;j<count;++j) {
                auto& r=spatial->entries.records[link];r[0]=0xec63;r[1]=word();r[2]=j+1<count?0xec630000u|(link+1):0xffffffffu;++link;
            }
        }
        std::array<SourceShadowSelectionCache,4> caches{};std::vector<std::uint32_t> expected;
        for(auto& cache:caches)for(auto& e:cache.entries)e.candidate.owner=0; //Declared cold original cache bytes.
        std::size_t last_main_count=0;
        const auto operations=word();
        for(unsigned step=0;step<operations;++step) {
            const auto kind=word();const auto local_player=std::bit_cast<std::int32_t>(word());
            SourceEffectLightCollectionContext context;context.visibility.query_mode=std::bit_cast<std::int32_t>(word());
            context.flags=word();context.pass_mode=std::bit_cast<std::int32_t>(word());
            const auto frame=std::bit_cast<std::int32_t>(word());const bool cinematic=word();const auto eye=vector();
            SourceEffectLightCandidateContext candidate;for(auto& v:candidate.depth_plane)v=scalar();candidate.pixel_scale=scalar();
            candidate.admission={1,0,0,2,{}};
            context.visibility.sphere_center=eye;context.visibility.sphere_radius=scalar();context.visibility.frustums=&frustums;
            context.containing_cluster=std::int16_t(local_player);context.local_player=std::bit_cast<std::uint32_t>(local_player);
            const auto size=word(),changes=word();if(size>30000 || changes>size)throw FormatError("Excessive light view checkpoint");
            expected.resize(size);for(unsigned i=0;i<changes;++i) {const auto at=word(),value=word();if(at>=size)throw FormatError("Invalid light view delta");expected[at]=value;}
            source_effect_lights_update_view(*pool,definition,eye);
            auto view=source_effect_lights_query(*pool,*spatial,definition,state,
                kind?SourceEffectLightQueryKind::Light:SourceEffectLightQueryKind::Main,context);
            appends+=view.visible.size();if(!kind) {source_effect_lights_select(view,caches,local_player,candidate,frame,cinematic);selected+=view.selected.size();}
            std::vector<std::uint32_t> actual{state.epoch};
            for(const auto& r:pool->records)actual.insert(actual.end(),r.begin(),r.end());
            actual.push_back(kind?0:std::uint32_t(view.visible.size()));
            // A nested query leaves the previous main queue count intact.
            if(kind)actual.back()=std::uint32_t(last_main_count);
            else last_main_count=view.visible.size();
            for(const auto& r:state.main_queue)for(auto v:std::array{r.id,std::uint32_t(r.flags),r.other,std::uint32_t(r.association)})actual.push_back(v);
            actual.push_back(kind?std::uint32_t(view.visible.size()):0);
            for(const auto& r:state.light_queue)for(auto v:std::array{r.id,std::uint32_t(r.flags),r.other,std::uint32_t(r.association)})actual.push_back(v);
            for(const auto& cache:caches) {
                for(const auto& e:cache.entries) {
                    const auto& c=e.candidate;
                    for(auto v:std::array{c.flags,c.identity,std::bit_cast<std::uint32_t>(c.owner),
                        std::bit_cast<std::uint32_t>(c.requested_fade),std::bit_cast<std::uint32_t>(c.coverage),
                        std::bit_cast<std::uint32_t>(e.slope),std::bit_cast<std::uint32_t>(e.fade),
                        std::bit_cast<std::uint32_t>(e.last_seen_frame),std::bit_cast<std::uint32_t>(e.created_frame)})actual.push_back(v);
                }
                actual.push_back(cache.count);
            }
            if(actual.size()!=expected.size())throw FormatError("Original light view checkpoint extent differs");
            for(unsigned i=0;i<actual.size();++i) {
                ++compared;if(actual[i]!=expected[i]) {
                    if(failures<20)std::cerr<<"case="<<n<<" step="<<step<<" word="<<i<<" native="<<std::hex<<actual[i]<<" original="<<expected[i]<<std::dec<<'\n';++failures;
                }
            }
            ++checkpoints;
        }
    }
    if(failures)throw FormatError("Original light view differs in "+std::to_string(failures)+" words");
    if(cases!=8 || checkpoints!=256 || appends<100 || selected<100 || file.peek()!=std::char_traits<char>::eof())throw FormatError("Original light view coverage missing");
    std::cout<<"PASS "<<checkpoints<<" original multi-view checkpoints, "<<appends<<" appends, "<<selected<<" selections, "<<compared<<" exact complete record/queue/cache words\n";
    return 0;
}catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
