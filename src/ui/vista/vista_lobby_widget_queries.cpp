#include "halo2/vista_lobby_widget_queries.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <bit>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista lobby widget query service");
  return f(std::forward<A>(a)...);
}
std::uint32_t read(std::span<const std::byte> b,std::size_t o,std::size_t n=4) {
  if(o>b.size()||n>b.size()-o)throw FormatError("Vista lobby query read exceeds retained bank");
  std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[o+i]))<<(8*i);return v;
}
void write(std::span<std::byte> b,std::size_t o,std::uint32_t v,std::size_t n=4) {
  if(o>b.size()||n>b.size()-o)throw FormatError("Vista lobby query write exceeds retained bank");
  for(std::size_t i=0;i<n;++i)b[o+i]=std::byte(std::uint8_t(v>>(8*i)));
}
std::span<std::byte> bank(VistaWidgetIdentity id,const VistaLobbyWidgetQueryServices& s) {
  if(!id)throw FormatError("Vista lobby query requires the original non-null widget");
  return call(s.bank,id);
}
VistaWidgetIdentity field(VistaWidgetIdentity id,std::size_t o,const VistaLobbyWidgetQueryServices& s) {
  return {read(bank(id,s),o)};
}
VistaWidgetIdentity root(VistaWidgetIdentity id,const VistaLobbyWidgetQueryServices& s) {
  auto owner=field(id,0x10,s);
  if(owner) {
    for(;;) {const auto next=field(owner,0x10,s);if(!next)return owner;owner=next;}
  }
  return read(bank(id,s),4)==0?id:VistaWidgetIdentity{};
}
std::uint32_t query(VistaWidgetIdentity id,bool window,const VistaLobbyWidgetQueryServices& s) {
  return window?call(s.virtual38,id):call(s.virtual34,id);
}
VistaWidgetIdentity selected(VistaWidgetIdentity id,std::uint32_t argument,const VistaLobbyWidgetQueryServices& s) {
  // x86 evaluates virtual38 first, then virtual34, then manager609EFD.
  const auto window=query(id,true,s);const auto family=query(id,false,s);
  return call(s.focused,argument,family,window);
}
}
std::uint32_t vista_lobby_widget_duration() noexcept{return 0;}
void vista_lobby_widget_prepare() noexcept{}
void vista_lobby_widget_focus_notification(std::uint32_t) noexcept{}
std::uint32_t vista_lobby_widget_root80(VistaWidgetIdentity id,const VistaLobbyWidgetQueryServices& s) {
  return read(bank(id,s),0x74);
}
std::uint32_t vista_lobby_widget_root84(VistaWidgetIdentity id,const VistaLobbyWidgetQueryServices& s) {
  return read(bank(id,s),0x78);
}
std::uint32_t vista_lobby_widget_virtual34(VistaWidgetIdentity id,const VistaLobbyWidgetQueryServices& s) {
  const auto owner=root(id,s);
  if(!owner)throw FormatError("Original Vista virtual34 has no parent screen");
  return call(s.root80,owner);
}
std::uint32_t vista_lobby_widget_virtual38(VistaWidgetIdentity id,const VistaLobbyWidgetQueryServices& s) {
  const auto owner=root(id,s);return owner?call(s.root84,owner):4u;
}
bool vista_lobby_widget_focused(VistaWidgetIdentity id,std::uint32_t argument,const VistaLobbyWidgetQueryServices& s) {
  if(!root(id,s))return false;
  auto owner=selected(id,argument,s);
  while(owner) {if(owner==id)return true;owner=field(owner,0x10,s);}return false;
}
void vista_lobby_widget_focus(VistaWidgetIdentity id,std::uint32_t argument,std::uint8_t acquire,const VistaLobbyWidgetQueryServices& s) {
  VistaWidgetIdentity old;
  if(acquire) {
    old=selected(id,argument,s);
    const auto window=query(id,true,s);const auto family=query(id,false,s);
    call(s.set_focused,argument,family,window,id);
    call(s.gained,id,argument);
  }else {
    if(!vista_lobby_widget_focused(id,argument,s))return;
    old=selected(id,argument,s);
    // The replacement parent is sampled before either virtual query.
    const auto parent=field(id,0x10,s);
    const auto window=query(id,true,s);const auto family=query(id,false,s);
    call(s.set_focused,argument,family,window,parent);
  }
  while(old) {
    if(vista_lobby_widget_focused(old,argument,s))return;
    call(s.lost,old,argument);
    old=field(old,0x10,s); //re-read after the notification callback.
  }
}
VistaWidgetIdentity vista_lobby_widget_child(VistaWidgetIdentity id,std::uint32_t ordinal,std::uint32_t kind,const VistaLobbyWidgetQueryServices& s) {
  if(kind!=6&&kind!=8)throw FormatError("Unsupported original Vista direct-child query kind");
  auto remaining=std::uint16_t(ordinal);
  if(std::bit_cast<std::int16_t>(remaining)<0)return {};
  auto child=field(id,0x14,s);
  while(child) {
    if(read(bank(child,s),4)==kind) {
      if(remaining==0)return child;
      --remaining;
    }
    child=field(child,0x18,s);
  }
  return {};
}
void vista_lobby_widget_get_bounds(VistaWidgetIdentity id,std::span<std::byte,8> out,const VistaLobbyWidgetQueryServices& s) {
  auto b=bank(id,s);write(out,0,read(b,0x20));write(out,4,read(b,0x24));
}
void vista_lobby_widget_set_bounds(VistaWidgetIdentity id,std::span<const std::byte,8> in,const VistaLobbyWidgetQueryServices& s) {
  auto b=bank(id,s);write(b,0x20,read(in,0));write(b,0x24,read(in,4));
}
void vista_lobby_widget_child_layout(VistaWidgetIdentity id,const VistaLobbyWidgetQueryServices& s) {
  auto b=bank(id,s);
  write(b,0xa20,vista_lobby_widget_child(id,39,8,s).value);
  const auto second=vista_lobby_widget_child(id,40,8,s);
  const auto chat=read(b,0x1c6a,1);write(b,0xa24,second.value);
  if(!chat) {
    write(bank({read(b,0xa20)},s),0x6e,0,1);
    write(bank({read(b,0xa24)},s),0x6e,0,1);
  }
  if(const auto text=vista_lobby_widget_child(id,51,6,s)) {
    std::array<std::byte,8> bounds;vista_lobby_widget_get_bounds(text,bounds,s);
    //642F78 uses DWORDs, retains low16 of2*bottom-top; other WORDs survive.
    write(bounds,4,2u*read(bounds,4)-read(bounds,0),2);
    vista_lobby_widget_set_bounds(text,bounds,s);
  }
  call(s.base_layout,id);
}
} //namespace halo2
