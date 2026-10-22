#include "halo2/native_vista_widget_image.hpp"
#include <openssl/evp.h>
#include <algorithm>
#include <fstream>
#include <limits>

namespace halo2 {
namespace {
constexpr std::uint64_t source_limit = std::uint64_t{1} << 32;
constexpr std::uint32_t pe_read = 0x40000000u, pe_write = 0x80000000u;
void require(bool condition, const char* message) {
  if (!condition) throw FormatError(message);
}
bool power_of_two(std::uint32_t x) { return x && !(x & (x - 1)); }
std::uint32_t value(std::span<const std::byte> file, std::size_t at,
                    std::size_t size) {
  require(at <= file.size() && size <= file.size() - at,
          "Truncated Vista PE field");
  std::uint32_t out{};
  for (std::size_t i = 0; i < size; ++i)
    out |= std::uint32_t(std::to_integer<unsigned>(file[at + i])) << (i * 8);
  return out;
}
std::uint64_t mapped_size(const VistaWidgetPeSection& s) {
  return std::max(s.virtual_size, s.raw_size);
}
bool intersects(std::uint64_t a, std::uint64_t n,
                std::uint64_t b, std::uint64_t m) {
  return n && m && a < b + m && b < a + n;
}
struct Range { bool header; std::size_t index, offset; };
Range locate(const VistaWidgetPeLayout& layout, std::uint32_t at, std::size_t n) {
  require(n && n <= source_limit - at, "Empty or wrapped Vista PE range");
  require(at >= layout.image_base, "Vista PE range precedes image");
  const auto rva = std::uint64_t(at) - layout.image_base;
  if (rva < layout.header_size && n <= layout.header_size - rva)
    return {true, 0, std::size_t(rva)};
  for (std::size_t i = 0; i < layout.sections.size(); ++i) {
    const auto& s = layout.sections[i];
    if (rva >= s.rva && rva - s.rva < mapped_size(s) &&
        n <= mapped_size(s) - (rva - s.rva))
      return {false, i, std::size_t(rva - s.rva)};
  }
  throw FormatError("Vista PE range crosses a bank or reaches unmapped bytes");
}
std::vector<std::byte> copy_selected(std::span<const std::byte> file) {
  require(file.size() == NativeVistaWidgetImage::selected_file_size,
          "Unselected Vista executable size");
  return {file.begin(), file.end()};
}
std::vector<std::byte> read_selected(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  require(bool(input), "Cannot open selected Vista executable");
  const auto end = input.tellg();
  require(end >= 0 && std::uint64_t(end) == NativeVistaWidgetImage::selected_file_size,
          "Unselected Vista executable size");
  std::vector<std::byte> file(NativeVistaWidgetImage::selected_file_size);
  input.seekg(0);
  require(bool(input.read(reinterpret_cast<char*>(file.data()),
                          std::streamsize(file.size()))),
          "Short selected Vista executable read");
  return file;
}
void authenticate(std::span<const std::byte> file) {
  require(file.size() == NativeVistaWidgetImage::selected_file_size,
          "Unselected Vista executable size");
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  unsigned int size{};
  require(EVP_Digest(file.data(), file.size(), digest.data(), &size,
                     EVP_sha256(), nullptr) == 1 && size == 32,
          "Vista executable SHA-256 failed");
  constexpr char digits[] = "0123456789abcdef";
  std::array<char, 64> hex{};
  for (std::size_t i = 0; i < 32; ++i) {
    hex[i * 2] = digits[digest[i] >> 4];
    hex[i * 2 + 1] = digits[digest[i] & 15];
  }
  require(std::string_view(hex.data(), hex.size()) ==
              NativeVistaWidgetImage::selected_sha256,
          "Unselected Vista executable SHA-256");
}
} // namespace

VistaWidgetPeLayout inspect_vista_widget_pe(std::span<const std::byte> file) {
  require(file.size() >= 64 && value(file, 0, 2) == 0x5a4d,
          "Invalid Vista PE DOS header");
  const auto pe = std::uint64_t(value(file, 0x3c, 4));
  require(pe >= 64 && pe <= file.size() && file.size() - pe >= 24 &&
              value(file, std::size_t(pe), 4) == 0x4550,
          "Invalid Vista PE signature");
  require(value(file, std::size_t(pe + 4), 2) == 0x14c,
          "Vista data owner requires I386 PE");
  const auto count = value(file, std::size_t(pe + 6), 2);
  const auto optional_size = value(file, std::size_t(pe + 20), 2);
  const auto optional = pe + 24, table = optional + optional_size;
  require(count && count <= 96 && optional_size >= 96 &&
              optional <= file.size() && optional_size <= file.size() - optional &&
              value(file, std::size_t(optional), 2) == 0x10b,
          "Invalid Vista PE32 optional header");
  require(value(file, std::size_t(optional + 92), 4) <= (optional_size - 96) / 8,
          "Vista PE directory table exceeds optional header");
  VistaWidgetPeLayout out{value(file, std::size_t(optional + 28), 4),
      value(file, std::size_t(optional + 56), 4),
      value(file, std::size_t(optional + 60), 4),
      value(file, std::size_t(optional + 32), 4),
      value(file, std::size_t(optional + 36), 4), {}};
  require(out.image_base && out.image_size && out.header_size &&
              out.image_size <= source_limit - out.image_base &&
              out.header_size <= out.image_size && out.header_size <= file.size(),
          "Invalid Vista PE image/header extent");
  require(power_of_two(out.section_alignment) && power_of_two(out.file_alignment) &&
              out.section_alignment >= out.file_alignment &&
              (out.section_alignment < 4096
                  ? out.section_alignment == out.file_alignment
                  : out.file_alignment >= 512 && out.file_alignment <= 65536) &&
              out.header_size % out.file_alignment == 0 &&
              out.image_size % out.section_alignment == 0,
          "Unsupported Vista PE alignment");
  require(table <= out.header_size && count * 40 <= out.header_size - table,
          "Vista PE section table exceeds headers");
  out.sections.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto at = std::size_t(table + i * 40);
    VistaWidgetPeSection s{};
    std::copy_n(file.begin() + std::ptrdiff_t(at), 8, s.name.begin());
    s.virtual_size = value(file, at + 8, 4); s.rva = value(file, at + 12, 4);
    s.raw_size = value(file, at + 16, 4); s.raw_offset = value(file, at + 20, 4);
    s.characteristics = value(file, at + 36, 4);
    require(mapped_size(s) && s.rva >= out.header_size &&
                s.rva % out.section_alignment == 0 && s.rva <= out.image_size &&
                mapped_size(s) <= std::uint64_t(out.image_size) - s.rva,
            "Invalid Vista PE virtual section extent");
    if (s.raw_size)
      require(s.raw_size % out.file_alignment == 0 &&
                  s.raw_offset % out.file_alignment == 0 &&
                  s.raw_offset >= out.header_size && s.raw_offset <= file.size() &&
                  s.raw_size <= file.size() - s.raw_offset,
              "Invalid Vista PE raw section extent");
    for (const auto& prior : out.sections)
      require(!intersects(s.rva, mapped_size(s), prior.rva, mapped_size(prior)) &&
                  !intersects(s.raw_offset, s.raw_size, prior.raw_offset, prior.raw_size),
              "Overlapping Vista PE section banks");
    out.sections.push_back(s);
  }
  return out;
}

