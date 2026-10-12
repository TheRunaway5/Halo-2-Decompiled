#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace halo2 {
// Literal owned resident tag payload. Source addresses and pointer words are
// retained without host fixups, resource activation or runtime defaults.
class SourceTagMetadata {
public:
  SourceTagMetadata(std::uint32_t datum, std::string group,
                    std::uint32_t address, std::span<const std::byte> payload)
      : datum_(datum), address_(address), group_(std::move(group)),
        bytes_(payload.begin(), payload.end()) {
    if (!address || group_.size() != 4 || payload.empty() ||
        std::uint64_t(address) + payload.size() > (std::uint64_t{1} << 32))
      throw std::invalid_argument("Invalid resident source tag payload");
  }
  std::uint32_t datum() const { return datum_; }
  const std::string &group() const { return group_; }
  std::uint32_t source_address() const { return address_; }
  std::size_t size() const { return bytes_.size(); }
  std::span<const std::byte> bytes_at(std::uint32_t address,
                                    std::size_t extent) const {
    if (address < address_)
      throw std::out_of_range("Source tag pointer precedes owned payload");
    const auto offset = std::uint64_t(address) - address_;
    if (offset > bytes_.size() || extent > bytes_.size() - offset)
      throw std::out_of_range("Source tag pointer escapes owned payload");
    return std::span<const std::byte>(bytes_).subspan(std::size_t(offset), extent);
  }
private:
  std::uint32_t datum_, address_;
  std::string group_;
  std::vector<std::byte> bytes_;
};
} // namespace halo2
