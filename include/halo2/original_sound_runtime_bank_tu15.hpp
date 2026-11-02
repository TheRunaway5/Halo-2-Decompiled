#pragma once
#include "halo2/prepared_original_sound_bank.hpp"
#include "halo2/sound_variation.hpp"
#include "halo2/source_sound_cache_read_tu15.hpp"
#include <functional>

namespace halo2 {
struct OriginalSoundRuntimeBindingTu15 {
    TagHandle sound;
    std::shared_ptr<const PreparedOriginalSoundBank> bank;
};
struct OriginalSoundRuntimeStringsTu15 {
    // Real immutable native string ownership. Resolution is a data import,
    // not an original executable call or a saved CPU outcome.
    std::shared_ptr<const void> owner;
    std::function<std::uint32_t(std::uint32_t,std::string_view)> resolve;
    std::function<std::string(std::uint32_t)> lookup;
};
// Already admitted original-ABI native tables. Their indices remain stable
// when original bank rows are appended. The strong owner retains these staged
// views. External nested pointers keep their enclosing allocation owner, which
// the live publisher must retain alongside this graph. This graph never reads
// a prefix's external nested allocation or supplies a World/readiness lease.
struct OriginalSoundRuntimePrefixTu15 {
    std::shared_ptr<const void> owner;
    std::array<std::span<const std::byte>,11> tables;
    std::span<const std::byte> sound_classes;
};
// Owned original-ABI tables and sector resources. This does not construct a
// tag directory, SoundSystem, source clock, voice, device or readiness flag.
// A live publisher must reserve the logical range, install the returned roots
// under its actual tag identities, and include the mutable bytes in restores.
class OriginalSoundRuntimeBankTu15 final {
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit OriginalSoundRuntimeBankTu15(std::unique_ptr<Impl>);
public:
    ~OriginalSoundRuntimeBankTu15();
    static std::shared_ptr<OriginalSoundRuntimeBankTu15> create(
        std::uint32_t logical_address,std::uint32_t available_bytes,
        std::shared_ptr<const std::vector<Tag>> actual_native_tags,
        std::span<const OriginalSoundRuntimeBindingTu15>,
        const OriginalSoundRuntimeStringsTu15&,
        const OriginalSoundRuntimePrefixTu15& = {});
    std::uint32_t address()const;
    std::span<const std::byte> bytes()const;
    std::span<std::byte> memory(std::uint32_t,std::size_t);
    std::uint32_t gestalt_root()const;
    std::uint32_t class_root()const;
    std::uint32_t sound_root(TagHandle)const;
    TagBlock block(unsigned)const;
    // Prefix descriptors retain their existing external resource owner.
    // Only descriptors at/after this ordinal can use this bank's resources.
    std::uint32_t original_chunk_first()const;
    SoundVariationDefinition load(TagHandle,unsigned pitch=0)const;
    EncodedSound encoded(TagHandle,unsigned pitch,unsigned permutation)const;
    // Exact sector data from a real descriptor owner. No handle is made ready
    // and no resource-cache/store result is synthesized by reading these bytes.
    void read_resource(std::uint32_t native_chunk_descriptor,std::size_t offset,
                       std::span<std::byte> destination)const;
    // Exact whole-sector sound-cache read. Both the physical descriptor and
    // its current first8 bytes must match this admitted original resource.
    // It neither consumes nor supplies the runtime cache handle at+8.
    std::vector<std::byte> read_cache_resource(const SourceSoundCacheReadTu15&)const;
};
}
