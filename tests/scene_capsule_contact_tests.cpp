#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/posed_collision.hpp"
#include "halo2/simulation.hpp"
#include <iostream>

using namespace halo2;

int main(int argc, char **argv) try {
  if (argc != 3) throw FormatError("Supply original Headlong cache and TU1.1 XBE");
  if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))
    return 77;
  const auto original = OriginalData::load_xbe(argv[2]);
  Cache cache(argv[1], {}, original);
  const auto content = cache.load_content(0, true);
  Simulation simulation(content, 0, cache.spawns().front(), {}, {}, {},
                        SourceSceneGameContext{2, 2});
  const auto &world = simulation.collision();

  // Diagnostic placement and walking speed captured from the ordinary-input
  // Headlong match. The floor and capsule dimensions come from owned tags.
  const Vec3 start{18.6632f, -7.67134f, 7.50009f};
  const Vec3 origin = start + Vec3{0, 0, .02f};
  const Vec3 delta{0, 0, -.04f};
  const auto terrain = cache.load_raw_terrain_collision(0);
  std::array<std::uint32_t, 8> intact;
  intact.fill(0xffffffffu);
  const auto source = raw_collision(terrain.bsps.at(0), *original,
                                    origin, delta, 9, intact, 256);
  const CollisionWorld bsp(content.collision);
  const auto floor = bsp.raycast(origin, {0, 0, -1}, .04f);
  if (!source.hit || !floor || floor->source.record != 0 ||
      floor->source.surface != source.hit->surface || floor->normal.z < .99f ||
      std::abs(floor->distance - source.hit->fraction * .04f) > .00001f)
    throw FormatError("Headlong floor differs from the original terrain query");

  Body body;
  body.position = start;
  body.radius = content.player.radius;
  body.height = content.player.height;
  body.maximum_slope_cosine = content.player.maximum_slope_cosine;
  for (unsigned tick = 0; tick < 60; ++tick) {
    body.velocity.x = 1.464f;
    body.velocity.y = 0;
    body.velocity.z -= retail_gravity * tick_seconds;
    world.move(body, tick_seconds);
    if (body.position.z < floor->position.z - .001f)
      throw FormatError("Scene contacts starved the authored floor response");
  }
  if (!body.grounded)
    throw FormatError("Headlong corner lost its authored floor support");
  std::cout << "Headlong: original floor surface " << source.hit->surface
            << ", 60 scene capsule steps retain support\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
