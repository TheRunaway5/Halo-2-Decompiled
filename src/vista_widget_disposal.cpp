#include "halo2/vista_widget_disposal.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista widget disposal service");
  return f(std::forward<A>(a)...);
}
std::uint32_t read(const VistaWidgetDisposalServices& s,std::uint32_t at,std::size_t size=4) {
  return call(s.read,at,size);
}
void write(const VistaWidgetDisposalServices& s,std::uint32_t at,std::uint32_t value,std::size_t size=4) {
  call(s.write,at,value,size);
}
} //namespace
void vista_widget_disposal_detach(VistaWidgetIdentity parent,VistaWidgetIdentity child,
    const VistaWidgetDisposalServices& s) {
  auto current=read(s,parent.value+0x14);
  while(current&&current!=child.value)current=read(s,current+0x18);
  if(!current)return;
  auto previous=read(s,child.value+0x1c);
  if(previous)write(s,previous+0x18,read(s,child.value+0x18));
  const auto next=read(s,child.value+0x18);
  if(next){previous=read(s,child.value+0x1c);write(s,next+0x1c,previous);}
  if(child.value==read(s,parent.value+0x14))write(s,parent.value+0x14,read(s,child.value+0x18));
  write(s,child.value+0x10,0);write(s,child.value+0x18,0);
  write(s,child.value+0x1c,0);write(s,child.value+0xc,0xffffffffu);
}
void vista_widget_dispose_kinds(VistaWidgetIdentity screen,std::uint32_t first,
    std::uint32_t last,const VistaWidgetDisposalServices& s) {
  if(std::bit_cast<std::int32_t>(first)>std::bit_cast<std::int32_t>(last))return;
  auto count=last-first+1;auto slot=screen.value+first*4+0xa04;
  do {
    const VistaWidgetIdentity child{read(s,slot)};
    if(child.value) {
      if(read(s,child.value+0x10)==screen.value)vista_widget_disposal_detach(screen,child,s);
      if(read(s,child.value+0x6c,1)) {
        auto vtable=read(s,child.value);const auto shutdown=read(s,vtable+8);
        call(s.shutdown,child,shutdown);
        vtable=read(s,child.value);const auto destructor=read(s,vtable);
        write(s,child.value+0x6c,0,1);
        call(s.destructor,child,destructor,0u);call(s.free,child);
      }
      write(s,slot,0);
    }
    slot+=4;
  }while(--count);
}
} //namespace halo2
