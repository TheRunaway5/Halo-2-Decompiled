#pragma once
#include <filesystem>
#include <optional>

namespace halo2 {
struct MacOSApplicationLayout {
    std::filesystem::path data, resources;
};
// No directory is created. Unbundled development executables retain the
// existing source-installation discovery path.
std::optional<MacOSApplicationLayout> macos_application_layout();
}
