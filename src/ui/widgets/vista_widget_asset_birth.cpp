#include "halo2/vista_widget_asset_birth.hpp"
#include "halo2/vista_widget_constructor.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A>decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista asset birth service");
  return f(std::forward<A>(a)...);
}
std::span<std::byte> bank(VistaWidgetIdentity id,std::size_t extent,
    const VistaWidgetAssetBirthServices& services) {
  if(!id || std::uint64_t(id.value)+extent>0x100000000ull)
    throw FormatError("Invalid Vista asset widget source32 extent");
  auto bytes=call(services.bank,id);
  if(bytes.size()<extent)throw FormatError("Truncated Vista asset widget bank");
  return bytes;
}
std::uint32_t get(std::span<const std::byte> bytes,std::size_t at,unsigned n=4) {
  std::uint32_t value{};
  for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(bytes[at+i])<<(i*8);
  return value;
}
void put(std::span<std::byte> bytes,std::size_t at,std::uint32_t value,unsigned n=4) {
  for(unsigned i=0;i<n;++i)bytes[at+i]=std::byte(value>>(i*8));
}
std::uint32_t read(const VistaWidgetAssetBirthServices& services,std::uint32_t at,
    std::size_t size=4) {return call(services.read,at,size);}
bool zero(std::uint32_t bits,std::uint32_t mxcsr) {
  const auto absolute=bits&0x7fffffffu;
  return !absolute || ((mxcsr&0x40u) && absolute<0x00800000u);
}
} //namespace

VistaWidgetIdentity vista_widget_bitmap_construct(VistaWidgetIdentity id,
    std::uint32_t definition,const VistaWidgetAssetBirthServices& services) {
  auto bytes=bank(id,0x98,services);
  if((services.mxcsr&0x1f80u)!=0x1f80u)
    throw FormatError("Unmasked Vista bitmap constructor SSE exceptions are not admitted");
  vista_widget_construct(bytes,id,8,0,services.cached_ui_clock);
  put(bytes,0,0x7cfdd4);put(bytes,0x70,definition);
  const auto now=call(services.cached_ui_clock);
  auto current=get(bytes,0x70);
  put(bytes,0x74,now);put(bytes,0x78,0);put(bytes,0x8c,0);
  put(bytes,0x90,0,2);put(bytes,0x94,0);put(bytes,0x80,0);put(bytes,0x7c,0);
  const auto x=read(services,current+0x30);
  put(bytes,0x84,zero(x,services.mxcsr)?0x3f800000u:x);
  const auto y=read(services,current+0x34);
  put(bytes,0x88,zero(y,services.mxcsr)?0x3f800000u:y);
  put(bytes,0x68,read(services,current+4,2)-1u,2);
  call(services.bitmap_residency,read(services,current+0x1c));
  current=get(bytes,0x70);
  const auto datum=read(services,current+0x1c);
  std::uint32_t bitmap{},frame{};
  if(datum!=0xffffffffu) {
    const auto table=read(services,0x87cd50u);
    const auto offset=read(services,table+((datum&0xffffu)<<4)+8);
    bitmap=offset+read(services,0x87cd54u);
    if(bitmap) {
      const auto offset=read(services,bitmap+0x48);
      const auto images=offset==0xffffffffu?0u:read(services,0x882290u)+offset;
      const auto ordinal=std::bit_cast<std::int16_t>(std::uint16_t(get(bytes,0x90,2)));
      frame=images+std::uint32_t(std::int32_t(ordinal))*0x74u;
    }
  }
  std::array<std::uint16_t,4> bounds{};
  if(current && (read(services,current,1)&4u)) {
    const auto left=read(services,current+0xc,2);
    bounds[1]=bounds[3]=std::uint16_t(left);
    bounds[0]=std::uint16_t(read(services,current+0xe,2));
    bounds[2]=std::uint16_t(read(services,current+0x2a,2));
  } else if(frame) {
    const auto left=read(services,current+0xc,2);
    bounds[1]=std::uint16_t(left);
    const auto top=read(services,current+0xe,2);
    bounds[0]=std::uint16_t(top);
    bounds[3]=std::uint16_t(read(services,frame+4,2)+left);
    bounds[2]=std::uint16_t(top-read(services,frame+6,2));
  }
  for(unsigned i=0;i<4;++i)put(bytes,0x20+i*2,bounds[i],2);
  put(bytes,0x90,0,2);
  if(bitmap) {
    const auto requested=read(services,get(bytes,0x70)+0xa,2);
    if(std::bit_cast<std::int16_t>(std::uint16_t(requested))>=0 &&
        std::int32_t(requested)<std::bit_cast<std::int32_t>(read(services,bitmap+0x44)))
      put(bytes,0x90,requested,2);
  }
  put(bytes,0x6a,read(services,definition+0x20,2),2);
  return id;
}

VistaWidgetIdentity vista_widget_grid_construct(VistaWidgetIdentity id,
    std::uint32_t row,std::uint32_t definition,const VistaWidgetAssetBirthServices& services) {
  auto bytes=bank(id,0x78,services);
  vista_widget_construct(bytes,id,10,0,services.cached_ui_clock);
  put(bytes,0,0x7d3d3c);put(bytes,0x70,row);put(bytes,0,0x7d0614);
  put(bytes,0x74,definition);
  return id;
}
} //namespace halo2
