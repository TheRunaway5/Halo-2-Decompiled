#include "halo2/source_decal_frame_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

using namespace halo2;
namespace {
constexpr std::uint32_t none=0xffffffffu,clock_address=0x02000000u;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address,unsigned count=4) {
    const auto bytes=memory.view(address,count);std::uint32_t out{};
    for(unsigned n=0;n<count;++n)out|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return out;
}
void store(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value,unsigned count=4) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<count;++n)bytes[n]=std::byte(value>>(n*8u));
    memory.write(address,std::span(bytes).first(count));
}
void clock_input(SourceRuntimeMemory& memory,std::uint32_t tick) {
    std::array<std::byte,4> pointer{};
    for(unsigned n=0;n<4;++n)pointer[n]=std::byte(clock_address>>(n*8u));
    memory.mount(0x51df54u,pointer,SourceRuntimeMemory::Access::ReadWrite,"Declared test clock owner pointer");
    const std::array<std::byte,12> clock{};
    memory.mount(clock_address,clock,SourceRuntimeMemory::Access::ReadWrite,"Declared test clock input; absent from captured fixture");
    store(memory,clock_address+4u,std::bit_cast<std::uint32_t>(1.f/30.f));
    store(memory,clock_address+8u,tick);
}
SourceRuntimeMemory fixture_memory(const std::filesystem::path& path,const nlohmann::json& fixture) {
    SourceRuntimeMemory memory;
    for(const auto& row:fixture.at("regions")) {
        std::vector<std::byte> bytes;
        if(row.contains("file")) {
            std::ifstream input(path.parent_path()/row.at("file").get<std::string>(),std::ios::binary);
            if(!input)throw std::runtime_error("Completed original decal fixture backing is missing");
            for(char c;input.get(c);)bytes.push_back(std::byte(static_cast<unsigned char>(c)));
        } else {
            const auto value=row.at("value").get<std::uint32_t>();
            for(unsigned n=0;n<4;++n)bytes.push_back(std::byte(value>>(n*8u)));
        }
        memory.mount(row.at("address"),bytes,SourceRuntimeMemory::Access::ReadWrite,"Authentic completed original decal fixture");
    }
    return memory;
}
const SourceDecalFrameAllocationTu15& allocation(const SourceDecalFrameTu15& frame,std::uint32_t token) {
    const auto at=std::find_if(frame.allocations.begin(),frame.allocations.end(),[&](const auto& a){return a.cache_datum==token;});
    if(at==frame.allocations.end())throw std::runtime_error("Missing frame allocation");
    return *at;
}
bool same_float(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same_vertex(const SourceDecalCacheVertex& a,const SourceDecalCacheVertex& b) {
    return a.packed_projection==b.packed_projection && same_float(a.position.x,b.position.x) &&
        same_float(a.position.y,b.position.y) && same_float(a.position.z,b.position.z) &&
        std::ranges::equal(a.projection,b.projection,[](float x,float y){return same_float(x,y);});
}
template<class F> void rejected(F&& function,const char* message) {
    bool failed=false;try{function();}catch(const std::exception&){failed=true;}
    require(failed,message);
}
std::uint32_t record_address(const SourceRuntimeMemory& memory,unsigned slot) {
    return word(memory,word(memory,0x4f7220u)+0x44u)+slot*64u;
}
std::uint32_t first_vertex(const SourceRuntimeMemory& memory,std::uint32_t token) {
    const auto owner=word(memory,0x515f28u),pool=word(memory,owner+0x64u);
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    const auto page=source_raw_datum_lookup(pool,token,storage);
    require(page!=0,"Fixture cache token is not live");
    return (word(memory,word(memory,0x515f24u)+4u)|0x80000000u)+word(memory,page+8u)*64u;
}
void check_original_capture(const SourceRuntimeMemory& memory,const SourceDecalFrameTu15& frame) {
    const auto original=source_decal_vertex_cache_tu15(memory);
    require(frame.records.size()==28 && frame.allocations.size()==20 && frame.heads.size()==17,
        "Authentic original record/allocation/head counts changed");
    require(original.size()==frame.records.size(),"Frame lost original records");
    std::size_t unique_quads{},record_quads{};
    for(const auto& a:frame.allocations)unique_quads+=a.quads.size();
    require(unique_quads==22,"Aliases duplicated original vertex pages");
    for(std::size_t n=0;n<original.size();++n) {
        const auto& a=original[n];const auto& b=frame.records[n];
        require(a.datum==b.datum && a.definition==b.definition && a.cache_datum==b.cache_datum &&
            a.tick==b.tick && a.tint==b.tint && a.cluster==b.cluster && a.layer==b.layer &&
            a.flags==b.flags && same_float(a.lifetime,b.lifetime) && same_float(a.decay,b.decay) &&
            a.primary_datum==b.primary_datum && a.next_cluster_datum==b.next_cluster_datum &&
            a.bitmap_index==b.bitmap_index && a.quads.size()==b.quad_count,
            "Frame changed original retained metadata");
        record_quads+=b.quad_count;
        const auto& pages=allocation(frame,b.cache_datum).quads;
        const auto raw=first_vertex(memory,b.cache_datum);
        for(std::size_t q=0;q<a.quads.size();++q)for(unsigned v=0;v<4;++v) {
            require(same_vertex(a.quads[q][v],pages[q][v]),"Frame changed original XYZ/CMP projection values");
            const auto at=raw+std::uint32_t(q)*64u+v*16u;
            require(std::bit_cast<std::uint32_t>(pages[q][v].position.x)==word(memory,at) &&
                std::bit_cast<std::uint32_t>(pages[q][v].position.y)==word(memory,at+4u) &&
                std::bit_cast<std::uint32_t>(pages[q][v].position.z)==word(memory,at+8u) &&
                pages[q][v].packed_projection==word(memory,at+12u),"Frame failed raw original vertex-byte preservation");
        }
    }
    require(record_quads==32,"Original retained prefix quad count changed");
    const auto context=word(memory,0x4f721cu);
    for(const auto& head:frame.heads)require(head.datum==word(memory,context+(head.layer*512u+head.cluster)*4u),
        "Frame changed the original nonempty head bank");
}
}

