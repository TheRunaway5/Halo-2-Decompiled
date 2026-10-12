#include "halo2/source_physical_cache.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
namespace halo2 { namespace {
constexpr std::uint32_t none=0xffffffffu;
std::int32_t i32(std::uint32_t n){return std::bit_cast<std::int32_t>(n);}
class Owner {
 const SourcePhysicalCacheServices &p;const SourceRawDatumPoolObserver &o;
 std::uint32_t r(std::uint32_t a,unsigned n=4)const {
  const std::span<const std::byte>b=p.storage.read?p.storage.read(a,n):
    (p.storage.memory?std::span<const std::byte>(p.storage.memory(a,n)):std::span<const std::byte>{});
  if(b.size()!=n)throw std::runtime_error("Original physical cache input owner unavailable");
  std::uint32_t v=0;for(unsigned k=0;k<n;++k)v|=std::to_integer<std::uint32_t>(b[k])<<(8*k);return v;
 }
 void w(std::uint32_t pc,std::uint32_t a,std::uint32_t v,unsigned n=4)const {
  const auto before=r(a,n);if(n<4)v&=(1u<<(n*8))-1;
  if(!p.storage.memory)throw std::runtime_error("Original physical cache output owner unavailable");
  auto b=p.storage.memory(a,n);if(b.size()!=n)throw std::runtime_error("Original physical cache output truncated");
  for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(8*k));if(o)o({pc,a,before,v,n});
 }
 std::uint32_t at(std::uint32_t pool,std::uint32_t h)const{return r(pool+0x44)+(h&0xffff)*r(pool+0x24);}
 std::uint32_t page_entry(std::uint32_t pool,std::uint32_t h)const{return r(pool+0x44)+(h&0xffff)*24;}
 bool bit(std::uint32_t pool,std::uint32_t index)const{return (r(r(pool+0x48)+(index>>5)*4)&(1u<<(index&31)))!=0;}
 bool busy(std::uint32_t callback,std::uint32_t handle)const {
  if(!p.busy)throw std::runtime_error("Original physical cache busy callback owner unavailable");
  return p.busy(callback,handle);
 }
public:
 Owner(const SourcePhysicalCacheServices &services,const SourceRawDatumPoolObserver &observer):p(services),o(observer){}
 void erase(std::uint32_t allocator,std::uint32_t handle){
  const auto entry=page_entry(r(allocator+0x64),handle),callback=r(allocator+0x20);
  if(callback){if(!p.deleted)throw std::runtime_error("Original physical cache delete callback owner unavailable");p.deleted(callback,handle);}
  if(r(entry+0x10)==none)w(0x13efe0,allocator+0x3c,r(entry+0xc));
  else w(0x13efd7,page_entry(r(allocator+0x64),r(entry+0x10))+0xc,r(entry+0xc));
  if(r(entry+0xc)!=none)w(0x13effc,page_entry(r(allocator+0x64),r(entry+0xc))+0x10,r(entry+0x10));
  else w(0x13f00e,allocator+0x40,r(entry+0x10));
  source_raw_datum_release_unchecked(r(allocator+0x64),handle,p.storage,o);
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
     if(callback)unavailable=busy(callback,next);
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
  const auto handle=source_raw_datum_allocate(pool,p.storage,o);if(handle==none){
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
};
} // namespace
std::uint32_t source_physical_cache_allocate(std::uint32_t allocator,std::uint32_t bytes,
 std::int32_t threshold,const SourcePhysicalCacheServices &services,const SourceRawDatumPoolObserver &observer){
 return Owner(services,observer).pages(allocator,bytes,std::bit_cast<std::uint32_t>(threshold));
}
void source_physical_cache_release(std::uint32_t allocator,std::uint32_t handle,
 const SourcePhysicalCacheServices &services,const SourceRawDatumPoolObserver &observer){
 Owner(services,observer).erase(allocator,handle);
}
} // namespace halo2
