#include "halo2/collision.hpp"
#include "halo2/frontend.hpp"
#include "halo2/math.hpp"
#include "halo2/scripts.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace halo2;

namespace {
void observer_precision_regression() {
  // Independent unchanged retail16FE90 outputs, including both derivatives.
  // Provenance: tests/fixtures/menu_observer_precision_original.provenance.json.
  struct Case {
    unsigned tick;
    std::uint32_t seconds, dt;
    std::array<std::uint32_t, 21> before;
    std::array<std::uint32_t, 9> target;
    std::array<std::uint32_t, 21> after;
  };
  constexpr std::array<Case, 3> original{{
    {845, 0x4183bbbcu, 0x3d088889u,
     {0xc22db92cu, 0x41eb3d18u, 0x43783fc8u,
      0xbde6c0e9u, 0x3f7bbea8u, 0xbe11cc81u,
      0x3de4223du, 0x3e1e9bcau, 0x3f7b4bf9u,
      0x3ff916f8u, 0xbfcd2e00u, 0xc07fed0eu,
      0x3dbc0608u, 0x3cb48ba0u, 0x3d7455a0u,
      0x3e85715cu, 0xbe176b80u, 0xbd14d600u,
      0xbc243402u, 0xbb0051c0u, 0xb91e2800u},
     {0xc247404bu, 0x42b3112au, 0x438c49ccu,
      0x3f3609c2u, 0x3c505868u, 0xbf33f721u,
      0x3f32d244u, 0x3dcf447eu, 0x3f35597fu},
     {0xc22dfb75u, 0x41ebaa68u, 0x437861e9u,
      0xbde289d5u, 0x3f7bb043u, 0xbe14f879u,
      0x3de349a9u, 0x3e2176d7u, 0x3f7b31f0u,
      0x3ff7f9a4u, 0xbfcc9880u, 0xc07fd854u,
      0x3dbcb29cu, 0x3cb50ac0u, 0x3d7462c0u,
      0x3e85f3b2u, 0xbe007bc0u, 0xbd1f4560u,
      0xbc1f80c2u, 0xbaddec80u, 0xba113e00u}},
    {846, 0x41837778u, 0x3d088889u,
     {0xc22dfb75u, 0x41ebaa68u, 0x437861e9u,
      0xbde289d5u, 0x3f7bb043u, 0xbe14f879u,
      0x3de349a9u, 0x3e2176d7u, 0x3f7b31f0u,
      0x3ff7f9a4u, 0xbfcc9880u, 0xc07fd854u,
      0x3dbcb29cu, 0x3cb50ac0u, 0x3d7462c0u,
      0x3e85f3b2u, 0xbe007bc0u, 0xbd1f4560u,
      0xbc1f80c2u, 0xbaddec80u, 0xba113e00u},
     {0xc247404bu, 0x42b3112au, 0x438c49ccu,
      0x3f3609c2u, 0x3c505868u, 0xbf33f721u,
      0x3f32d244u, 0x3dcf447eu, 0x3f35597fu},
     {0xc22e3d70u, 0x41ec1758u, 0x43788406u,
      0xbdde4f26u, 0x3f7ba0ebu, 0xbe182653u,
      0x3de2742bu, 0x3e245413u, 0x3f7b173au,
      0x3ff6db48u, 0xbfcc1c00u, 0xc07fc242u,
      0x3dbd5a52u, 0x3cb57800u, 0x3d747d00u,
      0x3e86737cu, 0xbdd3a800u, 0xbd29ab40u,
      0xbc1ad5e8u, 0xbabbb400u, 0xba79a400u}},
    {847, 0x41833334u, 0x3d088889u,
     {0xc22e3d70u, 0x41ec1758u, 0x43788406u,
      0xbdde4f26u, 0x3f7ba0ebu, 0xbe182653u,
      0x3de2742bu, 0x3e245413u, 0x3f7b173au,
      0x3ff6db48u, 0xbfcc1c00u, 0xc07fc242u,
      0x3dbd5a52u, 0x3cb57800u, 0x3d747d00u,
      0x3e86737cu, 0xbdd3a800u, 0xbd29ab40u,
      0xbc1ad5e8u, 0xbabbb400u, 0xba79a400u},
     {0xc247404bu, 0x42b3112au, 0x438c49ccu,
      0x3f3609c2u, 0x3c505868u, 0xbf33f721u,
      0x3f32d244u, 0x3dcf447eu, 0x3f35597fu},
     {0xc22e7f1du, 0x41ec8428u, 0x4378a61fu,
      0xbdda1342u, 0x3f7b9094u, 0xbe1b5644u,
      0x3de1a14fu, 0x3e2733d3u, 0x3f7afbd3u,
      0x3ff5bbf8u, 0xbfcbb740u, 0xc07faae0u,
      0x3dbdfd18u, 0x3cb5d320u, 0x3d74a520u,
      0x3e86f0cau, 0xbda6e480u, 0xbd3406c0u,
      0xbc163372u, 0xba99f9c0u, 0xbab06000u}},
  }};
  for (const auto &sample : original) {
    auto motion = std::bit_cast<CameraMotion>(sample.before);
    const auto target = std::bit_cast<std::array<float, 9>>(sample.target);
    motion.step({target[0], target[1], target[2]},
                {target[3], target[4], target[5]},
                {target[6], target[7], target[8]},
                std::bit_cast<float>(sample.seconds),
                std::bit_cast<float>(sample.dt));
    const auto actual = std::bit_cast<std::array<std::uint32_t, 21>>(motion);
    for (unsigned word = 0; word < actual.size(); ++word)
      if (actual[word] != sample.after[word])
        throw FormatError("original observer precision differs at tick " +
                          std::to_string(sample.tick) + " word " +
                          std::to_string(word));
  }
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc < 2)
      throw FormatError("menu camera test requires the original maps directory");
    observer_precision_regression();
    CameraMotion capped;
    const auto target_basis = authored_basis({1.570796327f, 0, 0});
    capped.step({1000, 0, 0}, target_basis.forward, target_basis.up,
                .05f, 1.f / 30);
    // 1701F0 completes the current group after a cap, then subsequent linked
    // groups see the canceled duration. Stale rotation acceleration must not
    // leak into a following camera_set.
    if (capped.acceleration[3] != 0 || capped.acceleration[4] != 0 ||
        capped.acceleration[5] != 0)
      throw FormatError("observer cap retains canceled orientation acceleration");
    Cache cache(std::filesystem::path(argv[1]) / "mainmenu.map");
    const auto level = cache.load_content(0);
    Presentation presentation(level);
    Camera fallback;
    SceneObjects scene(level, 0);
    ScriptVm vm(level.scripts, [&](const auto &name, const auto &args) {
      if (name == "camera_control")
        presentation.control(args.at(0).truth(), fallback);
      else if (name == "camera_set")
        presentation.camera_point(std::int16_t(args.at(0).bits),
                                  int(args.at(1).number()));
      else if (name != "print" && name != "cinematic_start" &&
               !name.starts_with("cinematic_lighting_") &&
               name != "cinematic_lightmap_shadow_enable" &&
               name != "object_set_function_variable" &&
               name != "object_uses_cinematic_lighting" &&
               !name.starts_with("rasterizer_bloom_override"))
        throw FormatError("unexpected menu service: " + name);
      return ScriptValue{};
    }, [&](int name, unsigned) { return scene.resolve(name); });
    const auto placement = std::find_if(level.objects.begin(), level.objects.end(),
        [&](const auto &o) {
          const auto tag = std::find_if(cache.tags().begin(), cache.tags().end(),
              [&](const auto &t) { return t.datum == o.definition; });
          return tag != cache.tags().end() &&
                 tag->name == "scenarios\\objects\\covenant\\military\\capital_ship\\capital_ship";
        });
    if (placement == level.objects.end())
      throw FormatError("original Covenant carrier placement missing");
    const auto model = cache.load_model(placement->definition, true, placement->variant);
    auto vertices = pose_scenery_model(cache, model);
    const auto basis = authored_basis(placement->rotation);
    for (auto &v : vertices)
      v.position = placement->position + basis.rotate(v.position * placement->scale);
    std::vector<CollisionTriangle> triangles;
    for (std::size_t i = 0; i < vertices.size(); i += 3)
      triangles.push_back({vertices[i].position, vertices[i+1].position,
                           vertices[i+2].position});
    const CollisionWorld ship(std::move(triangles));
    std::ofstream trace;
    if (argc > 2) {
      trace.open(argv[2]);
      trace << std::setprecision(9);
      trace << "tick,x,y,z,forward_x,forward_y,forward_z,carrier_crossing\n";
    }
    vm.start_level();
    vm.tick(0);
    auto previous = presentation.camera(fallback).position;
    if (presentation.camera(fallback).field_of_view != 70 ||
        !presentation.camera(fallback).horizontal_field_of_view)
      throw FormatError("menu camera has the wrong retail field of view");
    // Original 16C2F0/16C840/16F570/16FE90/170FD0 instruction trace,
    // retail executable SHA256 03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d.
    const std::array<std::pair<unsigned, Vec3>, 8> reference{{
      {339, {-5.147011757f, -22.54858398f, 223.0232697f}},
      {590, {-21.17002869f, 7.087688446f, 228.3628235f}},
      {840, {-43.16904449f, 29.18959045f, 247.7156067f}},
      {1840, {41.35720825f, 106.5515747f, 242.6726074f}},
      {2090, {33.70905304f, 68.32554626f, 240.6701660f}},
      {2490, {-13.20393753f, -4.907053947f, 226.4817352f}},
      {2690, {-16.61230278f, -24.96101570f, 223.5674438f}},
      {2990, {-2.538602829f, -35.51549149f, 219.5925751f}}}};
    float maximum_reference_error = 0;
    unsigned crossings = 0;
    float quarter = 0;
    unsigned moving_intervals = 0, repeated_render_samples = 0;
    const auto start = level.camera_points.at(6).position;
    const auto delta = level.camera_points.at(7).position - start;
    for (unsigned tick = 1; tick <= 2990; ++tick) {
      vm.tick(tick);
      presentation.tick();
      const auto camera = presentation.camera(fallback);
      for (const auto &[at, position] : reference)
        if (tick == at)
          maximum_reference_error = std::max(maximum_reference_error,
                                             length(camera.position - position));
      const auto segment = camera.position - previous;
      const auto distance = length(segment);
      const auto hit = distance > 1e-6f ?
          ship.raycast(previous, segment / distance, distance) : std::optional<RayHit>{};
      const bool crossing = bool(hit);
      if (hit && trace)
        std::cout << "Crossing tick " << tick << " material "
                  << model.mesh.materials.at(vertices.at(hit->triangle * 3).material).name << '\n';
      crossings += crossing;
      if (tick == 214)
        quarter = dot(camera.position - start, delta) / dot(delta, delta);
      if (tick > 100 && presentation.camera_time() > 1 && length(segment) > .002f) {
        ++moving_intervals;
        auto rendered = camera.position;
        for (float fraction : {.25f, .5f, .75f}) {
          const auto next = presentation.camera(fallback, fraction).position;
          repeated_render_samples += length(next - rendered) < .000001f;
          rendered = next;
        }
        // Presentation sampling must leave the authoritative 30 Hz state alone.
        if (length(presentation.camera(fallback).position - camera.position) != 0)
          throw FormatError("render sampling advances the simulation camera");
      }
      if (trace)
        trace << tick << ',' << camera.position.x << ',' << camera.position.y << ','
              << camera.position.z << ',' << camera.forward.x << ','
              << camera.forward.y << ',' << camera.forward.z << ',' << crossing << '\n';
      previous = camera.position;
    }
    if (!vm.diagnostics().empty())
      throw FormatError("authored menu camera script faulted");
    std::cout << "Authored flythrough quarter progress=" << quarter
              << "; Covenant carrier surface crossings=" << crossings << '\n';
    std::cout << "Moving intervals=" << moving_intervals
              << "; repeated 120 Hz render samples=" << repeated_render_samples << '\n';
    std::cout << "Original trace anchor maximum error=" << maximum_reference_error << '\n';
    // Retail observer's zero-derivative quintic: 6*t^5-15*t^4+10*t^3.
    if (std::abs(quarter - .103515625f) > .002f)
      throw FormatError("menu camera uses the wrong transition curve");
    if (argc > 3) {
      std::ifstream original(argv[3]);
      std::string line;
      std::getline(original, line);
      Vec3 last{};
      bool have_last = false;
      unsigned original_crossings = 0;
      while (std::getline(original, line)) {
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream row(line);
        unsigned tick;
        Vec3 p;
        row >> tick >> p.x >> p.y >> p.z;
        const auto segment = p - last;
        const auto distance = length(segment);
        if (have_last && distance > 1e-6f) {
          const auto hit = ship.raycast(last, segment / distance, distance);
          if (hit) {
            ++original_crossings;
            std::cout << "Original crossing tick " << tick << " material "
                      << model.mesh.materials.at(vertices.at(hit->triangle * 3).material).name << '\n';
          }
        }
        last = p;
        have_last = true;
      }
      std::cout << "Original trace carrier crossings=" << original_crossings << '\n';
    }
    if (crossings)
      throw FormatError("authored menu camera crosses the Covenant carrier mesh");
    if (!moving_intervals || repeated_render_samples)
      throw FormatError("menu travel repeats 30 Hz poses at 120 Hz");
    if (maximum_reference_error > .002f)
      throw FormatError("menu script/director timing differs from original trace");
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
