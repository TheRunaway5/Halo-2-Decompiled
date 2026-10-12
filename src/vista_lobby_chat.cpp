#include "halo2/vista_lobby_chat.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
bool vista_lobby_chat_available(std::span<const std::byte> ui,
    const VistaLobbyChatServices& services) {
  if(ui.size()<12)throw FormatError("Vista lobby chat requires the actual presentation context field");
  std::uint32_t context{};
  for(unsigned i=0;i<4;++i)context|=std::to_integer<std::uint32_t>(ui[8+i])<<(i*8);
  if(context!=2)return true;
  if(!services.check_privilege)
    throw FormatError("Vista lobby chat requires its consumed XUser privilege service");
  std::uint32_t allowed{};
  const auto result=services.check_privilege(0,252,allowed);
  return result==0 && allowed!=0;
}
} // namespace halo2
