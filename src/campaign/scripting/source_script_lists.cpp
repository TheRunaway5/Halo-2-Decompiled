#include "halo2/source_script_lists.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t read(std::span<const std::uint8_t> b,unsigned p,unsigned n=4) {
  std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint32_t(b[p+i])<<(8*i);return v;
}
void write(std::span<std::uint8_t> b,unsigned p,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[p+i]=std::uint8_t(v>>(8*i));
}
template<class Pool> auto row(Pool &pool,std::uint32_t datum) {
  auto b=pool.lookup(datum);
  if(b.empty())throw std::invalid_argument("Source HS list received a stale full datum");
  return b;
}
}
std::uint32_t source_hs_volume_objects(const SourceHsObjectListServices &lists,
    const SourceHsVolumeServices &volume) {
  if(!lists.allocate || !lists.append || !volume.admit || !volume.candidates || !volume.contains)
    throw std::invalid_argument("Source HS volume query has no service owner");
  const auto list=lists.allocate();
  if(!volume.admit())return list;
  const auto candidates=volume.candidates();
  if(candidates.size()>128)throw std::invalid_argument("Source HS spatial query exceeded128 candidates");
  for(const auto object:candidates)if(object!=0xffffffffu && volume.contains(object))lists.append(list,object);
  return list;
}
SourceHsListBank::SourceHsListBank()
    :headers_("object list header",48,12),references_("object list reference",128,12) {}
std::uint32_t SourceHsListBank::allocate() {
  auto id=headers_.allocate();
  if(id==none){collect();id=headers_.allocate();}
  if(id!=none){auto b=row(headers_,id);write(b,6,0,2);write(b,8,none);}
  return id;
}
void SourceHsListBank::append(std::uint32_t list,std::uint32_t object) {
  auto h=row(headers_,list);const auto reference=references_.allocate();
  if(reference!=none) {
    auto r=row(references_,reference);write(r,4,object);write(r,8,read(h,8));write(h,8,reference);
  }
  //The source increments count even if the reference pool was exhausted.
  write(h,6,std::uint16_t(read(h,6,2)+1u),2);
}
std::uint16_t SourceHsListBank::count(std::uint32_t list)const {
  return list==none?0:std::uint16_t(read(row(headers_,list),6,2));
}
std::uint32_t SourceHsListBank::get(std::uint32_t list,std::int16_t index)const {
  if(list==none)return none;
  auto next=read(row(headers_,list),8);
  std::uint32_t object=none;
  for(unsigned step=0;next!=none;++step) {
    if(step>=128)throw std::invalid_argument("Source HS reference chain is cyclic");
    const auto r=row(references_,next);object=read(r,4);next=read(r,8);
    if(index<=0 || object==none)return object;
    --index;
  }
  return none;
}
std::vector<std::uint32_t> SourceHsListBank::objects(std::uint32_t list)const {
  std::vector<std::uint32_t> result;if(list==none)return result;
  auto next=read(row(headers_,list),8);
  while(next!=none) {
    if(result.size()>=128)throw std::invalid_argument("Source HS reference chain is cyclic");
    const auto r=row(references_,next);next=read(r,8);const auto object=read(r,4);
    if(object==none)break;
    result.push_back(object);
  }
  return result;
}
void SourceHsListBank::roots(std::span<const std::uint32_t> roots) {
  auto b=headers_.element_storage();const auto bits=headers_.bitmap();
  for(unsigned i=0;i<unsigned(headers_.high_water());++i)
    if(bits[i/32]&(1u<<(i%32)))write(b.subspan(i*12,12),4,0,2);
  for(const auto list:roots)if(list!=none) {
    auto h=row(headers_,list);write(h,4,std::uint16_t(read(h,4,2)+1u),2);
  }
}
void SourceHsListBank::collect() {
  const auto bits=headers_.bitmap();
  for(unsigned i=0;i<unsigned(headers_.high_water());++i) {
    if(!(bits[i/32]&(1u<<(i%32))))continue;
    auto h=headers_.element_storage().subspan(i*12,12);
    if(read(h,4,2)!=0)continue;
    const auto id=(read(h,0,2)<<16)|i;auto next=read(h,8);unsigned visited=0;
    while(next!=none) {
      if(++visited>128)throw std::invalid_argument("Source HS reference chain is cyclic");
      const auto id=next;next=read(row(references_,id),8);references_.release_unchecked(id);
    }
    headers_.release_unchecked(id);
  }
}
}
