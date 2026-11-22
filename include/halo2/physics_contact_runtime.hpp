#pragma once
#include "halo2/physics_solver.hpp"
#include "halo2/native_tu15_normalize_vector.hpp"
#include <optional>
#include <thread>
#include <typeinfo>

namespace halo2 {
struct NativeTu15NormalizeOutcome;
struct NativeTu15NormalizeConstants;
struct NativeTu15NormalizeStats;
// Active MMX aliases physical x87 registers and makes every tag valid. Some
// physical registers can be unknown when entry supplied only live x87 values;
// the mask records that absence instead of inventing the dead register bits.
struct SourcePhysicsMmxAlias {
    std::array<std::array<std::byte,10>,8> physical;
    std::uint8_t known_mask;
};
// Retained source call state. A caller must supply its real live ABI state;
// register and stack defaults are not substitutes for missing call owners.
struct SourcePhysicsContactRuntimeAbi {
    // EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI.
    std::array<std::uint32_t,8> gp;
    std::array<std::array<std::uint32_t,4>,8> xmm;
    std::uint32_t eflags;
    std::uint32_t fs_base;
    // Live logical ST(0)..ST(depth-1); physical TOP comes from x87_status.
    // Sparse tag stacks and stack faults require another explicit owner.
    std::array<long double,8> x87;
    unsigned x87_depth;
    std::uint16_t x87_status;
    // Present on retained call state. A fresh legacy caller may leave this
    // absent and supply controls through the entry's explicit FP argument.
    std::optional<SourcePhysicsFactoryFloatingPoint> floating_point;
    // Present only while MMX owns the register file. TOP is then zero and
    // x87_depth is zero; physical tags are all valid. EMMS empties the tags.
    std::optional<SourcePhysicsMmxAlias> active_mmx;
};
struct SourcePhysicsContactRuntimeCall {
    std::uint32_t instruction;
    std::uint32_t target;
    bool tail;
};
struct SourcePhysicsContactRuntimeServices {
    std::function<void(std::uint32_t)> instruction;
    SourcePhysicsReciprocalSqrtEstimate reciprocal_sqrt;
    std::function<std::uint64_t()> timestamp_counter;
    // Full callee ownership includes its actual register/FP and stack ABI.
    // The call instruction has already pushed the real return address.
    // An absent owner stops there; it never returns fabricated contact data.
    using RequiredOwner = std::function<void(SourcePhysicsContactMemory &,
        const SourcePhysicsContactRuntimeCall &, SourcePhysicsContactRuntimeAbi &,
        const SourcePhysicsContactRecordObserver &)>;
    RequiredOwner required_owner;
    // Optional exact cursor contract for the live owner. Outside the listed
    // service PCs and active traces, instruction() only publishes this cursor.
    // References outlive the synchronous native entry; other executors retain
    // the full callback. A replaced/wrapped callback also keeps full delivery;
    // its callable type must match the explicitly admitted one. Default
    // services keep observing every instruction.
    struct InstructionCursor {
        std::uint32_t *position{};
        const bool *first_trace{},*second_trace{};
        std::span<const std::uint32_t> service_sites;
        std::thread::id executor;
        const std::type_info *callback_type{};
        // Permit literal-PC dispatch only after the state has checked the
        // exact live service-site set. Generic descriptors keep their search.
        bool literal_service_sites{};
        // The owning caller has qualified a synchronous observation-only
        // store collector. It may record or reject the completed write, but
        // must not mutate source state/backing or instruction/trace policy.
        // Arbitrary observers retain full instruction delivery by default.
        bool observation_only_stores{};
    } instruction_cursor;
    // Optional semantic30BD0 owner. With this pointer set, qualified ordinary
    // calls may bypass required_owner and internal instruction/stack events.
    // Owner-observing tools keep this null; the bridge also refuses arbitrary
    // callbacks, all store observers, active traces and non-direct vector or
    // stack storage. Constants may have exact bounded read-view owners.
    // Raw adapters must have no retained-memory policy registry. Banks with
    // registry policies use the guarded service below, even with direct pages.
    NativeTu15NormalizeOutcome (*normalize_vector_30bd0)(SourcePhysicsContactMemory&,
        std::uint32_t,const SourcePhysicsContactRecordObserver&,NativeTu15NormalizeConstants){};
    NativeTu15NormalizeStats *normalize_vector_30bd0_stats{};
    // A retained-memory owner uses this service instead of the raw resident
    // pointer when registry policies also apply. Refusal runs no source work.
    // Success includes the final stack residue under the same policy lease;
    // the bridge then publishes only the returned register/FP state.
    std::function<std::optional<NativeTu15NormalizeOutcome>(SourcePhysicsContactMemory&,
        std::uint32_t,std::uint32_t,const SourcePhysicsContactRecordObserver&,
        NativeTu15NormalizeConstants)> normalize_vector_30bd0_guarded;
    // An audited owner may certify its exact callable/body and captured owner
    // for synchronous native parents. On every return or exception, that
    // owner and its descendants preserve the caller's instruction callback,
    // cursor descriptor and trace policy. A replacement/wrapper needs its own
    // qualification. Matching immutable copies/moves may retain ownership.
    // The matcher is observation-only, noexcept, and must check the actual
    // callable/capture; a generic boolean or target-type guess is insufficient.
    struct RequiredOwnerPolicy {
        const std::type_info *callback_type{};
        std::thread::id executor;
        const void *owner{};
        bool (*matches)(const RequiredOwner &,const void *) noexcept{};
    } required_owner_policy;
    // Only the World can seal its immutable, address-stable services table.
    // Copies used for remapping, observation or other owners start unsealed.
    class ImmutableInstructionCursor {
    public:
        ImmutableInstructionCursor()=default;
        ImmutableInstructionCursor(const ImmutableInstructionCursor &) noexcept {}
        ImmutableInstructionCursor &operator=(const ImmutableInstructionCursor &) noexcept {
            services_=nullptr;return *this;
        }
        bool identifies(const SourcePhysicsContactRuntimeServices &services)const noexcept {
            return services_==&services;
        }
    private:
        friend class PhysicsLiveWorldTu15;
        // Set once during construction of the const table, before publication.
        mutable const SourcePhysicsContactRuntimeServices *services_{};
    } immutable_instruction_cursor;
    // Optional grouped store for the forward 1C5993 and 27B2DD AI workspace
    // clears. The World admits the exact services table and storage owner.
    // A refusal leaves data and write accounting untouched for the scalar loop.
    std::function<bool(const SourcePhysicsContactRuntimeServices &,
        SourcePhysicsContactMemory &,std::uint32_t site,std::uint32_t workspace)> clear_ai_search_workspace;
    // Optional synchronous 27C2B2 continuation. The exact live World keeps
    // its native write policy lease through the retained stack/row effects.
    // Refusal precedes the original instruction cursor and memory effects.
    using AiPathEdgeRingCallback = void(*)(void *,std::uint32_t &);
    std::function<bool(const SourcePhysicsContactRuntimeServices &,
        SourcePhysicsContactMemory &,std::uint32_t stack,void *,AiPathEdgeRingCallback)> run_ai_path_edge_ring;
};
// Canonical 1.1 entry identifies a statically compiled complete source body.
// Image selection binds authenticated TU1.5 constants, targets and tables.
// No instruction decoder, binary execution or packet replay runs in production.
void source_run_physics_contact_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
