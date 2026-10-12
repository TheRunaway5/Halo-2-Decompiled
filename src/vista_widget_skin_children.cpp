#include "halo2/vista_widget_skin_children.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista skin-child service");
  return f(std::forward<A>(a)...);
}
std::uint32_t get(std::span<const std::byte> bytes,std::size_t at,unsigned n=2) {
  std::uint32_t value{};
  for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(bytes[at+i])<<(i*8);
  return value;
}
void put(std::span<std::byte> bytes,std::size_t at,std::uint32_t value,unsigned n=2) {
  for(unsigned i=0;i<n;++i)bytes[at+i]=std::byte(value>>(i*8));
}
void translate(VistaWidgetSkinBounds& bytes,std::uint32_t x,std::uint32_t y) {
  put(bytes,2,get(bytes,2)+x);put(bytes,6,get(bytes,6)+x);
  put(bytes,0,get(bytes,0)+y);put(bytes,4,get(bytes,4)+y);
}
void store(VistaWidgetIdentity id,std::size_t offset,std::span<const std::byte> value,
    const VistaWidgetSkinServices& s) {
  if(!id || std::uint64_t(id.value)+offset+value.size()>0x100000000ull)
    throw FormatError("Invalid Vista skin child source32 extent");
  auto bytes=call(s.bank,id);
  if(bytes.size()<offset+value.size())throw FormatError("Truncated Vista skin child bank");
  //6116D2 copies two ordered DWORDs, rather than a memmove snapshot.
  if(value.size()==8) {
    const auto first=get(value,0,4);put(bytes,offset,first,4);
    const auto second=get(value,4,4);put(bytes,offset+4,second,4);
  } else for(std::size_t i=0;i<value.size();++i)bytes[offset+i]=value[i];
}
void identifier(VistaWidgetIdentity id,std::uint32_t ordinal,const VistaWidgetSkinServices& s) {
  std::array<std::byte,2> bytes{};put(bytes,0,ordinal);store(id,0xa,bytes,s);
}
} //namespace
void vista_widget_skin_children(VistaWidgetIdentity parent,std::span<const std::byte,4> point,
    std::uint32_t skin,const VistaWidgetSkinServices& s) {
  const auto read=[&](std::uint32_t at,std::size_t n=4){return call(s.read,at,n);};
  const auto count=[&](std::uint32_t off){return std::bit_cast<std::int32_t>(read(skin+off));};
  const auto record=[&](std::uint32_t off,std::uint32_t displacement) {
    const auto offset=read(skin+off);
    const auto base=offset==0xffffffffu?0u:read(0x882290u)+offset;
    return base+displacement;
  };
  const auto adjusted=[&](VistaWidgetIdentity child) {
    auto bounds=call(s.bounds,child);
    const auto top=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(get(bounds,0))));
    const auto bottom=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(get(bounds,4))));
    translate(bounds,0,std::uint32_t(bottom>=top?bottom-top:top-bottom));
    return bounds;
  };
  for(std::int32_t i=0;i<count(0x24);++i) {
    const auto definition=record(0x28,std::uint32_t(i)*0x38u);
    const auto child=call(s.bitmap,parent,definition);
    if(child) {
      auto bounds=adjusted(child);
      const auto x=get(point,0),y=get(point,2);
      translate(bounds,x,y);store(child,0x20,bounds,s);identifier(child,std::uint32_t(i),s);
    }
  }
  for(std::int32_t i=0;i<count(0x2c);++i) {
    const auto definition=record(0x30,std::uint32_t(i)*0x24u);
    const auto child=call(s.kind9,parent,definition);
    if(child) {
      auto bounds=adjusted(child);
      const auto x=get(point,0),y=get(point,2);
      translate(bounds,x,y);store(child,0x20,bounds,s);identifier(child,std::uint32_t(i),s);
    }
  }
  for(std::int32_t i=0;i<count(0x34);++i) {
    const auto definition=record(0x38,std::uint32_t(i)*0x18u);
    for(std::int32_t row=0;row<std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(definition+0x11,1))));++row) {
      const auto child=call(s.grid,parent,point,std::uint32_t(row),definition);
      if(child) {
        (void)adjusted(child); // Source translates this scratch only; no6116D2.
        identifier(child,std::uint32_t(i),s);
      }
    }
  }
  for(std::int32_t i=0;i<count(0x1c);++i) {
    const auto definition=record(0x20,std::uint32_t(i)*0x2cu);
    const auto child=call(s.text,parent,std::uint32_t(i),definition);
    if(child) {
      auto bounds=call(s.bounds,child);
      const auto x=get(point,0),y=get(point,2);
      translate(bounds,x,y);store(child,0x20,bounds,s);
    }
  }
}
} //namespace halo2
