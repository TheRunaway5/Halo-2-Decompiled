#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace halo2 {
// Declared loaded-cache inputs derived from the original PC index/table bytes.
// Returning these values does not load a cache or publish original globals.
struct VistaWidgetCachePointers {
  std::uint32_t tag_table{},tag_base{},block_base{},scenario{},globals{};
  std::array<std::pair<std::uint32_t,std::uint32_t>,5> bindings() const;
  std::uint32_t value(std::uint32_t original_global_address) const;
};

// Immutable owned source bytes. The file's 2,048-byte header is retained
// separately; region is exactly [header DWORD+10, EOF], including index header,
// table, padding, every metadata/block/frame byte and trailing bytes.
// Source32 identities are supplied by the caller, never derived from a host
// pointer. Borrowed spans remain valid for this bank's lifetime.
class VistaWidgetCacheBank final {
public:
  explicit VistaWidgetCacheBank(std::span<const std::byte> pc_file);
  VistaWidgetCacheBank(const VistaWidgetCacheBank&)=default;
  VistaWidgetCacheBank(VistaWidgetCacheBank&&)=delete;
  VistaWidgetCacheBank& operator=(const VistaWidgetCacheBank&)=delete;
  VistaWidgetCacheBank& operator=(VistaWidgetCacheBank&&)=delete;

  std::span<const std::byte,2048> file_header() const { return header_; }
  std::span<const std::byte> region() const { return region_; }
  std::span<const std::byte> index_header() const;
  std::span<const std::byte> tag_table() const;
  std::span<const std::byte> directory_row(std::uint32_t ordinal) const;
  std::uint32_t source_file_size() const { return file_size_; }
  std::uint32_t source_index() const { return index_; }
  std::uint32_t tag_count() const { return count_; }
  std::uint32_t scenario_datum() const { return scenario_; }
  std::uint32_t globals_datum() const { return globals_; }

  // Whole mapped region must fit source32. Reads reject unmapped/wrapping
  // extents instead of producing zero bytes. Scalar width is exactly 1, 2 or 4.
  std::span<const std::byte> bytes(std::uint32_t source_base,
      std::uint32_t source_address,std::size_t size) const;
  std::uint32_t read(std::uint32_t source_base,std::uint32_t source_address,
      std::size_t bytes) const;
  std::uint32_t source_address(std::uint32_t source_base,
      std::uint32_t relative_offset,std::size_t size=0) const;

  // Original 639623/639640/63965D/63967A use only the datum's low WORD, without
  // matching its salt or inspecting its group. The physical table read is
  // bounded by the actual retained region; there is no invented salt policy.
  std::uint32_t tag_address(std::uint32_t source_base,std::uint32_t datum) const;
  std::span<const std::byte> tag_metadata(std::uint32_t source_base,
      std::uint32_t datum) const;
  // Original 40809B reads block+4 and treats only FFFFFFFF as NULL. Count is not
  // consulted. Offset zero resolves to the supplied cache base.
  std::uint32_t block_address(std::uint32_t source_base,
      std::uint32_t block_source_address) const;
  VistaWidgetCachePointers loaded_pointers(std::uint32_t source_base) const;

private:
  void require_base(std::uint32_t source_base) const;
  std::span<const std::byte> relative(std::uint64_t offset,std::size_t size) const;
  std::array<std::byte,2048> header_;
  std::vector<std::byte> region_;
  std::uint32_t file_size_{},index_{},table_{},count_{},scenario_{},globals_{};
};
} // namespace halo2
