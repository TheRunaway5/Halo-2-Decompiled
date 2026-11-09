#include "halo2/native_tu15_island_partition_temporary.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
struct Sites{std::uint32_t stride,initial_data,initial_count,initial_flags,arena_cursor,arena_remaining,
  data,original_data,flags,count,destructor_cursor,destructor_remaining;};
Sites sites(NativeTu15IslandPartitionTemporaryKind kind){
  if(kind==NativeTu15IslandPartitionTemporaryKind::indices)return{4,0x2eba40,0x2eba42,0x2eba45,0x2eba80,0x2eba83,0x2eba89,0x2eba91,0x2eba94,0x2eba9b,0x2eb8ac,0x2eb8af};
  if(kind==NativeTu15IslandPartitionTemporaryKind::edges)return{8,0x317800,0x317802,0x317805,0x317840,0x317843,0x317849,0x317851,0x317854,0x31785b,0x3178cc,0x3178cf};
  throw std::runtime_error("Unknown native partition temporary element kind");
}
std::uint32_t read(SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
void write(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t site,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{site,p,{},{}};if(o){e.before.resize(4);m.read(p,e.before);e.written.assign(b.begin(),b.end());}m.write(p,b);if(o)o(e);}
bool negative(std::uint32_t v){return std::bit_cast<std::int32_t>(v)<0;}
template<class F>void require(const F&f,NativeTu15IslandLifetimeStats*stats,const char*why){if(!f){if(stats)++stats->unbound_services;throw std::runtime_error(why);}}
}
NativeTu15ArrayHeader native_tu15_construct_island_partition_temporary_with_phases(SourcePhysicsContactMemory&m,
 NativeTu15ArrayHeader header,const std::function<std::uint32_t()>&acquire_count,NativeTu15IslandPartitionTemporaryKind kind,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandLifetimeServices&s,
 const NativeTu15PartitionTemporaryPhases&phases,
 const SourcePhysicsContactRecordObserver&o,NativeTu15IslandLifetimeStats*stats){
  const auto pc=sites(kind);if(stats)++stats->temporary_constructors;const auto p=header.identity;
  NativeTu15PartitionTemporaryPhaseData phase;
  const auto notify=[&](NativeTu15PartitionTemporaryPhase stage){if(phases.observe)phases.observe(stage,phase);};
  write(m,o,pc.initial_data,p,0);write(m,o,pc.initial_count,p+4u,0);write(m,o,pc.initial_flags,p+8u,0x80000000u);
  notify(NativeTu15PartitionTemporaryPhase::initialized);
  const auto count=acquire_count();
  const auto arena=read(m,0x48c938u),remaining=read(m,arena+0xcu),bytes=(count*pc.stride+15u)&0xfffffff0u;std::uint32_t block{};
  phase.count=count;phase.arena=arena;phase.available=remaining;phase.bytes=bytes;
  notify(NativeTu15PartitionTemporaryPhase::allocation_inputs);
  if(std::bit_cast<std::int32_t>(bytes)>std::bit_cast<std::int32_t>(remaining)){
    const auto table=read(m,arena);phase.table=table;notify(NativeTu15PartitionTemporaryPhase::slow_allocation_table);
    const NativeTu15PhysicsVirtualOwner owner{read(m,table+0x24u)};
    require(s.allocate_arena,stats,"Native TU1.5 partition temporary requires actual arena allocation");if(stats)++stats->slow_allocations;block=s.allocate_arena(arena,owner,bytes,fp);
  }else{
    notify(NativeTu15PartitionTemporaryPhase::fast_allocation_begin);
    const auto cursor=read(m,arena+8u);block=cursor;phase.cursor=cursor;
    notify(NativeTu15PartitionTemporaryPhase::fast_allocation_inputs);
    write(m,o,pc.arena_cursor,arena+8u,cursor+bytes);write(m,o,pc.arena_remaining,arena+0xcu,remaining-bytes);if(stats)++stats->fast_allocations;
    notify(NativeTu15PartitionTemporaryPhase::fast_allocation_complete);
  }
  phase.block=block;notify(NativeTu15PartitionTemporaryPhase::allocation_complete);
  write(m,o,pc.data,p,block);notify(NativeTu15PartitionTemporaryPhase::data_published);
  write(m,o,pc.original_data,p+0xcu,block);write(m,o,pc.flags,p+8u,count|0x80000000u);
  notify(NativeTu15PartitionTemporaryPhase::capacity_published);
  write(m,o,pc.count,p+0x10u,count);return header;
}
NativeTu15ArrayHeader native_tu15_construct_island_partition_temporary(SourcePhysicsContactMemory&m,
 NativeTu15ArrayHeader header,std::uint32_t count,NativeTu15IslandPartitionTemporaryKind kind,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandLifetimeServices&s,
 const SourcePhysicsContactRecordObserver&o,NativeTu15IslandLifetimeStats*stats){
  return native_tu15_construct_island_partition_temporary_with_phases(m,header,[count]{return count;},kind,fp,s,{},o,stats);
}
void native_tu15_destroy_island_partition_temporary_with_phases(SourcePhysicsContactMemory&m,
 NativeTu15ArrayHeader header,NativeTu15IslandPartitionTemporaryKind kind,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15IslandLifetimeServices&s,const NativeTu15PartitionTemporaryPhases&phases,const SourcePhysicsContactRecordObserver&o,
 NativeTu15IslandLifetimeStats*stats){
  const auto pc=sites(kind);if(stats)++stats->temporary_destructors;const auto p=header.identity,count=read(m,p+0x10u),block=read(m,p+0xcu),arena=read(m,0x48c938u),bytes=(count*pc.stride+15u)&0xfffffff0u;
  NativeTu15PartitionTemporaryPhaseData phase;phase.count=count;phase.block=block;phase.arena=arena;phase.bytes=bytes;
  const auto notify=[&](NativeTu15PartitionTemporaryPhase stage){if(phases.observe)phases.observe(stage,phase);};
  notify(NativeTu15PartitionTemporaryPhase::release_inputs);
  bool slow=false;if(bytes){const auto boundary=read(m,arena+0x14u);phase.boundary=boundary;
    notify(NativeTu15PartitionTemporaryPhase::release_boundary);slow=block==boundary||!boundary;}
  if(slow){const auto table=read(m,arena);phase.table=table;notify(NativeTu15PartitionTemporaryPhase::slow_release_table);
    const NativeTu15PhysicsVirtualOwner owner{read(m,table+0x28u)};require(s.release_arena,stats,"Native TU1.5 partition temporary requires actual arena release");if(stats)++stats->slow_releases;s.release_arena(arena,owner,block,bytes,fp);}
  else{const auto cursor=read(m,arena+8u),remaining=read(m,arena+0xcu);phase.cursor=cursor;phase.available=remaining;
    notify(NativeTu15PartitionTemporaryPhase::fast_release_inputs);
    write(m,o,pc.destructor_cursor,arena+8u,cursor-bytes);write(m,o,pc.destructor_remaining,arena+0xcu,remaining+bytes);if(stats)++stats->fast_releases;}
  const auto flags=read(m,p+8u);phase.flags=flags;notify(NativeTu15PartitionTemporaryPhase::release_capacity);if(!negative(flags)){
    const auto allocator=read(m,0x48c938u),table=read(m,allocator);phase.allocator=allocator;phase.table=table;
    notify(NativeTu15PartitionTemporaryPhase::owned_release_allocator);
    const auto data=read(m,p);phase.data=data;notify(NativeTu15PartitionTemporaryPhase::owned_release_data);
    const auto owner=read(m,table+0x14u);
    require(s.allocation.release,stats,"Native TU1.5 partition temporary requires actual owned-array release");if(stats)++stats->owned_array_releases;s.allocation.release({allocator,owner,data,(flags&0x7fffffffu)*pc.stride,0x12u},fp);
  }
}
void native_tu15_destroy_island_partition_temporary(SourcePhysicsContactMemory&m,
 NativeTu15ArrayHeader header,NativeTu15IslandPartitionTemporaryKind kind,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15IslandLifetimeServices&s,const SourcePhysicsContactRecordObserver&o,NativeTu15IslandLifetimeStats*stats){
  native_tu15_destroy_island_partition_temporary_with_phases(m,header,kind,fp,s,{},o,stats);
}
} // namespace halo2
