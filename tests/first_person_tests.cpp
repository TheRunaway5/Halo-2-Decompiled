#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/scene.hpp"
#include "halo2/transform.hpp"
#include <fstream>
#include <iostream>
#include <limits>

namespace {
using namespace halo2;
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
template <class F> void rejects(F f) {
  bool rejected = false;
  try {
    f();
  } catch (const FormatError &) {
    rejected = true;
  }
  check(rejected, "malformed first person request accepted");
}
RenderModel triangle(float bind_y) {
  RenderModel model;
  ModelBone bone;
  bone.name = 1;
  bone.bind.translation = {0, bind_y, 0};
  bone.inverse = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
  model.bones.push_back(bone);
  model.mesh.vertices.resize(3);
  for (auto &vertex : model.mesh.vertices)
    vertex.position = {1, 0, 0};
  model.skin.resize(3);
  return model;
}
AnimationClip fixture(bool camera) {
  AnimationClip clip;
  clip.frame_count = 1;
  clip.nodes = {1};
  clip.parents = {-1};
  clip.components = {0};
  clip.channels.resize(1);
  clip.frames.resize(1);
  if (camera) {
    clip.nodes.push_back(0x0e0000e0);
    clip.parents.push_back(0);
    clip.components.push_back(3);
    clip.channels.resize(2);
    clip.frames.resize(2);
    clip.channels[1][0] = {{0}, {{0, 0, .707106781f, .707106781f}}};
    clip.channels[1][1] = {{0}, {{.2f, 0, 0, 0}}};
  }
  return clip;
}
void synthetic() {
  const auto weapon = triangle(1), arms = triangle(2);
  auto bounded=triangle(2);
  bounded.bones[0].inverse[10]=-2;
  const auto bounded_node=pose_model_node(bounded,0);
  const auto bounded_vertices=pose_model(bounded);
  check(length(bounded_node.position-Vec3{0,2,0})<1e-6f &&
        length(bounded_vertices[0].position-Vec3{1,0,0})<1e-6f,
        "object bounds must use posed node before the inverse skin bind");
  rejects([&]{pose_model_node(bounded,1);});
  const auto clip = fixture(false);
  auto pose =
      pose_first_person_models(weapon, arms, clip, 0, {.02f, .03f, .04f});
  check(length(pose.weapon[0].position - Vec3{1.02f, 2.03f, .04f}) < 1e-6f,
        "shared graph must use arms bind channels after weapon channels");
  check(length(pose.weapon[0].position - pose.arms[0].position) < 1e-6f,
        "arms and weapon must share the same sampled nodes");
  auto camera_clip = fixture(true);
  pose = pose_first_person_models(weapon, arms, camera_clip, 0,
                                  {.02f, .03f, .04f});
  check(length(pose.weapon[0].position - Vec3{.03f, -.82f, .04f}) < 1e-6f,
        "camera inverse must precede the authored offset and graph nodes");
  auto invalid = camera_clip;
  invalid.parents[0] = 1;
  rejects([&] { pose_first_person_models(weapon, arms, invalid, 0, {}); });
  invalid = camera_clip;
  invalid.loop_frame = invalid.frame_count;
  rejects([&] { pose_first_person_models(weapon, arms, invalid, 0, {}); });
  rejects([&] { pose_first_person_models(weapon, arms, camera_clip, -1, {}); });
  rejects([&] {
    pose_first_person_models(weapon, arms, camera_clip, 0,
                             {std::numeric_limits<float>::quiet_NaN(), 0, 0});
  });
  Camera view;
  view.field_of_view = 70;
  view.horizontal_field_of_view = true;
  for (const float aspect : {4.f / 3.f, 16.f / 9.f, 1.6f}) {
    const float vertical = camera_vertical_field_of_view(view, aspect);
    check(std::abs(std::tan(vertical * .5f) * aspect -
                   std::tan(70.f * .017453292f * .5f)) < 1e-6f,
          "player horizontal FOV must remain authored across viewport sizes");
  }
  const auto base = camera_transform({10, 20, 30}, {0, 1, 0}, {1, 0, 0});
  check(length(base.basis[1] - Vec3{0, 0, 1}) < 1e-6f,
        "camera roll must rotate the first person basis");
}
void retail(const std::filesystem::path &directory, std::ostream *trace) {
  Cache cache(directory / "01b_spacestation.map");
  const auto content = cache.load_content(0);
  check(std::abs(content.player.camera_field_of_view - 1.22173047f) < 1e-7f,
        "retail Chief camera tag must supply 70 degree horizontal FOV");
  const auto arms = cache.load_model(content.first_person_hands, false);
  unsigned clips = 0, frames = 0;
  if (trace)
    *trace << "weapon\taction\tframe\tmesh\tvertices\tmin_x\tmin_y\tmin_z\tmax_"
              "x\tmax_y\tmax_z\n";
  for (const std::string weapon_name : {"battle_rifle", "smg"}) {
    const auto weapon = std::find_if(
        content.weapons.begin(), content.weapons.end(), [&](const auto &entry) {
          return entry.second.name.ends_with("\\" + weapon_name);
        });
    check(weapon != content.weapons.end(),
          "missing retail first person weapon");
    const auto model =
        cache.load_model(weapon->second.first_person_model, false);
    for (const std::string action :
         {"idle", "fire_1", "reload_full", "reload_empty"}) {
      const auto clip = cache.load_action(weapon->second.first_person_animation,
                                          "any", "any", "any", action);
      check(std::find(clip.nodes.begin(), clip.nodes.end(), 0x0e0000e0u) !=
                clip.nodes.end(),
            "retail first person clip must retain camera_control node");
      for (unsigned frame = 0; frame < clip.frame_count; ++frame) {
        const auto pose =
            pose_first_person_models(model, arms, clip, float(frame) / 30,
                                     weapon->second.first_person_offset);
        for (const auto &[name, vertices] : {std::pair{"weapon", &pose.weapon},
                                             std::pair{"arms", &pose.arms}}) {
          check(!vertices->empty(), "empty retail first person mesh");
          Vec3 low{1e6f, 1e6f, 1e6f}, high{-1e6f, -1e6f, -1e6f};
          for (const auto &vertex : *vertices) {
            const auto p = vertex.position;
            check(std::isfinite(p.x) && std::isfinite(p.y) &&
                      std::isfinite(p.z),
                  "nonfinite retail first person position");
            low = minimum(low, p);
            high = maximum(high, p);
          }
          if (trace)
            *trace << weapon_name << '\t' << action << '\t' << frame << '\t'
                   << name << '\t' << vertices->size() << '\t' << low.x << '\t'
                   << low.y << '\t' << low.z << '\t' << high.x << '\t' << high.y
                   << '\t' << high.z << '\n';
        }
        ++frames;
      }
      ++clips;
    }
  }
  check(clips == 8 && frames == 407,
        "retail first person action coverage changed");
  std::cout << "retail first person: " << clips << " clips / " << frames
            << " frames, both meshes checked\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    synthetic();
    std::ofstream trace;
    if (argc > 2) {
      trace.open(argv[2]);
      if (!trace)
        throw std::runtime_error("cannot write first person pose trace");
      trace.precision(9);
    }
    if (argc > 1)
      retail(argv[1], trace ? &trace : nullptr);
    std::cout << "first person transform/pose tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
