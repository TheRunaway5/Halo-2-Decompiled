#include "halo2/vista_widget_grid_identifiers.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
void vista_widget_grid_assign_identifiers(VistaWidgetIdentity grid,
    const VistaLobbyIdentifierServices& services) {
  vista_lobby_assign_widget_identifiers(grid,services);
  //620449 writes after all recursive child virtual+4 calls have returned.
  if(!services.bank)throw FormatError("Missing consumed Vista grid bank owner");
  auto bank=services.bank(grid);
  if(bank.size()<=0x6e)throw FormatError("Truncated consumed Vista grid visibility field");
  bank[0x6e]=std::byte{};
}
} //namespace halo2
