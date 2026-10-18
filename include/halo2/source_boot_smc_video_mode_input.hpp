#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace halo2 {
class SourceRuntimeMemory;

// Explicit inputs to the observed kernel publication, never a selected mode
// or a device-success result. H2SMCR1 files contain the three input DWORDs;
// they contain no expected publication, EEPROM keys or device state.
class SourceBootSmcVideoModeInput final {
public:
  SourceBootSmcVideoModeInput(std::uint32_t prior, std::uint32_t status,
      std::uint32_t response, std::string owner);
  static SourceBootSmcVideoModeInput load(const std::filesystem::path &);
  std::array<std::uint32_t,3> inputs() const noexcept { return inputs_; }
  std::uint32_t published_value() const noexcept;
  const std::string &owner() const noexcept { return owner_; }
private:
  std::array<std::uint32_t,3> inputs_;
  std::string owner_;
};

// Authenticate the untouched TU1.5 import/getter and mount the computed word
// at its encoded native import address as immutable owned data. The original
// getter still executes. No SMBus transport or Xbox kernel pointer is claimed.
void source_mount_tu15_boot_smc_video_mode_input(SourceRuntimeMemory &,
    const SourceBootSmcVideoModeInput &);
}
