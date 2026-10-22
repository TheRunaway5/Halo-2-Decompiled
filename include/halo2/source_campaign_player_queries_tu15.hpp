#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/source_script.hpp"

namespace halo2 {
// TU1.5 2B3790 reads the actual game options. The cooperative byte is
// meaningful only for campaign type1; this query does not create a session.
bool source_campaign_is_cooperative_tu15(const SourcePhysicsContactMemory &);
// TU1.5 2A9190: allocate a fresh HS list, visit occupied player slots in
// ascending order and prepend every non-NONE current unit. Source list
// allocation/append remain explicit owners; players/objects are never copied.
std::uint32_t source_campaign_script_players_tu15(
    const SourcePhysicsContactMemory &,const SourceHsObjectListServices &);
}
