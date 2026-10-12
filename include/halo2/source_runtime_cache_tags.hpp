#pragma once
#include "halo2/source_runtime_memory.hpp"
#include <array>

namespace halo2 {
// Native cache I/O delivery. These are the cache's literal address ranges,
// header datums and table pointer, including the Cache owner's balance writes.
// They are not an emulated cache-loader completion or a captured game bank.
struct SourceRuntimeCacheTagImage {
  struct Region {std::uint32_t address;std::vector<std::byte> bytes;};
  std::vector<Region> regions;
  std::uint32_t index{},table{},count{},scenario{},globals{};
};
struct SourceRuntimeCacheBspImage {
  SourceRuntimeCacheTagImage::Region region;
  std::uint32_t mapping{},index{},file_offset{},bsp{},lightmap{};
  std::array<std::byte,0x44> mapping_bytes;
};
struct SourceRuntimeCacheAnimationImage {
  std::array<std::byte,20> descriptor;
  std::vector<std::byte> bytes;
};

// Delivers actual cache bytes into the existing physical bank and binds the
// native cache owner to TU1.1 consumers. The scenario/globals selection follows
// 124159..124199's literal index/header operands. This native I/O boundary does
// not run the original cache startup, BSP fixups, world creation or tag activation.
// All ranges and outputs are checked before publishing any input bytes.
void source_bind_runtime_cache_tags_1_1(SourceRuntimeMemory &,
                                       const SourceRuntimeCacheTagImage &);
// Real BSP resource bytes for124290's1244B0 I/O boundary. Publication of the
// sbsp/ltmp tag-table addresses remains with complete original124290.
void source_deliver_runtime_cache_bsp_1_1(SourceRuntimeMemory &,
                                        const SourceRuntimeCacheBspImage &);
// Native selected-BSP consumer bindings after124290 has published its actual
// tag addresses; corresponding source operands are11D8AD..11D909.
void source_bind_runtime_cache_bsp_1_1(SourceRuntimeMemory &,
                                     const SourceRuntimeCacheBspImage &);
// TU1.5 native cache I/O bindings, authenticated against actual125437..125474
// and11EA1D..11EA69. Complete1255C0 retains BSP tag-table publication.
void source_bind_runtime_cache_tags_tu15(SourceRuntimeMemory &,
                                        const SourceRuntimeCacheTagImage &);
void source_deliver_runtime_cache_bsp_tu15(SourceRuntimeMemory &,
                                         const SourceRuntimeCacheBspImage &);
void source_bind_runtime_cache_bsp_tu15(SourceRuntimeMemory &,
                                      const SourceRuntimeCacheBspImage &);
} // namespace halo2
