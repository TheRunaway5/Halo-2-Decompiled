#pragma once
#include "halo2/tags/types.hpp"
#include <vector>

namespace halo2::detail {
// Imported native owners retain the original signed-low16 slot contract.
// Prefer the existing append order, then reuse only null lower slots. Existing
// live identities, including unloaded definitions, are never reassigned.
TagHandle insert_native_tag(std::vector<Tag>&, std::string group, std::string name);
}
