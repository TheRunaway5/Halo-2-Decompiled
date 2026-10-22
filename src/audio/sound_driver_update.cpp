#include "halo2/sound_driver_update.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 { namespace {
class Owner {
 const SoundDriverUpdateBindings &b;const SoundDriverUpdateServices &p;
 const SoundDriverOwnersObserver &o;
 std::span<std::byte> memory(std::uint32_t a,std::size_t n)const{
  if(!p.memory)throw std::runtime_error("Original driver update memory unavailable");
  if(std::uint64_t(a)+n>0x100000000ull)throw std::out_of_range("Driver update span wraps");
  auto v=p.memory(a,n);if(v.size()!=n)throw std::out_of_range("Driver update span truncated");return v;
 }
 std::uint32_t r(std::uint32_t a,unsigned n=4)const{
  const auto raw=memory(a,n);std::uint32_t value=0;for(unsigned k=0;k<n;++k)value|=std::to_integer<std::uint32_t>(raw[k])<<(8*k);return value;
 }
 void w(std::uint32_t pc,std::uint32_t a,std::uint32_t value,unsigned n=4){
  const auto before=r(a,n);auto raw=memory(a,n);for(unsigned k=0;k<n;++k)raw[k]=std::byte(value>>(8*k));
  if(o)o({pc,a,n,before,value&(n==4?0xffffffffu:((1u<<(n*8))-1))});
 }
 void method(std::uint32_t identity,std::uint32_t slot,SoundDriverVirtualThis abi,std::span<std::byte> output={}){
  const auto vtable=r(identity),address=r(vtable+slot);
  if(!p.native_virtual_call)throw std::runtime_error("Original sound stream method unavailable");
  p.native_virtual_call({identity,vtable,slot,address,abi,output});
 }
 static std::int16_t signed16(std::uint32_t n){return std::bit_cast<std::int16_t>(std::uint16_t(n));}
public:
 Owner(const SoundDriverUpdateBindings &bindings,const SoundDriverUpdateServices &services,
       const SoundDriverOwnersObserver &observer):b(bindings),p(services),o(observer){}
 void reload(){
  if(!p.load_section)throw std::runtime_error("Original DSP section lifetime owner unavailable");
  const auto loaded=p.load_section(b.section_name);
  if(!loaded)return;
  if(r(b.optional_sound_flag,1)&&r(b.quiesce_flag,1)){
   if(!p.source_quiesce)throw std::runtime_error("Original550A0 sound quiescence owner unavailable");
   p.source_quiesce(b.quiesce_record);w(0x19305c,b.quiesce_clear_word,0,2);
  }
  const SoundDriverEffectsRequest request{b.section_name,{9,10},1,r(b.effects_context_pointer)};
  if(!p.download_effects)throw std::runtime_error("Original DSP effects download owner unavailable");
  p.download_effects(request);
  if(!p.source_restore)throw std::runtime_error("Original53500 sound restore owner unavailable");
  p.source_restore();
  if(!p.free_section)throw std::runtime_error("Original DSP section release owner unavailable");
  p.free_section(b.section_name);
 }
 void update(std::span<std::byte,4> status){
  if(r(r(b.driver_pointer)+0x2ad4)!=r(b.effects_generation)){
   reload();const auto driver=r(b.driver_pointer),generation=r(b.effects_generation);
   w(0x220d37,driver+0x2a08,0x101,2);w(0x220d40,driver+0x2ad4,generation);
  }
  if(!p.do_work)throw std::runtime_error("Original DirectSoundDoWork owner unavailable");
  p.do_work();
  auto driver=r(b.driver_pointer);std::uint32_t index=0,offset=0;
  if(signed16(r(driver+4,2))<=0)return;
  do{
   const auto record=driver+offset+0xc;
   if((r(record+1,1)&7)==1){
    method(r(record+0x24),0xc,SoundDriverVirtualThis::Stack,status);
    // Actual original code consumes bit16 of its caller backing regardless
    // of the method's unconsumed HRESULT. Failure is not rewritten to ready.
    if((std::to_integer<unsigned>(status[2])&1)==0){
     method(r(record+0x24),0x18,SoundDriverVirtualThis::Stack);
     method(r(record+0x30),0x1c,SoundDriverVirtualThis::Ecx);
     w(0x220d9d,record+1,r(record+1,1)&0xf8,1);
     if(!p.source_release_packets)throw std::runtime_error("Original2B4BF0 packet release owner unavailable");
     p.source_release_packets(record);w(0x220da8,record+3,r(record+3,1)&0xfb,1);
    }
   }
   // All four original table destinations are220DC4. Out-of-range signed
   // three-bit values also fall through there; no extra action is invented.
   const auto dispatch_mode=r(record+1,1);(void)dispatch_mode;
   driver=r(b.driver_pointer);++index;offset+=0x34;
  }while(std::bit_cast<std::int32_t>(index)<signed16(r(driver+4,2)));
 }
};
} // namespace
void source_sound_driver_reload_effects(const SoundDriverUpdateBindings&b,const SoundDriverUpdateServices&p,const SoundDriverOwnersObserver&o){Owner(b,p,o).reload();}
void source_sound_driver_update(const SoundDriverUpdateBindings&b,std::span<std::byte,4>status,const SoundDriverUpdateServices&p,const SoundDriverOwnersObserver&o){Owner(b,p,o).update(status);}
} // namespace halo2
