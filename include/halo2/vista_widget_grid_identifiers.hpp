#pragma once
#include "halo2/vista_lobby_initializer.hpp"

namespace halo2 {
// Complete grid virtual+4 owner620441. It performs genuine611E23 identifier
// traversal first, including each child's own virtual+4, then clears this
// retained grid's visibility byte+6E. No constructor or clock is synthesized.
void vista_widget_grid_assign_identifiers(VistaWidgetIdentity,
    const VistaLobbyIdentifierServices&);
} //namespace halo2
