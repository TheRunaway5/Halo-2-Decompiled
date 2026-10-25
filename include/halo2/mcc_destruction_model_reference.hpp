#pragma once
#include "halo2/mcc_gameplay_reference.hpp"

namespace halo2 {
// Restores the original null effect in the exact reviewed Flood-clump model
// response. Stable checked metadata and full local donor identity are required.
// Unknown authored tuples stay unchanged. No effect or damage is executed.
std::optional<MccGameplayReferenceReceipt> convert_mcc_destruction_model_references(
    TagGroup,std::string_view,std::uint32_t,const MccGameplayReferenceAccess&);
}
