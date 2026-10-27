#include "halo2/native_tu15_broad_phase.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
class Memory {
 SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
 Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
 std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;}
 void word(std::uint32_t site,std::uint32_t p,std::uint32_t v,unsigned n=4){std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8u*i));SourcePhysicsContactRecordStore event{site,p,{},{}};if(o_){event.before.resize(n);m_.read(p,event.before);event.written.assign(b.begin(),b.begin()+n);}m_.write(p,std::span<const std::byte>(b).first(n));if(o_)o_(event);}
};
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
bool positive(std::uint32_t n){return less(0,n);}
std::uint32_t align16(std::uint32_t n){return(n+15u)&0xfffffff0u;}
template<class F>void require(const F&f,const char*why){if(!f)throw std::runtime_error(why);}
std::uint32_t allocate(Memory&m,std::uint32_t bytes,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15BroadPhaseServices&s,std::uint32_t cursor_site,std::uint32_t remaining_site){
 const NativeTu15BroadPhaseAllocator allocator{m.word(0x48c938u)};const auto remaining=m.word(allocator.identity+0xcu);
 if(less(remaining,bytes)){
  const auto table=m.word(allocator.identity);const NativeTu15PhysicsVirtualOwner owner{m.word(table+0x24u)};
  require(s.allocate_block,"Native TU1.5 broad phase requires actual arena allocation owner");return s.allocate_block(allocator,owner,bytes,fp);
 }
 const auto result=m.word(allocator.identity+8u);m.word(cursor_site,allocator.identity+8u,result+bytes);m.word(remaining_site,allocator.identity+0xcu,remaining-bytes);return result;
}
void release(Memory&m,std::uint32_t block,std::uint32_t bytes,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15BroadPhaseServices&s,std::uint32_t cursor_site,std::uint32_t remaining_site){
 const NativeTu15BroadPhaseAllocator allocator{m.word(0x48c938u)};
 if(bytes){const auto boundary=m.word(allocator.identity+0x14u);if(block==boundary||!boundary){
  const auto table=m.word(allocator.identity);const NativeTu15PhysicsVirtualOwner owner{m.word(table+0x28u)};require(s.release_block,"Native TU1.5 broad phase requires actual arena release owner");s.release_block(allocator,owner,block,bytes,fp);return;
 }}
 const auto cursor=m.word(allocator.identity+8u),remaining=m.word(allocator.identity+0xcu);
 m.word(cursor_site,allocator.identity+8u,cursor-bytes);m.word(remaining_site,allocator.identity+0xcu,remaining+bytes);
}
std::uint32_t profile(Memory&m,std::uint32_t cursor,std::uint32_t limit,unsigned phase,const NativeTu15BroadPhaseServices&s){
 if(cursor>=limit)return cursor;
 constexpr std::array labels{0x41f0c0u,0x41f0b4u,0x41f0a4u,0x41f098u,0x41cabcu};
 constexpr std::array label_sites{0x321733u,0x321823u,0x32189fu,0x3218efu,0x321a1fu};
 constexpr std::array time_sites{0x321743u,0x321833u,0x3218afu,0x3218ffu,0x321a2fu};
 constexpr std::array cursor_sites{0x321749u,0x321839u,0x3218b5u,0x321905u,0x321a35u};
 m.word(label_sites[phase],cursor,labels[phase]);require(s.timestamp_counter,"Native TU1.5 broad phase requires actual timestamp source");m.word(time_sites[phase],cursor+4u,std::uint32_t(s.timestamp_counter()));cursor+=12u;m.word(cursor_sites[phase],0x48c974u,cursor);return cursor;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class NumericEnvironment {
 alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
 NumericEnvironment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
  if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 broad-phase numeric fault owner unavailable");
  mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);const std::uint16_t tags=0xffff;std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&tags,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
 }
 ~NumericEnvironment(){asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr_);}
};
#endif
}
void native_tu15_construct_broad_phase_pairs(SourcePhysicsContactMemory&memory,
 NativeTu15BroadPhasePairArray array,std::uint32_t reserve,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15BroadPhaseServices&s,const SourcePhysicsContactRecordObserver&observe){
 Memory m(memory,observe);const auto p=array.identity;m.word(0x317800u,p,0);m.word(0x317802u,p+4u,0);m.word(0x317805u,p+8u,0x80000000u);
 const auto data=allocate(m,align16(reserve*8u),fp,s,0x317840u,0x317843u);
 m.word(0x317849u,p,data);m.word(0x317851u,p+0xcu,data);m.word(0x317854u,p+8u,reserve|0x80000000u);m.word(0x31785bu,p+0x10u,reserve);
}
void native_tu15_destroy_broad_phase_pairs(SourcePhysicsContactMemory&memory,
 NativeTu15BroadPhasePairArray array,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15BroadPhaseServices&s,const SourcePhysicsContactRecordObserver&observe){
 Memory m(memory,observe);const auto p=array.identity;const auto reserve=m.word(p+0x10u),block=m.word(p+0xcu);
 release(m,block,align16(reserve*8u),fp,s,0x3178ccu,0x3178cfu);
 const auto capacity=m.word(p+8u);if(!(capacity&0x80000000u)){
  const NativeTu15BroadPhaseAllocator allocator{m.word(0x48c938u)};const auto table=m.word(allocator.identity);const auto data=m.word(p);const NativeTu15PhysicsVirtualOwner owner{m.word(table+0x14u)};
  require(s.release_owned_pairs,"Native TU1.5 broad phase requires actual owned pair-array release");s.release_owned_pairs(allocator,owner,data,(capacity&0x7fffffffu)*8u,0x12u,fp);
 }
}
void native_tu15_update_broad_phase(SourcePhysicsContactMemory&memory,NativeTu15PhysicsIsland island,
 NativeTu15CollisionDispatcher dispatcher,NativeTu15PhysicsStepConfiguration configuration,
 NativeTu15BroadPhaseScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
 const NativeTu15BroadPhaseServices&s,const SourcePhysicsContactRecordObserver&observe){
 (void)configuration;Memory m(memory,observe);const auto p=island.identity;
 auto limit=m.word(0x48c978u),cursor=m.word(0x48c974u);profile(m,cursor,limit,0,s);
 native_tu15_construct_broad_phase_pairs(memory,scratch.first,m.word(m.word(p+0x20u)+0x238u),fp,s,observe);
 native_tu15_construct_broad_phase_pairs(memory,scratch.second,m.word(m.word(p+0x20u)+0x238u),fp,s,observe);
 const auto allocation_count=m.word(p+0x40u);const auto bound_bytes=align16(allocation_count*32u);
 const auto bounds=allocate(m,bound_bytes,fp,s,0x3217b5u,0x3217b8u);const auto entry_bytes=align16(allocation_count*4u);
 const auto entries=allocate(m,entry_bytes,fp,s,0x3217eeu,0x3217f1u);
 const auto world=m.word(p+0x20u);const auto gravity=m.word(world+0xccu);std::uint32_t margin{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 {
  NumericEnvironment environment(fp,depth);const auto gravity_bits=m.word(gravity+8u);asm volatile("flds %0"::"m"(gravity_bits):"memory","st");cursor=m.word(0x48c974u);limit=m.word(0x48c978u);const auto scale=m.word(0x46a0ccu);
  asm volatile("fmuls %1\n\tfstps %0":"=m"(margin):"m"(scale):"memory","st");
 }
#else
 (void)depth;throw std::runtime_error("Exact native TU1.5 broad-phase numeric backend unavailable");
#endif
 cursor=profile(m,cursor,limit,1,s);
 auto body_count=m.word(p+0x40u);auto body_cursor=m.word(p+0x3cu);auto bound_cursor=bounds,entry_cursor=entries;
 if(std::bit_cast<std::int32_t>(body_count-1u)>=0){
  do{
   const auto shape_field=m.word(body_cursor)+0xcu;const auto broad_entry=shape_field?shape_field+0x10u:0u;
   m.word(0x321863u,entry_cursor,broad_entry);const NativeTu15BroadPhaseShape shape{m.word(shape_field)};const auto transform=m.word(shape_field+8u);const auto table=m.word(shape.identity);const NativeTu15PhysicsVirtualOwner owner{m.word(table+0x18u)};
   require(s.shape_bounds,"Native TU1.5 broad phase requires actual shape bound owner");s.shape_bounds(shape,owner,transform+0x20u,margin,bound_cursor,fp);
   bound_cursor+=32u;body_cursor+=4u;entry_cursor+=4u;--body_count;
  }while(body_count);
  cursor=m.word(0x48c974u);
 }
 limit=m.word(0x48c978u);profile(m,cursor,limit,2,s);
 const auto sweep_count=m.word(p+0x40u);const auto table=m.word(dispatcher.identity);const NativeTu15PhysicsVirtualOwner sweep_owner{m.word(table+0x14u)};
 require(s.sweep,"Native TU1.5 broad phase requires actual dispatcher sweep owner");s.sweep(dispatcher,sweep_owner,entries,bounds,sweep_count,scratch.first,scratch.second,fp);
 cursor=m.word(0x48c974u);limit=m.word(0x48c978u);profile(m,cursor,limit,3,s);
 auto pending_count=m.word(p+0x78u);
 if(positive(pending_count)){
  const auto old_count=m.word(scratch.first.identity+4u);const auto combined=old_count+pending_count;const auto capacity=m.word(scratch.first.identity+8u)&0x7fffffffu;
  if(less(capacity,combined)){auto grown=capacity*2u;if(!less(combined,grown))grown=combined;require(s.grow_pairs,"Native TU1.5 broad phase requires actual pair growth owner");s.grow_pairs(scratch.first,grown,8u,fp);}
  pending_count=m.word(p+0x78u);m.word(0x321947u,scratch.first.identity+4u,combined);
  if(positive(pending_count)){
   std::uint32_t index{},offset=old_count*8u;
   do{
    const auto pending=m.word(p+0x74u);const auto first=m.word(pending+index*8u);const auto first_data=m.word(scratch.first.identity);m.word(0x32195eu,first_data+offset,first);
    const auto second=m.word(pending+index*8u+4u);const auto second_data=m.word(scratch.first.identity);m.word(0x321969u,second_data+offset+4u,second);
    const auto count=m.word(p+0x78u);++index;offset+=8u;if(!less(index,count))break;
   }while(true);
  }
  m.word(0x321978u,p+0x78u,0);
 }
 release(m,entries,entry_bytes,fp,s,0x3219afu,0x3219b2u);release(m,bounds,bound_bytes,fp,s,0x3219e1u,0x3219e4u);
 const auto second_count=m.word(scratch.second.identity+4u),second_data=m.word(scratch.second.identity);
 const auto first_count=m.word(scratch.first.identity+4u),first_data=m.word(scratch.first.identity);
 require(s.consume_pairs,"Native TU1.5 broad phase requires actual321510 pair consumer");s.consume_pairs(island,{first_data,first_count},{second_data,second_count},fp);
 m.word(0x321a02u,p+0x30u,1,1);limit=m.word(0x48c978u);cursor=m.word(0x48c974u);profile(m,cursor,limit,4,s);
 native_tu15_destroy_broad_phase_pairs(memory,scratch.second,fp,s,observe);native_tu15_destroy_broad_phase_pairs(memory,scratch.first,fp,s,observe);
}
} // namespace halo2
