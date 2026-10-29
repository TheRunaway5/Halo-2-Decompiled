#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_presentation_resource_regions_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
namespace {
void need(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F&& fn){try{fn();}catch(const std::exception&){return;}throw FormatError("Corrupt presentation resource owner was admitted");}
int run(int argc,char **argv) {
    const bool legacy=argc>1&&std::string_view(argv[1])=="--legacy-journal";
    const unsigned first=legacy?2u:1u;
    if(unsigned(argc)<first+1u || !std::filesystem::is_regular_file(argv[first]))return 77;
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Native resource regression requires owned scratch");
    Cache cache(argv[first],{},OriginalData::native_tu15());
    const auto files=std::filesystem::path(scratch)/"native-world";std::filesystem::create_directories(files);
    PhysicsLiveWorldTu15 world(cache,files);world.initialize_map();
    world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
    auto &memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const auto put=[&](std::uint32_t at,std::uint32_t value){memory.write(at,std::as_bytes(std::span(&value,1)));};
    const auto pool=word(0x4f0418u),rows=word(pool+0x44u);
    const auto allocator=word(0x4f0424u),block_pool=word(allocator+0x64u),block_rows=word(block_pool+0x44u);
    const auto table=word(0x4f0414u);
    // Declared spatial-byte controls test journal selection independently of
    // object birth/Match scheduling. The companion actual presented-pose test
    // owns real B95D0 root/sphere/spatial-link restoration and native palettes.
    constexpr std::uint32_t spatial_at=0x71000000u;
    const std::array<std::byte,64> spatial{};
    memory.mount(spatial_at,spatial,SourceRuntimeMemory::Access::ReadWrite,"declared presentation spatial-byte control");
    std::vector<std::pair<std::uint32_t,std::uint32_t>> descriptors;
    for(const auto &tag:cache.tags())if(tag.address&&tag.group=="jmad") {
        const auto graph=word(table+(tag.datum&65535u)*16u+8u);if(!graph)continue;
        const auto first=word(graph+0xb0u);
        for(unsigned n=0;n<word(graph+0xacu);++n) {
            const auto at=first+n*20u,bytes=word(at+4u);
            if(bytes&&bytes<=word(0x4f0428u))descriptors.emplace_back(bytes,at);
        }
    }
    std::sort(descriptors.begin(),descriptors.end(),std::greater<>());
    need(descriptors.size()>=1100,"Actual Dune regression lacks its resource-pressure domain");
    unsigned loads{},evictions{};
    const auto observed=[&](const SourcePhysicsContactRecordStore &store){evictions+=store.instruction==0x12645fu;};
    const auto load=[&](unsigned n) {
        auto abi=world.prepare_source_call(std::array{0u});abi.gp[0]=descriptors.at(n).second;
        world.run_actual(0x1262f0u,abi,observed);++loads;
    };
    // Actual512-slot LRU ownership is exercised before the two callers overlap.
    for(unsigned n=0;n<768;++n)load(n);
    need(evictions>0,"Native resource regression did not exercise real eviction");
    const SourcePresentationResourceRegionsTu15 resources(memory);
    need(!resources.persistent(spatial_at,spatial.size()),
        "Native simulation spatial inputs were classified as resource admissions");
    unsigned adoption=768;while(adoption<descriptors.size()&&word(descriptors[adoption].second+12u)!=UINT32_MAX)++adoption;
    need(adoption+232<descriptors.size(),"Native resource regression lacks a cold adoption resource");
    struct Store{std::uint32_t at;std::vector<std::byte> before;};std::vector<Store> undo;
    std::map<std::uint32_t,std::byte> spatial_before;unsigned observer_owner{};
    source_runtime_register_write_observer(memory,&observer_owner,[&](auto at,auto bytes) {
        if(at>=0x60000000u&&std::uint64_t(at)+bytes<=0x60100000ull)return;
        const bool persistent=resources.persistent(at,bytes);
        if(persistent&&!legacy)return;
        const auto before=memory.view(at,bytes);undo.push_back({at,{before.begin(),before.end()}});
        if(!persistent)for(unsigned n=0;n<before.size();++n)spatial_before.try_emplace(at+n,before[n]);
    });
    try {
        auto changed=spatial;changed.fill(std::byte{0x15});memory.write(spatial_at,changed);
        load(adoption);
    }catch(...){source_runtime_unregister_write_observer(memory,&observer_owner);throw;}
    source_runtime_unregister_write_observer(memory,&observer_owner);
    for(unsigned n=adoption+1u;n<adoption+128u;++n)load(n);
    for(auto it=undo.rbegin();it!=undo.rend();++it)memory.write(it->at,it->before);
    need(spatial_before.size()==spatial.size(),"Spatial rollback omitted a declared input store");
    for(const auto &[at,value]:spatial_before)
        need(memory.view(at,1)[0]==value,"Presented adoption failed to restore an actual spatial store");
    // Before the fix this exact two-caller sequence leaves live rows above a
    // restored frontier, then12633E indexesFFFF after16E770 rejects reuse.
    for(unsigned n=adoption+128u;n<adoption+232u;++n)load(n);
    unsigned tracked{},blocks{};
    for(unsigned n=0;n<512;++n) {
        const auto tracker=word(rows+n*16u)&65535u,block=word(block_rows+n*24u)&65535u;
        need(tracker==block,"Native LRU/tracker full slot ownership diverged");
        tracked+=tracker!=0;blocks+=block!=0;
        if(tracker)need(n<word(pool+0x38u)&&n<word(block_pool+0x38u),"Live native resource remained above a restored frontier");
    }
    need(tracked==word(pool+0x3cu)&&blocks==word(block_pool+0x3cu),"Native resource pool census differs from retained rows");
    // Malformed actual owner metadata remains a strict error.
    const auto corrupt=[&](auto at,auto value){const auto saved=word(at);put(at,value);
        try{rejects([&]{(void)SourcePresentationResourceRegionsTu15(memory);});}catch(...){put(at,saved);throw;}put(at,saved);};
    corrupt(pool+0x24u,15u);corrupt(word(0x556d90u)+8u,table+16u);
    const auto graph_tag=std::ranges::find_if(cache.tags(),[&](const auto &tag){if(!tag.address||tag.group!="jmad")return false;
        const auto graph=word(table+(tag.datum&65535u)*16u+8u);return graph&&word(graph+0xacu)>0;});
    need(graph_tag!=cache.tags().end(),"Native descriptor bounds control lacks an actual graph");
    const auto graph_row=table+(graph_tag->datum&65535u)*16u,graph=word(graph_row+8u);
    corrupt(graph+0xb0u,graph+word(graph_row+12u));corrupt(graph_row+4u,graph_tag->datum^1u);
    rejects([&]{(void)resources.persistent(pool-1u,2u);});
    const nlohmann::json report{{"passed",true},{"map",argv[first]},{"native_resource_loads",loads},
        {"native_evictions",evictions},{"declared_spatial_bytes_restored",spatial_before.size()},
        {"live_trackers",tracked},{"live_lru_blocks",blocks},
        {"strict_owner_controls",5},{"legacy_journal",legacy}};
    if(unsigned(argc)>first+1u)std::ofstream(argv[first+1u])<<report.dump(2)<<'\n';
    std::cout<<report.dump()<<'\n';return 0;
}
}
int main(int argc,char **argv)try{return run(argc,argv);}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
