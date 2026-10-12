#include "halo2/camera_motion.hpp"
#include "halo2/transform.hpp"
#include <bit>

namespace halo2 {
namespace {
float normalize(Vec3 &v) {
  const double x = v.x, y = v.y, z = v.z;
  const double magnitude = std::sqrt((x * x + y * y) + z * z);
  if (std::abs(magnitude) < double(.0001f))
    return 0;
  const float n = float(magnitude);
  v = v * (1.f / n);
  return n;
}
Vec3 rotation_between(Vec3 forward, Vec3 up, Vec3 target_forward,
                      Vec3 target_up) {
  // 170BB0 -> 170C00 -> 1424F0 -> 141F60 -> 11D790. The retail
  // register ABI passes the current basis first, then the target basis.
  const auto relative = compose_transform(
      camera_transform({}, target_forward, target_up),
      inverse_transform(camera_transform({}, forward, up)));
  const auto m = std::bit_cast<std::array<float, 9>>(relative.basis);
  std::array<float, 4> q;
  const float trace = m[4] + m[0] + m[8];
  if (trace > 0) {
    // Retail 141F87 adds one in x87 precision, before the 141F95 float store.
    q[3] = float(std::sqrt(double(trace) + 1.0) * .5);
    const float inverse = .25f / q[3];
    q[0] = (m[5] - m[7]) * inverse;
    q[1] = (m[6] - m[2]) * inverse;
    q[2] = (m[1] - m[3]) * inverse;
  } else {
    unsigned i = m[4] > m[0] ? 1 : 0;
    if (m[8] > m[i * 4])
      i = 2;
    const auto j = (i + 1) % 3, k = (i + 2) % 3;
    q[i] = float(std::sqrt(double(m[i * 4] - m[j * 4] - m[k * 4] + 1.f)) * .5);
    const float inverse = .25f / q[i];
    q[3] = (m[j * 3 + k] - m[k * 3 + j]) * inverse;
    q[j] = (m[i * 3 + j] + m[j * 3 + i]) * inverse;
    q[k] = (m[i * 3 + k] + m[k * 3 + i]) * inverse;
  }
  if (q[3] < 0)
    for (auto &v : q)
      v = 0.f - v;
  Vec3 axis{q[0], q[1], q[2]};
  const float magnitude = normalize(axis);
  const float angle = float(std::atan2(double(magnitude), double(q[3]))) * 2.f;
  return axis * angle;
}
void rotate(Vec3 angle_axis, Vec3 &a, Vec3 &b) {
  const float angle = normalize(angle_axis);
  if (angle == 0)
    return;
  const float s = float(std::sin(double(angle))),
              c = float(std::cos(double(angle))), remainder = 1.f - c;
  const auto &axis = angle_axis;
  const float ka = (a.x * axis.x + axis.z * a.z + axis.y * a.y) * remainder;
  const float pa1 = axis.x * a.z - a.x * axis.z,
              pa2 = a.x * axis.y - axis.x * a.y;
  a.x = a.x * c + ka * axis.x - (axis.z * a.y - axis.y * a.z) * s;
  a.y = ka * axis.y + c * a.y - pa1 * s;
  a.z = ka * axis.z + c * a.z - pa2 * s;
  const float kb = (b.z * axis.z + b.y * axis.y + b.x * axis.x) * remainder;
  const float pb1 = b.z * axis.x - b.x * axis.z,
              pb2 = b.x * axis.y - b.y * axis.x;
  b.x = b.x * c + kb * axis.x - (b.y * axis.z - b.z * axis.y) * s;
  b.y = b.y * c + kb * axis.y - pb1 * s;
  b.z = b.z * c + kb * axis.z - pb2 * s;
}
} // namespace
void CameraMotion::step(Vec3 target, Vec3 target_forward, Vec3 target_up,
                        float seconds, float dt) {
  if (!std::isfinite(seconds) || seconds < 0 || !std::isfinite(dt) || dt <= 0)
    throw FormatError("invalid observer duration");
  const auto dp = target - position;
  const auto dr = rotation_between(forward, up, target_forward, target_up);
  const std::array<float, 6> delta{dp.x, dp.y, dp.z, dr.x, dr.y, dr.z};
  std::array<float, 6> fifth{}, fourth{}, third{}, displacement{};
  float t = seconds - dt;
  if (t > 0) {
    const float inverse = 1.f / seconds, inverse2 = inverse * inverse,
                inverse3 = inverse2 * inverse, inverse4 = inverse3 * inverse,
                inverse5 = inverse4 * inverse;
    for (unsigned i = 0; i < 6; ++i) {
      fifth[i] = acceleration[i] * inverse3 * .5f -
                 (delta[i] * inverse5 * 6.f + velocity[i] * inverse4 * 3.f);
      fourth[i] = delta[i] * inverse4 * 15.f + velocity[i] * inverse3 * 7.f -
                  acceleration[i] * inverse2;
      third[i] = acceleration[i] * inverse * .5f -
                 (delta[i] * inverse3 * 10.f + velocity[i] * inverse2 * 4.f);
    }
    const float t2 = t * t, t3 = t2 * t;
    // Acceleration caps cancel linked channels together, as in 1701F0.
    for (unsigned group = 0; group < 2; ++group) {
      const bool canceled = seconds == 0;
      for (unsigned i = group * 3; i < group * 3 + 3; ++i) {
        acceleration[i] = canceled ? 0.f
            : fifth[i] * t3 * 20.f + fourth[i] * t2 * 12.f +
                  third[i] * t * 6.f;
        const float cap = group == 0 ? 1500.f : 100000.f;
        if (std::abs(acceleration[i]) > cap)
          seconds = 0;
      }
    }
    t = seconds - dt;
  } else
    acceleration.fill(0);
  if (t > 0) {
    const float t2 = t * t, t3 = t2 * t, t4 = t3 * t, t5 = t4 * t;
    for (unsigned i = 0; i < 6; ++i) {
      velocity[i] = fifth[i] * t4 * 5.f + fourth[i] * t3 * 4.f +
                    third[i] * t2 * 3.f;
      displacement[i] = fifth[i] * t5 + fourth[i] * t4 + third[i] * t3 + delta[i];
    }
    position = position + Vec3{displacement[0], displacement[1], displacement[2]};
    rotate({displacement[3], displacement[4], displacement[5]}, forward, up);
  } else {
    position = target;
    forward = target_forward;
    up = target_up;
    velocity.fill(0);
  }
  if (!(std::abs(dot(forward, forward) - 1.f) < .001f &&
        std::abs(dot(up, up) - 1.f) < .001f &&
        std::abs(up.z * forward.z + forward.x * up.x + up.y * forward.y) < .001f)) {
    const auto left = cross(up, forward);
    up = cross(forward, left);
    normalize(forward);
    normalize(up);
  }
}
} // namespace halo2

extern "C" void halo2_observer_step(const float *input, const float *target,
                                     float seconds, float dt, float *output) {
  std::array<float, 21> words;
  std::copy_n(input, words.size(), words.begin());
  auto state = std::bit_cast<halo2::CameraMotion>(words);
  state.step({target[0], target[1], target[2]}, {target[3], target[4], target[5]},
             {target[6], target[7], target[8]}, seconds, dt);
  const auto result = std::bit_cast<std::array<float, 21>>(state);
  std::copy(result.begin(), result.end(), output);
}
extern "C" void halo2_authored_basis(const float *euler, float *output) {
  const auto basis = halo2::authored_basis({euler[0], euler[1], euler[2]});
  const auto result = std::bit_cast<std::array<float, 9>>(basis);
  std::copy(result.begin(), result.end(), output);
}
