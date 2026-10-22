#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_bitmap_pending_header.hpp"
#include "halo2/source_runtime_memory.hpp"
namespace halo2::private_cpu_bitmap {
// Owns actual source metadata and retained encoded image bytes. This CPU
// owner implies no GPU allocation or source-cache readiness.
struct Owner {
 Tag bitmap;unsigned frame{},count{},source_first{};
 std::vector<std::byte> root,rows,encoded;
 std::array<std::byte,0x18> common;
 SourceBitmapPendingHeaderMetadata metadata;
 std::array<unsigned,5> header(unsigned actual_encoded_address)const;
};
Owner project(const Tag&,unsigned frame,std::span<const std::byte> complete_root,
 unsigned rows_address,std::span<const std::byte> complete_rows,const BitmapResource&,const Texture&,
 bool resource_only=false);
Owner load(const Cache&,TagHandle,unsigned frame,bool resource_only=false);
struct Delivery {
 std::array<std::byte,0x18> common;
 unsigned header{},encoded{},encoded_bytes{},source_clock{},cached_until{};
};
// Plan only these sparse delivery fields: common[0,0x18), encoded_bytes+0x34/4,
// header+0x50/4, encoded+0x54/4 and cached_until+0x70/4. Root owns authenticated
// Cache/canonical admission, row namespace/extent allocation and publication.
// These are CLOSED CPU resource fields, not GPU residency or stream handles.
Delivery delivery(const Owner&,const SourceRuntimeMemory&,
 NativeRuntimeAddress actual_encoded,NativeRuntimeAddress actual_header);

}
