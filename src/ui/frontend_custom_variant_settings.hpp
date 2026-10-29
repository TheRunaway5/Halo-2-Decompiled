#pragma once
#include "halo2/frontend.hpp"
#include <optional>
#include <string>
#include <vector>

namespace halo2::detail {
// These are menu contexts, not imported tag datums or variant parameter IDs.
// Positive contexts also fit MenuEntry's signed value field.
inline constexpr unsigned frontend_custom_variant_category=0x7ffe0000u;
inline constexpr unsigned frontend_defusal_fuse_datum=0x7ffe0002u;
inline constexpr unsigned frontend_defusal_plant_datum=0x7ffe0003u;
inline constexpr unsigned frontend_defusal_disarm_datum=0x7ffe0004u;

bool frontend_custom_variant(const UiVariant &);
std::vector<unsigned> frontend_custom_variant_parameters(const UiVariant &);
// Synthetic metadata stays outside FrontendContent's authored parameter catalog.
std::optional<UiParameter> frontend_custom_variant_parameter(const UiVariant &,unsigned datum);
// Stock metadata is returned unchanged. Custom modes omit fixed or ineffective
// controls and retain only choices admitted by their actual rules adapter.
std::optional<UiParameter> frontend_custom_variant_parameter(const UiVariant &,const UiParameter &);
int frontend_variant_parameter_value(const UiVariant &,const UiParameter &);
std::string frontend_custom_variant_description(const UiVariant &);
} // namespace halo2::detail
