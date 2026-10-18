#pragma once
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include <memory>

namespace halo2 {
class SourcePhysicsContactMemory;
struct EncodedSound;
// Reference arithmetic operands only. These never replace the actual MCC
// codec/rate/chunks, serialized SampleSize or decoder-produced PCM duration.
struct MccSoundOriginalTimingOperands {
    std::uint32_t encoded_bytes{};
    std::uint8_t sample_rate{},encoding{},compression{};
    auto operator<=>(const MccSoundOriginalTimingOperands&)const=default;
};
enum class MccSoundTimingGapKind {
    UnknownOwner,CustomOwner,NoOriginalKey,NaturalPitchNotRestored,
    InvalidNativeProjection,EncodedResourceMismatch
};
struct MccSoundTimingGap {
    TagHandle sound,gestalt;
    std::uint32_t source_range{},source_permutation{};
    MccSoundTimingGapKind kind{};
    std::string reason;
};
struct MccSoundTimingBinding {
    TagHandle sound,gestalt;
    std::uint32_t source_root{},native_root{};
    std::uint32_t source_range{},source_permutation{};
    std::uint32_t native_range{},native_permutation{};
    unsigned relative_pitch{},relative_permutation{};
    std::string path,range_name,permutation_name;
    MccSoundVanillaDigest named_key{},encoded_payload{};
    MccSoundOriginalTimingOperands original;
};
// Admitted original stored milliseconds, qualified once by the complete
// finite metadata/encoded-resource policy. Absence means unavailable, never
// permission to use a provider's current or PCM-derived duration. This is an
// immutable expectation, not a live-memory lease: consumers must retain the
// plan and validate the current epoch/full-salt root/entire20-byte header
// inside their existing owned-read transaction before using original_ms.
struct MccSoundTimingStoredReference {
    TagHandle sound,gestalt;
    std::uint32_t source_root{},native_root{};
    std::int32_t original_ms{};
    std::array<std::byte,20> expected_native_header;
};
// Distinct authored U16 milliseconds consumed by original21F670. A retained
// record proves one exact named/encoded permutation, including its complete
// final native rows. It is not a current-memory lease: the consumer must
// validate the current epoch, full-salt roots and all immutable bytes within
// its owned-read scope (range bytes4/5 remain mutable selectors). Zero is a
// valid admitted value. Absence always means unavailable.
struct MccSoundTimingPermutationReference {
    TagHandle sound,gestalt;
    std::uint32_t source_root{},native_root{};
    std::uint32_t source_range{},source_permutation{};
    std::uint32_t native_range{},native_permutation{};
    unsigned relative_pitch{},relative_permutation{};
    std::uint16_t original_ms{};
    MccSoundVanillaDigest named_key{},encoded_payload{};
    std::array<std::byte,20> expected_native_header;
    std::array<std::byte,12> expected_native_range;
    std::array<std::byte,16> expected_native_permutation;
};
class MccSoundTimingPlan final {
public:
    std::span<const MccSoundTimingBinding> bindings()const noexcept;
    std::span<const MccSoundTimingGap> gaps()const noexcept;
    std::span<const MccSoundVanillaOwnerResult> owners()const noexcept;
    std::span<const MccSoundTimingStoredReference> stored_references()const noexcept;
    std::span<const MccSoundTimingPermutationReference> permutation_references()const noexcept;
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit MccSoundTimingPlan(std::shared_ptr<const Impl>);
    friend MccSoundTimingPlan build_mcc_sound_timing_reference(
        const MccSoundVanillaSource&,const MccSoundRuntimePlan&);
    friend class MccSoundTimingBoundPlan;
    friend struct MccSoundTimingAccess;
};
// Owner-atomic, immutable construction: authenticate the complete raw owner,
// all typed named intervals, the complete supplied V4 native projection and
// actual encoded payload identity before admitting any binding for an owner.
// Unknown/custom owners and the three known Wraith natural-pitch omissions
// remain explicit gaps. No source/native byte or existing plan field changes.
MccSoundTimingPlan build_mcc_sound_timing_reference(
    const MccSoundVanillaSource&,const MccSoundRuntimePlan&);
MccSoundTimingPlan build_mcc_sound_timing_reference(
    const Cache&,const MccSoundRuntimePlan&,
    const std::function<std::uint32_t(std::uint32_t,std::size_t)>& relocate);

// Separate current-memory ownership. An actual shared source-memory owner is
// required; no numeric pointer/identity constructor exists. A producer must
// invalidate/rebind this lease at every bank release/restore/rebind, including
// identical-byte restores. Rebinding always replaces its private epoch.
// Bind/resource/lookup transactions and rebind/invalidate share one lock;
// an owner cannot be rebound halfway through a read transaction. Concurrent
// mutation of the actual memory outside that producer contract is forbidden.
// Creation fixes the owning thread. Bind/authenticate/lookup/rebind must run
// on that same producer thread; wrong-thread calls reject before any read.
// Invalidation may run from teardown and waits for an in-progress transaction.
// Runtime producers must retain their existing executor/call gate; this lease
// does not make unsynchronized writes by another thread safe.
// Runtime integration is deliberately outside this pure module.
class MccSoundTimingMemoryOwner final {
public:
    explicit MccSoundTimingMemoryOwner(std::shared_ptr<const SourcePhysicsContactMemory>);
    ~MccSoundTimingMemoryOwner();
    MccSoundTimingMemoryOwner(const MccSoundTimingMemoryOwner&)=delete;
    MccSoundTimingMemoryOwner& operator=(const MccSoundTimingMemoryOwner&)=delete;
    void rebind(std::shared_ptr<const SourcePhysicsContactMemory>);
    void invalidate()noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend struct MccSoundTimingAccess;
};
class MccSoundTimingBoundPlan final {
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit MccSoundTimingBoundPlan(std::shared_ptr<const Impl>);
    friend MccSoundTimingBoundPlan bind_mcc_sound_timing_memory(
        const MccSoundTimingPlan&,const std::shared_ptr<const MccSoundTimingMemoryOwner>&);
    friend struct MccSoundTimingAccess;
};
// Check the real current tag table/globals, complete salted roots and owned
// immutable duration inputs before issuing this opaque owner/epoch proof.
MccSoundTimingBoundPlan bind_mcc_sound_timing_memory(
    const MccSoundTimingPlan&,const std::shared_ptr<const MccSoundTimingMemoryOwner>&);

class MccSoundTimingResourceProof final {
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit MccSoundTimingResourceProof(std::shared_ptr<const Impl>);
    friend MccSoundTimingResourceProof authenticate_mcc_sound_timing_resource(
        const MccSoundTimingBoundPlan&,const std::shared_ptr<const MccSoundTimingMemoryOwner>&,
        TagHandle,unsigned,unsigned,std::shared_ptr<const EncodedSound>);
    friend struct MccSoundTimingAccess;
};
// Hash the actual immutable encoded buffer and check all packet/format/name
// facts. The resulting proof retains that actual buffer, its full binding and
// the current memory epoch. A digest or caller numeric token is insufficient.
MccSoundTimingResourceProof authenticate_mcc_sound_timing_resource(
    const MccSoundTimingBoundPlan&,const std::shared_ptr<const MccSoundTimingMemoryOwner>&,
    TagHandle sound,unsigned relative_pitch,unsigned relative_permutation,
    std::shared_ptr<const EncodedSound>);

struct MccSoundTimingEntry {
    std::uint32_t pc{},return_address{};
    std::uint32_t eax{},ebx{},ecx{},edx{},esi{},edi{},esp{};
    // Original caller stack at ESP: return, range pointer, actual encoded size.
    std::array<std::uint32_t,3> stack{};
};
enum class MccSoundTimingLookupStatus {
    Reference,NoReference,StaleOrForeignOwner,StaleOrForeignResource,
    WrongCaller,WrongTagOwner,WrongRangeOrPermutation,ChangedNativeMetadata,WrongThread,
    ReentrantLeaseOperation
};
struct MccSoundTimingLookup {
    MccSoundTimingLookupStatus status{MccSoundTimingLookupStatus::NoReference};
    std::optional<MccSoundOriginalTimingOperands> original;
};
// Read-only lookup before the original routine executes any instruction.
// Require genuine21F590/21F6F8, full current ownership, exact merged intervals,
// current immutable bytes and actual-resource proof. No GPR/EFLAGS/stack/x87
// write or arithmetic is performed. NoReference never manufactures a duration.
MccSoundTimingLookup lookup_mcc_sound_original_timing(
    const MccSoundTimingBoundPlan&,const std::shared_ptr<const MccSoundTimingMemoryOwner>&,
    const MccSoundTimingEntry&,const MccSoundTimingResourceProof&);
// Perform the same complete validation and retain the actual memory owner,
// epoch and exclusive lease lock until this callback returns or throws. The
// callback receives only the four reference operands; it never receives a
// fabricated duration or an altered source-memory view from this module.
// The producer must still hold its executor/call gate. The callback must not
// invoke any timing lease operation (including invalidate); nested operations
// that can report failure reject before reads. Teardown invalidation on another
// thread waits for the complete transaction. Exceptions propagate with the
// lease unlocked. A non-Reference status never invokes the callback.
MccSoundTimingLookupStatus with_mcc_sound_original_timing(
    const MccSoundTimingBoundPlan&,const std::shared_ptr<const MccSoundTimingMemoryOwner>&,
    const MccSoundTimingEntry&,const MccSoundTimingResourceProof&,
    const std::function<void(const MccSoundOriginalTimingOperands&)>&);
} // namespace halo2

