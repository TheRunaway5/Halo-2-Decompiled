#include "halo2/sound_cache_request.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <unordered_set>
namespace halo2 { namespace {
constexpr std::uint32_t none=0xffffffffu;
std::int32_t i32(std::uint32_t n){return std::bit_cast<std::int32_t>(n);}
class Owner {
 const SoundCacheRequestState &s;const SoundCacheStartupConstants &c;
 const SoundCacheRequestServices &p;const SoundCacheRequestObserver &o;
 std::span<std::byte> memory(std::uint32_t a,std::size_t n)const{
  if(!p.memory)throw std::runtime_error("Original cache memory owner unavailable");
  if(std::uint64_t(a)+n>0x100000000ull)throw std::out_of_range("Cache memory span wraps");
  auto b=p.memory(a,n);if(b.size()!=n)throw std::out_of_range("Cache memory span truncated");return b;
 }
 std::uint32_t r(std::uint32_t a,unsigned n=4)const{
  auto b=memory(a,n);std::uint32_t v=0;for(unsigned k=0;k<n;++k)v|=std::to_integer<std::uint32_t>(b[k])<<(8*k);return v;
 }
 void w(std::uint32_t pc,std::uint32_t a,std::uint32_t v,unsigned n=4){
  auto before=r(a,n);auto b=memory(a,n);for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(8*k));
  if(o)o({pc,a,n,before,v&(n==4?none:((1u<<(8*n))-1))});
 }
 std::uint32_t at(std::uint32_t pool,std::uint32_t h)const{return r(pool+0x44)+(h&0xffff)*r(pool+0x24);}
 std::uint32_t page_entry(std::uint32_t pool,std::uint32_t h)const{return r(pool+0x44)+(h&0xffff)*24;}
 std::uint32_t cache_entry(std::uint32_t h)const{return r(s.entry_pool+0x44)+(h&0xffff)*16;}
 bool bit(std::uint32_t pool,std::uint32_t index)const{return (r(r(pool+0x48)+(index>>5)*4)&(1u<<(index&31)))!=0;}
 void clear_and_salt(std::uint32_t pool,std::uint32_t object,bool specific){
  const auto n=r(pool+0x24);for(std::uint32_t k=0;k<n/4;++k)w(specific?0x16d48e:0x16d353,object+k*4,0);
  for(std::uint32_t k=(n/4)*4;k<n;++k)w(specific?0x16d495:0x16d35a,object+k,0,1);
  w(specific?0x16d49b:0x16d360,object,r(pool+0x40,2),2);
  w(specific?0x16d49e:0x16d363,pool+0x40,r(pool+0x40,2)+1,2);
  if(r(pool+0x40,2)==0xffff)w(specific?0x16d4ab:0x16d36e,pool+0x40,0x8000,2);
 }
 std::uint32_t allocate(std::uint32_t pool){
  auto index=r(pool+0x34),high=r(pool+0x38);
  while(i32(index)<i32(high)&&bit(pool,index))++index;
  if(!(i32(index)<i32(high)&&index!=none))index=high;
  if(i32(index)>=i32(r(pool+0x20))||index==none)return none;
  auto bitmap=r(pool+0x48)+(index>>5)*4;w(0x16d32e,bitmap,r(bitmap)|(1u<<(index&31)));
  w(0x16d33c,pool+0x3c,r(pool+0x3c)+1);w(0x16d33f,pool+0x34,index+1);
  if(i32(high)<=i32(index))w(0x16d344,pool+0x38,index+1);
  auto object=at(pool,index);clear_and_salt(pool,object,false);return (r(object,2)<<16)|index;
 }
 std::uint32_t allocate_specific(std::uint32_t pool,std::uint32_t handle){
  const auto index=handle&0xffff;if(i32(index)>=i32(r(pool+0x20)))return none;
  const auto object=at(pool,index);if(r(object,2))return none;
  const auto bitmap=r(pool+0x48)+(index>>5)*4;w(0x16d3d2,bitmap,r(bitmap)|(1u<<(index&31)));
  w(0x16d3dd,pool+0x3c,r(pool+0x3c)+1);
  if(i32(index)>=i32(r(pool+0x38)))w(0x16d3e6,pool+0x38,index+1);
  clear_and_salt(pool,object,true);w(0x16d3f1,object,handle>>16,2);return handle;
 }
 void free(std::uint32_t pool,std::uint32_t handle){
  const auto index=handle&0xffff,object=at(pool,handle),stride=r(pool+0x24);
  if(r(pool+0x2a,1)&8){
   for(std::uint32_t k=0;k<stride/4;++k)w(0x16d4f1,object+k*4,0xbabababa);
   for(std::uint32_t k=(stride/4)*4;k<stride;++k)w(0x16d4f8,object+k,0xba,1);
  }
  const auto bitmap=r(pool+0x48)+(index>>5)*4;
  w(0x16d518,bitmap,r(bitmap)&~(1u<<(index&31)));w(0x16d51a,object,0,2);
  if(i32(index)<i32(r(pool+0x34)))w(0x16d524,pool+0x34,index);
  if(index+1==r(pool+0x38)){
   auto scan=object;do{scan-=stride;w(0x16d53c,pool+0x38,r(pool+0x38)-1);
    if(i32(r(pool+0x38))<1)break;}while(!r(scan,2));
  }
  w(0x16d54d,pool+0x3c,r(pool+0x3c)-1);
 }
 bool busy(std::uint32_t handle){
  const auto entry=cache_entry(handle);
  return r(entry+4,1)||r(entry+5,1)||!r(entry+2,1);
 }
 void erase(std::uint32_t allocator,std::uint32_t handle){
  const auto pool=r(allocator+0x64),entry=page_entry(pool,handle);
  const auto callback=r(allocator+0x20);
  if(callback){if(callback!=c.delete_callback_identity)throw std::runtime_error("Unowned cache page delete callback");
   const auto cached=cache_entry(handle);w(0x21ab0d,r(cached+0xc)+8,none);free(s.entry_pool,handle);
  }
  if(r(entry+0x10)==none)w(0x13efe0,allocator+0x3c,r(entry+0xc));
  else w(0x13efd7,page_entry(pool,r(entry+0x10))+0xc,r(entry+0xc));
  if(r(entry+0xc)!=none)w(0x13effc,page_entry(pool,r(entry+0xc))+0x10,r(entry+0x10));
  else w(0x13f00e,allocator+0x40,r(entry+0x10));
  free(pool,handle);
 }
 struct Window{std::uint32_t before=none,time=0,start=0,size=0;};
 bool preferable(std::uint32_t a,const Window &x,const Window &y)const{
  switch(r(a+0x2c)){
   case 0:return x.time<y.time||(x.time==y.time&&i32(x.size)<i32(y.size));
   case 1:{const auto age_x=std::min(r(a+0x38)-x.time,300u),age_y=std::min(r(a+0x38)-y.time,300u);
    if(i32(age_x+150)<i32(age_y))return false;
    return i32(age_y+150)<i32(age_x)||i32(x.size)<i32(y.size);}
   default:{const auto ax=r(a+0x38)-x.time,ay=r(a+0x38)-y.time;
    return i32(y.size*ax)>i32(x.size*ay);}
  }
 }
 std::uint32_t pages(std::uint32_t a,std::uint32_t bytes,std::uint32_t threshold){
  const auto shift=r(a+0x34)&31;
  auto need=std::uint32_t(i32(bytes)>>shift);if(bytes&((1u<<shift)-1))++need;
  const auto band=std::uint32_t(std::clamp(i32(threshold),0,8));
  if(i32(need)>=i32(r(a+0x44+band*4))&&band>0)return none;
  const auto pool=r(a+0x64);std::array<Window,256> windows{};
  std::uint32_t begin=0,end=0,cursor=0,next=r(a+0x3c),previous=none,oldest=none,oldest_time=0;
  Window chosen;bool found=false;
  while(i32(cursor)<i32(r(a+0x30))){
   const auto following=(end+1)&255;
   if(following!=begin){windows[end]={previous,0,cursor,0};end=following;}
   std::uint32_t age=0,extent=0;bool unavailable=false;
   if(next==none){extent=r(a+0x30)-cursor;cursor=r(a+0x30);}
   else{
    const auto entry=page_entry(pool,next);
    if(cursor!=r(entry+8)){extent=r(entry+8)-cursor;cursor=r(entry+8);}
    else{
     extent=r(entry+4);age=r(entry+0x14);const auto callback=r(a+0x24);
     if(callback){if(callback!=c.busy_callback_identity)throw std::runtime_error("Unowned cache page busy callback");unavailable=busy(next);}
     const auto tick=r(entry+0x14);
     if(threshold+tick>=r(a+0x38))unavailable=true;
     else if(!unavailable&&(oldest==none||tick<oldest_time)){oldest=next;oldest_time=tick;}
     previous=next;next=r(entry+0xc);cursor=r(entry+4)+r(entry+8);
    }
   }
   if(unavailable)begin=end;
   else{
    auto k=begin;
    while(k!=end){auto &window=windows[k];window.time=std::max(window.time,age);window.size+=extent;
     if(i32(window.size)>=i32(need)){
      if(!found||preferable(a,window,chosen)){chosen=window;found=true;}
      begin=(begin+1)&255;
     }k=(k+1)&255;
    }
   }
  }
  if(!found){if(threshold==band)w(0x13eb96,a+0x44+band*4,std::uint32_t(std::min(i32(r(a+0x44+band*4)),i32(need))));
   return none;}
  for(std::uint32_t index=0;i32(index)<i32(r(pool+0x38));++index){
   if(!bit(pool,index))continue;
   const auto entry=at(pool,index);const auto handle=(r(entry,2)<<16)|index;
   if(i32(r(entry+8))<i32(need+chosen.start)&&i32(chosen.start)<i32(r(entry+4)+r(entry+8)))erase(a,handle);
  }
  if(r(pool+0x3c)==r(pool+0x20)&&oldest!=none){
   if(chosen.before==oldest)chosen.before=r(page_entry(pool,oldest)+0x10);
   erase(a,oldest);
  }
  const auto handle=allocate(pool);if(handle==none){
   if(threshold==band)w(0x13eb96,a+0x44+band*4,std::uint32_t(std::min(i32(r(a+0x44+band*4)),i32(need))));
   return none;
  }
  const auto entry=page_entry(pool,handle);
  if(chosen.before==none){
   const auto head=r(a+0x3c);w(0x13ef07,entry+0x10,none);
   if(head==none){w(0x13ef0f,a+0x40,handle);w(0x13ef12,entry+0xc,r(a+0x3c));w(0x13ef15,a+0x3c,handle);}
   else{w(0x13ef26,page_entry(pool,head)+0x10,handle);w(0x13ef2c,entry+0xc,r(a+0x3c));w(0x13ef2f,a+0x3c,handle);}
  }else{
   const auto predecessor=page_entry(pool,chosen.before),successor=r(predecessor+0xc);
   if(successor==none){w(0x13ef4b,entry+0x10,r(a+0x40));w(0x13ef4e,a+0x40,handle);}
   else{const auto q=page_entry(pool,successor);w(0x13ef95,entry+0x10,r(q+0x10));w(0x13ef98,q+0x10,handle);}
   w(0x13ef5d,entry+0xc,r(predecessor+0xc));w(0x13ef60,predecessor+0xc,handle);
  }
  w(0x13ef6b,entry+8,chosen.start);w(0x13ef6e,entry+4,need);w(0x13ef74,entry+0x14,r(a+0x38));return handle;
 }
 void load(std::uint32_t sound,std::uint32_t chunk){
  const auto handle=pages(s.page_allocator,r(chunk+4)&0x3fffffffu,0);if(handle==none)return;
  const auto page=page_entry(r(s.page_allocator+0x64),handle);
  const auto buffer=(r(page+8)<<(r(s.page_allocator+0x34)&31))+s.physical_buffer;
  (void)allocate_specific(s.entry_pool,handle);const auto entry=cache_entry(handle);
  w(0x21ab85,chunk+8,handle);w(0x21ab88,entry+8,sound);w(0x21ab8b,entry+0xc,chunk);
  w(0x21ab8e,entry+4,0,1);w(0x21ab92,entry+5,0,1);w(0x21ab96,entry+3,0,1);
  auto extent=r(chunk+4)&0x3fffffffu;if(extent&0x1ff)extent=(extent|0x1ff)+1;
  if(!p.async_read)throw std::runtime_error("Original cache async read owner unavailable");
  p.async_read({r(chunk),extent,buffer,0,entry+2,5,4});
 }
public:
 Owner(const SoundCacheRequestState&state,const SoundCacheStartupConstants&constants,
       const SoundCacheRequestServices&services,const SoundCacheRequestObserver&observer):s(state),c(constants),p(services),o(observer){}
 std::uint32_t request(std::uint32_t sound,std::uint32_t chunk,std::uint32_t flags){
  if(r(chunk+8)==none&&sound!=none&&(flags&2))load(sound,chunk);
  std::uint32_t result=0;if(r(chunk+8)!=none){
   const auto handle=r(chunk+8);w(0x21a9d3,page_entry(r(s.page_allocator+0x64),handle)+0x14,r(s.page_allocator+0x38));
   const auto entry=cache_entry(r(chunk+8));
   if(flags&1)while(!r(entry+2,1)){if(!p.yield)throw std::runtime_error("Original cache blocking yield unavailable");p.yield();}
   if(r(entry+2,1)){if(!r(entry+3,1))w(0x21aa1e,entry+3,1,1);result=2;}
   if(flags&4){w(0x21aa32,entry+4,r(entry+4,1)+1,1);result|=4;}
  }
  if(!(result&2)&&r(chunk+8)!=none)return result|1;
  return result&~1u;
 }
};
} // namespace
std::uint32_t source_sound_cache_request(const SoundCacheRequestState&s,std::uint32_t sound,
 std::uint32_t chunk,std::uint32_t flags,const SoundCacheStartupConstants&c,
 const SoundCacheRequestServices&p,const SoundCacheRequestObserver&o){return Owner(s,c,p,o).request(sound,chunk,flags);}
} // namespace halo2
