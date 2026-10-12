#pragma once
#include "halo2/object_light_contacts.hpp"

namespace halo2 {
struct SourceObjectRenderAttributionOutputs {
    std::uint32_t render_model=0xffffffffu,collision=0xffffffffu;
    bool operator==(const SourceObjectRenderAttributionOutputs &) const=default;
};
struct SourceObjectRenderFlag31EventResult {
    std::uint32_t body_flags{};
    bool model_attribution_available{};
    SourceObjectRenderAttributionOutputs attribution;
};
// The bounded B7B40 construction event: BF9A0's AL predicate followed by the
// update of object+4 bit31. Other31 flag bits and unrelated construction state
// remain the caller's actual input. This is not a current-owner/lifetime builder.
// Use Cache::load_object_light_contact_definition for authored lookup. A missing
// optional is unresolved and throws; resolved model/render/collision NONE is a
// valid rejection. Original BF9A0 dereferences metadata pointers without null
// guards, so a missing non-NONE pointer cannot be translated to rejection.
// BF9A0 preserves caller outputs on NONE references; with all three references
// present it writes both datums even when the signed PRT/leaf counts reject.
// Only AL and these outputs are represented, not the unconsumed high EAX bits.
SourceObjectRenderFlag31EventResult source_object_render_flag31_construction_event(
    std::uint32_t current_body_flags,
    const std::optional<ObjectLightContactDefinition> &resolved_definition,
    SourceObjectRenderAttributionOutputs caller_outputs={});
} // namespace halo2