namespace halo2 {
// An opaque lease of a real memory owner and its current epoch. It does not
// require a timing-plan binding and does not certify any tag or metadata.
// Rebind/restore/invalidate makes an existing lease stale, even if bytes match.
class MccSoundTimingReadLease final {
public:
    MccSoundTimingReadLease(const MccSoundTimingReadLease&)=default;
    MccSoundTimingReadLease&operator=(const MccSoundTimingReadLease&)=default;
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit MccSoundTimingReadLease(std::shared_ptr<const Impl>);
    friend struct MccSoundTimingOwnedReadAccess;
};
class MccSoundTimingOwnedReader final {
public:
    MccSoundTimingOwnedReader(const MccSoundTimingOwnedReader&)=default;
    MccSoundTimingOwnedReader&operator=(const MccSoundTimingOwnedReader&)=default;
    // Copies owned bytes under the active transaction. A retained copy of the
    // reader cannot read after that transaction or on another thread.
    std::vector<std::byte> read(std::uint32_t address,std::size_t bytes)const;
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
    explicit MccSoundTimingOwnedReader(std::shared_ptr<Impl>);
    friend struct MccSoundTimingOwnedReadAccess;
};
enum class MccSoundTimingReadStatus {
    Executed,StaleOwner,WrongThread,ReentrantLeaseOperation
};
MccSoundTimingReadLease capture_mcc_sound_timing_read_lease(
    const std::shared_ptr<const MccSoundTimingMemoryOwner>&);
// The producer still holds its executor/call gate. The transaction retains
// the actual owner, memory and epoch under the same timing lease mutex and
// execution guard. The callback may only copy bytes through its reader; it
// must not call any timing lease operation, including invalidate. Invalidation
// from another thread waits. Throws unlock the lease and revoke the reader.
// Executed reports callback execution, not canonical metadata admission.
MccSoundTimingReadStatus with_mcc_sound_timing_owned_read(
    const MccSoundTimingReadLease&,
    const std::function<void(const MccSoundTimingOwnedReader&)>&);
// A current-epoch check performs no memory reads. The caller must use a held
// transaction when byte validation and consumption need one lifetime scope.
MccSoundTimingReadStatus check_mcc_sound_timing_read_lease(
    const MccSoundTimingReadLease&);
} // namespace halo2
