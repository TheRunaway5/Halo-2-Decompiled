#include "halo2/vista_widget_locale.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista widget locale service");
  return f(std::forward<A>(a)...);
}
std::uint32_t read(const VistaWidgetLocaleServices& s,std::uint32_t at) {
  return call(s.read,at,std::size_t{4});
}
void write(const VistaWidgetLocaleServices& s,std::uint32_t at,std::uint32_t value,std::size_t size=4) {
  call(s.write,at,value,size);
}
void assign(VistaWidgetIdentity widget,std::uint32_t scratch,const VistaWidgetLocaleServices& s) {
  const auto widget_vtable=read(s,widget.value);
  const auto style_target=read(s,widget_vtable+0x4c);
  const auto style=call(s.style,widget,style_target);
  const auto style_vtable=read(s,style);
  const auto assign_target=read(s,style_vtable+4);
  call(s.assign,assign_target,style,scratch);
}
constexpr std::array<std::uint32_t,26> ids{
  0,0x0f0006cc,0x110006cd,0x100006ce,0x130006cf,0x110006d0,
  0x0e0006d1,0x180006d2,0x090006d3,0x080006d4,0x1a0006d5,
  0x180006d6,0x080006d7,0x080006d8,0x230006d9,0x250006da,
  0x240006db,0x1c0006dc,0x230006dd,0x1c0006de,0x1c0006df,
  0x180006e0,0x040006e1,0x16000873,0x19000874,0x11000975};
} //namespace

std::uint32_t vista_widget_locale_context(const VistaWidgetLocaleServices& s) {
  const auto interface=call(s.interface);
  if(!interface)return 0;
  const auto datum=read(s,interface+4);
  return datum==0xffffffffu?0u:call(s.globals,datum);
}
VistaWidgetIdentity vista_widget_locale_root(VistaWidgetIdentity widget,const VistaWidgetLocaleServices& s) {
  auto parent=read(s,widget.value+0x10);
  if(!parent)return {read(s,widget.value+4)==0?widget.value:0};
  for(;;) {
    const auto next=read(s,parent+0x10);
    if(!next)return {parent};
    parent=next;
  }
}
void vista_widget_screen_locale(VistaWidgetIdentity root,std::uint32_t id,
    std::uint32_t output,const VistaWidgetLocaleServices& s) {
  write(s,output,0,2);
  if(id==0xffffffffu)return;
  const auto screen=read(s,root.value+0x70);
  const auto datum=call(s.definition,screen);
  if(datum==0xffffffffu)return;
  const auto tag=call(s.screen_tag,datum);
  if(!tag)return;
  const auto list=read(s,tag+0x1c);
  call(s.localized,list,id,output);
}
void vista_widget_footer_locale_lookup(std::uint32_t frame,const VistaWidgetLocaleServices& s) {
  const auto context=vista_widget_locale_context(s);
  const auto index=read(s,frame+0xd8);
  for(std::uint32_t i=0;i<ids.size();++i) {
    write(s,frame+4+i*8,i);write(s,frame+8+i*8,ids[i]);
  }
  if(index>25||!context)return;
  const auto list=read(s,context+0x144);
  if(list==0xffffffffu)return;
  const auto output=read(s,frame+0xdc);
  const auto id=read(s,frame+8+index*8);
  call(s.localized,list,id,output);
}
void vista_widget_header_locale(VistaWidgetIdentity widget,std::uint32_t id,
    std::uint32_t scratch,const VistaWidgetLocaleServices& s) {
  if(id==0xffffffffu)return;
  const auto root=vista_widget_locale_root(widget,s);
  if(!root.value)return;
  write(s,scratch,0,2);
  vista_widget_screen_locale(root,id,scratch,s);
  assign(widget,scratch,s);
}
void vista_widget_footer_locale(VistaWidgetIdentity widget,std::uint32_t argument,
    std::uint32_t scratch,std::uint32_t frame,const VistaWidgetLocaleServices& s) {
  if(!vista_widget_locale_context(s))return;
  const auto index=read(s,argument);
  // Actual push order: output, then enum; caller body clears the first WORD.
  write(s,frame+0xdc,scratch);write(s,frame+0xd8,index);
  write(s,scratch,0,2);
  vista_widget_footer_locale_lookup(frame,s);
  assign(widget,scratch,s);
}
} //namespace halo2
