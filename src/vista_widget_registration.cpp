#include "halo2/vista_widget_registration.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista widget registration service");
  return f(std::forward<A>(a)...);
}
void extent(std::span<const std::byte> b,std::size_t at,std::size_t n) {
  if(at>b.size() || b.size()-at<n)throw FormatError("Unowned Vista widget registration field");
}
std::uint32_t read(std::span<const std::byte> b,std::size_t at,unsigned n) {
  extent(b,at,n);std::uint32_t v{};
  for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned char>(b[at+i]))<<(8*i);
  return v;
}
void write(std::span<std::byte> b,std::size_t at,std::uint32_t v,unsigned n) {
  extent(b,at,n);for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));
}
std::span<std::byte> bank(const VistaWidgetRegistrationServices& s,VistaWidgetIdentity id) {
  if(!id)throw FormatError("Null consumed Vista widget identity");
  return call(s.bank,id);
}
std::uint32_t field(const VistaWidgetRegistrationServices& s,VistaWidgetIdentity id,std::size_t at,unsigned n=4) {
  return read(bank(s,id),at,n);
}
void set_field(const VistaWidgetRegistrationServices& s,VistaWidgetIdentity id,std::size_t at,std::uint32_t value,unsigned n=4) {
  write(bank(s,id),at,value,n);
}
VistaWidgetIdentity link(const VistaWidgetRegistrationServices& s,VistaWidgetIdentity id,std::size_t at) {
  return {field(s,id,at)};
}
void dynamic_children(VistaWidgetLifetime& state,const VistaWidgetRegistrationServices& s) {
  ++state.pane_generation;
  const auto pane=call(s.pane,state.screen);
  if(pane) {
    const bool animate=(field(s,state.screen,0x42,1)&3)!=0;
    auto create=[&](VistaWidgetCreateKind kind,const VistaWidgetRecord& record,
                    std::int32_t element,std::size_t ordinal,bool assign) {
      const auto child=call(s.create,state.screen,VistaWidgetCreate{kind,record,element});
      if(!child)return;
      if(animate)call(s.animate,child,field(s,state.screen,0x34));
      if(assign)set_field(s,child,0x0a,std::uint16_t(ordinal),2);
    };
    for(std::size_t i=0;i<pane->bitmaps.size();++i)create(VistaWidgetCreateKind::bitmap,pane->bitmaps[i],0,i,true);
    for(std::size_t i=0;i<pane->scenes.size();++i)create(VistaWidgetCreateKind::scene,pane->scenes[i],0,i,true);
    for(std::size_t i=0;i<pane->grids.size();++i) {
      for(std::int32_t row=0;row<std::bit_cast<std::int8_t>(std::uint8_t(read(pane->grids[i].bytes,0x11,1)));++row)
        create(VistaWidgetCreateKind::grid,pane->grids[i],row,i,true);
    }
    for(std::size_t i=0;i<pane->texts.size();++i) {
      if(i>0x7fffffffu)throw FormatError("Unowned Vista text index");
      create(VistaWidgetCreateKind::text,pane->texts[i],std::int32_t(i),i,false);
    }
    for(std::size_t i=0;i<pane->tables.size();++i)create(VistaWidgetCreateKind::table,pane->tables[i],0,i,true);
  }
  call(s.base_layout,state.screen);
}
void embedded(VistaWidgetLifetime& state,const VistaWidgetRegistration& r,const VistaWidgetRegistrationServices& s) {
  const auto ordinal=std::bit_cast<std::int16_t>(std::uint16_t(field(s,state.screen,0x9f8,2)));
  if(ordinal<0 || ordinal>=std::bit_cast<std::int32_t>(r.words[1]))return;
  if(ordinal!=0)throw FormatError("Unowned Vista multi-pane registration descriptor");
  const auto screen=call(s.screen,field(s,state.screen,0x70));
  const auto pane=call(s.pane,state.screen);
  if(!screen || !pane)return;
  set_field(s,state.screen,0x68,std::uint16_t(pane->animation-1),2);
  const auto count=std::bit_cast<std::int32_t>(r.words[2]);
  if(count>0) {
    const auto n=std::min(pane->button_count,count);
    if(n>0 && std::size_t(n)>r.buttons.size())throw FormatError("Unowned Vista button identity table");
    for(std::int32_t i=0;i<n;++i) {
      const auto child=r.buttons[std::size_t(i)];
      vista_widget_append(state.screen,child,s);call(s.button,child);
    }
  } else if(r.words[4]) {
    const VistaWidgetIdentity list{r.words[4]};
    vista_widget_append(state.screen,list,s);call(s.list,list,std::uint8_t{});
    if(screen->flags&2) {
      set_field(s,list,0xa6,1,1);
      call(s.list_callback,list.value+0xa8,state.screen.value+0xa44);
    }
  }
  dynamic_children(state,s);
}
}

