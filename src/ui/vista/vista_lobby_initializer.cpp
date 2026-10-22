#include "halo2/vista_lobby_initializer.hpp"
#include "halo2/vista_lobby_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <set>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& function,A&&... arguments) {
  if(!function)throw FormatError("Missing consumed Vista lobby initializer owner");
  return function(std::forward<A>(arguments)...);
}
void require(std::span<const std::byte> bank,std::size_t end) {
  if(bank.size()<end)throw FormatError("Truncated consumed Vista lobby initializer bank");
}
std::uint32_t read(std::span<const std::byte> bank,std::size_t offset,unsigned size=4) {
  require(bank,offset+size);std::uint32_t value{};
  for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(bank[offset+i])<<(8*i);
  return value;
}
void write(std::span<std::byte> bank,std::size_t offset,std::uint32_t value,unsigned size=4) {
  require(bank,offset+size);
  for(unsigned i=0;i<size;++i)bank[offset+i]=std::byte(std::uint8_t(value>>(8*i)));
}
std::span<std::byte> bank(VistaWidgetIdentity identity,const VistaLobbyIdentifierServices& services) {
  if(!identity)throw FormatError("Missing source widget in Vista identifier traversal");
  return call(services.bank,identity);
}
void link_callback(std::span<std::byte> screen,std::uint32_t base,std::size_t head,std::size_t node) {
  write(screen,node+8,base+std::uint32_t(head));
  auto previous=read(screen,head);
  if(!previous){write(screen,head,base+std::uint32_t(node));return;}
  std::set<std::uint32_t> visited;
  for(;;) {
    if(previous<base || std::uint64_t(previous)-base+12>screen.size() || !visited.insert(previous).second)
      throw FormatError("Unowned or cyclic Vista lobby callback chain");
    const auto offset=std::size_t(previous-base);const auto next=read(screen,offset+4);
    if(next){previous=next;continue;}
    write(screen,offset+4,base+std::uint32_t(node));write(screen,node,previous);return;
  }
}
void validate_map(std::span<std::byte> screen,std::span<std::byte> globals,
    const VistaLobbyInitializerServices& services) {
  VistaLobbyInitializerMapOutputs selected;
  const auto variant=call(services.variant);
  const auto valid=call(services.selected_map,selected);
  bool admitted=false;
  if(valid && variant) {
    if(!selected.first)throw FormatError("Missing consumed Vista selected-map first output");
    if(*selected.first!=0xffffffff)admitted=true;
    else {
      const auto engine=std::bit_cast<std::int32_t>(read(*variant,0x44));
      admitted=engine>=1 && engine<=9;
    }
  }
  if(!admitted)write(screen,0x3486,1,1);
  if(!call(services.favorite) || !call(services.host))return;
  VistaLobbyInitializerMapOutputs favorite_map;
  const auto favorite_variant=call(services.variant);
  (void)call(services.selected_map,favorite_map); // Source ignores AL here.
  if(favorite_map.third) {
    write(globals,0x135,0,1);write(globals,0x130,favorite_map.third);
    write(globals,0x138,1);write(globals,0x140,1,1);
  }
  if(!favorite_variant)throw FormatError("Missing consumed Vista favorite variant owner");
  if(!(read(*favorite_variant,0,1)&1)) {
    require(*favorite_variant,0x130);write(globals,0x134,0,1);
    std::copy_n(favorite_variant->begin(),0x130,globals.begin());
    write(globals,0x13c,1);write(globals,0x140,1,1);
  }
}
} //namespace

void vista_lobby_assign_widget_identifiers(VistaWidgetIdentity widget,
    const VistaLobbyIdentifierServices& services) {
  if(services.observed)services.observed(widget);
  auto current=bank(widget,services);auto root=VistaWidgetIdentity{read(current,0x10)};
  std::set<std::uint32_t> ancestors;
  while(root) {
    if(!ancestors.insert(root.value).second)throw FormatError("Cyclic Vista widget parent chain");
    const auto parent=read(bank(root,services),0x10);
    if(!parent)break;
    root={parent};
  }
  if(!root && read(current,4)==0)root=widget;
  auto root_bank=bank(root,services);
  write(root_bank,0x7c,read(root_bank,0x7c)+1);
  const auto index=read(root_bank,0x7c);
  auto child=VistaWidgetIdentity{read(current,0x14)};
  write(current,0xc,index);
  std::set<std::uint32_t> siblings;
  while(child) {
    if(!siblings.insert(child.value).second)throw FormatError("Cyclic Vista widget child chain");
    call(services.child_method,child);
    child={read(bank(child,services),0x18)};
  }
}

void vista_lobby_chat_manager_construct(std::span<std::byte> manager,std::uint32_t base) {
  require(manager,vista_lobby_chat_manager_bank_size);
  if(!base || std::uint64_t(base)+vista_lobby_chat_manager_bank_size>(std::uint64_t{1}<<32))
    throw FormatError("Invalid Vista chat manager source identity");
  write(manager,0,0);write(manager,4,5);write(manager,8,0);write(manager,0xc,0);
  write(manager,0x1e04,1,1);write(manager,0x88,0,2);write(manager,0x1e00,base+0x88);
  for(unsigned i=0;i<30;++i)write(manager,0x10+4*i,0);
  write(manager,0x1dfc,0xffffffff);
}

