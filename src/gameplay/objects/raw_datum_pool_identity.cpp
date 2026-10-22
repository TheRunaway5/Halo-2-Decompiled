#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
struct Owner {
  const SourceRawDatumPoolServices &s;const SourceRawDatumPoolObserver &o;
  std::uint32_t r(std::uint32_t p,unsigned n=4) const {
    if(!s.read && !s.memory)throw std::runtime_error("Original datum identity storage unavailable");
    const std::span<const std::byte>b=s.read?s.read(p,n):s.memory(p,n);
    if(b.size()!=n)throw std::runtime_error("Original datum identity extent unavailable");
    std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
  }
  void w(std::uint32_t pc,std::uint32_t p,std::uint32_t v,unsigned n=4) const {
    const auto before=r(p,n);if(n<4)v&=(1u<<(8*n))-1;
    if(!s.memory)throw std::runtime_error("Original datum identity write owner unavailable");
    auto b=s.memory(p,n);if(b.size()!=n)throw std::runtime_error("Original datum identity write extent unavailable");
    for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8*i));
    if(o)o({pc,p,before,v,n});
  }
  void fill(std::uint32_t p,std::uint32_t n,std::uint32_t v,std::uint32_t pc4,std::uint32_t pc1) const {
    for(std::uint32_t i=0;i<(n>>2);++i)w(pc4,p+4*i,v);
    for(std::uint32_t i=0;i<(n&3);++i)w(pc1,p+(n&~3u)+i,v,1);
  }
};
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
}
std::uint32_t source_raw_datum_allocate_at(std::uint32_t pool,std::int32_t index,
    const SourceRawDatumPoolServices&s,const SourceRawDatumPoolObserver&o) {
  Owner a{s,o};if(index<0 || index>=signed_word(a.r(pool+0x20)))return 0xffffffffu;
  const auto element=a.r(pool+0x44)+a.r(pool+0x24)*std::uint32_t(index);
  if(a.r(element,2))return 0xffffffffu;
  const auto bitmap=a.r(pool+0x48)+std::uint32_t(index/32)*4;
  a.w(0x16d443,bitmap,a.r(bitmap)|(1u<<(index&31)));
  const auto count=a.r(pool+0x3c),high=a.r(pool+0x38);
  a.w(0x16d44e,pool+0x3c,count+1);
  if(index>=signed_word(high))a.w(0x16d457,pool+0x38,std::uint32_t(index)+1);
  a.fill(element,a.r(pool+0x24),0,0x16d48e,0x16d495);
  a.w(0x16d49b,element,a.r(pool+0x40,2),2);
  a.w(0x16d49e,pool+0x40,a.r(pool+0x40,2)+1,2);
  if(a.r(pool+0x40,2)==0xffffu)a.w(0x16d4ab,pool+0x40,0x8000,2);
  return (a.r(element,2)<<16)|std::uint32_t(index);
}
std::uint32_t source_raw_datum_allocate_identity(std::uint32_t pool,std::uint32_t datum,
    const SourceRawDatumPoolServices&s,const SourceRawDatumPoolObserver&o) {
  Owner a{s,o};const auto index=datum&65535u;
  if(signed_word(index)>=signed_word(a.r(pool+0x20)))return 0xffffffffu;
  const auto element=a.r(pool+0x44)+a.r(pool+0x24)*index;
  if(a.r(element,2))return 0xffffffffu;
  const auto bitmap=a.r(pool+0x48)+(index>>5)*4;
  a.w(0x16d3d2,bitmap,a.r(bitmap)|(1u<<(index&31)));
  const auto count=a.r(pool+0x3c),high=a.r(pool+0x38);
  a.w(0x16d3dd,pool+0x3c,count+1);
  if(signed_word(index)>=signed_word(high))a.w(0x16d3e6,pool+0x38,index+1);
  a.fill(element,a.r(pool+0x24),0,0x16d48e,0x16d495);
  a.w(0x16d49b,element,a.r(pool+0x40,2),2);
  a.w(0x16d49e,pool+0x40,a.r(pool+0x40,2)+1,2);
  if(a.r(pool+0x40,2)==0xffffu)a.w(0x16d4ab,pool+0x40,0x8000,2);
  a.w(0x16d3f1,element,datum>>16,2);return datum;
}
void source_raw_datum_release_unchecked(std::uint32_t pool,std::uint32_t datum,
    const SourceRawDatumPoolServices&s,const SourceRawDatumPoolObserver&o) {
  Owner a{s,o};const auto index=datum&65535u,stride=a.r(pool+0x24);
  auto element=a.r(pool+0x44)+stride*index;
  if(a.r(pool+0x2a,1)&8u)a.fill(element,stride,0xbabababa,0x16d4f1,0x16d4f8);
  const auto bitmap=a.r(pool+0x48)+(index/32)*4;
  a.w(0x16d518,bitmap,a.r(bitmap)&~(1u<<(index&31)));
  a.w(0x16d51a,element,0,2);
  if(signed_word(index)<signed_word(a.r(pool+0x34)))a.w(0x16d524,pool+0x34,index);
  if(index+1==a.r(pool+0x38))for(;;) {
    element-=a.r(pool+0x24);const auto high=a.r(pool+0x38)-1;
    a.w(0x16d53c,pool+0x38,high);
    if(signed_word(high)<=0 || a.r(element,2))break;
  }
  a.w(0x16d54d,pool+0x3c,a.r(pool+0x3c)-1);
}
std::uint32_t source_raw_datum_lookup(std::uint32_t pool,std::uint32_t datum,
    const SourceRawDatumPoolServices&s) {
  Owner a{s,{}};if(datum==0xffffffffu)return 0;
  const auto index=datum&65535u;
  if(signed_word(index)>=signed_word(a.r(pool+0x38)))return 0;
  const auto element=a.r(pool+0x44)+a.r(pool+0x24)*index;
  const auto salt=a.r(element,2);
  return salt && salt==(datum>>16)?element:0;
}
} // namespace halo2
