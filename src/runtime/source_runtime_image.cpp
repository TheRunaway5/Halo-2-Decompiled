#include "halo2/source_runtime_image.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <fstream>

namespace halo2 {
namespace {
std::uint32_t r(std::span<const std::byte> data, std::size_t p) {
  if (p > data.size() || data.size() - p < 4)
    throw FormatError("Truncated source runtime XBE");
  std::uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i)
    value |= std::to_integer<unsigned>(data[p + i]) << (8 * i);
  return value;
}
SourceRuntimeMemory load(const std::filesystem::path &path, SourcePhysicsContactImage image,
                         const SourceRuntimeKernelHardwareInfo *hardware,
                         const SourceRuntimeKernelTickCount *tick=nullptr,
                         const SourceRuntimeKernelBootSmcVideoMode *boot_mode=nullptr) {
  if (image != SourcePhysicsContactImage::MapPack11 && image != SourcePhysicsContactImage::TitleUpdate15)
    throw FormatError("Unverified source runtime image selection");
  if (hardware && (image != SourcePhysicsContactImage::TitleUpdate15 ||
      hardware->address < 0x80010000u || hardware->address > 0xfffffff8u || hardware->owner.empty()))
    throw FormatError("Kernel hardware data requires its actual TU1.5 resource address and owner");
  if(tick && (tick->address<0x60000000u || tick->address>0xfffffffcu || tick->owner.empty()))
    throw FormatError("Kernel tick-count data requires its actual native resource and owner");
  if(boot_mode && (image!=SourcePhysicsContactImage::TitleUpdate15 ||
      boot_mode->address<0x60000000u || boot_mode->address>0xfffffffcu || boot_mode->owner.empty()))
    throw FormatError("Boot SMC data requires its actual TU1.5 resource and owner");
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  if (!in)
    throw FormatError("Source runtime image is unavailable: " + path.string());
  const auto count = in.tellg();
  if (count < 0x168 || count > 16 * 1024 * 1024)
    throw FormatError("Invalid source runtime image extent");
  std::vector<std::byte> bytes(static_cast<std::size_t>(count));
  in.seekg(0);
  if (!in.read(reinterpret_cast<char *>(bytes.data()), bytes.size()))
    throw FormatError("Truncated source runtime image");
  // Same complete-image fingerprint used by OriginalData's supported-image
  // gate. Authenticate the exact bytes mounted here, avoiding a second file
  // read whose contents could differ from the selected image.
  std::uint64_t fingerprint = 14695981039346656037ull;
  for (auto b : bytes)
    fingerprint =
        (fingerprint ^ std::to_integer<unsigned>(b)) * 1099511628211ull;
  const bool later = image == SourcePhysicsContactImage::TitleUpdate15;
  if (fingerprint != (later ? 0x08aeec2701bca325ull : 0x598bf549dd8fb828ull))
    throw FormatError("Source runtime requires the authenticated selected image");
  const auto base = r(bytes, 0x104), headers = r(bytes, 0x108), sections = r(bytes, 0x11c),
             table = r(bytes, 0x120);
  if (r(bytes, 0) != 0x48454258u || table < base || sections > 128 ||
      headers < 0x168 || headers > bytes.size())
    throw FormatError("Invalid source runtime image section table");
  SourceRuntimeMemory memory;
  const std::string selected = later ? "TU1.5" : "TU1.1";
  memory.mount(base, std::span<const std::byte>(bytes).first(headers),
               SourceRuntimeMemory::Access::ReadOnly, "authenticated literal " + selected + " XBE header");
  bool bound = false,tick_bound=false,boot_mode_bound=false;
  for (unsigned i = 0; i < sections; ++i) {
    const auto p = std::uint64_t(table) - base + std::uint64_t(i) * 56;
    const auto flags = r(bytes, p), address = r(bytes, p + 4),
               extent = r(bytes, p + 8), raw = r(bytes, p + 12),
               length = r(bytes, p + 16);
    if (!extent || length > extent || raw > bytes.size() ||
        length > bytes.size() - raw)
      throw FormatError("Invalid source runtime image section");
    std::vector<std::byte> region(extent);
    std::copy_n(bytes.begin() + raw, length, region.begin());
    if (hardware && address <= 0x41b66cu && std::uint64_t(address)+length >= 0x41b670u) {
      const auto offset = 0x41b66cu-address;
      if (bound || r(region,offset) != 0x80000142u)
        throw FormatError("TU1.5 hardware import cell does not retain ordinal322");
      for (unsigned k=0;k<4;++k) region[offset+k]=std::byte(hardware->address>>(k*8));
      bound = true;
    }
    const auto tick_cell=later?0x41b774u:0x417cd4u;
    if(tick && address<=tick_cell && std::uint64_t(address)+length>=std::uint64_t(tick_cell)+4u) {
      const auto offset=tick_cell-address;
      if(tick_bound || r(region,offset)!=0x8000009cu)
        throw FormatError("Selected tick-count import cell does not retain ordinal156");
      for(unsigned k=0;k<4;++k)region[offset+k]=std::byte(tick->address>>(8*k));
      tick_bound=true;
    }
    constexpr auto boot_mode_cell=0x41b784u;
    if(boot_mode && address<=boot_mode_cell &&
        std::uint64_t(address)+length>=std::uint64_t(boot_mode_cell)+4u) {
      const auto offset=boot_mode_cell-address;
      if(boot_mode_bound || r(region,offset)!=0x80000164u)
        throw FormatError("TU1.5 boot SMC import cell does not retain ordinal356");
      for(unsigned k=0;k<4;++k)region[offset+k]=std::byte(boot_mode->address>>(8*k));
      boot_mode_bound=true;
    }
    memory.mount(address, region,
                 (flags & 1u) ? SourceRuntimeMemory::Access::ReadWrite
                              : SourceRuntimeMemory::Access::ReadOnly,
                 "selected " + selected + " section " + std::to_string(i));
  }
  if (hardware) {
    if (!bound) throw FormatError("TU1.5 hardware data import cell is unavailable");
    memory.mount(hardware->address,hardware->bytes,SourceRuntimeMemory::Access::ReadOnly,
                 hardware->owner);
  }
  if(tick) {
    if(!tick_bound)throw FormatError("Selected tick-count import cell is unavailable");
    memory.mount(tick->address,tick->bytes,SourceRuntimeMemory::Access::ReadWrite,tick->owner);
  }
  if(boot_mode) {
    if(!boot_mode_bound)throw FormatError("TU1.5 boot SMC import cell is unavailable");
    memory.mount(boot_mode->address,boot_mode->bytes,SourceRuntimeMemory::Access::ReadOnly,boot_mode->owner);
  }
  return memory;
}
} // namespace
SourceRuntimeMemory load_source_runtime_image(const std::filesystem::path &path,
                                             SourcePhysicsContactImage image) {
  return load(path,image,nullptr);
}
SourceRuntimeMemory load_source_runtime_image(const std::filesystem::path &path,
    SourcePhysicsContactImage image,const SourceRuntimeKernelHardwareInfo &hardware) {
  return load(path,image,&hardware);
}
SourceRuntimeMemory load_source_runtime_image(const std::filesystem::path &path,
    SourcePhysicsContactImage image,const SourceRuntimeKernelTickCount &tick) {
  return load(path,image,nullptr,&tick);
}
SourceRuntimeMemory load_source_runtime_image_1_1(const std::filesystem::path &path) {
  return load_source_runtime_image(path, SourcePhysicsContactImage::MapPack11);
}
SourceRuntimeMemory load_source_runtime_image(const std::filesystem::path &path,
    SourcePhysicsContactImage image,const SourceRuntimeKernelBootSmcVideoMode &boot_mode) {
  return load(path,image,nullptr,nullptr,&boot_mode);
}
} // namespace halo2
