#include "halo2/vista_mainmenu_platform_gate.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> bank,std::size_t offset) {
  std::uint32_t value{};
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bank[offset+i])<<(i*8);
  return value;
}
void put(std::span<std::byte> bank,std::size_t offset,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)bank[offset+i]=std::byte(value>>(i*8));
}
}
bool vista_mainmenu_platform_gate_initialize(std::span<std::byte> bank,
    const VistaMainmenuPlatformGateServices& s) {
  if(bank.size()<12)throw std::invalid_argument("Vista platform gate requires its complete owner bank");
  if(!s.allocate_pbuffer || !s.set_pbuffer_byte || !s.initialize_licensing ||
      !s.create_event || !s.close_event || !s.open_license || !s.consume_right ||
      !s.query_license_status || !s.close_license || !s.read_status || !s.free_status)
    throw std::invalid_argument("Vista platform gate requires every real platform/licensing service");
  auto pointer=word(bank,0);
  s.allocate_pbuffer(2,pointer);
  put(bank,0,pointer);
  s.set_pbuffer_byte(word(bank,0),0,0);
  s.set_pbuffer_byte(word(bank,0),1,0);
  s.initialize_licensing();
  bank[4]=std::byte{1};
  const auto event=s.create_event();
  put(bank,8,event);
  if(bank[4]==std::byte{}) {
    if(event) {s.close_event(event);put(bank,8,0);}
    bank[4]=std::byte{};
  }
  bool licensed=false;
  const auto opened=s.open_license();
  auto result=opened.hresult;
  VistaLicenseStatusResult status{0,0,0};
  if(result>=0) {
    result=s.consume_right(opened.handle,vista_mainmenu_license_right);
    if(result>=0) {
      status=s.query_license_status(opened.handle);
      result=status.hresult;
    }
  }
  if(opened.handle)s.close_license(opened.handle);
  if(result>=0 && status.pointer) {
    for(std::uint32_t i=0;i<status.count;++i)if(s.read_status(status.pointer,i)==1) {
      licensed=true;
      break;
    }
    s.free_status(status.pointer);
  }
  if(!licensed)s.set_pbuffer_byte(word(bank,0),0,1);
  return licensed;
}
} // namespace halo2
