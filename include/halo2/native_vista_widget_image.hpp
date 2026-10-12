#pragma once
#include "halo2/native_vista_widget_memory.hpp"
#include <array>
#include <filesystem>
#include <string_view>

namespace halo2 {
struct VistaWidgetPeSection {
  std::array<std::byte, 8> name;
  std::uint32_t virtual_size, rva, raw_size, raw_offset, characteristics;
};
struct VistaWidgetPeLayout {
  std::uint32_t image_base, image_size, header_size;
  std::uint32_t section_alignment, file_alignment;
  std::vector<VistaWidgetPeSection> sections;
};
// Bounded I386 PE32 format inspection only: these functions do not authenticate
// an executable or construct a selected Vista owner. A requested range must fit
// one header or section bank. File raw bytes are retained, including raw padding;
// only a section's declared VirtualSize beyond its raw bytes is zero-filled.
// Image alignment gaps, overlays and unmapped bytes are never made accessible.
VistaWidgetPeLayout inspect_vista_widget_pe(std::span<const std::byte> file);
std::vector<std::byte> vista_widget_pe_initial_range(
    std::span<const std::byte> file, std::uint32_t original_va, std::size_t bytes);

// Immutable original data, authenticated from the exact retained file bytes.
// Original VAs remain data identities, never callable native pointers. Importing
// operands/vtables/seed bytes does not execute relocations, imports, startup,
// constructors, allocation, font/cache readiness or global publication.
class NativeVistaWidgetImage final {
public:
  static constexpr std::size_t selected_file_size = 14697848;
  static constexpr std::string_view selected_sha256 =
      "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109";
  enum class MountPolicy { SectionDeclared, ReadOnly };
  explicit NativeVistaWidgetImage(const std::filesystem::path&);
  explicit NativeVistaWidgetImage(std::span<const std::byte>);
  NativeVistaWidgetImage(const NativeVistaWidgetImage&) = delete;
  NativeVistaWidgetImage& operator=(const NativeVistaWidgetImage&) = delete;
  NativeVistaWidgetImage(NativeVistaWidgetImage&&) = delete;
  NativeVistaWidgetImage& operator=(NativeVistaWidgetImage&&) = delete;

  const VistaWidgetPeLayout& layout() const noexcept { return layout_; }
  std::span<const std::byte> file_bytes() const noexcept { return file_; }
  std::span<const std::byte> headers() const noexcept;
  std::span<const std::byte> section_raw(std::size_t index) const;
  // Views are immutable and stable for this owner's lifetime.
  std::span<const std::byte> bytes(std::uint32_t original_va, std::size_t) const;
  // Copy only the caller-selected range into the actual registry. Headers use
  // a native readonly input policy. Section permissions preserve PE READ/WRITE
  // declarations or narrow to readonly; they do not emulate Windows pages.
  // Registry overlap rejection preserves actual manager/cache authority.
  void mount(NativeVistaWidgetMemory&, std::uint32_t original_va, std::size_t,
      MountPolicy, std::string owner) const;
private:
  struct RetainedFile {};
  NativeVistaWidgetImage(std::vector<std::byte>, RetainedFile);
  std::vector<std::byte> file_;
  VistaWidgetPeLayout layout_;
  std::vector<std::vector<std::byte>> sections_;
};
} // namespace halo2
