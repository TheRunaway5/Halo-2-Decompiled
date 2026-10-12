#include "halo2/vista_widget_child_bounds.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... args) {
  if(!f)throw FormatError("Missing consumed Vista child-bounds owner");
  return f(std::forward<A>(args)...);
}
std::uint32_t load(std::span<const std::byte> b,std::size_t offset,std::size_t n=4) {
  if(offset>b.size()||n>b.size()-offset)throw FormatError("Vista child-bounds read exceeds retained bank");
  std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[offset+i]))<<(8*i);return v;
}
void store(std::span<std::byte> b,std::size_t offset,std::uint32_t value,std::size_t n=2) {
  if(offset>b.size()||n>b.size()-offset)throw FormatError("Vista child-bounds write exceeds retained bank");
  for(std::size_t i=0;i<n;++i)b[offset+i]=std::byte(std::uint8_t(value>>(8*i)));
}
std::span<std::byte> bank(VistaWidgetIdentity id,const VistaWidgetChildBoundsServices& s) {
  if(!id)throw FormatError("Original Vista child-bounds requires a non-null widget");
  return call(s.bank,id);
}
std::int16_t word(std::span<const std::byte> b,std::size_t offset) {
  return std::bit_cast<std::int16_t>(std::uint16_t(load(b,offset,2)));
}
bool negative_difference(std::uint32_t a,std::uint32_t b) {
  return std::bit_cast<std::int16_t>(std::uint16_t(a-b))<0;
}
}
std::uint8_t vista_widget_bitmap_bounds_excluded(VistaWidgetIdentity id,const VistaWidgetChildBoundsServices& s) {
  const auto record=load(bank(id,s),0x70);
  return record?std::uint8_t(call(s.read,record,std::size_t{1})&1u):0;
}
std::uint32_t vista_widget_kind9_bounds_excluded(VistaWidgetIdentity id,const VistaWidgetChildBoundsServices& s) {
  const auto record=load(bank(id,s),0x70);
  return record?(call(s.read,record,std::size_t{4})&1u):0;
}
void vista_widget_child_bounds(VistaWidgetIdentity id,std::span<std::byte,8> out,const VistaWidgetChildBoundsServices& s) {
  store(out,2,0x7fff);store(out,6,0x8000);store(out,0,0x8000);store(out,4,0x7fff);
  auto child=VistaWidgetIdentity{load(bank(id,s),0x14)};
  while(child) {
    const auto kind=load(bank(child,s),4);
    const bool excluded=kind==8?vista_widget_bitmap_bounds_excluded(child,s)!=0:
        kind==9?vista_widget_kind9_bounds_excluded(child,s)!=0:false;
    if(!excluded) {
      const auto bounds=call(s.bounds,child);
      if(word(bounds,2)<word(out,2))store(out,2,load(bounds,2,2));
      if(word(bounds,6)>word(out,6))store(out,6,load(bounds,6,2));
      if(word(bounds,0)>word(out,0))store(out,0,load(bounds,0,2));
      if(word(bounds,4)<word(out,4))store(out,4,load(bounds,4,2));
    }
    child={load(bank(child,s),0x18)}; //read after providers and output stores.
  }
  if(negative_difference(load(out,6,2),load(out,2,2))||
      negative_difference(load(out,0,2),load(out,4,2)))store(out,0,0,4);
}
} //namespace halo2
