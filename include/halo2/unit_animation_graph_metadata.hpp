#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace halo2 {
// Owned authored jmad payload. Original pointer words remain opaque addresses,
// resolved within this exact tag. This does not apply resource postprocessing,
// choose a channel, set residency bits or provide a runtime graph state.
class UnitAnimationGraphMetadata {
public:
  UnitAnimationGraphMetadata(std::uint32_t graph, std::uint32_t address,
                             std::span<const std::byte> payload)
      : graph_(graph), address_(address), bytes_(payload.begin(), payload.end()) {
    if (payload.size() < 0xc4)
      throw std::runtime_error("Source animation graph header is truncated");
  }
  std::uint32_t graph() const { return graph_; }
  std::uint32_t source_address() const { return address_; }
  std::size_t size() const { return bytes_.size(); }
  std::span<const std::byte> header() const {
    return std::span<const std::byte>(bytes_).first(0xc4);
  }
  std::span<const std::byte> bytes_at(std::uint32_t address,
                                    std::size_t extent) const {
    if (address < address_)
      throw std::runtime_error("Source animation pointer precedes owned graph");
    const auto offset = std::size_t(address - address_);
    if (offset > bytes_.size() || extent > bytes_.size() - offset)
      throw std::runtime_error("Source animation pointer escapes owned graph");
    return std::span<const std::byte>(bytes_).subspan(offset, extent);
  }
private:
  std::uint32_t graph_, address_;
  std::vector<std::byte> bytes_;
};
} // namespace halo2
