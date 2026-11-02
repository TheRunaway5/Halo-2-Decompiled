#include "halo2/native_tu15_object_spatial_packet.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
  std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t out{};
  for(unsigned i=0;i<n;++i)out|=std::to_integer<unsigned>(b[i])<<(i*8u);return out;
}
void store(SourcePhysicsContactMemory&m,std::uint32_t site,std::uint32_t p,
    std::span<const std::byte>b,const SourcePhysicsContactRecordObserver&observe){
  SourcePhysicsContactRecordStore event{site,p,{},{}};
  if(observe){event.before.resize(b.size());m.read(p,event.before);event.written.assign(b.begin(),b.end());}
  m.write(p,b);if(observe)observe(event);
}
template<class Sink>void packet(SourcePhysicsContactMemory&m,std::uint32_t datum,
    Sink&&sink,NativeTu15ObjectSpatialPacketStats*stats){
  if(stats)++stats->packets;
  const auto manager=word(m,0x4ecbd0u),rows=word(m,manager+0x44u);
  const auto object=word(m,rows+(datum&0xffffu)*12u+8u);
  const auto object_flags=word(m,object+4u);std::uint32_t flags{};
  if(object_flags&(1u<<9u))flags=native_tu15_object_spatial_flags(m,datum,stats);
  const auto type=std::bit_cast<std::int8_t>(std::uint8_t(word(m,object+0xaau,1)));
  sink(0xba945u,2u,std::uint32_t(flags),2u);
  sink(0xba949u,0u,std::uint32_t(std::int32_t(type)),2u);
  auto value=word(m,object+0x40u);sink(0xba954u,4u,value,4u);
  value=word(m,object+0x44u);sink(0xba959u,8u,value,4u);
  value=word(m,object+0x48u);sink(0xba95fu,12u,value,4u);
  value=word(m,object+0x4cu);sink(0xba965u,16u,value,4u);
}
template<class Read>std::uint32_t update_record(SourcePhysicsContactMemory&m,
    std::uint32_t head,std::uint32_t key,std::uint32_t table,
    const std::function<std::uint32_t()>&count,Read&&read,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectSpatialPacketStats*stats){
  if(stats)++stats->record_searches;
  auto datum=word(m,head);if(datum==0xffffffffu){if(stats)++stats->missing_rows;return datum;}
  const auto stride=word(m,table+0x24u),rows=word(m,table+0x44u);
  for(;;){
    const auto row=rows+(datum&0xffffu)*stride;const auto current_key=word(m,row+4u);
    if(stats)++stats->searched_rows;
    if(current_key==key){
      if(stats)++stats->matched_rows;const auto bytes=count();
      read.begin(); // Original source parameter is fetched even for zero bytes.
      std::array<std::byte,4>b{};std::uint32_t offset{};
      for(std::uint32_t n=0;n<(bytes>>2u);++n){read(offset,b);store(m,0x2a0030u,row+12u+offset,b,observe);offset+=4u;if(stats)++stats->word_copies;}
      for(unsigned n=0;n<(bytes&3u);++n){read(offset,std::span(b).first(1));store(m,0x2a0037u,row+12u+offset,std::span(b).first(1),observe);++offset;if(stats)++stats->byte_copies;}
      if(!bytes&&stats)++stats->zero_copies;return row;
    }
    datum=word(m,row+8u);if(datum==0xffffffffu){if(stats)++stats->missing_rows;return datum;}
  }
}
struct IdentitySource {
  SourcePhysicsContactMemory&m;std::function<std::uint32_t()>parameter;std::uint32_t identity{};
  void begin(){identity=parameter();}
  void operator()(std::uint32_t offset,std::span<std::byte>b){m.read(identity+offset,b);}
};
struct HostSource {
  const NativeTu15ObjectSpatialPacketData&packet;
  void begin(){}
  void operator()(std::uint32_t offset,std::span<std::byte>b){
    if(std::uint64_t(offset)+b.size()>packet.bytes.size())throw std::runtime_error("Native TU1.5 spatial host packet extent");
    std::copy_n(packet.bytes.begin()+offset,b.size(),b.begin());
  }
};
template<class Source>void update_chain(SourcePhysicsContactMemory&m,
    std::uint32_t root,std::uint32_t datum,const NativeTu15SpatialChainParameters&parameters,
    Source&&source,const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectSpatialPacketStats*stats){
  if(stats)++stats->chain_updates;const auto bytes=parameters.bytes();
  while(datum!=0xffffffffu){
    const auto outer_table=word(m,root+8u),rows=word(m,outer_table+0x44u),inner_table=word(m,root+4u);
    const auto stride=word(m,outer_table+0x24u),row=rows+(datum&0xffffu)*stride;
    const auto slot=std::bit_cast<std::int16_t>(std::uint16_t(word(m,row+4u,2)));
    const auto heads=word(m,root),head=heads+std::uint32_t(std::int32_t(slot))*4u;
    const auto key=parameters.key();if(stats)++stats->chain_rows;
    (void)update_record(m,head,key,inner_table,[bytes]{return bytes;},source,observe,stats);
    datum=word(m,row+8u);
  }
}
}
std::uint32_t native_tu15_object_spatial_flags(SourcePhysicsContactMemory&m,
    std::uint32_t datum,NativeTu15ObjectSpatialPacketStats*stats){
  if(stats)++stats->flag_queries;
  const auto manager=word(m,0x4ecbd0u),rows=word(m,manager+0x44u),header=rows+(datum&0xffffu)*12u;
  const auto object=word(m,header+8u),header_flags=word(m,header+2u,1);
  const auto type=std::bit_cast<std::int8_t>(std::uint8_t(word(m,object+0xaau,1)));
  std::uint32_t out{};if((header_flags&0x10u)||(word(m,object+4u,1)&1u))out=1u;
  const auto flags=word(m,object+4u);
  if(flags&(1u<<20u))out|=2u;
  if(!(flags&(1u<<22u)))out|=4u;
  if(!(flags&(1u<<31u)))out|=8u;
  if(type==0){if(word(m,object+0x10au,1)&4u)out|=0x10u;if(word(m,object+0x348u,1)&8u)out|=0x20u;}
  else if(type==11){if(word(m,object+0x12cu)&2u)out|=0x40u;}
  return out;
}
void native_tu15_write_object_spatial_packet(SourcePhysicsContactMemory&m,
    std::uint32_t datum,std::uint32_t output,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ObjectSpatialPacketStats*stats){
  packet(m,datum,[&](auto site,auto offset,auto value,unsigned width){std::array<std::byte,4>b{};
    for(unsigned i=0;i<width;++i)b[i]=std::byte(value>>(8u*i));store(m,site,output+offset,std::span(b).first(width),observe);},stats);
}
NativeTu15ObjectSpatialPacketData native_tu15_build_object_spatial_packet(
    SourcePhysicsContactMemory&m,std::uint32_t datum,NativeTu15ObjectSpatialPacketStats*stats){
  NativeTu15ObjectSpatialPacketData out;
  packet(m,datum,[&](auto,auto offset,auto value,unsigned width){for(unsigned i=0;i<width;++i)out.bytes[offset+i]=std::byte(value>>(8u*i));},stats);return out;
}
std::uint32_t native_tu15_update_spatial_record(SourcePhysicsContactMemory&m,
    std::uint32_t head,std::uint32_t key,std::uint32_t table,
    const NativeTu15SpatialCopyParameters&parameters,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ObjectSpatialPacketStats*stats){
  return update_record(m,head,key,table,parameters.bytes,IdentitySource{m,parameters.source},observe,stats);
}
void native_tu15_update_object_spatial_chain(SourcePhysicsContactMemory&m,
    std::uint32_t root,std::uint32_t first,std::uint32_t source,
    const NativeTu15SpatialChainParameters&parameters,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ObjectSpatialPacketStats*stats){
  update_chain(m,root,first,parameters,IdentitySource{m,[source]{return source;}},observe,stats);
}
void native_tu15_update_object_spatial_chain(SourcePhysicsContactMemory&m,
    std::uint32_t root,std::uint32_t first,std::uint32_t key,std::uint32_t bytes,
    std::uint32_t source,const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectSpatialPacketStats*stats){
  native_tu15_update_object_spatial_chain(m,root,first,source,{[key]{return key;},[bytes]{return bytes;}},observe,stats);
}
void native_tu15_update_object_spatial_chain(SourcePhysicsContactMemory&m,
    std::uint32_t root,std::uint32_t first,std::uint32_t key,
    const NativeTu15ObjectSpatialPacketData&data,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ObjectSpatialPacketStats*stats){
  update_chain(m,root,first,{[key]{return key;},[]{return 20u;}},HostSource{data},observe,stats);
}
} // namespace halo2
