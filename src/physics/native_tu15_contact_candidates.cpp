#include "halo2/native_tu15_contact_candidates.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
using Vector=NativeTu15ContactRecordVector;
using Branch=NativeTu15ContactCandidateBranch;
using Release=NativeTu15ContactCandidateRelease;
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
template<class F>void require(const F&f,const char*name){if(!f)throw std::runtime_error(name);}
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
    Vector vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 contact candidate requires aligned vector storage");std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector v{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)v[n]|=std::to_integer<unsigned>(b[n*4u+k])<<(k*8u);return v;}
    void bytes(std::uint32_t site,std::uint32_t p,std::span<const std::byte>b,bool*committed=nullptr){SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(committed)*committed=true;if(observer_)observer_(event);}
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4,bool*committed=nullptr){std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));bytes(site,p,std::span(b).first(width),committed);}
    void vector(std::uint32_t site,std::uint32_t p,const Vector&v){if(p&15u)throw std::runtime_error("Native TU1.5 contact candidate requires aligned vector storage");std::array<std::byte,16>b{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)b[n*4+k]=std::byte(v[n]>>(k*8u));bytes(site,p,b);}
    NativeTu15ContactVirtualOwner owner(std::uint32_t object,unsigned slot)const{return{word(word(object)+slot)};}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    void scalar_pop(std::uint32_t site,std::uint32_t p){alignas(16)std::array<std::byte,108>prior{};std::uint32_t bits;asm volatile("fnsave %0\n\tfrstor %0\n\tfstps %1":"=m"(prior),"=m"(bits)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");bool committed=false;try{word(site,p,bits,4,&committed);}catch(...){if(!committed)asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned host_simd_;bool active_{};
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),host_simd_(_mm_getcsr()){
        if(depth>7)throw std::runtime_error("Native TU1.5 contact candidate requires its overfull-stack owner");
        if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 contact candidate requires its unsupported FP owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_simd_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 contact candidate requires its dependency FP owner");const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto value=f();resume();return value;}}catch(...){resume();throw;}}
    // Unwinding does not resume the interrupted numerical expression. Publish
    // its current FP prefix once, retain genuine cleanup-service FP changes,
    // and let the outer destructor restore the unrelated host context.
    template<class F>void cleanup(F&&f){if(active_)publish();f();}
};
void push(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void multiply(std::uint32_t value){asm volatile("fmuls %0"::"m"(value):"memory","st");}
void add(std::uint32_t value){asm volatile("fadds %0"::"m"(value):"memory","st");}
std::uint32_t pop(){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");return value;}
__m128 vector(const Vector&v){__m128 result;std::memcpy(&result,v.data(),16);return result;}
__m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
unsigned exceeds(__m128 margin,__m128 delta){asm volatile("cmpltps %1,%0":"+x"(margin):"x"(delta):"memory");return unsigned(_mm_movemask_ps(margin))&7u;}
#endif
}
void native_tu15_update_contact_candidates(SourcePhysicsContactMemory&memory,
 NativeTu15ContactProducer producer,NativeTu15ContactPayload first,NativeTu15ContactPayload second,
 NativeTu15ContactContext context,NativeTu15ContactVirtualOwner factory,NativeTu15ContactCandidateScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ContactCandidateServices&s,
 const SourcePhysicsContactRecordObserver&observer,NativeTu15ContactCandidateStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 if(scratch.identity&15u)throw std::runtime_error("Native TU1.5 candidate packets require alignment");
 Memory m(memory,observer);Environment environment(fp,depth);const auto self=producer.identity,p=scratch.identity,keys=p+0x180u,temp=p+0x2cu;
 if(stats)++stats->calls;
 const auto second_motion=m.word(second.identity+8u),first_motion=m.word(first.identity+8u);
 NativeTu15InverseGeometryMemory inverse(memory,p+0x390u,second_motion+0x20u,first_motion+0x20u,observer);
 environment.dependency([&]{native_tu15_compose_inverse_affine_geometry(inverse,fp);});
 push(m.word(context.identity+8u));const NativeTu15ContactChild first_shape{m.word(first.identity)};
 multiply(m.word(0x46a0ccu));const auto first_table=m.word(first_shape.identity);const auto half=pop();
 push(half);add(m.word(context.identity+8u));const auto margin=pop();
 const NativeTu15ContactVirtualOwner bound_owner{m.word(first_table+0x18u)};
 environment.dependency([&]{require(s.bounds,"Native TU1.5 candidates require actual shape bounds owner");s.bounds(first_shape,bound_owner,p+0x390u,margin,p+0x40u,fp);});
 const auto test_cached=m.word(0x46e468u,1);const auto minimum=m.vector(p+0x40u),maximum=m.vector(p+0x50u);
 if(test_cached){const auto previous_maximum=m.vector(self+0x30u),mask=m.vector(0x461950u);const auto delta=_mm_and_ps(subtract(vector(previous_maximum),vector(maximum)),vector(mask));const auto scalar=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(half)));const auto threshold=_mm_shuffle_ps(scalar,scalar,0);
  if(!exceeds(threshold,delta)){const auto previous_minimum=m.vector(self+0x20u);const auto minimum_delta=_mm_and_ps(subtract(vector(previous_minimum),vector(minimum)),vector(mask));if(!exceeds(threshold,minimum_delta)){if(stats)++stats->unchanged_bounds;return;}}
 }
 m.vector(0x2f37e3,self+0x20u,minimum);m.vector(0x2f37e7,self+0x30u,maximum);
 const NativeTu15ContactChild second_shape{m.word(second.identity)};
 m.word(0x2f37ed,keys,p+0x18cu);m.word(0x2f37f4,keys+4u,0);m.word(0x2f37fb,keys+8u,0x80000080u);
 const auto key_release=[&]{const auto capacity=m.word(keys+8u);if(!less(capacity,0)){const auto allocator=m.word(0x48c938u);const auto table=m.word(allocator);const auto bytes=(capacity&0x7fffffffu)*4u;const auto data=m.word(keys);const NativeTu15ContactVirtualOwner owner{m.word(table+0x14u)};require(s.release,"Native TU1.5 candidates require actual key-buffer release");s.release(allocator,owner,data,bytes,0x12u,Release::unwind_keys,fp);}};
 native_contact_detail::UnwindGuard keys_guard([&]{environment.cleanup(key_release);});
 const auto query_table=m.word(second_shape.identity);keys_guard.arm();const NativeTu15ContactVirtualOwner query_owner{m.word(query_table+0x30u)};
 environment.dependency([&]{require(s.query_keys,"Native TU1.5 candidates require actual child-key query");s.query_keys(second_shape,query_owner,p+0x40u,keys,fp);});
 const auto unsorted=m.word(0x48cbb8u,1),container=m.word(second_shape.identity+0xcu),motion=m.word(second.identity+8u);
 m.word(0x2f382e,p+0x70u,second.identity);m.word(0x2f3832,p+0x6cu,motion);m.word(0x2f3836,p+0x60u,container);
 const auto release_agent=[&](std::uint32_t agent,Branch branch){const NativeTu15ContactVirtualOwner owner=m.owner(agent,0x14u);environment.dependency([&]{require(s.release_agent,"Native TU1.5 candidates require actual agent release");s.release_agent({agent},owner,branch,fp);});if(stats)++stats->released;};
 const auto reserve=[&](std::uint32_t header,std::uint32_t capacity,std::uint32_t count){if(less(capacity,count)){const auto doubled=capacity*2u;const auto next=less(count,doubled)?doubled:count;environment.dependency([&]{require(s.reserve_entries,"Native TU1.5 candidates require actual entry-array reserve");s.reserve_entries(header,next,12u,fp);});if(stats)++stats->reserve_calls;}};
 const auto create=[&](std::uint32_t key_pointer,std::uint32_t cached_key,
   NativeTu15ContactChild collection,std::uint32_t user_data,Branch branch){
  const auto table=m.word(collection.identity);const NativeTu15ContactVirtualOwner resolve_owner{m.word(table+0x34u)};
  // Only the merge branch recovers the comparison key from retained local
  // storage; the other two branches retain their actual source key read.
  const auto key=branch==Branch::sorted_merge?m.word(p+0x10u):cached_key;
  const auto child=environment.dependency([&]{require(s.resolve_child,"Native TU1.5 candidates require actual child resolution");return s.resolve_child(collection,resolve_owner,key,p+0x80u,branch,fp);});
  const auto published_key=branch==Branch::sorted_merge?m.word(p+0x10u):key;
  m.word(branch==Branch::unsorted?0x2f3a47u:branch==Branch::sorted_merge?0x2f3bdcu:0x2f3c9bu,p+0x68u,published_key);
  std::uint32_t filter_receiver{};
  if(branch==Branch::sorted_merge)filter_receiver=m.word(context.identity+4u);
  m.word(branch==Branch::unsorted?0x2f3a4fu:branch==Branch::sorted_merge?0x2f3be3u:0x2f3c9fu,p+0x64u,child.identity);
  const auto current_key=m.word(key_pointer);
  if(branch!=Branch::sorted_merge)filter_receiver=m.word(context.identity+4u);
  const auto filter_table=m.word(filter_receiver),parent_collection=m.word(p+0x60u);
  const NativeTu15ContactVirtualOwner filter_owner{m.word(filter_table)};
  const auto result=environment.dependency([&]{require(s.filter,"Native TU1.5 candidates require actual filter owner");return s.filter(filter_receiver,filter_owner,p+0x1fu,context,first,{p+0x64u},parent_collection,current_key,branch,fp);});
  const bool accepted=m.word(result,1)!=0;
  if(!accepted)return std::pair{native_tu15_invalid_contact_agent(),false};
  const auto agent=environment.dependency([&]{require(s.create_agent,"Native TU1.5 candidates require actual selected contact factory");return s.create_agent(factory,first,{p+0x64u},context,user_data,branch,fp);});if(stats)++stats->created;return std::pair{agent,true};
 };
 std::uint32_t final_key_data{};
 if(unsorted){
  if(stats)++stats->unsorted;const auto user_data=m.word(self+8u);auto records=m.word(self+0xcu);auto key_begin=m.word(keys);final_key_data=key_begin;auto count=m.word(self+0x10u);auto record_end=records+count*12u;auto key_end=key_begin+m.word(keys+4u)*4u;auto current_key=key_begin;
  for(auto record=records;record!=record_end;record+=12u){
   if(current_key!=key_end&&m.word(record)==m.word(current_key)){current_key+=4u;continue;}
   auto scan=key_begin;bool retained=false;current_key=scan;
   if(scan!=key_end){const auto record_key=m.word(record);for(;scan!=key_end;scan+=4u)if(record_key==m.word(scan)){retained=true;break;}}
   current_key=retained?scan+4u:scan;
   if(retained)continue;
   release_agent(m.word(record+8u),Branch::unsorted);const auto old_count=m.word(self+0x10u),data=m.word(self+0xcu);m.word(0x2f38c1,self+0x10u,old_count-1u);
   const auto index=std::bit_cast<std::int32_t>(record-data)/12;auto offset=std::uint32_t(index)*12u;auto destination_index=std::uint32_t(index);
   while(less(destination_index,m.word(self+0x10u))){const auto destination=m.word(self+0xcu)+offset;const auto source=destination+12u;m.word(0x2f38f9,destination,m.word(source));m.word(0x2f38fe,destination+4u,m.word(source+4u));m.word(0x2f3904,destination+8u,m.word(source+8u));++destination_index;offset+=12u;}
   record-=12u;record_end-=12u;key_begin=m.word(keys);final_key_data=key_begin;
  }
  count=m.word(self+0x10u);if(m.word(keys+4u)!=count){records=m.word(self+0xcu);record_end=records+count*12u;key_end=key_begin+m.word(keys+4u)*4u;auto record=records;
   for(auto key_pointer=key_begin;key_pointer!=key_end;key_pointer+=4u,record+=12u){if(record!=record_end&&m.word(record)==m.word(key_pointer))continue;
    const auto next_count=count+1u;const auto index=std::bit_cast<std::int32_t>(key_pointer-key_begin)>>2;const auto remaining=count-std::uint32_t(index);const auto capacity=m.word(self+0x14u)&0x7fffffffu;reserve(self+0xcu,capacity,next_count);
    const auto offset=std::uint32_t(index)*12u,data=m.word(self+0xcu);for(auto n=remaining;less(0,n);--n){const auto destination=data+offset+n*12u,source=destination-12u;m.word(0x2f3a08,destination,m.word(source));m.word(0x2f3a0d,destination+4u,m.word(source+4u));m.word(0x2f3a13,destination+8u,m.word(source+8u));}
    const auto insertion_data=m.word(self+0xcu);const NativeTu15ContactChild collection{m.word(p+0x60u)};
    m.word(0x2f3a32,self+0x10u,next_count);record=insertion_data+offset;const auto cached_key=m.word(key_pointer);const auto [agent,accepted]=create(key_pointer,cached_key,collection,user_data,Branch::unsorted);(void)accepted;m.word(0x2f3a9b,record+8u,agent.identity);m.word(0x2f3aa0,record,m.word(key_pointer));count=m.word(self+0x10u);record_end=m.word(self+0xcu)+count*12u;key_begin=m.word(keys);final_key_data=key_begin;
   }
  }
 }else{
  if(stats)++stats->sorted;auto key_count=m.word(keys+4u);m.word(0x2f3ae1,p+0x24u,0,1);
  if(less(1u,key_count)){require(s.sort_keys,"Native TU1.5 candidates require actual key sort");environment.dependency([&]{const auto comparator=m.word(p+0x24u),data=m.word(keys);s.sort_keys(data,0,key_count-1u,comparator,fp);});key_count=m.word(keys+4u);}
  const auto user_data=m.word(self+8u);auto record=m.word(self+0xcu);const auto record_count=m.word(self+0x10u);auto key_pointer=m.word(keys);const auto record_end=record+record_count*12u,key_end=key_pointer+key_count*4u;
  environment.dependency([&]{require(s.initialize_entries,"Native TU1.5 candidates require actual entry-buffer constructor");s.initialize_entries(temp,key_count,fp);});
  native_contact_detail::UnwindGuard temporary_guard([&]{environment.cleanup([&]{require(s.destroy_entries,"Native TU1.5 candidates require actual entry-buffer destructor");s.destroy_entries(temp,fp);});});
  const auto capacity=m.word(temp+8u)&0x7fffffffu;key_count=m.word(keys+4u);temporary_guard.arm();reserve(temp,capacity,key_count);m.word(0x2f3b73,temp+4u,key_count);auto destination=m.word(temp);
  while(record!=record_end&&key_pointer!=key_end){const auto key=m.word(key_pointer),old_key=m.word(record);m.word(0x2f3b91,p+0x10u,key);
   if(key==old_key){m.word(0x2f3b9d,destination,m.word(record));m.word(0x2f3ba2,destination+4u,m.word(record+4u));m.word(0x2f3bae,destination+8u,m.word(record+8u));destination+=12u;record+=12u;key_pointer+=4u;}
   else if(key<old_key){const NativeTu15ContactChild collection{m.word(p+0x60u)};const auto [agent,accepted]=create(key_pointer,key,collection,user_data,Branch::sorted_merge);m.word(accepted?0x2f3c1fu:0x2f3c36u,destination+8u,agent.identity);m.word(accepted?0x2f3c24u:0x2f3c3bu,destination,m.word(key_pointer));destination+=12u;key_pointer+=4u;}
   else{const auto agent=m.word(record+8u);if(agent)release_agent(agent,Branch::sorted_merge);record+=12u;}
  }
  for(;record!=record_end;record+=12u){const auto agent=m.word(record+8u);if(agent)release_agent(agent,Branch::sorted_tail);}
  for(;key_pointer!=key_end;key_pointer+=4u,destination+=12u){const auto cached_key=m.word(key_pointer);const NativeTu15ContactChild collection{m.word(p+0x60u)};const auto [agent,accepted]=create(key_pointer,cached_key,collection,user_data,Branch::sorted_tail);(void)accepted;m.word(0x2f3ce7,destination+8u,agent.identity);m.word(0x2f3cf0,destination,m.word(key_pointer));}
  const auto owner_capacity=m.word(self+0x14u);auto count=m.word(temp+4u);if(less(owner_capacity&0x7fffffffu,count)){
   if(!less(owner_capacity,0)){const auto data=m.word(self+0xcu),allocator=m.word(0x48c938u),table=m.word(allocator);const NativeTu15ContactVirtualOwner owner{m.word(table+0x14u)};environment.dependency([&]{require(s.release,"Native TU1.5 candidates require actual entry-buffer release");s.release(allocator,owner,data,(owner_capacity&0x7fffffffu)*12u,0x12u,Release::replacement,fp);});}
   const auto requested=m.word(temp+4u),allocator=m.word(0x48c938u),table=m.word(allocator);const NativeTu15ContactVirtualOwner owner{m.word(table+0x10u)};const auto data=environment.dependency([&]{require(s.allocate,"Native TU1.5 candidates require actual entry-buffer allocation");return s.allocate(allocator,owner,requested*12u,0x12u,fp);});count=m.word(temp+4u);m.word(0x2f3d4a,self+0xcu,data);m.word(0x2f3d4d,self+0x14u,count);
  }
  auto output=m.word(self+0xcu);m.word(0x2f3d55,self+0x10u,count);if(less(0,count)){auto input=m.word(temp);for(auto remaining=count;remaining;--remaining,input+=12u,output+=12u){m.word(0x2f3d66,output,m.word(input));m.word(0x2f3d6b,output+4u,m.word(input+4u));const auto last=m.word(input+8u);m.word(0x2f3d78,output+8u,last);}}
  temporary_guard.disarm();environment.dependency([&]{require(s.destroy_entries,"Native TU1.5 candidates require actual entry-buffer destructor");s.destroy_entries(temp,fp);});final_key_data=m.word(keys);
 }
 // The source disarms its owned-key cleanup before the final allocator call.
 const auto final_capacity=m.word(keys+8u);keys_guard.disarm();if(!less(final_capacity,0)){const auto allocator=m.word(0x48c938u),table=m.word(allocator);const NativeTu15ContactVirtualOwner owner{m.word(table+0x14u)};environment.dependency([&]{require(s.release,"Native TU1.5 candidates require actual key-buffer release");s.release(allocator,owner,final_key_data,(final_capacity&0x7fffffffu)*4u,0x12u,Release::final_keys,fp);});}
#else
 (void)memory;(void)producer;(void)first;(void)second;(void)context;(void)factory;(void)scratch;(void)fp;(void)depth;(void)s;(void)observer;(void)stats;throw std::runtime_error("Exact native TU1.5 candidate backend unavailable");
#endif
}
} // namespace halo2
