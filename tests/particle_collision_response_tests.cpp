#include "halo2/particle_collision.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  try {
    if (argc != 3)
      throw halo2::FormatError("Supply original collider fixtures");
    std::ifstream input(argv[1], std::ios::binary);
    const auto read = [&]<class T>(T &v) {
      if (!input.read(reinterpret_cast<char *>(&v), sizeof(v)))
        throw halo2::FormatError("Short original collider fixture");
    };
    std::array<char, 4> magic;
    std::uint32_t count;
    read(magic);
    read(count);
    if (magic != std::array<char, 4>{'P', 'C', 'R', '1'} || count != 2048)
      throw halo2::FormatError("Invalid original collider fixture header");
    for (unsigned i = 0; i < count; ++i) {
      halo2::ParticleSimulationState particle, expected;
      halo2::ParticleCollisionHit hit;
      float friction, restitution;
      read(particle);
      read(hit.position);
      read(hit.normal);
      read(friction);
      read(restitution);
      read(expected);
      halo2::source_particle_collision_response(particle, hit, friction,
                                                restitution);
      if (std::memcmp(&particle, &expected, sizeof(particle)))
        throw halo2::FormatError(
            "Original collider response mismatch at fixture " +
            std::to_string(i));
    }
    if (input.peek() != std::char_traits<char>::eof())
      throw halo2::FormatError("Extra original collider fixtures");
    std::ifstream gains(argv[2], std::ios::binary);
    gains.read(magic.data(), magic.size());
    std::uint32_t gain_count{};
    gains.read(reinterpret_cast<char *>(&gain_count), sizeof(gain_count));
    if (magic != std::array<char, 4>{'P', 'C', 'G', '1'} || gain_count != 2048)
      throw halo2::FormatError(
          "Invalid original collision-child gain fixtures");
    for (unsigned n = 0; n < gain_count; ++n) {
      halo2::Vec3 velocity;
      std::uint32_t expected{};
      gains.read(reinterpret_cast<char *>(&velocity), sizeof(velocity));
      gains.read(reinterpret_cast<char *>(&expected), sizeof(expected));
      if (!gains || std::bit_cast<std::uint32_t>(
                        halo2::source_particle_collision_child_gain(
                            velocity)) != expected)
        throw halo2::FormatError("Original collision-child gain differs at " +
                                 std::to_string(n));
    }
    if (gains.peek() != std::char_traits<char>::eof())
      throw halo2::FormatError("Extra original gain fixtures");
    std::cout << count
              << " original collider responses and 2048 child gains byte exact "
                 "in both updates\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
