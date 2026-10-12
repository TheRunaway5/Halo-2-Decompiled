#include "halo2/sound_capabilities.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
template<class F>const F&need(const F&f,const char*m){if(!f)throw std::runtime_error(m);return f;}
class Owner {
 const SoundCapabilitiesBindings &b;const SoundCapabilitiesServices &p;const SoundCapabilitiesObserver &observe;
 std::span<std::byte>memory(std::uint32_t a,std::size_t n)const{auto q=need(p.memory,"Source SDK accounting memory unavailable")(a,n);if(q.size()!=n)throw std::runtime_error("Source SDK accounting extent unavailable");return q;}
public:
 Owner(const SoundCapabilitiesBindings &bindings,const SoundCapabilitiesServices &services,const SoundCapabilitiesObserver &observer):b(bindings),p(services),observe(observer){}
 std::uint32_t read(std::uint32_t a,unsigned n=4)const{auto q=memory(a,n);std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(q[i])<<(8*i);return v;}
 void write(std::uint32_t pc,std::uint32_t a,std::uint32_t v,unsigned n=4){const auto before=read(a,n),mask=n==4?0xffffffffu:((1u<<(8*n))-1);auto q=memory(a,n);for(unsigned i=0;i<n;++i)q[i]=std::byte(v>>(8*i));if(observe)observe({pc,a,n,before,v&mask});}
 void zero(std::uint32_t a,std::uint32_t n,std::uint32_t dword_pc,std::uint32_t byte_pc){const auto words=n>>2;for(std::uint32_t i=0;i<words;++i)write(dword_pc,a+4u*i,0);const auto tail=words*4u;for(std::uint32_t i=0;i<(n&3u);++i)write(byte_pc,a+tail+i,0,1);}
 std::int32_t caps(std::uint32_t device,std::uint32_t output){
  //381BEE adjusts the interface pointer;380810 never dereferences it.
  const auto object=device?device-8u:0u;(void)object;
  const bool entered=need(p.current_irql,"Actual SDK thread IRQL unavailable")()==0;
  if(entered)need(p.enter_critical_section,"Actual SDK critical-section enter unavailable")(b.critical_section);
  if(read(b.fault)){
   if(entered)need(p.leave_critical_section,"Actual SDK critical-section leave unavailable")(b.critical_section);
   return std::bit_cast<std::int32_t>(0x80004005u);
  }
  write(0x380841,output,read(b.free_2d));write(0x380849,output+4,read(b.free_3d));write(0x380853,output+8,read(b.free_sge));
  const auto pool=read(b.pool_bytes),contiguous=read(b.contiguous_bytes);write(0x380866,output+12,contiguous+pool);
  if(entered)need(p.leave_critical_section,"Actual SDK critical-section leave unavailable")(b.critical_section);
  return 0;
 }
 std::uint32_t allocate(std::uint32_t tag,std::uint32_t bytes,std::uint32_t clear){
  auto result=std::uint32_t{0};
  if(const auto pool=read(b.private_pool_pointer)){
   result=need(p.source_private_allocate,"Complete386BE7 private SDK pool owner unavailable")(pool,bytes);
   if(result&&clear)zero(result,bytes,0x383fc1,0x383fc8);
  }
  if(result)return result;
  result=need(p.raw_pool_allocate,"Actual SDK raw pool allocation unavailable")(bytes+4u,tag);
  if(!result)return 0;
  const auto capacity=need(p.raw_pool_capacity,"Actual SDK raw pool capacity unavailable")(result);
  const auto statistics=read(b.statistics_pointer);write(0x383eee,statistics,read(statistics)+capacity);
  if(clear)zero(result,capacity,0x383f03,0x383f0a);
  write(0x383feb,result,0x6c6f6f70);return result+4u;
 }
 std::uint32_t apu(std::uint32_t a,std::uint32_t parameters,const SoundCapabilitiesConstructorConstants &c){
  write(0x384c89,a,c.reference_vtable);write(0x384c8f,a+4,1);
  write(0x38ad67,a+8,c.base_vtable);write(0x38b259,b.device_context_pointer,a+0x1c);
  if(parameters){
   const auto method=read(read(parameters)+4);
   if(method!=c.reference_add_method)throw std::runtime_error("Actual SDK reference virtual method owner unavailable");
   write(0x380350,parameters+4,read(parameters+4)+1u);
  }
  write(0x38ad7b,a+0xc,parameters);
  write(0x384c9e,a,c.apu_vtable);write(0x384ca4,a+8,c.apu_interface_vtable);
  write(0x384caa,a+0x58,b.free_2d);write(0x384cb1,a+0x5c,b.free_3d);
  const auto sge=a+0x60;
  write(0x38689f,sge+8,sge+4);write(0x3868a2,sge+4,sge+4);
  write(0x3868a7,sge,c.sge_vtable);write(0x3868ad,sge+0x1c,b.free_sge);
  write(0x3868b4,sge+0x10,sge+0xc);write(0x3868b7,sge+0xc,sge+0xc);
  for(unsigned i=0;i<3;++i){const auto head=a+0x488+8*i;write(0x384cc6,head+4,head);write(0x384cc9,head,head);}
  write(0x384cd7,a+0x4a4,a+0x4a0);write(0x384cdd,a+0x4a0,a+0x4a0);
  write(0x384ce5,a+0x4ac,a+0x4a8);write(0x384ceb,a+0x4a8,a+0x4a8);
  write(0x384cf3,a+0x4b4,a+0x4b0);write(0x384cf9,a+0x4b0,a+0x4b0);
  write(0x384cfe,read(a+0x58),0xc0);write(0x384d07,read(a+0x5c),0x40);return a;
 }
 void descriptor(std::uint32_t a,std::uint32_t start,std::uint32_t count,std::uint32_t head){
  const auto next=read(head);write(0x386847,a+4,head);write(0x38684c,a,next);write(0x38684e,next+4,a);write(0x386854,read(a+4),a);
  write(0x38685b,a+0xc,0);write(0x38685f,a+0x10,0);write(0x386863,a+8,start,2);write(0x38686c,a+0xa,count,2);
 }
 std::int32_t sge(std::uint32_t a,std::uint32_t input){
  const auto count=input&0xffffu;write(0x3868eb,read(a+0x1c),count);
  const auto allocation=allocate(0x61645344,(count*5u+5u)*4u,1);
  // OriginalNEG/SBB/AND/ADD gives0 orE_OUTOFMEMORY, not a provider result.
  const auto status=allocation?0u:0x8007000eu;write(0x386913,a+0x14,allocation);
  if(!allocation)return std::bit_cast<std::int32_t>(status);
  descriptor(allocation,0,input,a+4);write(0x386929,a+0x18,allocation);
  for(std::uint32_t i=0;i<count;++i){const auto node=allocation+(i+1u)*0x14u,head=a+0xc;
   write(0x38694c,node,head);const auto prior=read(head+4);write(0x386951,node+4,prior);write(0x386954,head+4,node);write(0x38695a,read(node+4),node);
  }
  return 0;
 }
};
}
std::int32_t source_sound_capabilities(std::uint32_t device,std::uint32_t output,const SoundCapabilitiesBindings &b,const SoundCapabilitiesServices &p,const SoundCapabilitiesObserver &o){return Owner(b,p,o).caps(device,output);}
std::uint32_t source_sound_apu_construct(std::uint32_t apu,std::uint32_t parameters,const SoundCapabilitiesBindings &b,const SoundCapabilitiesConstructorConstants &c,const SoundCapabilitiesServices &p,const SoundCapabilitiesObserver &o){return Owner(b,p,o).apu(apu,parameters,c);}
std::uint32_t source_sound_sdk_allocate(std::uint32_t tag,std::uint32_t bytes,std::uint32_t clear,const SoundCapabilitiesBindings &b,const SoundCapabilitiesServices &p,const SoundCapabilitiesObserver &o){return Owner(b,p,o).allocate(tag,bytes,clear);}
std::int32_t source_sound_sge_initialize(std::uint32_t sge,std::uint32_t count,const SoundCapabilitiesBindings &b,const SoundCapabilitiesServices &p,const SoundCapabilitiesObserver &o){return Owner(b,p,o).sge(sge,count);}
} // namespace halo2
