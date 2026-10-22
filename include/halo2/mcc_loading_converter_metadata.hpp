#pragma once

#include <nlohmann/json_fwd.hpp>

namespace halo2 {
// Receipt provenance is portable; actual native dependency admission belongs
// to the setup runtime before conversion and publication.
void validate_mcc_loading_converter_metadata(const nlohmann::json& audio);
} // namespace halo2