void vista_widget_detach(VistaWidgetIdentity parent,VistaWidgetIdentity child,const VistaWidgetRegistrationServices& s) {
  auto current=link(s,parent,0x14);
  while(current && current!=child)current=link(s,current,0x18);
  if(!current)return;
  const auto previous=link(s,child,0x1c),next=link(s,child,0x18);
  if(previous)set_field(s,previous,0x18,next.value);
  if(next)set_field(s,next,0x1c,previous.value);
  if(link(s,parent,0x14)==child)set_field(s,parent,0x14,next.value);
  set_field(s,child,0x10,0);set_field(s,child,0x18,0);set_field(s,child,0x1c,0);set_field(s,child,0x0c,0xffffffffu);
}
void vista_widget_append(VistaWidgetIdentity parent,VistaWidgetIdentity child,const VistaWidgetRegistrationServices& s) {
  const auto old=link(s,child,0x10);
  if(old)vista_widget_detach(old,child,s);
  auto last=link(s,parent,0x14);
  if(last) {
    for(auto next=link(s,last,0x18);next;next=link(s,last,0x18))last=next;
    set_field(s,last,0x18,child.value);set_field(s,child,0x10,parent.value);set_field(s,child,0x1c,last.value);
  } else {set_field(s,parent,0x14,child.value);set_field(s,child,0x10,parent.value);}
}
void vista_widget_register(VistaWidgetLifetime& state,const VistaWidgetRegistration& r,const VistaWidgetRegistrationServices& s) {
  //610A44 resolves metadata before initializing the two retained text owners.
  const auto screen=call(s.screen,field(s,state.screen,0x70));
  call(s.header,state.screen);call(s.footer,state.screen,1u);
  if(!screen)return;
  set_field(s,state.screen,0x9fc,(screen->flags>>5)&1,1);
  if(screen->pane_count<=0)return;
  if(screen->pane_count>1 && r.words[0])throw FormatError("Unowned Vista multi-list registration descriptor");
  if(screen->pane_count==1 || (screen->flags&2)) {
    if(r.words[1]==0xffffffffu)throw FormatError("Unowned forced Vista widget registration");
    set_field(s,state.screen,0x9f8,0,2);
    embedded(state,r,s);
  }
  call(s.shape,state.screen,std::uint16_t(screen->shape-1));
}
void vista_widget_change_pane(VistaWidgetLifetime& state,std::uint16_t request_word,const VistaWidgetRegistrationServices& s) {
  const auto delta=std::bit_cast<std::int8_t>(std::uint8_t(field(s,state.screen,0x9fb,1)));
  const auto desired=std::uint16_t(std::int32_t(delta)+request_word);
  auto child=link(s,state.screen,0x14);
  //60EF5E clears the retained lookup cache before any shutdown callback.
  for(std::size_t at=0xa04;at<=0xa40;at+=4)set_field(s,state.screen,at,0);
  while(child) {
    const auto kind=field(s,child,4);
    const auto next=link(s,child,0x18);
    const bool embedded_text=kind==6 && (child.value==state.screen.value+0x80 || child.value==state.screen.value+0x53c);
    const bool retained_kind=kind==0 || kind==1 || kind==3 || kind==4 || kind==5;
    if(!retained_kind && !embedded_text) {
      vista_widget_detach(state.screen,child,s);call(s.shutdown,child);
      if(field(s,child,0x6c,1)) {call(s.destructor,child,0u);call(s.free,child);}
    }
    child=next;
  }
  set_field(s,state.screen,0x9f8,desired,2);
  dynamic_children(state,s);
}
} //namespace halo2
