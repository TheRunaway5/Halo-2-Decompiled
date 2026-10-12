#include "halo2/vista_widget_child_creation.hpp"
#include "halo2/vista_widget_constructor.hpp"
#include "halo2/cache.hpp"
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista child creation owner");
  return f(std::forward<A>(a)...);
}
std::span<std::byte> bank(VistaWidgetIdentity id,std::size_t count,
    const VistaWidgetAssetBirthServices& s) {
  if(!id || std::uint64_t(id.value)+count>0x100000000ull)
    throw FormatError("Invalid consumed Vista child creation identity");
  auto b=call(s.bank,id);
  if(b.size()<count)throw FormatError("Truncated consumed Vista child creation bank");
  return b;
}
std::uint32_t get(std::span<const std::byte> b,std::size_t at,unsigned n=4) {
  std::uint32_t v{};
  for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);
  return v;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));
}
std::uint32_t read(const VistaWidgetAssetBirthServices& s,std::uint32_t at,
    std::size_t n=4) {return call(s.read,at,n);}
void append(VistaWidgetIdentity parent,VistaWidgetIdentity child,
    const VistaWidgetChildCreationServices& s) {
  put(bank(child,0x70,s.birth),0x6c,1,1);
  VistaWidgetRegistrationServices registration;
  registration.bank=s.birth.bank;
  vista_widget_append(parent,child,registration);
}
std::uint32_t controller_index(std::uint32_t mask) {
  for(unsigned i=0;i<4;++i)if(mask&(1u<<i))return i;
  return 0xffffffffu;
}
} //namespace

VistaWidgetIdentity vista_widget_kind9_construct(VistaWidgetIdentity id,
    std::uint32_t definition,const VistaWidgetAssetBirthServices& s) {
  auto b=bank(id,0x88,s);
  vista_widget_construct(b,id,9,0,s.cached_ui_clock);
  put(b,0,0x7d3b14);put(b,0x70,definition);put(b,0x84,0xffff,2);
  for(unsigned offset=0x74;offset<0x84;offset+=4)put(b,offset,0);
  auto current=get(b,0x70);
  put(b,0x20,read(s,current+0x1c));put(b,0x24,read(s,current+0x20));
  put(b,0x68,read(s,definition+4,2),2);
  current=get(b,0x70);
  put(b,0x6a,read(s,definition+8,2),2);
  const auto bitmap=read(s,current+0x10);
  if(bitmap!=0xffffffffu)call(s.bitmap_residency,bitmap);
  return id;
}

VistaWidgetIdentity vista_widget_skin_text_construct(VistaWidgetIdentity id,
    std::uint32_t index,bool short_string,const VistaWidgetAssetBirthServices& s) {
  auto b=bank(id,0xbe,s);
  const auto mask=index==0xffffffffu?0u:1u<<(index&31u);
  vista_widget_text_base_construct(b,id,mask,s.cached_ui_clock);
  put(b,0,short_string?0x7cf244u:0x7cf29cu);
  vista_widget_string_construct(b.subspan(0x78),id.value+0x78u);
  put(b,0x78,short_string?0x7cf218u:0x7cf22cu);put(b,0xbc,0,2);
  return id;
}

VistaWidgetIdentity vista_widget_create_bitmap(VistaWidgetIdentity parent,
    std::uint32_t definition,const VistaWidgetChildCreationServices& s) {
  const auto id=call(s.allocate,0x98u,0u);
  if(id) {vista_widget_bitmap_construct(id,definition,s.birth);append(parent,id,s);}
  return id;
}
VistaWidgetIdentity vista_widget_create_kind9(VistaWidgetIdentity parent,
    std::uint32_t definition,const VistaWidgetChildCreationServices& s) {
  const auto id=call(s.allocate,0x88u,0u);
  if(id) {vista_widget_kind9_construct(id,definition,s.birth);append(parent,id,s);}
  return id;
}
VistaWidgetIdentity vista_widget_create_grid(VistaWidgetIdentity parent,
    std::span<const std::byte,4> position,std::uint32_t row,
    std::uint32_t definition,const VistaWidgetChildCreationServices& s) {
  const auto id=call(s.allocate,0x78u,0u);
  if(id) {
    vista_widget_grid_construct(id,row,definition,s.birth);append(parent,id,s);
    call(s.grid_initialize,id,position);
  }
  return id;
}
VistaWidgetIdentity vista_widget_create_text(VistaWidgetIdentity parent,
    std::uint32_t ordinal,std::uint32_t definition,const VistaWidgetChildCreationServices& s) {
  const bool short_string=(read(s.birth,definition,1)&0x10u)!=0;
  const auto id=call(s.allocate,short_string?0xfcu:0x4bcu,0u);
  if(id) {
    const auto controller=controller_index(get(bank(parent,0xa,s.birth),8,2));
    vista_widget_skin_text_construct(id,controller,short_string,s.birth);
    append(parent,id,s);
    call(s.text_descriptor,id,ordinal,definition);
    call(s.assign_text,id,0x79bd7cu);
  }
  return id;
}
} //namespace halo2
