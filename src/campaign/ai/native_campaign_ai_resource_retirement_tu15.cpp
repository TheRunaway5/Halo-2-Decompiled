#include "halo2/native_campaign_ai_resource_retirement_tu15.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include <array>

namespace halo2 {
NativeCampaignAiResourceRetirementRequiredOwnerTu15::NativeCampaignAiResourceRetirementRequiredOwnerTu15(
    std::uint32_t entry, std::uint32_t identity) :
    std::runtime_error("Native TU1.5 resource retirement requires its actual remaining owner"),
    entry_(entry), identity_(identity) {}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory, std::uint32_t address,
                   unsigned width = 4) {
    std::array<std::byte, 4> bytes{};
    memory.read(address, std::span(bytes).first(width));
    std::uint32_t value{};
    for (unsigned n = 0; n < width; ++n)
        value |= std::to_integer<unsigned>(bytes[n]) << (n * 8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory, const SourcePhysicsContactRecordObserver &observe,
             std::uint32_t site, std::uint32_t address, std::uint32_t value,
             unsigned width = 4) {
    SourcePhysicsContactRecordStore event{site, address, {}, std::vector<std::byte>(width)};
    if (observe) { event.before.resize(width); memory.read(address, event.before); }
    for (unsigned n = 0; n < width; ++n) event.written[n] = std::byte(value >> (n * 8u));
    memory.write(address, event.written);
    if (observe) observe(event);
}
}
void native_campaign_ai_unassociated_resource_retire_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t datum, const SourcePhysicsContactRecordObserver &) {
    const auto initial_pool = read(memory, 0x4f2d24u);
    const auto record = read(memory, initial_pool + 0x44u) + (datum & 0xffffu) * 0x28u;
    if (read(memory, record + 0x10u))
        throw NativeCampaignAiResourceRetirementRequiredOwnerTu15(0x12fd40u, record);
    // The callback reloads the pool after its association branch. A callback
    // can alias list/container storage, so callers cannot cache this read.
    const auto pool = read(memory, 0x4f2d24u);
    native_tu15_data_array_delete(memory, pool, datum);
}
void native_campaign_ai_resource_metadata_retire_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t container, std::uint32_t datum,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto initial_pool = read(memory, container + 0x64u);
    const auto record = read(memory, initial_pool + 0x44u) + (datum & 0xffffu) * 0x18u;
    const auto callback = read(memory, container + 0x20u);
    if (callback) {
        if (callback != 0x12fd10u)
            throw NativeCampaignAiResourceRetirementRequiredOwnerTu15(callback, datum);
        native_campaign_ai_unassociated_resource_retire_tu15(memory, datum, observe);
    }
    const auto previous = read(memory, record + 0x10u);
    if (previous != 0xffffffffu) {
        const auto pool = read(memory, container + 0x64u);
        const auto rows = read(memory, pool + 0x44u);
        const auto next = read(memory, record + 0xcu);
        publish(memory, observe, 0x140347u, rows + (previous & 0xffffu) * 0x18u + 0xcu, next);
    } else {
        const auto next = read(memory, record + 0xcu);
        publish(memory, observe, 0x140350u, container + 0x3cu, next);
    }
    const auto next = read(memory, record + 0xcu);
    if (next != 0xffffffffu) {
        const auto pool = read(memory, container + 0x64u);
        const auto rows = read(memory, pool + 0x44u);
        const auto fresh_previous = read(memory, record + 0x10u);
        publish(memory, observe, 0x14036cu, rows + (next & 0xffffu) * 0x18u + 0x10u, fresh_previous);
    } else {
        const auto fresh_previous = read(memory, record + 0x10u);
        publish(memory, observe, 0x14037eu, container + 0x40u, fresh_previous);
    }
    const auto pool = read(memory, container + 0x64u);
    native_tu15_data_array_delete(memory, pool, datum);
}
void native_campaign_ai_resource_backing_retire_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t user, const NativeCampaignAiResourceProtectionTu15 &protection,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto initial_datum = read(memory, user - 0x24u);
    const auto pool = read(memory, 0x4f2d24u);
    const auto rows = read(memory, pool + 0x44u);
    const auto protected_address = read(memory, user - 0x1cu);
    const auto record = rows + (initial_datum & 0xffffu) * 0x28u;
    const auto protected_bytes = read(memory, user - 0x18u);
    // The original wrapper does not read its import selector for an empty
    // byte range. The service owns actual host pages and has no owed return.
    if (protected_bytes) {
        const auto owner = read(memory, 0x41b648u);
        if (owner != 0x800000b6u)
            throw NativeCampaignAiResourceRetirementRequiredOwnerTu15(owner, protected_address);
        protection.protect_pages(protected_address, protected_bytes, 0x404u);
    }
    publish(memory, observe, 0x13005bu, record + 2u, read(memory, record + 2u, 1) & 0xfeu, 1);
    const auto previous = read(memory, user - 4u);
    const auto next = read(memory, user - 8u);
    publish(memory, observe, 0x130067u, user - 0x20u, 0);
    if (previous) publish(memory, observe, 0x130070u, previous + 0x1cu, next);
    std::uint32_t container{}, datum{};
    if (next) {
        container = read(memory, 0x4f2d34u);
        publish(memory, observe, 0x13007du, next + 0x20u, previous);
        datum = read(memory, user - 0x24u);
    } else {
        datum = read(memory, user - 0x24u);
        container = read(memory, 0x4f2d34u);
        publish(memory, observe, 0x130094u, 0x4f2d2cu, previous);
    }
    native_campaign_ai_resource_metadata_retire_tu15(memory, container, datum, observe);
}
}
