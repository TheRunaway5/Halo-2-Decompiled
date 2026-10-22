#include "halo2/vista_session_phase.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t at,unsigned n=4) {
  if(at>b.size()||n>b.size()-at)throw FormatError("Vista phase owner address outside source bank");
  std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[at+i]))<<(8*i);return v;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));
}
void admit(std::span<const std::byte> b) {
  if(b.size()<vista_session_source_size)throw FormatError("Vista phase requires complete session owner");
}
bool host(std::span<const std::byte> b) {
  const auto state=get(b,0x73a0);return state>=3&&state<=8&&get(b,0x71c0)==get(b,0x74);
}
} // namespace
void vista_session_phase_write(std::span<std::byte> source,std::uint32_t requested,const VistaSessionPhaseServices& services) {
  admit(source);auto b=source.first(vista_session_source_size);if(get(b,0x4c68)==requested)return;
  const auto count=std::bit_cast<std::int32_t>(get(b,0x84));
  if(count>0 && std::uint64_t(0x71c6)+std::uint64_t(count-1)*0x1c>=b.size())
    throw FormatError("Vista phase peer marks exceed supplied owner bank");
  if(!services.network_clock)throw FormatError("Missing Vista phase network clock owner");
  bool marked{};
  for(std::int32_t i=0;i<count;++i) {
    const auto at=0x71c6+std::size_t(i)*0x1c;
    if(get(b,at-1,1)){put(b,at,1,1);marked=true;}
  }
  put(b,0x796c,1,1);put(b,0x7970,get(b,0x4c68));
  const auto first=services.network_clock();
  put(b,0x4c68,requested);put(b,0x4c64,get(b,0x4c64)+1);put(b,0x7974,first);
  const auto second=services.network_clock();
  put(b,0x4c60,get(b,0x4c60)+1);put(b,0x4c6c,second);
  if(!marked&&get(b,0x796c,1))put(b,0x796c,0,1);
}
bool vista_session_phase_request(std::span<std::byte> source,std::uint32_t requested,const VistaSessionPhaseServices& services) {
  admit(source);auto b=source.first(vista_session_source_size);if(!host(b))return false;
  const auto state=get(b,0x73a0);
  if(state>=5&&state<=8){vista_session_phase_write(b,requested,services);return true;}
  const auto peer=get(b,0x64);const auto at=std::uint64_t(0x71c4)+std::uint64_t(peer)*0x1c;
  if(at+8>b.size())throw FormatError("Vista phase association exceeds supplied owner bank");
  if(get(b,std::size_t(at)+1,1)) {
    if(!services.send)throw FormatError("Missing Vista phase transport owner");
    VistaSessionPhaseMessage message{get(b,8),get(b,0x14),get(b,std::size_t(at)+4),{}};
    put(message.payload,0,get(b,0x20));put(message.payload,4,get(b,0x24));
    put(message.payload,8,1,1);put(message.payload,0xc,requested);services.send(message);
  }return true;
}
bool vista_session_pcr_host(std::span<const std::byte> b,std::uint8_t enabled) {
  if(!enabled)return false;
  admit(b);return host(b);
}
bool vista_session_pcr_continue(std::span<std::byte> b,std::uint32_t kind,const VistaSessionPhaseServices& s) {
  if(kind==0)return false;
  if(kind==1)return true;
  admit(b);if(!host(b))return false;return vista_session_phase_request(b,1,s);
}
} // namespace halo2
