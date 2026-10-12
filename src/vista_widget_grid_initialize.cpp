#include "halo2/vista_widget_grid_initialize.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... args) {
  if(!f)throw FormatError("Missing consumed Vista grid initialization service");
  return f(std::forward<A>(args)...);
}
std::uint32_t load(std::span<const std::byte> bytes,std::size_t at,unsigned count=4) {
  std::uint32_t value{};
  for(unsigned i=0;i<count;++i)value|=std::to_integer<std::uint32_t>(bytes[at+i])<<(i*8);
  return value;
}
void store(std::span<std::byte> bytes,std::size_t at,std::uint32_t value,unsigned count=4) {
  for(unsigned i=0;i<count;++i)bytes[at+i]=std::byte(value>>(i*8));
}
std::uint32_t read(const VistaWidgetGridInitializeServices& s,std::uint32_t at,std::size_t count=4) {
  return call(s.read,at,count);
}
std::int32_t signed_byte(std::uint32_t value) {
  return std::bit_cast<std::int8_t>(std::uint8_t(value));
}
}
void vista_widget_grid_initialize(VistaWidgetIdentity id,std::span<const std::byte,4> position,
    const VistaWidgetGridInitializeServices& s) {
  if(!id || std::uint64_t(id.value)+0x78>0x100000000ull)
    throw FormatError("Invalid Vista grid source32 extent");
  auto bank=call(s.bank,id);
  if(bank.size()<0x78)throw FormatError("Truncated Vista grid source bank");
  auto definition=load(bank,0x74);
  const auto datum=read(s,definition+8);
  if(datum==0xffffffffu)return;
  const auto table=read(s,0x87cd50u);
  const auto offset=read(s,table+((datum&0xffffu)<<4)+8);
  const auto skin=offset+read(s,0x87cd54u);
  definition=load(bank,0x74);
  const auto rows=std::max(signed_byte(read(s,definition+0x12,1)),1);
  const auto columns=std::max(signed_byte(read(s,definition+0x13,1)),1);
  const auto order=signed_byte(read(s,definition+0x10,1));
  std::int32_t x=-1,y=-1;
  if(order==0) {
    const auto row=std::bit_cast<std::int32_t>(load(bank,0x70));
    x=row/columns;y=row%columns;
  }else if(order==1) {
    const auto row=std::bit_cast<std::int32_t>(load(bank,0x70));
    y=row/rows;x=row%rows;
  }
  // Literal clamp/compare sequence rejects out-of-range cells; it does not
  // clamp them into a valid skin row or coerce unsupported order values.
  if(std::clamp(y,0,columns-1)!=y || std::clamp(x,0,rows-1)!=x)return;
  const auto origin=read(s,definition+0xc);
  std::array<std::byte,4> point{};store(point,0,origin);
  auto vertical=std::uint16_t(read(s,definition+0x16,2)*std::uint32_t(y));
  vertical=std::uint16_t(vertical+load(position,0,2));
  store(point,0,std::uint16_t(load(point,0,2)+vertical),2);
  auto horizontal=std::uint16_t(read(s,definition+0x14,2)*std::uint32_t(x));
  horizontal=std::uint16_t(horizontal+load(position,2,2));
  store(point,2,std::uint16_t(load(point,2,2)+horizontal),2);
  call(s.skin_children,id,std::span<const std::byte,4>{point},skin);
  const auto bounds=call(s.child_bounds,id);
  //6116D2 copies the first DWORD and then the second into widget+20/+24.
  store(bank,0x20,load(bounds,0));store(bank,0x24,load(bounds,4));
}
} //namespace halo2
