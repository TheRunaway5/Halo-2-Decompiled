#pragma once

#include "halo2/native_weapon_effect_birth_color_observer.hpp"
#include "halo2/source_physics_store_observers_tu15.hpp"
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"

namespace frame_misc_harness {
struct BirthObserverFixture {
    SourceWeaponEffectPresentation presentation;
    const SourcePhysicsContactRecordObserver empty;
    NativeWeaponEffectBirthColorCollector collector{presentation, empty};
    NativeWeaponEffectBirthColorCollector::Borrow borrow = collector.borrow();
};

template<class Native>
void check_birth_observer_refusals(Case c, Native native) {
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [](std::uint32_t) {};
    std::uint32_t position{};
    auto &cursor = services.instruction_cursor;
    cursor.position = &position;
    cursor.service_sites = halo2::native_census_detail::service_sites;
    cursor.callback_type = &services.instruction.target_type();
    cursor.executor = std::this_thread::get_id();
    cursor.literal_service_sites = true;

    const auto before = c.memory.clone();
    const auto refuse = [&](const SourcePhysicsContactRecordObserver &observe) {
        WorldState state{c.memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
        state.load(c.abi);
        if (native(state) || position || !c.memory.same(before))
            throw std::runtime_error("Unauthenticated birth observer admitted or touched live state");
    };

    SourceWeaponEffectPresentation presentation;
    SourcePhysicsContactRecordObserver outer;
    const NativeWeaponEffectBirthColorCollector collector{presentation, outer};
    refuse(collector.observer());
    {
        auto borrow = collector.borrow();
        const SourcePhysicsContactRecordObserver copy = collector.observer();
        const SourcePhysicsContactRecordObserver wrapper = [&](const auto &store) { collector.observer()(store); };
        const SourcePhysicsContactRecordObserver other = [](const auto &) {};
        const auto tee = source_physics_store_observers_tu15(collector.observer(), other);
        refuse(copy);
        refuse(wrapper);
        refuse(tee);
        outer = other;
        refuse(collector.observer());
        outer = {};
        bool trace = true;
        cursor.first_trace = &trace;
        refuse(collector.observer());
        cursor.first_trace = nullptr;
    }
    refuse(collector.observer());
}
} // namespace frame_misc_harness
