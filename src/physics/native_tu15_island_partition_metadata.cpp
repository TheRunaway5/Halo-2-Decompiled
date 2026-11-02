#include "halo2/native_tu15_island_partition_metadata.hpp"
#include <bit>
namespace halo2 {
namespace {
class Metadata {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
  Metadata(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
  std::uint32_t read(std::uint32_t p){std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t v{};
    for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
  void write(std::uint32_t site,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};
    for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{site,p,{},{}};
    if(o_){e.before.resize(4);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}
    m_.write(p,b);if(o_)o_(e);
  }
};
bool less_equal(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<=std::bit_cast<std::int32_t>(b);}
}
void native_tu15_add_island_constraint_info(SourcePhysicsContactMemory&memory,NativeTu15PhysicsIsland island,
 NativeTu15IslandConstraintInfo info,const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandPartitionMetadataStats*stats){
  if(stats)++stats->additions;Metadata m(memory,observe);auto maximum=m.read(info.identity),current=m.read(island.identity+8u);
  if(!less_equal(current,maximum))maximum=current;m.write(0x3200c0u,island.identity+8u,maximum);
  const auto next=m.read(info.identity+4u);if(less_equal(maximum,next))maximum=next;
  current=m.read(island.identity+0xcu);m.write(0x3200cfu,island.identity+8u,maximum);
  const auto rows=m.read(info.identity+4u);m.write(0x3200d7u,island.identity+0xcu,current+rows);
  const auto atoms=m.read(info.identity+8u),old_atoms=m.read(island.identity+0x10u),old_aux=m.read(island.identity+0x18u);
  m.write(0x3200e5u,island.identity+0x10u,old_atoms+atoms);const auto aux=m.read(info.identity+0xcu);
  m.write(0x3200edu,island.identity+0x18u,old_aux+aux);
}
void native_tu15_remove_island_constraint_info(SourcePhysicsContactMemory&memory,NativeTu15PhysicsIsland island,
 NativeTu15IslandConstraintInfo info,const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandPartitionMetadataStats*stats){
  if(stats)++stats->removals;Metadata m(memory,observe);const auto rows=m.read(info.identity+4u),old_rows=m.read(island.identity+0xcu);
  m.write(0x320108u,island.identity+0xcu,old_rows-rows);const auto atoms=m.read(info.identity+8u),old_atoms=m.read(island.identity+0x10u),old_aux=m.read(island.identity+0x18u);
  m.write(0x320116u,island.identity+0x10u,old_atoms-atoms);const auto aux=m.read(info.identity+0xcu);
  m.write(0x32011eu,island.identity+0x18u,old_aux-aux);
}
void native_tu15_add_island_partition_weight(SourcePhysicsContactMemory&memory,NativeTu15PhysicsIsland island,
 std::uint32_t weight,const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandPartitionMetadataStats*stats){
  if(stats)++stats->weight_updates;Metadata m(memory,observe);const auto before=m.read(island.identity+0x14u);
  m.write(0x320139u,island.identity+0x14u,before+weight);
}
std::uint32_t native_tu15_default_motion_partition_weight()noexcept{return 0x80u;}
} // namespace halo2
