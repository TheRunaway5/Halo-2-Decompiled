#pragma once

#include "halo2/physics_contact_record.hpp"

namespace halo2 {

// 13A820..13A857: publish the new BSP with empty visibility masks before
// dispatching any new-BSP callbacks.
void native_campaign_bsp_reset_visibility_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});

// Row 60, 14E1D0 with no pending transition trigger: clear each occupied
// player's saved cluster, then run the original 154E70 player-status update.
// A pending trigger needs the earlier relocation owner, before any reset.
void native_campaign_bsp_reset_players_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});

}
