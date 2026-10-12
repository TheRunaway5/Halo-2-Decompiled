#include "halo2/vista_widget_manager_lifetime.hpp"
#include "halo2/cache.hpp"
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Vista manager lifetime reached an unbound genuine service");
  return f(std::forward<A>(a)...);
}
void put(VistaWidgetIdentity id,std::uint32_t offset,std::uint32_t value,
    const VistaWidgetManagerLifetimeServices& s,std::size_t width=4) {
  call(s.write,id.value+offset,value,width);
}
std::uint32_t get(VistaWidgetIdentity id,std::uint32_t offset,
    const VistaWidgetManagerLifetimeServices& s,std::size_t width=4) {
  return call(s.read,id.value+offset,width);
}
void base_construct(VistaWidgetIdentity id,const VistaWidgetManagerLifetimeServices& s) {
  put(id,0,0x7d2858,s);put(id,4,4,s);put(id,8,0,s);put(id,0xc,0,s);
  put(id,0x1c,0,s); // Complete60B0BC at manager+10: only descriptor+C.
  put(id,0x30,0,s);put(id,0x34,0,s);put(id,0x38,0,s);
}
void extended_construct(VistaWidgetIdentity id,const VistaWidgetManagerLifetimeServices& s) {
  base_construct(id,s);put(id,0x3c,0,s);put(id,0x40,0,s,1);
  put(id,0x41,0,s,1);put(id,0,0x7d2888,s);
}
void extended_clear(VistaWidgetIdentity id,const VistaWidgetManagerLifetimeServices& s) {
  for(std::uint32_t at=0x3c;at<=0x60;at+=4)put(id,at,0,s);
  put(id,0x68,0xffffffffu,s); // +64 is deliberately preserved.
}
void base_reset(VistaWidgetIdentity id,const VistaWidgetManagerLifetimeServices& s) {
  for(std::uint32_t at=8;at<=0x38;at+=4)put(id,at,0,s);
}
void base_shutdown(VistaWidgetIdentity id,const VistaWidgetManagerLifetimeServices& s) {
  for(const auto at:{8u,0xcu,0x30u}) {
    const VistaWidgetIdentity child{get(id,at,s)};
    if(child){call(s.dispose,child);put(id,at,0,s);}
  }
  put(id,0x38,0,s);put(id,0x34,0,s); // Source637628 order.
}
} //namespace
VistaWidgetIdentity vista_widget_manager_construct(VistaWidgetIdentity id,
    VistaWidgetManagerBirth kind,const VistaWidgetManagerLifetimeServices& s) {
  switch(kind) {
  case VistaWidgetManagerBirth::base637BA8:base_construct(id,s);break;
  case VistaWidgetManagerBirth::extended637EF6:extended_construct(id,s);break;
  case VistaWidgetManagerBirth::derived639051:
    base_construct(id,s);put(id,0,0x7d2908,s);put(id,0x3c,0,s);put(id,0x40,0,s);break;
  case VistaWidgetManagerBirth::derived63907E:
    base_construct(id,s);put(id,0,0x7d2938,s);break;
  case VistaWidgetManagerBirth::derived639090:
    extended_construct(id,s);put(id,0,0x7d2968,s);break;
  case VistaWidgetManagerBirth::global6390A2:
    base_construct(id,s);put(id,0,0x7d29a0,s);
    for(std::uint32_t at=0x3c;at<=0x60;at+=4)put(id,at,0,s);
    put(id,0x64,0xfa,s);put(id,0x68,0xffffffffu,s);break;
  default:throw FormatError("Unknown Vista manager constructor entry");
  }
  return id;
}
VistaWidgetIdentity vista_widget_ui_construct(VistaWidgetIdentity id,
    const VistaWidgetManagerLifetimeServices& s) {
  vista_widget_manager_construct({id.value+0x2c},VistaWidgetManagerBirth::global6390A2,s);
  for(std::uint32_t i=0;i<5;++i)
    vista_widget_manager_construct({id.value+0x98+i*0x44},VistaWidgetManagerBirth::derived639090,s);
  for(std::uint32_t i=0;i<5;++i)
    vista_widget_manager_construct({id.value+0x1ec+i*0x3c},VistaWidgetManagerBirth::base637BA8,s);
  for(std::uint32_t i=0;i<5;++i)
    vista_widget_manager_construct({id.value+0x318+i*0x3c},VistaWidgetManagerBirth::derived63907E,s);
  for(std::uint32_t i=0;i<5;++i) {
    const VistaWidgetIdentity manager{id.value+0x444+i*0x44};
    extended_construct(manager,s);put(manager,0,0x7cb8ec,s);put(manager,0x40,1,s,1);
  }
  vista_widget_manager_construct({id.value+0x598},VistaWidgetManagerBirth::derived639051,s);
  base_construct({id.value+0x5dc},s);
  // Complete638DF8, including its original +C-before-+8 stores.
  const VistaWidgetIdentity request{id.value+0x21c0};
  put(request,0,0,s);put(request,4,0,s);put(request,0xc,0,s);put(request,8,0,s);
  put(request,0x10,0xffffffffu,s);
  // Complete638B3D.
  put(id,0x21e0,1,s);put(id,0x21e4,0,s);
  // Complete6383EF. The text buffer and unused bytes are preserved.
  const VistaWidgetIdentity queue{id.value+0x21e8};
  put(queue,0,0,s);put(queue,4,5,s);put(queue,8,0,s);put(queue,0xc,0,s);
  put(queue,0x1e04,1,s,1);put(queue,0x88,0,s,2);
  put(queue,0x1e00,queue.value+0x88,s);
  for(std::uint32_t at=0x10;at<=0x84;at+=4)put(queue,at,0,s);
  put(queue,0x1dfc,0xffffffffu,s);return id;
}
void vista_widget_manager_reset(VistaWidgetIdentity id,VistaWidgetManagerReset kind,
    const VistaWidgetManagerLifetimeServices& s) {
  switch(kind) {
  case VistaWidgetManagerReset::base637BD7:base_reset(id,s);break;
  case VistaWidgetManagerReset::extended637F34:put(id,0x3c,0,s);base_reset(id,s);break;
  case VistaWidgetManagerReset::global6390E2:extended_clear(id,s);base_reset(id,s);break;
  default:throw FormatError("Unknown Vista manager reset entry");
  }
}
void vista_widget_manager_shutdown(VistaWidgetIdentity id,VistaWidgetManagerShutdown kind,
    const VistaWidgetManagerLifetimeServices& s) {
  switch(kind) {
  case VistaWidgetManagerShutdown::base637628:base_shutdown(id,s);break;
  case VistaWidgetManagerShutdown::global63910E:
    base_shutdown(id,s);extended_clear(id,s);break;
  case VistaWidgetManagerShutdown::extended638211:
    if(!get(id,0x41,s,1))call(s.extended_close,id);
    for(const auto at:{8u,0xcu,0x30u}) {
      const VistaWidgetIdentity child{get(id,at,s)};
      if(child){call(s.dispose,child);put(id,at,0,s);}
    }
    put(id,0x34,0,s);put(id,0x38,0,s);put(id,0x41,0,s,1);break;
  default:throw FormatError("Unknown Vista manager shutdown entry");
  }
}
void vista_widget_manager_retire(VistaWidgetIdentity id,
    const VistaWidgetManagerLifetimeServices& s) {
  const VistaWidgetIdentity child{get(id,0x30,s)};
  if(child){call(s.dispose,child);put(id,0x30,0,s);}
}
} //namespace halo2
