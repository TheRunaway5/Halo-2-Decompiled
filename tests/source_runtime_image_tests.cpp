#include "halo2/source_runtime_image.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
std::uint32_t r(std::span<const std::byte> b, std::size_t p) {
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v |= std::to_integer<unsigned>(b[p + i]) << (8 * i);
  return v;
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2 || argc == 3, "Supply selected original XBE and optional1.5 selection");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    if (argc == 3) check(std::string(argv[2]) == "1.5", "Unverified image selection");
    const bool later = argc == 3;
    auto memory = halo2::load_source_runtime_image(argv[1], later ?
        halo2::SourcePhysicsContactImage::TitleUpdate15 : halo2::SourcePhysicsContactImage::MapPack11);
    std::ifstream in(argv[1], std::ios::binary | std::ios::ate);
    std::vector<std::byte> bytes(static_cast<std::size_t>(in.tellg()));
    in.seekg(0);
    check(bool(in.read(reinterpret_cast<char *>(bytes.data()), bytes.size())),
          "Read original source bytes");
    const auto table = r(bytes, 0x120) - r(bytes, 0x104),
               count = r(bytes, 0x11c);
    const auto header = memory.view(r(bytes, 0x104), r(bytes, 0x108));
    check(std::equal(header.begin(), header.end(), bytes.begin()), "Literal original header bytes");
    bool header_writable = true;
    try { memory.mutable_view(r(bytes, 0x104), r(bytes, 0x108)); }
    catch (const std::runtime_error &) { header_writable = false; }
    check(!header_writable, "Native literal-header read-only input policy");
    std::size_t total = 0;
    for (unsigned i = 0; i < count; ++i) {
      const auto p = table + i * 56, flags = r(bytes, p), va = r(bytes, p + 4),
                 extent = r(bytes, p + 8), raw = r(bytes, p + 12),
                 length = r(bytes, p + 16);
      const auto mounted = memory.view(va, extent);
      check(std::equal(mounted.begin(), mounted.begin() + length,
                       bytes.begin() + raw),
            "Complete original raw section bytes");
      for (auto b : mounted.subspan(length))
        check(b == std::byte{}, "Loader-zeroed virtual tail");
      bool writable = true;
      try {
        memory.mutable_view(va, extent);
      } catch (const std::runtime_error &) {
        writable = false;
      }
      check(writable == bool(flags & 1u), "Original section write permission");
      total += extent;
    }
    // Import alone must not pretend that any runtime initialization occurred.
    check(r(memory.view(later ? 0x4ecbd0 : 0x4e78d0, 4), 0) == 0 &&
              r(memory.view(later ? 0x52be5c : 0x526248, 4), 0) == 0,
          "Source pool slots remain unconstructed");
    if(later) {
      constexpr std::uint32_t backing=0x60212000u;
      const halo2::SourceRuntimeKernelTickCount clock{backing,
          {std::byte{0x78},std::byte{0x56},std::byte{0x34},std::byte{0x12}},"test native tick-count resource"};
      auto resolved=halo2::load_source_runtime_image(argv[1],halo2::SourcePhysicsContactImage::TitleUpdate15,clock);
      check(r(memory.view(0x41b774u,4),0)==0x8000009cu,"Literal data import keeps ordinal156");
      check(r(resolved.view(0x41b774u,4),0)==backing && r(resolved.view(backing,4),0)==0x12345678u,
          "Native data import resolves to retained clock backing");
      resolved.mutable_view(backing,4)[0]=std::byte{0xaa};
      check(r(resolved.view(backing,4),0)==0x123456aau,"Clock owner can refresh its retained data");
      bool writable=true;
      try {resolved.mutable_view(0x41b774u,4);}catch(const std::runtime_error&){writable=false;}
      check(!writable,"Import resolution preserves the original section protection");
    }
    std::cout << count << " authenticated original sections, " << total
              << " retained section bytes, " << header.size()
              << " literal header bytes and loader permissions match\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
