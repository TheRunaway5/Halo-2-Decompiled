#include "halo2/vista_lobby_factory.hpp"
#include "halo2/cache.hpp"
#include <utility>

namespace halo2 {
namespace {
std::uint32_t read(std::span<const std::byte> b,std::size_t o,std::size_t n=4) {
  if(o>b.size()||n>b.size()-o)throw FormatError("Vista lobby factory read exceeds its retained bank");
  std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::uint32_t(std::to_integer<std::uint8_t>(b[o+i]))<<(8*i);return v;
}
void write(std::span<std::byte> b,std::size_t o,std::uint32_t v,std::size_t n=4) {
  if(o>b.size()||n>b.size()-o)throw FormatError("Vista lobby factory write exceeds its retained bank");
  for(std::size_t i=0;i<n;++i)b[o+i]=std::byte(std::uint8_t(v>>(8*i)));
}
template<class F,class... A> decltype(auto) call(const F& f,A&&... args) {
  if(!f)throw FormatError("Vista lobby factory/manager reached an unbound genuine service");
  return f(std::forward<A>(args)...);
}
template<class F,class... A> decltype(auto) virtual_call(const F& f,VistaWidgetIdentity id,A&&... args) {
  if(!id)throw FormatError("Vista lobby manager reached a null virtual source object");
  return call(f,id,std::forward<A>(args)...);
}
std::span<std::byte> bank(VistaWidgetIdentity id,const VistaLobbyManagerServices& s) {
  if(!id)throw FormatError("Vista lobby factory/manager reached a null source object");
  return call(s.bank,id);
}
VistaLobbyFactoryDescriptor descriptor(std::span<std::byte> b) {
  if(b.size()<0x30)throw FormatError("Vista lobby manager has no retained descriptor");
  return std::span<std::byte,32>(b.data()+0x10,32);
}
VistaWidgetIdentity slot(std::span<const std::byte> b,std::size_t o){return{read(b,o)};}
} //namespace

void vista_lobby_manager_pending(VistaWidgetIdentity manager,VistaWidgetIdentity screen,
    VistaLobbyFactoryDescriptor request,const VistaLobbyManagerServices& s) {
  auto b=bank(manager,s);
  if(slot(b,0xc)==screen)write(b,0xc,0);
  const auto old=slot(b,0xc);
  if(old){call(s.dispose,old);write(b,0xc,0);}
  write(b,0xc,screen.value);
  // REP MOVSD is forward and each source DWORD is read after the prior store.
  for(std::size_t i=0;i<8;++i)write(b,0x10+4*i,read(request,4*i));
}

void vista_lobby_register_screen(VistaWidgetIdentity screen,VistaLobbyFactoryDescriptor request,
    const VistaLobbyManagerServices& s) {
  const auto window=virtual_call(s.window,screen);
  std::uint32_t address{};
  switch(read(request,4)) {
  case 0:address=window==4?0xd71e78u:0;break;
  case 1:address=0xd71bf8u+window*60u;break;
  case 2:address=window==4?0xd71ebcu:0;break;
  case 3:address=0xd71accu+window*60u;break;
  case 4:address=0xd71d24u+window*68u;break;
  case 5:address=0xd71978u+window*68u;break;
  default:break;
  }
  if(!address){virtual_call(s.destructor,screen,0u);call(s.free,screen);return;}
  if(!(read(request,0,1)&1u)&&read(bank(screen,s),0x70)!=9)call(s.sound,3u);
  call(s.pending,VistaWidgetIdentity{address},screen,request);
}

VistaWidgetIdentity vista_lobby_factory(bool alternate,VistaLobbyFactoryDescriptor request,
    VistaLobbyFactoryGlobals& globals,const VistaLobbyFactoryServices& s) {
  const auto screen=call(s.allocate,std::uint32_t(vista_lobby_constructor_bank_size),0u);
  if(screen) {
    // These three source loads occur after allocation, before the constructor.
    const VistaLobbyConstructorArguments args{read(request,4),read(request,8),read(request,2,2)};
    vista_lobby_construct(bank(screen,s.manager),screen.value,args,globals.construction,s.read_cached_ui_clock);
  }
  globals.current=screen;
  write(bank(screen,s.manager),0x6c,1,1);
  vista_lobby_register_screen(screen,request,s.manager);
  globals.alternate=alternate?1:0; //actual60B36B, strictly after registration.
  return screen;
}

void vista_lobby_manager_activate(VistaWidgetIdentity manager,VistaLobbyManagerGraphics& graphics,
    const VistaLobbyManagerServices& s) {
  const auto renderer=graphics.identity;
  if(renderer&&call(s.graphics,renderer)<0){graphics.failed=1;return;}
  auto b=bank(manager,s);
  auto pending=slot(b,0xc);
  if(pending) {
    // BL is captured before virtual54 and survives descriptor mutation/reentry.
    const auto immediate=read(b,0x10,1)&1u;
    virtual_call(s.prepare,pending);
    call(s.focus,slot(b,0xc),0u,1u);
    virtual_call(s.initialize,slot(b,0xc),descriptor(b));
    virtual_call(s.child_layout,slot(b,0xc));
    virtual_call(s.base_layout,slot(b,0xc));
    call(s.animate,slot(b,0xc),immediate);
    if(immediate)virtual_call(s.parameters,slot(b,0xc),std::span<std::byte,16>(b.data()+0x20,16));
    if(slot(b,8)) {
      const auto retiring=slot(b,0x30);
      if(retiring)call(s.dispose,retiring);
      const auto active=slot(b,8);
      if(active){write(b,0x30,active.value);call(s.animate,active,immediate?2u:3u);}
    }
    pending=slot(b,0xc);
    write(b,8,pending.value);write(b,0xc,0);
    if(!call(s.focused,pending,0u))call(s.focus,slot(b,8),0u,1u);
  }
  pending=slot(b,0xc);if(pending)virtual_call(s.update,pending);
  const auto active=slot(b,8);if(active)virtual_call(s.update,active);
  const auto retiring=slot(b,0x30);if(retiring)virtual_call(s.update,retiring);
}
} //namespace halo2
