#pragma once

#include "halo2/native_weapon_effect_birth_color_observer.hpp"
#include <cstdlib>

namespace halo2::native_particle_birth_observer_detail {
inline bool qualified(const SourcePhysicsContactRecordObserver &observer) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_BIRTH_OBSERVER");
        return value && *value;
    }();
    return !reference && NativeWeaponEffectBirthColorCollector::qualified(observer);
}
} // namespace halo2::native_particle_birth_observer_detail
