#include "halo2/vista_widget_text_assignment.hpp"
#include "halo2/cache.hpp"
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... args) {
  if(!f)throw FormatError("Missing consumed Vista text assignment service");
  return f(std::forward<A>(args)...);
}
void require(std::span<std::byte> bank,VistaWidgetIdentity identity,std::size_t at,std::size_t n) {
  if(!identity||at>bank.size()||n>bank.size()-at||
      std::uint64_t(identity.value)+at+n>(std::uint64_t{1}<<32))
    throw FormatError("Invalid consumed Vista text assignment bank");
}
std::uint32_t font(VistaWidgetIdentity identity,const VistaWidgetTextAssignmentServices& s) {
  auto bank=call(s.bank,identity);require(bank,identity,4,4);std::uint32_t value{};
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bank[4+i])<<(i*8);
  return value;
}
void word(std::span<std::byte> bank,std::size_t at,std::uint16_t value) {
  bank[at]=std::byte(value);bank[at+1]=std::byte(value>>8);
}
std::uint32_t assign(VistaWidgetIdentity style,VistaUtf16View input,
    const VistaCloneStringServices& s,std::size_t capacity) {
  auto bank=call(s.bank,style);require(bank,style,0,0x44+capacity*2);
  vista_utf16_copy({style.value+0x44u,bank.subspan(0x44)},input,
      static_cast<std::uint32_t>(capacity),s.failure);
  const auto count=vista_utf16_length(call(s.text,style));
  const bool reset=(std::to_integer<unsigned>(bank[0x3d])&0x80)==0;
  word(bank,0x3e,static_cast<std::uint16_t>(count));
  if(reset)word(bank,0x3c,0);
  return count;
}
} //namespace
std::uint32_t vista_widget_text_assign(VistaWidgetIdentity widget,VistaUtf16View input,
    const VistaWidgetTextAssignmentServices& s) {
  const auto style=call(s.style,widget);return call(s.assign_string,style,input);
}
void vista_widget_text_prepare_assign(VistaWidgetIdentity widget,VistaUtf16View input,
    const VistaWidgetTextAssignmentServices& s) {
  const auto first=call(s.style,widget);const auto selected_font=font(first,s);
  if(call(s.prepare_text,input,selected_font)) {
    const auto current=call(s.style,widget);(void)call(s.assign_string,current,input);
  }
}
void vista_widget_text_font(VistaWidgetIdentity widget,std::uint32_t value,
    const VistaWidgetTextAssignmentServices& s) {
  const auto style=call(s.style,widget);auto bank=call(s.bank,style);require(bank,style,4,4);
  for(unsigned i=0;i<4;++i)bank[4+i]=std::byte(value>>(i*8));
}
std::uint32_t vista_widget_short_string_assign(VistaWidgetIdentity style,VistaUtf16View input,
    const VistaCloneStringServices& s) {return assign(style,input,s,0x20);}
std::uint32_t vista_widget_long_string_assign(VistaWidgetIdentity style,VistaUtf16View input,
    const VistaCloneStringServices& s) {return assign(style,input,s,0x200);}
VistaWidgetIdentity vista_widget_skin_style_identity(VistaWidgetIdentity widget) {
  return {widget.value+0x78u};
}
std::uint32_t vista_widget_skin_text_identity(VistaWidgetIdentity style) {return style.value+0x44u;}
} //namespace halo2
