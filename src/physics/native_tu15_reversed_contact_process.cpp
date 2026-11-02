#include "halo2/native_tu15_reversed_contact_process.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
  Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
  std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
  std::array<std::byte,16> vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native reversed contact requires source movaps alignment");std::array<std::byte,16>b{};m_.read_vector(p,b);return b;}
  void vector(std::uint32_t site,std::uint32_t p,const std::array<std::byte,16>&b){
    if(p&15u)throw std::runtime_error("Native reversed contact requires source movaps store alignment");SourcePhysicsContactRecordStore event{site,p,{},{}};
    if(o_){event.before.resize(16);m_.read(p,event.before);event.written.assign(b.begin(),b.end());}
    m_.write(p,b);if(o_)o_(event);
  }
};
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
void process(SourcePhysicsContactMemory&memory,NativeTu15IslandCollisionAgent agent,
    NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration configuration,
    NativeTu15IslandContactOutput output,NativeTu15PhysicsFrameFp&fp,const NativeTu15CompoundProcessServices&s,
    NativeTu15ReversedContactKind kind,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ReversedContactProcessStats*stats){
  Memory m(memory,observe);if(stats)++stats->calls;
  const auto header=m.word(output.identity),old_count=m.word(header+4u);
  if(!s.process_child)throw std::runtime_error("Native reversed contact requires its actual typed child owner");
  if(stats)++stats->child_calls;
  s.process_child({agent,{kind==NativeTu15ReversedContactKind::wrapped?0x2f6ca0u:0x2f7a50u},
    {second.identity},{first.identity},configuration,output},fp);
  if(stats)++stats->child_returns;
  native_tu15_reverse_appended_contact_normals(memory,output,old_count,kind,observe,stats);
}
}
void native_tu15_reverse_appended_contact_normals(SourcePhysicsContactMemory&memory,
    NativeTu15IslandContactOutput output,std::uint32_t first_index,NativeTu15ReversedContactKind kind,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ReversedContactProcessStats*stats){
  Memory m(memory,observe);auto header=m.word(output.identity);if(!less(first_index,m.word(header+4u)))return;
  auto offset=first_index*48u;
  do{
    const auto data=m.word(header),normal=data+offset+16u;auto value=m.vector(normal);const auto mask=m.vector(0x461960u);
    for(unsigned n=0;n<16;++n)value[n]^=mask[n];
    m.vector(kind==NativeTu15ReversedContactKind::wrapped?0x2f74c7u:0x2f7ff7u,normal,value);if(stats)++stats->normals_reversed;
    header=m.word(output.identity);const auto next_count=m.word(header+4u);++first_index;offset+=48u;if(!less(first_index,next_count))break;
  }while(true);
}
void native_tu15_process_reversed_wrapped_contact_agent(SourcePhysicsContactMemory&m,
    NativeTu15IslandCollisionAgent agent,NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
    NativeTu15PhysicsStepConfiguration configuration,NativeTu15IslandContactOutput output,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15CompoundProcessServices&s,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ReversedContactProcessStats*stats){
  process(m,agent,first,second,configuration,output,fp,s,NativeTu15ReversedContactKind::wrapped,observe,stats);
}
void native_tu15_process_reversed_compound_contact_agent(SourcePhysicsContactMemory&m,
    NativeTu15IslandCollisionAgent agent,NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
    NativeTu15PhysicsStepConfiguration configuration,NativeTu15IslandContactOutput output,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15CompoundProcessServices&s,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ReversedContactProcessStats*stats){
  process(m,agent,first,second,configuration,output,fp,s,NativeTu15ReversedContactKind::compound,observe,stats);
}
} // namespace halo2