void vista_lobby_chat_manager_clear(std::span<std::byte> manager,
    const std::function<void(std::uint32_t)>& release) {
  require(manager,vista_lobby_chat_manager_bank_size);
  for(unsigned i=0;i<30;++i) {
    const auto offset=0x10+4*i;const auto string=read(manager,offset);
    if(string)call(release,string);
    write(manager,offset,0);
  }
  write(manager,0,0);write(manager,8,0);write(manager,0x1e04,1,1);
}

void vista_lobby_initialize(VistaLobbyInitializerState& state,
    const VistaLobbyInitializerServices& services) {
  const auto base=state.screen.value;
  if(!base || std::uint64_t(base)+0x3594>(std::uint64_t{1}<<32) || !state.registration_table)
    throw FormatError("Invalid Vista lobby initializer source identities");
  auto screen=call(services.bank,state.screen);require(screen,0x3594);
  require(state.descriptor,32);require(state.globals,0x145);
  require(state.chat_manager,vista_lobby_chat_manager_bank_size);
  const auto definition=call(services.definition,read(screen,0x70));
  const auto context=call(services.context);const auto chat=call(services.chat_available,context);
  write(screen,0x1c6a,chat,1);
  if(!chat){write(screen,0xdca,0,1);write(screen,0x141a,0,1);}
  const auto factory=call(services.factory_flag);
  std::array<VistaWidgetIdentity,5> buttons{};unsigned count{};
  buttons[count++]={base+0xa5c};buttons[count++]={base+0xb5c};
  if(factory){buttons[count++]={base+0xc5c};buttons[count++]={base+0x12a4};}
  if(chat)buttons[count++]={base+0xd5c};
  VistaWidgetRegistration registration{{0,1,count,state.registration_table,0,0},{},std::span(buttons).first(count)};
  call(services.register_children,definition,registration);
  if(call(services.factory_flag)) {
    const auto caption=vista_lobby_layout_players_caption(1,call(services.language));
    const auto style=call(services.style,VistaWidgetIdentity{base+0x12a4});
    call(services.assign_string,style.identity,*caption);
    std::copy_n(screen.begin()+0x12c4,8,screen.begin()+0x358c);
    std::copy_n(screen.begin()+0xa7c,8,screen.begin()+0x3584);
  }
  link_callback(screen,base,0xb58,0x348c);link_callback(screen,base,0xc58,0x34a4);
  link_callback(screen,base,0xd58,0x34bc);
  if(read(screen,0x1c6a,1))link_callback(screen,base,0xe58,0x34d4);
  link_callback(screen,base,0x13a0,0x34ec);
  const auto chat_style=call(services.style,VistaWidgetIdentity{base+0xd5c});
  call(services.assign_string,chat_style.identity,std::u16string_view{});write(screen,0x12a0,0);
  write(screen,0x3508,0,1);write(screen,0x3500,0,1);write(screen,0x3504,0);write(screen,0x350c,0xffffffff);
  vista_lobby_assign_widget_identifiers(state.screen,{services.bank,services.identifier_child,services.identifier_observed});
  if(call(services.host)) {
    call(services.activate_focus,VistaWidgetIdentity{base+0xa5c});write(screen,0x13a8,base+0xa5c);
  }else{call(services.focus,state.screen,0u,1u);write(screen,0x13a8,0);}
  if(!(read(state.descriptor,0,1)&1))validate_map(screen,state.globals,services);
  (void)call(services.prepare_text,std::u16string_view{u"0123456789"},4u);
  write(state.globals,0x144,0,1);
  vista_lobby_chat_manager_clear(state.chat_manager,services.release_owned_string);
  write(state.chat_manager,0x1e04,1,1); // Separate638487 after second60B37B getter.
  write(screen,0x3514,0xffffffff);std::fill_n(screen.begin()+0x3518,0x60,std::byte{});
  write(screen,0x3578,1,1);write(screen,0x3579,0,1);write(screen,0x357c,0);write(screen,0x3580,2);
  const auto text=call(services.text,std::int16_t{50});
  if(text && read(screen,0x1c6a,1)) {
    const auto bounds=call(services.animated_bounds,text);
    const auto first_style=call(services.style,text);
    const std::array<std::uint32_t,4> vector{0x3f800000,read(first_style.bytes,8),read(first_style.bytes,0xc),read(first_style.bytes,0x10)};
    const VistaWidgetIdentity clone{base+0x13ac};call(services.append,clone);
    const auto second_style=call(services.style,text);const auto font=read(second_style.bytes,4);
    const auto animation=std::uint16_t(read(call(services.bank,text),0x68,2));
    call(services.initialize_clone,clone,1u,animation,vector,font,bounds);write(screen,0x1420,1,1);
  }
  write(state.globals,0x135,0,1);write(state.globals,0x130,0);write(state.globals,0x138,0);write(state.globals,0x134,0,1);
  std::fill_n(state.globals.begin(),0x130,std::byte{});write(state.globals,0x13c,0);
}
} //namespace halo2