int main(int argc,char** argv)try {
    if(argc!=2)throw std::runtime_error("Supply the authentic completed decal cache fixture");
    std::ifstream input(argv[1]);if(!input)return 77;
    nlohmann::json fixture;input>>fixture;
    for(const auto& row:fixture.at("regions"))if(row.contains("file") &&
        !std::filesystem::is_regular_file(std::filesystem::path(argv[1]).parent_path()/row.at("file").get<std::string>()))return 77;
    auto memory=fixture_memory(argv[1],fixture);
    rejected([&]{(void)source_decal_frame_tu15(memory);},"Capture invented a missing clock owner");
    clock_input(memory,120);
    const auto frame=source_decal_frame_tu15(memory);
    require(frame.tick==120,"Frame did not retain declared original clock input");
    check_original_capture(memory,frame);
    const auto repeated=source_decal_frame_tu15(memory);
    require(repeated.tick==frame.tick && repeated.records.size()==frame.records.size(),"Quiescent capture is not repeatable");
    for(const auto& a:frame.allocations) {
        const auto& b=allocation(repeated,a.cache_datum);
        require(a.quads.size()==b.quads.size(),"Repeated allocation prefix changed");
        for(std::size_t q=0;q<a.quads.size();++q)for(unsigned v=0;v<4;++v)
            require(same_vertex(a.quads[q][v],b.quads[q][v]),"Read-only repeat changed owned vertex bytes");
    }
    // A later source frame can reuse the original physical pages. The older
    // value must retain its own bytes and its original game clock.
    const auto vertex=first_vertex(memory,frame.records.front().cache_datum);
    const auto old=allocation(frame,frame.records.front().cache_datum).quads[0][0];
    store(memory,vertex,std::bit_cast<std::uint32_t>(old.position.x+.25f));
    store(memory,vertex+12u,old.packed_projection^1u);
    store(memory,clock_address+8u,121);
    const auto later=source_decal_frame_tu15(memory);
    require(frame.tick==120 && later.tick==121 && same_vertex(old,allocation(frame,frame.records.front().cache_datum).quads[0][0]),
        "An older frame borrowed mutable source storage");
    require(!same_vertex(old,allocation(later,frame.records.front().cache_datum).quads[0][0]),"Later capture ignored changed source pages");

    for(bool short_first:{false,true}) {
        auto aliases=fixture_memory(argv[1],fixture);clock_input(aliases,122);
        store(aliases,record_address(aliases,7)+0x2au,short_first?1u:3u,2);
        store(aliases,record_address(aliases,8)+0x2au,short_first?3u:1u,2);
        const auto captured=source_decal_frame_tu15(aliases);
        require(captured.records[7].cache_datum==captured.records[8].cache_datum && captured.allocations.size()==20 &&
            allocation(captured,captured.records[7].cache_datum).quads.size()==3,
            "Within-frame aliases did not retain exactly the longest actual prefix");
        require(captured.records[7].quad_count==(short_first?1u:3u) && captured.records[8].quad_count==(short_first?3u:1u),
            "Alias prefix deduplication changed a retained record draw count");
    }
    auto banks=fixture_memory(argv[1],fixture);clock_input(banks,123);
    const auto context=word(banks,0x4f721cu);
    const std::vector<std::byte> empty(7u*512u*4u,std::byte{255});banks.write(context,empty);
    for(unsigned layer=0;layer<7;++layer) {
        const auto address=record_address(banks,layer),datum=(word(banks,address,2)<<16u)|layer;
        store(banks,address+3u,layer,1);store(banks,address+0xau,511u-layer,2);
        store(banks,address+0x34u,none);store(banks,address+0x38u,datum);
        store(banks,context+(layer*512u+511u-layer)*4u,datum);
    }
    const auto seven=source_decal_frame_tu15(banks);
    require(seven.heads.size()==7,"Capture omitted an original head layer");
    for(unsigned layer=0;layer<7;++layer)require(seven.heads[layer].layer==layer && seven.heads[layer].cluster==511u-layer,
        "Capture changed one of the seven original head banks");

    const auto invalid=[&](auto change,const char* message) {auto bad=frame;change(bad);rejected([&]{validate_source_decal_frame_tu15(bad);},message);};
    invalid([](auto& f){f.heads.front().datum^=0x10000u;},"Stale full head salt was admitted");
    invalid([](auto& f){f.records.front().cache_datum^=0x10000u;},"Stale full cache salt was admitted");
    invalid([](auto& f){const auto handle=f.heads.front().datum;for(auto& r:f.records)if(r.datum==handle)r.next_cluster_datum=handle;},"Cyclic source head list was admitted");
    invalid([](auto& f){const auto handle=f.heads.front().datum;for(auto& r:f.records)if(r.datum==handle)r.primary_datum^=0x10000u;},"Stale full primary salt was admitted");
    invalid([](auto& f){f.records.push_back(f.records.front());},"Duplicate live record ordinal was admitted");
    invalid([](auto& f){f.allocations.push_back(f.allocations.front());},"Duplicate full allocation token was admitted");
    invalid([](auto& f){f.records.resize(1025,f.records.front());},"Original record capacity overflow was admitted");
    invalid([](auto& f){f.allocations.resize(1025,f.allocations.front());},"Original allocation count overflow was admitted");
    invalid([](auto& f){f.allocations.front().quads.resize(1281,f.allocations.front().quads.front());},"Original aggregate page capacity overflow was admitted");
    invalid([](auto& f){f.heads.resize(3585,f.heads.front());},"Original head-bank capacity overflow was admitted");
    invalid([](auto& f){f.records.front().quad_count=65535;},"Record prefix exceeded its retained allocation");
    invalid([](auto& f){f.heads.front().layer=7;},"Eighth head layer was admitted");
    invalid([](auto& f){f.heads.front().cluster=512;},"Head cluster outside its source bank was admitted");
    invalid([](auto& f){f.heads.front().cluster=511;},"Head cluster differed from its original linked record");
    invalid([](auto& f){const auto old=f.allocations.front().cache_datum,token=(old&0xffff0000u)|1024u;f.allocations.front().cache_datum=token;for(auto& r:f.records)if(r.cache_datum==old)r.cache_datum=token;},"Cache allocation ordinal outside the original pool was admitted");
    invalid([](auto& f){const auto old=f.allocations[1].cache_datum,token=(old&0xffff0000u)|(f.allocations[0].cache_datum&65535u);f.allocations[1].cache_datum=token;for(auto& r:f.records)if(r.cache_datum==old)r.cache_datum=token;},"Two live cache salts occupied the same source ordinal");
    auto stale=fixture_memory(argv[1],fixture);clock_input(stale,124);
    store(stale,record_address(stale,0)+4u,word(stale,record_address(stale,0)+4u)^0x10000u);
    rejected([&]{(void)source_decal_frame_tu15(stale);},"Capture accepted a retired full raw cache token");
    const auto packed=source_decal_frame_vertex_tu15({-0.f,2.f,3.f},1024u|(1024u<<11u)|(512u<<22u));
    require(same_float(packed.position.x,-0.f) && packed.projection[0]==-1024.f/1023.f &&
        packed.projection[1]==-1024.f/1023.f && packed.projection[2]==-512.f/511.f,
        "Signed 11/11/10 extreme decode changed source raw bits");
    std::cout<<"PASS authentic28 records/20 allocations/22 unique quads, raw XYZ/CMP, prefix aliases, owning clocks, seven head banks and malformed identity/capacity controls\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
