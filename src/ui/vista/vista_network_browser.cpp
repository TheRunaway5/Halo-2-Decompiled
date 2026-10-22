#include "halo2/vista_network_browser.hpp"
#include "halo2/cache.hpp"
#include "halo2/vista_session_storage.hpp"
namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t at) {
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);return v;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));
}
}
bool vista_network_browser_create_local(const VistaBrowserCreateServices&s) {
  if(!s.disconnect || !s.clear_report_counts || !s.birth_local || !s.configure_selected)
    throw FormatError("Vista browser Create requires its actual disconnect, report and session owners");
  s.disconnect(true);s.clear_report_counts();
  const auto created=s.birth_local();
  if(created)s.configure_selected(2);
  return created;
}
bool vista_network_browser_preflight_local(std::span<std::byte> b,bool current,std::uint32_t value) {
  if(b.size()<vista_session_source_size)throw FormatError("Vista browser preflight requires complete source session storage");
  const auto state=get(b,0x73a0);
  if(!current || state<3 || state>8 || get(b,0x71c0)!=get(b,0x74) || get(b,0x4c68)!=1)return false;
  if(state<5)throw FormatError("Vista browser preflight requires unbound remote transport");
  put(b,0x4c60,get(b,0x4c60)+1);put(b,0x4c90,value);return true;
}
} //namespace halo2
