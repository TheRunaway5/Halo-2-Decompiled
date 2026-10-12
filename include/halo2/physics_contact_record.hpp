#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
enum class SourcePhysicsContactImage { MapPack11, TitleUpdate15 };

// Reads/writes address the caller's real retained source storage. Missing body,
// property or contact backing must fail in the adapter; no empty/success default
// exists here. This interface neither allocates a bank nor admits a collision.
class SourcePhysicsContactMemory {
public:
    virtual ~SourcePhysicsContactMemory() = default;
    virtual void read(std::uint32_t address, std::span<std::byte> output) const = 0;
    virtual void write(std::uint32_t address, std::span<const std::byte> input) = 0;
};

struct SourcePhysicsContactConstruction {
    std::uint32_t row_address;              // EAX: allocated0x48-byte row.
    std::uint32_t contact_index_word;       // ECX: source consumes its low16.
    std::uint32_t source_key;               // EDX: opaque key, stored at+0x0C.
    // Original nine stack arguments (ret0x24): counterpart encoded key,
    // own body pointer, counterpart body pointer, fallback component datum,
    // flag word(low8), two opaque scalar words, two material words(low16).
    // Body pointers/property values/datum keys are distinct source identities.
    std::array<std::uint32_t, 9> stack_arguments;
};

struct SourcePhysicsContactRecordStore {
    std::uint32_t instruction, address;
    std::vector<std::byte> before, written;
    friend bool operator==(const SourcePhysicsContactRecordStore &,
                           const SourcePhysicsContactRecordStore &) = default;
};
using SourcePhysicsContactRecordObserver =
    std::function<void(const SourcePhysicsContactRecordStore &)>;

// Complete TU1.1 1D2DF0 / TU1.5 1D76B0 constructor, measured on the genuine
// source-created Lockout crate/BSP contact. Preserves all untouched row bytes
// and original write order; returns the original unchanged EAX row address.
// Encoded counterpart classes3/4 require the still-unmeasured52625C/52BE70
// lookup owner and throw at that boundary after preceding original stores.
// No point/normal/impulse, solver contact, bank allocation or lifetime is made.
std::uint32_t source_construct_physics_contact_record(
    SourcePhysicsContactMemory &, const SourcePhysicsContactConstruction &,
    SourcePhysicsContactImage, const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