std::vector<std::byte> vista_widget_pe_initial_range(
    std::span<const std::byte> file, std::uint32_t at, std::size_t n) {
  const auto layout = inspect_vista_widget_pe(file);
  const auto range = locate(layout, at, n);
  std::vector<std::byte> out(n);
  if (range.header) std::copy_n(file.begin() + std::ptrdiff_t(range.offset), n, out.begin());
  else {
    const auto& s = layout.sections[range.index];
    if (range.offset < s.raw_size)
      std::copy_n(file.begin() + std::ptrdiff_t(s.raw_offset + range.offset),
          std::min(n, std::size_t(s.raw_size) - range.offset), out.begin());
  }
  return out;
}

NativeVistaWidgetImage::NativeVistaWidgetImage(const std::filesystem::path& path)
    : NativeVistaWidgetImage(read_selected(path), RetainedFile{}) {}
NativeVistaWidgetImage::NativeVistaWidgetImage(std::span<const std::byte> file)
    : NativeVistaWidgetImage(copy_selected(file), RetainedFile{}) {}
NativeVistaWidgetImage::NativeVistaWidgetImage(std::vector<std::byte> file, RetainedFile)
    : file_(std::move(file)) {
  authenticate(file_);
  layout_ = inspect_vista_widget_pe(file_);
  sections_.reserve(layout_.sections.size());
  for (const auto& s : layout_.sections) {
    auto& bank = sections_.emplace_back(std::size_t(mapped_size(s)));
    std::copy_n(file_.begin() + s.raw_offset, s.raw_size, bank.begin());
  }
}
std::span<const std::byte> NativeVistaWidgetImage::headers() const noexcept {
  return std::span(file_).first(layout_.header_size);
}
std::span<const std::byte> NativeVistaWidgetImage::section_raw(std::size_t i) const {
  require(i < layout_.sections.size(), "Vista PE section index is absent");
  const auto& s = layout_.sections[i];
  return std::span(file_).subspan(s.raw_offset, s.raw_size);
}
std::span<const std::byte> NativeVistaWidgetImage::bytes(std::uint32_t at, std::size_t n) const {
  const auto range = locate(layout_, at, n);
  if (range.header) return headers().subspan(range.offset, n);
  return std::span(sections_[range.index]).subspan(range.offset, n);
}
void NativeVistaWidgetImage::mount(NativeVistaWidgetMemory& memory,
    std::uint32_t at, std::size_t n, MountPolicy policy, std::string owner) const {
  require(policy == MountPolicy::SectionDeclared || policy == MountPolicy::ReadOnly,
          "Invalid Vista PE mount policy");
  const auto range = locate(layout_, at, n);
  auto access = NativeVistaWidgetMemory::Access::ReadOnly;
  if (!range.header) {
    const auto flags = layout_.sections[range.index].characteristics;
    require(flags & pe_read, "Vista PE section does not declare readable data");
    if (policy == MountPolicy::SectionDeclared && (flags & pe_write))
      access = NativeVistaWidgetMemory::Access::ReadWrite;
  }
  memory.mount(at, bytes(at, n), access, std::move(owner));
}
} // namespace halo2
