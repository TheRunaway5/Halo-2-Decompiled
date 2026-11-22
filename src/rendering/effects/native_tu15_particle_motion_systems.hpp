#pragma once
#include "rendering/effects/native_tu15_particle_update_systems.hpp"

namespace halo2::native_effects::particle_motion {
using location_update::ParticleDatum;

enum class Movement { gravity_drag, second, third, fourth, none };

// A movement definition can evaluate three functions (gravity, linear drag
// and angular drag). Its constant channels are evaluated once per update.
// The others depend on the particle and are refreshed before integration.
inline constexpr unsigned channel_count = 3;
inline constexpr std::uint32_t shared_inputs = 0x107f0;
inline constexpr std::uint32_t particle_inputs = 0xf80f;
inline constexpr std::uint32_t reusable_inputs = ~particle_inputs;
inline constexpr std::uint8_t inactive_flags = 0x09;

// Intermediate scalar values also cross provider/observer boundaries. The
// frame publishes them at the same point as the original scalar operation.
enum class Scalar { gravity, linear_drag, angular_drag, unit, step };
enum class Access {
    step,
    linear_drag,
    angular_drag,
    one,
    gravity_scale,
    gravity_channel,
    vertical_velocity,
    spin_rate
};
template <class Frame> void integrate(Frame &frame) {
    const float dt = frame.load(Access::step);
    const float linear_rate = frame.load(Access::linear_drag);
    frame.publish_scalar(Scalar::gravity, 0.0f);
    const float angular_rate = frame.load(Access::angular_drag);
    const float one = frame.load(Access::one);
    float linear = sse::multiply(linear_rate, dt);
    frame.publish_scalar(Scalar::linear_drag, linear);
    const bool negative_linear = frame.greater(0.0f, linear);
    float angular = sse::multiply(angular_rate, dt);
    frame.publish_scalar(Scalar::angular_drag, angular);
    if (negative_linear)
        linear = 0.0f;
    else if (frame.greater(linear, one))
        linear = one;
    frame.publish_scalar(Scalar::linear_drag, linear);
    if (frame.greater(0.0f, angular))
        angular = 0.0f;
    else if (frame.greater(angular, one))
        angular = one;
    frame.publish_scalar(Scalar::angular_drag, angular);

    const float scale = frame.load(Access::gravity_scale);
    const float gravity = frame.load(Access::gravity_channel);
    const float impulse = sse::multiply(sse::multiply(scale, gravity), dt);
    frame.publish_scalar(Scalar::gravity, impulse);
    const float vz = frame.load(Access::vertical_velocity);
    const float accelerated = sse::subtract(vz, impulse);
    frame.publish_scalar(Scalar::step, accelerated);
    frame.store_accelerated(accelerated);

    const float retained = sse::subtract(one, linear);
    frame.publish_scalar(Scalar::gravity, retained);
    for (unsigned axis = 0; axis < 3; ++axis) {
        const float velocity = frame.load_velocity(axis);
        const float damped = sse::multiply(velocity, retained);
        frame.publish_scalar(Scalar::linear_drag, damped);
        frame.store_velocity(axis, damped);
    }
    const float retained_spin = sse::subtract(one, angular);
    frame.publish_scalar(Scalar::unit, retained_spin);
    const float spin = frame.load(Access::spin_rate);
    const float damped_spin = sse::multiply(retained_spin, spin);
    frame.publish_scalar(Scalar::unit, damped_spin);
    frame.store_spin(damped_spin);
}

template <class Frame> void update(Frame &frame) {
    const auto movement = frame.select_movement();
    if (movement != Movement::gravity_drag) {
        frame.dispatch_other(movement);
        return;
    }
    frame.begin_gravity_drag();
    frame.sample_shared_inputs();
    do {
        if (frame.constant_channels_empty())
            break;
        if (frame.constant_channel_selected())
            frame.sample_constant_channel();
    } while (frame.next_constant_channel(channel_count));

    if (frame.has_particles()) {
        do {
            frame.select_particle();
            if (frame.inactive(inactive_flags) || frame.past_lifetime())
                continue;
            frame.refresh_particle_inputs();
            do {
                if (!frame.channel_is_constant())
                    frame.sample_particle_channel();
            } while (frame.next_particle_channel(channel_count));
            integrate(frame);
        } while (frame.has_next_particle());
    }
    frame.finish_gravity_drag();
}
} // namespace halo2::native_effects::particle_motion
