#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// Required actor/order-reset owner26C600: keep the real actor<->script-key
// association and its original retained hash-table/free-list ownership.
// Hash/equality are the closed native algorithms installed by original1D0230;
// executable selectors are checked as metadata and are never executed.
// This is a dependency of ai_set_orders, not a complete orders implementation.
void native_campaign_ai_order_script_assignment_tu15(
    SourcePhysicsContactMemory &,std::uint32_t actor,std::uint32_t script_key);
}
