#include "halo2/vista_widget_text_clone.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <set>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista clone text service");
  return f(std::forward<A>(a)...);
}
std::uint32_t read(std::span<const std::byte> b,std::size_t at,unsigned n=4) {
  if(at>b.size()||n>b.size()-at)throw FormatError("Unowned Vista clone text field");
  std::uint32_t value{};for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);
  return value;
}
void write(std::span<std::byte> b,std::size_t at,std::uint32_t value,unsigned n=4) {
  if(at>b.size()||n>b.size()-at)throw FormatError("Unowned Vista clone text field");
  for(unsigned i=0;i<n;++i)b[at+i]=std::byte(std::uint8_t(value>>(8*i)));
}
std::span<std::byte> bank(const VistaWidgetTextCloneServices& s,VistaWidgetIdentity id) {
  if(!id)throw FormatError("Missing retained Vista clone text identity");
  return call(s.bank,id);
}
std::uint32_t signed_word(std::uint32_t value) {
  return std::bit_cast<std::uint32_t>(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
}
VistaWidgetIdentity root(VistaWidgetIdentity widget,const VistaWidgetTextCloneServices& s) {
  auto current=VistaWidgetIdentity{read(bank(s,widget),0x10)};std::set<std::uint32_t> visited;
  while(current) {
    if(!visited.insert(current.value).second)throw FormatError("Cyclic Vista clone text root");
    const auto parent=read(bank(s,current),0x10);if(!parent)return current;current={parent};
  }
  return read(bank(s,widget),4)==0?widget:VistaWidgetIdentity{};
}
void resolve(VistaWidgetIdentity owner,std::uint32_t string_id,std::span<std::byte,0x400> output,
    const VistaWidgetTextCloneServices& s) {
  write(output,0,0,2);if(string_id==0xffffffffu)return;
  const auto definition=call(s.definition,read(bank(s,owner),0x70));if(definition==0xffffffffu)return;
  const auto tag=call(s.tag,definition);if(!tag)return;
  call(s.localized,read(*tag,0x1c),string_id,output);
}
}
void vista_widget_text_descriptor(VistaWidgetIdentity widget,std::uint32_t identifier,
    std::span<const std::byte,0x2c> descriptor,std::span<std::byte,0x400> scratch,
    const VistaWidgetTextCloneServices& s) {
  if(s.consuming)s.consuming(widget,identifier,descriptor);
  const auto owner=root(widget,s);auto destination=bank(s,widget);
  write(destination,0xa,identifier,2);write(destination,0x70,signed_word(read(descriptor,6,2)));
  const auto flags=read(descriptor,0);write(scratch,0,0,2);
  const auto alignment=(flags&1)?0u:((flags&2)?1u:2u);
  const auto style_flags=(flags&4)?1u:0u;
  write(destination,0x68,std::uint16_t(read(descriptor,4,2)-1),2);
  write(destination,0x6a,read(descriptor,0x28,2),2);
  const auto font=(read(descriptor,0,1)&8)?1u:signed_word(read(descriptor,0xa,2));
  const std::array<std::uint32_t,3> color{read(descriptor,0x10),read(descriptor,0x14),read(descriptor,0x18)};
  resolve(owner,read(descriptor,0x24),scratch,s);
  write(destination,0x20,read(descriptor,0x1c));write(destination,0x24,read(descriptor,0x20));
  const auto style=call(s.style,widget);call(s.assign_string,style,std::span<const std::byte,0x400>{scratch});
  auto target=bank(s,style);write(target,4,font);write(target,8,color[0]);write(target,0xc,color[1]);write(target,0x10,color[2]);
  write(target,0x14,style_flags,2);write(target,0x18,0xffffffffu);write(target,0x1c,alignment);write(target,0x24,0xffffffffu);
  if(read(descriptor,0,1)&8) {
    const auto context=call(s.context);
    const auto first=call(s.style,widget);const auto bits=read(bank(s,first),0x14,2)|4u;
    const auto second=call(s.style,widget);write(bank(s,second),0x14,bits,2);
    if(context) {
      const auto scale=read(*context,0x4c);const auto third=call(s.style,widget);write(bank(s,third),0x20,scale);
    }
    const auto fourth=call(s.style,widget);write(bank(s,fourth),0x3c,0,2);
  }
  const auto final=call(s.style,widget);const auto text=call(s.text,final);(void)call(s.prepare_text,text,font);
}
void vista_widget_text_clone_initialize(VistaWidgetIdentity widget,std::uint32_t flags,
    std::uint32_t animation,std::span<const std::byte,16> vector,std::uint32_t font,
    std::span<const std::byte,8> bounds,std::span<std::byte,0x2c> descriptor,
    std::span<std::byte,0x400> text,const VistaWidgetTextCloneServices& s) {
  write(descriptor,0,signed_word(flags));write(descriptor,4,animation,2);
  write(descriptor,0xc,read(vector,0));write(descriptor,0x10,read(vector,4));
  const auto third=read(vector,8);write(descriptor,0x18,read(vector,0xc));write(descriptor,0xa,font,2);
  const auto first_bound=read(bounds,0),second_bound=read(bounds,4);write(descriptor,0x1c,first_bound);
  write(descriptor,6,0,2);write(descriptor,0x14,third);write(descriptor,0x20,second_bound);
  write(descriptor,0x24,0xffffffffu);write(descriptor,0x28,0x2775,2);
  vista_widget_text_descriptor(widget,0xffffffffu,std::span<const std::byte,0x2c>{descriptor},text,s);
}
void vista_widget_text_clone_flag(VistaWidgetIdentity widget,const VistaWidgetTextCloneServices& s) {
  write(bank(s,widget),0x74,1,1);
}
} //namespace halo2
