#include "halo2/native_tu15_phantom_body_velocity.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"

#include <array>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {

void require_retained(const SourcePhysicsContactMemory &memory, std::uint32_t address, unsigned width) {
    if (const auto *domain = dynamic_cast<const NativeTu15RetainedStoreObservationDomain *>(&memory))
        domain->require_retained_extent(address, width);
}

std::uint32_t read(const SourcePhysicsContactMemory &memory, std::uint32_t address, unsigned width = 4) {
    std::array<std::byte, 4> bytes{};
    require_retained(memory, address, width);
    memory.read(address, std::span(bytes).first(width));
    std::uint32_t result{};
    for (unsigned i = 0; i < width; ++i)
        result |= std::to_integer<std::uint32_t>(bytes[i]) << (8u * i);
    return result;
}

std::uint32_t body_cell(const SourcePhysicsContactMemory &memory, std::uint32_t component, std::uint32_t slot) {
    return read(memory, component + 0x70u) + slot * 0x60u + 0x40u;
}

void assign_velocity(SourcePhysicsContactMemory &memory, std::uint32_t motion, bool angular,
                     const std::array<std::uint32_t, 4> &words,
                     const NativeTu15RetainedStoreObserver &observer) {
    const auto owner = read(memory, read(memory, motion) + (angular ? 0x54u : 0x50u));
    if (owner != (angular ? 0x323a00u : 0x3239f0u))
        throw std::runtime_error("Native phantom velocity requires motion virtual=" + std::to_string(owner));
    const auto address = motion + (angular ? 0x50u : 0x40u);
    if (address & 15u)
        throw std::runtime_error("Native phantom velocity requires aligned MOVAPS destination");
    require_retained(memory, address, 16);
    const auto pc = angular ? 0x323a07u : 0x3239f7u;
    std::array<std::byte, 16> bytes{};
    for (unsigned i = 0; i < bytes.size(); ++i)
        bytes[i] = std::byte(words[i / 4u] >> (8u * (i % 4u)));
    const auto &callback = observer.callback();
    const bool observe = native_tu15_observes_retained_store(memory, callback, pc, address, 16);
    SourcePhysicsContactRecordStore event{pc, address, {}, {}};
    if (observe) {
        event.before.resize(16);
        memory.read(address, event.before);
    }
    memory.write(address, bytes);
    if (observe) {
        event.written.assign(bytes.begin(), bytes.end());
        callback(event);
    }
}

} // namespace

void native_tu15_set_phantom_body_velocity(
    SourcePhysicsContactMemory &memory, std::uint32_t component, std::uint32_t slot,
    NativeTu15PhantomVelocityKind kind, const NativeTu15VelocityWords &velocity,
    NativeTu15PhysicsFrameFp &fp, const NativeTu15IslandActivationServices &activation,
    const NativeTu15RetainedStoreObserver &observer, NativeTu15IslandActivationStats *activity) {
    const bool angular = kind == NativeTu15PhantomVelocityKind::angular;
    if (angular && (read(memory, component + 4u) & 2u))
        return;
    const auto cell = body_cell(memory, component, slot);
    auto body = read(memory, cell);
    if (read(memory, body + 0x40u, 1))
        return;

    const std::array<std::uint32_t, 4> words{velocity(0), velocity(1), velocity(2), 0};
    if (angular)
        body = read(memory, cell); // 1D9156 after the three caller DATA loads.
    const auto island = read(memory, body + 0x44u);
    const bool active = island && read(memory, island + 0x29u, 1);
    if (!active && read(memory, body + 0x44u))
        native_tu15_activate_physics_body(memory, {body}, fp, activation, observer.callback(), activity);
    assign_velocity(memory, read(memory, body + 0x3cu), angular, words, observer);
}

void native_tu15_add_phantom_body_linear_velocity(
    SourcePhysicsContactMemory &memory, std::uint32_t component, std::uint32_t slot,
    const NativeTu15VelocityWords &velocity, NativeTu15PhysicsFrameFp &fp, unsigned depth,
    const NativeTu15IslandActivationServices &activation, const NativeTu15RetainedStoreObserver &observer,
    NativeTu15IslandActivationStats *activity) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    using namespace joint_constraint_numeric;
    const auto body = read(memory, body_cell(memory, component, slot));
    const auto base = read(memory, body + 0x40u, 1)
                          ? read(memory, 0x474d04u)
                          : read(memory, body + 0x3cu) + 0x40u;
    const std::array<std::uint32_t, 3> current{read(memory, base), read(memory, base + 4u), read(memory, base + 8u)};
    std::array<std::uint32_t, 3> sum{};
    {
        Environment phase(fp, depth);
        for (unsigned i = 0; i < sum.size(); ++i) {
            const auto delta = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(velocity(i))));
            const auto old = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(current[i])));
            sum[i] = scalar_bits(add_scalar(delta, old));
        }
    }
    native_tu15_set_phantom_body_velocity(memory, component, slot, NativeTu15PhantomVelocityKind::linear,
                                         [&](unsigned i) { return sum.at(i); }, fp, activation, observer, activity);
#else
    (void)memory; (void)component; (void)slot; (void)velocity; (void)fp; (void)depth;
    (void)activation; (void)observer; (void)activity;
    throw std::runtime_error("Native phantom velocity requires exact ADDSS backend");
#endif
}

} // namespace halo2
