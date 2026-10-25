#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
enum class SourcePhysicsContactImage { MapPack11, TitleUpdate15 };

// Host bytes of complete committed 4KiB source pages in ordinary retained
// storage. Entries change only with topology, which already excludes every
// accessor; writable pages have ReadWrite access and no backing write guard.
struct SourceDirectPageIndex {
    struct Page { std::byte *bytes{}; bool writable{}; };
    using Block = std::array<Page, 1024>;
    std::array<std::unique_ptr<Block>, 1024> blocks;
};

// Reads/writes address the caller's real retained source storage. Missing body,
// property or contact backing must fail in the adapter; no empty/success default
// exists here. This interface neither allocates a bank nor admits a collision.
class SourcePhysicsContactMemory {
public:
    // Published only by owners whose page reads have no side effects. Null
    // entries, partial pages and page-crossing operands keep the virtual path.
    const SourceDirectPageIndex::Page *direct_page(std::uint32_t address,
                                                   std::size_t bytes) const noexcept {
        if (!direct_pages_ || !bytes || bytes > 4096u - (address & 4095u)) return nullptr;
        const auto &block = direct_pages_->blocks[address >> 22];
        if (!block) return nullptr;
        const auto &page = (*block)[(address >> 12) & 1023u];
        return page.bytes ? &page : nullptr;
    }
    virtual ~SourcePhysicsContactMemory() = default;
    virtual void read(std::uint32_t address, std::span<std::byte> output) const = 0;
    virtual void write(std::uint32_t address, std::span<const std::byte> input) = 0;
    // A source vector operand consists of four ordered scalar reads. Device
    // adapters keep that access/fault sequence, including32-bit address wrap.
    // Retained ordinary storage can specialize the side-effect-free access.
    // Callers consume the complete output only after this operation returns.
    virtual void read_vector(std::uint32_t address,std::span<std::byte,16> output) const {
        for(unsigned lane=0;lane<4;++lane)read(address+lane*4u,output.subspan(lane*4u,4));
    }
    // Optional capability for a synchronous native read-only block. Admission
    // inspects topology only: it must not read backing bytes, allocate, throw,
    // call an observer/guard, or change errno or floating-point state. The
    // complete range must belong to one committed ordinary storage region.
    // Its bytes and lifetime stay stable until the block returns, before any
    // source write, callback, or topology change. Device/custom adapters keep
    // their ordered reads by refusing this capability unless they own that
    // same contract. A refusal leaves the original read/fault prefix intact.
    virtual std::optional<std::span<const std::byte>> try_read_view(
        std::uint32_t,std::size_t) const noexcept { return std::nullopt; }
protected:
    const SourceDirectPageIndex *direct_pages_{};
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
