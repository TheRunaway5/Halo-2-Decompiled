#include "halo2/native_tu15_island_lifetime.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));
    std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::uint32_t v){
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{pc,p,{},{}};
    if(o){e.before.resize(4);m.read(p,e.before);e.written.assign(b.begin(),b.end());}m.write(p,b);if(o)o(e);
}
bool negative(std::uint32_t v){return std::bit_cast<std::int32_t>(v)<0;}
template<class F>void require(const F&f,NativeTu15IslandLifetimeStats*stats,const char*message){if(!f){if(stats)++stats->unbound_services;throw std::runtime_error(message);}}
}
void native_tu15_construct_island_temporary_records(SourcePhysicsContactMemory&m,NativeTu15ArrayHeader header,
 std::uint32_t count,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandLifetimeServices&s,
 const SourcePhysicsContactRecordObserver&o,NativeTu15IslandLifetimeStats*stats){
    if(stats)++stats->temporary_constructors;const auto p=header.identity;
    store(m,o,0x317a60u,p,0);store(m,o,0x317a62u,p+4u,0);store(m,o,0x317a65u,p+8u,0x80000000u);
    const auto arena=read(m,0x48c938u),remaining=read(m,arena+0xcu),bytes=(count*16u+15u)&0xfffffff0u;std::uint32_t block{};
    if(std::bit_cast<std::int32_t>(bytes)>std::bit_cast<std::int32_t>(remaining)){
        const auto table=read(m,arena);const NativeTu15PhysicsVirtualOwner owner{read(m,table+0x24u)};
        require(s.allocate_arena,stats,"Native TU1.5 island temporary requires actual arena allocation");
        if(stats)++stats->slow_allocations;block=s.allocate_arena(arena,owner,bytes,fp);
    }else{
        const auto previous=read(m,arena+8u);block=previous;store(m,o,0x317aa1u,arena+8u,previous+bytes);
        store(m,o,0x317aa4u,arena+0xcu,remaining-bytes);if(stats)++stats->fast_allocations;
    }
    store(m,o,0x317aaau,p,block);store(m,o,0x317ab2u,p+0xcu,block);
    store(m,o,0x317ab5u,p+8u,count|0x80000000u);store(m,o,0x317abcu,p+0x10u,count);
}
void native_tu15_destroy_island_temporary_records(SourcePhysicsContactMemory&m,NativeTu15ArrayHeader header,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandLifetimeServices&s,const SourcePhysicsContactRecordObserver&o,
 NativeTu15IslandLifetimeStats*stats){
    if(stats)++stats->temporary_destructors;const auto p=header.identity,count=read(m,p+0x10u),block=read(m,p+0xcu),arena=read(m,0x48c938u);
    const auto bytes=(count*16u+15u)&0xfffffff0u;bool slow=false;
    if(bytes){const auto boundary=read(m,arena+0x14u);slow=block==boundary||!boundary;}
    if(slow){
        const auto table=read(m,arena);const NativeTu15PhysicsVirtualOwner owner{read(m,table+0x28u)};
        require(s.release_arena,stats,"Native TU1.5 island temporary requires actual arena release");
        if(stats)++stats->slow_releases;s.release_arena(arena,owner,block,bytes,fp);
    }else{
        const auto previous=read(m,arena+8u),remaining=read(m,arena+0xcu);
        store(m,o,0x317b2bu,arena+8u,previous-bytes);store(m,o,0x317b2eu,arena+0xcu,remaining+bytes);
        if(stats)++stats->fast_releases;
    }
    const auto flags=read(m,p+8u);if(!negative(flags)){
        const auto allocator=read(m,0x48c938u),table=read(m,allocator),data=read(m,p);const auto owner=read(m,table+0x14u);
        require(s.allocation.release,stats,"Native TU1.5 island temporary requires actual owned-array release");
        if(stats)++stats->owned_array_releases;s.allocation.release({allocator,owner,data,(flags&0x7fffffffu)*16u,0x12u},fp);
    }
}
std::uint32_t native_tu15_destroy_physics_island(SourcePhysicsContactMemory&m,NativeTu15PhysicsIsland island,
 std::uint32_t disposing,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandLifetimeServices&s,
 const SourcePhysicsContactRecordObserver&o,NativeTu15IslandLifetimeStats*stats){
    if(stats)++stats->island_destructors;const auto p=island.identity;
    store(m,o,0x3213c0u,p,0x41f080u);store(m,o,0x3213c6u,p+0x1cu,0x41f074u);
    constexpr std::array<std::uint32_t,7>capacities{0x94u,0x88u,0x7cu,0x70u,0x60u,0x54u,0x44u};
    constexpr std::array<std::uint32_t,7>pointers{0x8cu,0x80u,0x74u,0x68u,0x58u,0x4cu,0x3cu};
    constexpr std::array<std::uint32_t,7>strides{8u,8u,8u,16u,4u,4u,4u};
    for(unsigned n=0;n<capacities.size();++n){const auto capacity=read(m,p+capacities[n]);if(negative(capacity))continue;
        std::uint32_t data{},allocator{},table{};
        if(n==6){allocator=read(m,0x48c938u);table=read(m,allocator);data=read(m,p+pointers[n]);}
        else{data=read(m,p+pointers[n]);allocator=read(m,0x48c938u);table=read(m,allocator);}
        const auto owner=read(m,table+0x14u);require(s.allocation.release,stats,"Native TU1.5 island destructor requires actual owned-array release");
        if(stats)++stats->owned_array_releases;s.allocation.release({allocator,owner,data,(capacity&0x7fffffffu)*strides[n],0x12u},fp);
    }
    store(m,o,0x3214eau,p+0x1cu,0x41d92cu);store(m,o,0x3214f1u,p,0x45edf0u);
    if(disposing&1u){
        const auto bytes=read(m,p+4u,2),allocator=read(m,0x48c938u),table=read(m,allocator),owner=read(m,table+0x14u);
        require(s.allocation.release,stats,"Native TU1.5 island disposal requires actual allocation release");
        if(stats)++stats->island_disposals;s.allocation.release({allocator,owner,p,bytes,0x2du},fp);
    }
    return p;
}
} // namespace halo2
