#include "halo2/sound_cache_lifecycle.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 { namespace {
constexpr std::uint32_t none=0xffffffffu;
std::int32_t signed32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
class Owner {
 const SoundCacheLifecycleBindings &b;const SoundCacheLifecycleServices &p;
 const SoundCacheLifecycleObserver &o;
 std::span<std::byte> memory(std::uint32_t a,std::size_t n)const{
  if(!p.memory)throw std::runtime_error("Original cache lifecycle memory unavailable");
  if(std::uint64_t(a)+n>0x100000000ull)throw std::out_of_range("Cache lifecycle span wraps");
  auto v=p.memory(a,n);if(v.size()!=n)throw std::out_of_range("Cache lifecycle span truncated");return v;
 }
 std::uint32_t r(std::uint32_t a,unsigned n=4)const{
  auto v=memory(a,n);std::uint32_t out=0;for(unsigned k=0;k<n;++k)out|=std::to_integer<std::uint32_t>(v[k])<<(k*8);return out;
 }
 void w(std::uint32_t pc,std::uint32_t a,std::uint32_t v,unsigned n=4){
  auto before=r(a,n);auto raw=memory(a,n);for(unsigned k=0;k<n;++k)raw[k]=std::byte(v>>(8*k));
  if(o)o({pc,a,n,before,v&(n==4?none:((1u<<(n*8))-1))});
 }
 bool occupied(std::uint32_t bitmap,std::uint32_t index)const{
  return (r(bitmap+(index>>5)*4)&(1u<<(index&31)))!=0;
 }
 std::uint32_t entry(std::uint32_t pool,std::uint32_t h)const{return r(pool+0x44)+(h&0xffff)*r(pool+0x24);}
 std::uint32_t page(std::uint32_t pool,std::uint32_t h)const{return r(pool+0x44)+(h&0xffff)*24;}
 void free(std::uint32_t pool,std::uint32_t handle){
  const auto index=handle&0xffff,object=entry(pool,handle),stride=r(pool+0x24);
  if(r(pool+0x2a,1)&8){
   for(std::uint32_t k=0;k<stride/4;++k)w(0x16d4f1,object+k*4,0xbabababa);
   for(std::uint32_t k=(stride/4)*4;k<stride;++k)w(0x16d4f8,object+k,0xba,1);
  }
  const auto bitmap=r(pool+0x48)+(index>>5)*4;
  w(0x16d518,bitmap,r(bitmap)&~(1u<<(index&31)));w(0x16d51a,object,0,2);
  if(signed32(index)<signed32(r(pool+0x34)))w(0x16d524,pool+0x34,index);
  if(index+1==r(pool+0x38)){
   auto scan=object;do{scan-=stride;w(0x16d53c,pool+0x38,r(pool+0x38)-1);
    if(signed32(r(pool+0x38))<=0)break;
   }while(!r(scan,2));
  }
  w(0x16d54d,pool+0x3c,r(pool+0x3c)-1);
 }
 void erase(std::uint32_t allocator,std::uint32_t handle,const SoundCacheStartupConstants &c){
  const auto pool=r(allocator+0x64),object=page(pool,handle),callback=r(allocator+0x20);
  if(callback){
   if(callback!=c.delete_callback_identity)throw std::runtime_error("Original cache delete callback unavailable");
   const auto cache_pool=r(b.entry_pool_pointer),cached=r(cache_pool+0x44)+(handle&0xffff)*16;
   w(0x21ab0d,r(cached+0xc)+8,none);free(cache_pool,handle);
  }
  if(r(object+0x10)==none)w(0x13efe0,allocator+0x3c,r(object+0xc));
  else w(0x13efd7,page(r(allocator+0x64),r(object+0x10))+0xc,r(object+0xc));
  if(r(object+0xc)==none)w(0x13f00e,allocator+0x40,r(object+0x10));
  else w(0x13effc,page(r(allocator+0x64),r(object+0xc))+0x10,r(object+0x10));
  free(r(allocator+0x64),handle);
 }
 std::uint32_t tick()const{
  if(!p.tick_count)throw std::runtime_error("Original cache drain host clock unavailable");
  return p.tick_count();
 }
public:
 Owner(const SoundCacheLifecycleBindings &bindings,const SoundCacheLifecycleServices &services,
       const SoundCacheLifecycleObserver &observer):b(bindings),p(services),o(observer){}
 void rebase(std::uint32_t allocator){
  const auto pool=r(allocator+0x64);w(0x13e9a6,allocator+0x38,257);
  std::uint32_t index=none;
  for(;;){
   ++index;if(signed32(index)<0)return;
   const auto high=r(pool+0x38);if(signed32(index)>=signed32(high))return;
   const auto bitmap=r(pool+0x48);
   while(!occupied(bitmap,index)){++index;if(signed32(index)>=signed32(high))return;}
   if(index==none)return;
   const auto object=r(pool+0x44)+r(pool+0x24)*index;if(!object)return;
   const auto age=0x7fffffffu-r(object+0x14);
   if(age>256)w(0x13ea03,object+0x14,0);
   else w(0x13ea13,object+0x14,256-age);
  }
 }
 void clock(bool audio){
  const auto allocator=r(b.page_allocator_pointer);
  if(r(allocator+0x38)==0x7fffffffu)rebase(allocator);
  else w(audio?0x127483:0x21a93e,allocator+0x38,r(allocator+0x38)+1);
  for(unsigned k=0;k<8;++k)w((audio?0x127486:0x21a941)+k*3,allocator+0x44+k*4,0x7fffffffu);
 }
 std::uint32_t alias(std::uint32_t chunk){
  const auto handle=r(chunk+8),cache_pool=r(b.entry_pool_pointer);
  const auto object=r(cache_pool+0x44)+(handle&0xffff)*16;
  if(r(object+5,1)<255)w(0x21aa82,object+5,r(object+5,1)+1,1);
  const auto current_handle=r(chunk+8),allocator=r(b.page_allocator_pointer),pool=r(allocator+0x64);
  const auto shift=r(allocator+0x34)&31,offset=r(page(pool,current_handle)+8)<<shift;
  return offset+r(b.physical_buffer_pointer);
 }
 void drain(const SoundCacheStartupConstants &c){
  const auto initial_tick=tick(),initial_pool=r(b.entry_pool_pointer);auto current_pool=initial_pool;
  std::uint32_t index=none;
  for(;;){
   ++index;if(signed32(index)<0)break;
   const auto high=r(initial_pool+0x38);if(signed32(index)>=signed32(high))break;
   const auto bitmap=r(initial_pool+0x48);
   while(!occupied(bitmap,index)){++index;if(signed32(index)>=signed32(high))goto finished;}
   if(index==none)break;
   const auto selected=r(initial_pool+0x44)+r(initial_pool+0x24)*index;
   const auto offset=(index&0xffff)*16;
   for(;;){
    const auto object=r(current_pool+0x44)+offset;
    if(!r(object+4,1)&&!r(object+5,1)&&r(object+2,1))break;
    if(!p.wait)throw std::runtime_error("Original cache drain work-mutex wait unavailable");
    p.wait(r(b.work_mutex_slot),none);
    std::uint32_t count=0;for(auto node=r(b.work_head);node;node=r(node+0x3c))++count;
    if(!p.release_mutex)throw std::runtime_error("Original cache drain mutex release unavailable");
    p.release_mutex(r(b.work_mutex_slot));w(0x21a8d9,b.requests_added,count);
    const auto now=tick();current_pool=r(b.entry_pool_pointer);
    if(now-initial_tick>=5000)break;
   }
   const auto handle=r(r(selected+0xc)+8);
   if(handle!=none){erase(r(b.page_allocator_pointer),handle,c);current_pool=r(b.entry_pool_pointer);}
  }
finished:
  w(0x21a85d,current_pool+0x29,0,1);
 }
 void cold_update(){
  const auto system=r(b.sound_system_pointer);
  if(r(system+0x78,1)&&r(system+0x79,1)&&r(system+0x7a,1))
   throw std::runtime_error("Active127290 audio-update owner unavailable; cold contract rejected");
  clock(true);
 }
};
} // namespace
void source_sound_cache_clock(const SoundCacheLifecycleBindings &b,const SoundCacheLifecycleServices &p,const SoundCacheLifecycleObserver&o){Owner(b,p,o).clock(false);}
void source_sound_cache_rebase(std::uint32_t a,const SoundCacheLifecycleServices&p,const SoundCacheLifecycleObserver&o){const SoundCacheLifecycleBindings unused{};Owner(unused,p,o).rebase(a);}
std::uint32_t source_sound_cache_alias(const SoundCacheLifecycleBindings&b,std::uint32_t chunk,const SoundCacheLifecycleServices&p,const SoundCacheLifecycleObserver&o){return Owner(b,p,o).alias(chunk);}
void source_sound_cache_drain(const SoundCacheLifecycleBindings&b,const SoundCacheStartupConstants&c,const SoundCacheLifecycleServices&p,const SoundCacheLifecycleObserver&o){Owner(b,p,o).drain(c);}
void source_sound_cache_audio_update_cold(const SoundCacheLifecycleBindings&b,const SoundCacheLifecycleServices&p,const SoundCacheLifecycleObserver&o){Owner(b,p,o).cold_update();}
} // namespace halo2
