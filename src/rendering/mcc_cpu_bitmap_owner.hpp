#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_bitmap_pending_header.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <map>
#include <optional>
namespace halo2::private_cpu_bitmap {
// Resource storage and CPU sampling are independently qualified domains.
// Raw prediction keeps encoded bytes; it does not admit a CPU pixel fetch.
bool raw_resource_format(unsigned format) noexcept;
bool cpu_sampling_format(unsigned format) noexcept;
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

// A single full-salt MCC bitmap frame retains its authored partitions in
// decoded cache form. The source request still selects a partition, its
// serialized offset and its exact sector-rounded transfer size.
class PredictionPartitions {
public:
    struct Descriptor {
        std::uint32_t encoded_offset, stored_bytes, payload_bytes;
    };
    static PredictionPartitions load(const Cache&, TagHandle, unsigned frame);
    std::optional<Descriptor> descriptor(unsigned partition) const;
    // Initial row for the explicitly selected 12F4A0/1300B0 cache request.
    // The MCC transport does not supply the Xbox LOD cutoff at +0x58.
    std::array<std::byte, 116> cold_source_row() const;
    std::vector<std::byte> read_sectors(unsigned partition, std::uint32_t encoded_offset,
                                       std::uint32_t transfer_bytes) const;
private:
    PredictionPartitions() = default;
    TagHandle bitmap_;
    std::array<std::byte, 0x18> common_{};
    std::array<Descriptor, 3> descriptors_{};
    std::array<std::vector<std::byte>, 3> payloads_;
};

// A texture tracker retains the original 116-byte row address. Keep that
// address stable for the life of the World and rebuild the host index from
// authenticated rows when the World restores its source memory.
class PredictionRows {
public:
    std::uint32_t bind(SourceRuntimeMemory&, const Cache&, TagHandle, unsigned frame);
    std::optional<std::vector<std::byte>> read(const SourceRuntimeMemory&, const Cache&,
        std::uint32_t row, unsigned partition, std::uint32_t offset, std::uint32_t bytes) const;
    static PredictionRows restore(const SourceRuntimeMemory&, const Cache&);
    void swap(PredictionRows& other) noexcept { rows_.swap(other.rows_); }
private:
    using Key = std::pair<std::uint32_t, unsigned>;
    std::map<Key, std::uint32_t> rows_;
};

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
