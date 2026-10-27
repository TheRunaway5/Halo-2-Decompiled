#pragma once
#include "halo2/physics_solver.hpp"
#include <optional>
#include <thread>
#include <typeinfo>

namespace halo2 {
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
    std::function<void(SourcePhysicsContactMemory &,
        const SourcePhysicsContactRuntimeCall &, SourcePhysicsContactRuntimeAbi &,
        const SourcePhysicsContactRecordObserver &)> required_owner;
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
