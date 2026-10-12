#pragma once
#include "halo2/source_bsp_admission.hpp"
#include <optional>

namespace halo2 {
// Literal165A20 fields. Optional absence means unresolved input; NONE is a
// resolved source datum. The definition/model lookups are current-owner
// lookups, without parent traversal or inferred material/shadow categories.
struct SourceShadowSecondaryOwner {
    std::uint32_t identity;
    std::optional<std::uint32_t> definition; // object+0
    std::optional<std::uint32_t> flags; // object+4
    std::optional<std::uint8_t> type; // byte object+AA, shift masked to31
};
struct SourceShadowSecondaryDefinition {
    std::uint32_t identity;
    std::optional<std::uint32_t> model; // definition+38, hlmt datum
};
struct SourceShadowSecondaryModel {
    std::uint32_t identity;
    std::optional<std::uint32_t> render_model; // hlmt+4, mode datum
};
// Header+4 is a signed short; rows beyond a positive count remain untouched.
// Storage remains resident when the root modelNONE branch clears count alone.
struct SourceShadowSecondaryQueue {
    std::int16_t count;
    std::span<SourceBspAdmissionRecord> storage;
};
struct SourceShadowSecondaryRows {
    SourceShadowSecondaryQueue clusters,objects,instances; // context+C/+14/+18
};
struct SourceShadowSecondaryClassification {
    std::uint32_t root; // actual165A20 argument1
    std::uint8_t root_receives; // argument2; every nonzero byte preserves1000
    std::uint32_t receiver_type_mask; // argument3, x86 byte shift masks to31
};
// Complete original165A20. Mutates only source counts/flags; ids, other stream,
// associations and inactive storage retain their exact input words. The fixed
// shadow caller supplies root,0,0. No queues/admission/default owner fields are
// created by this helper. Source-required absent lookup fields reject.
void source_shadow_secondary_classify(SourceShadowSecondaryRows&,
    std::span<const SourceShadowSecondaryOwner>,
    std::span<const SourceShadowSecondaryDefinition>,
    std::span<const SourceShadowSecondaryModel>,
    const SourceShadowSecondaryClassification&);
}
