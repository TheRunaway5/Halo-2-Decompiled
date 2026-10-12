#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_device.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/transform.hpp"
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <set>
#include <string>
#include <tuple>

using namespace halo2;
namespace {
using Owner = std::pair<std::string, std::uint32_t>;
using Origin = std::pair<unsigned, unsigned>;
std::array<std::uint32_t, 6> words(const Vertex &v) {
  return {std::bit_cast<std::uint32_t>(v.incident.x),
          std::bit_cast<std::uint32_t>(v.incident.y),
          std::bit_cast<std::uint32_t>(v.incident.z),
          std::bit_cast<std::uint32_t>(v.color.x),
          std::bit_cast<std::uint32_t>(v.color.y),
          std::bit_cast<std::uint32_t>(v.color.z)};
}
}
int main(int argc, char **argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--train-bounds") {
      const std::filesystem::path root = HALO2_SOURCE_DIR;
      Cache cache(root / "assets/map-pack/maps/triplicate.map");
      const auto content = cache.load_content(0, true);
      std::cout << std::setprecision(10);
      for (const auto &object : content.objects) {
        if (object.type != 7 || (object.placement != 0 && object.placement != 1)) continue;
        const auto definition = cache.load_source_machine_definition(object.definition);
        const auto placement = cache.load_source_machine_placement(object.placement);
        const auto selected = cache.load_initial_model_selection(object.definition, object.variant);
        const auto model = cache.load_model(object.definition, false, object.variant, selected, 5);
        const auto basis = authored_basis(object.rotation);
        const auto frame = source_object_transform(object.position, basis.forward, basis.up, object.scale, false);
        const auto local = source_machine_local_pose(model, definition, placement.initial_position, placement.initial_power);
        const auto nodes = pose_model_locals_nodes_world(model, frame, local);
        const auto vertices = pose_model_locals_world(model, frame, local);
        Vec3 minimum{1e30f, 1e30f, 1e30f}, maximum{-1e30f, -1e30f, -1e30f};
        for (const auto &vertex : vertices) {
          minimum = {std::min(minimum.x, vertex.position.x), std::min(minimum.y, vertex.position.y), std::min(minimum.z, vertex.position.z)};
          maximum = {std::max(maximum.x, vertex.position.x), std::max(maximum.y, vertex.position.y), std::max(maximum.z, vertex.position.z)};
        }
        const auto sphere = source_shadow_sphere(cache.load_object_shadow_definition(object.definition), nodes.front(), object.scale);
        const auto print = [](Vec3 v) { std::cout << v.x << ' ' << v.y << ' ' << v.z; };
        std::cout << "placement " << object.placement << " source_initial_position " << placement.initial_position << " power " << placement.initial_power << " node0 ";
        print(nodes.front().position); std::cout << " sphere "; print(sphere.center); std::cout << ' ' << sphere.radius << " mesh_min ";
        print(minimum); std::cout << " mesh_max "; print(maximum); std::cout << '\n';
      }
      return 0;
    }
    if (argc != 2) throw std::runtime_error("expected model stream fixture");
    const std::filesystem::path root = HALO2_SOURCE_DIR;
    if (!std::filesystem::exists(root / "assets/disc/maps/zanzibar.map")) return 77;
    std::ifstream input(argv[1]);
    if (!input) throw std::runtime_error("missing model stream fixture");
    std::map<Owner, std::map<Origin, std::array<std::uint32_t, 6>>> expected;
    std::map<Owner, unsigned> declared_sections;
    std::map<Owner, std::uint32_t> model_definitions;
    std::vector<Owner> controls;
    unsigned source_vertices = 0, source_sections = 0, nonzero_radiance_components = 0;
    std::string line;
    while (std::getline(input, line)) {
      if (line.empty() || line.front() == '#') continue;
      std::istringstream row(line);
      char kind;
      std::string map;
      std::uint32_t datum;
      row >> kind >> map >> std::hex >> datum >> std::dec;
      const Owner owner{map, datum};
      if (kind == 'H') {
        std::uint32_t model;
        row >> std::hex >> model;
        if (!model_definitions.emplace(owner, model).second) throw std::runtime_error("duplicate source model definition");
      } else if (kind == 'M') {
        unsigned count;
        row >> count;
        if (!declared_sections.emplace(owner, count).second)
          throw std::runtime_error("duplicate fixture model");
      } else if (kind == 'S') {
        unsigned section, count;
        row >> section >> count;
        ++source_sections;
      } else if (kind == 'V') {
        unsigned section, vertex;
        std::array<std::uint32_t, 6> value;
        row >> section >> vertex >> std::hex;
        for (auto &v : value) row >> v;
        for (unsigned component = 3; component < 6; ++component) {
          const auto scalar = std::bit_cast<float>(value[component]);
          if (scalar > 0 && scalar < 1) ++nonzero_radiance_components;
        }
        if (!expected[owner].emplace(Origin{section, vertex}, value).second)
          throw std::runtime_error("duplicate fixture vertex");
        ++source_vertices;
      } else if (kind == 'C') controls.push_back(owner);
      else throw std::runtime_error("unexpected fixture row");
      if (!row) throw std::runtime_error("truncated model stream row");
    }
    if (expected.size() != 7 || source_vertices != 23459 || source_sections != 42 ||
        controls.size() != 6 || nonzero_radiance_components != 5841)
      throw std::runtime_error("literal all21 extra stream inventory changed");
    std::map<std::string, std::unique_ptr<Cache>> caches;
    const auto cache_for = [&](const std::string &map) -> Cache & {
      auto [at, inserted] = caches.try_emplace(map);
      if (inserted) {
        auto path = root / "assets/disc/maps" / (map + ".map");
        if (!std::filesystem::exists(path)) path = root / "assets/map-pack/maps" / (map + ".map");
        at->second = std::make_unique<Cache>(path);
      }
      return *at->second;
    };
    std::uint64_t expanded = 0, posed = 0, default_vertices = 0, train_black = 0;
    unsigned covered_vertices = 0, covered_sections = 0;
    for (const auto &[owner, values] : expected) {
      auto model = cache_for(owner.first).load_model(owner.second, false);
      if (model.mesh.vertices.empty() || model.vertex_origins.size() != model.mesh.vertices.size())
        throw std::runtime_error("model lacks authored expanded identities");
      std::set<Origin> covered;
      std::set<unsigned> sections;
      const auto verify = [&](const std::vector<Vertex> &vertices) {
        for (unsigned i = 0; i < vertices.size(); ++i) {
          const auto &origin = model.vertex_origins[i];
          const auto at = values.find({origin.section, origin.vertex});
          if (at == values.end() || words(vertices[i]) != at->second)
            throw std::runtime_error("decoded/posed model extra stream mismatch: " + owner.first + " " + model.name);
          if (owner.first == "triplicate") {
            if (vertices[i].color.x != 0 || vertices[i].color.y != 0 || vertices[i].color.z != 0)
              throw std::runtime_error("Triplicate PRT-lightmap radiance is not authored black");
            ++train_black;
          }
          covered.insert({origin.section, origin.vertex});
          sections.insert(origin.section);
        }
      };
      verify(model.mesh.vertices);
      expanded += model.mesh.vertices.size();
      const Transform root_transform{1.5f, {{{0, 1, 0}, {-1, 0, 0}, {0, 0, 1}}}, {3, -7, 2}};
      const auto vertices = pose_model_world(model, root_transform);
      if (vertices.size() != model.mesh.vertices.size()) throw std::runtime_error("pose changed vertex identities");
      verify(vertices);
      posed += vertices.size();
      const auto definition = model_definitions.at(owner);
      const auto selection = cache_for(owner.first).load_initial_model_selection(definition);
      if (!selection.resolved()) throw std::runtime_error("extra-stream fixture has unresolved default model selection");
      for (unsigned lod = 0; lod < 6; ++lod) {
        model = cache_for(owner.first).load_model(definition, false, 0, selection, lod);
        verify(model.mesh.vertices);
        expanded += model.mesh.vertices.size();
        const auto output = pose_model_world(model, root_transform);
        verify(output);
        posed += output.size();
      }
      if (covered.size() != values.size() || sections.size() != declared_sections.at(owner))
        throw std::runtime_error("six authored LOD collectors did not cover every frozen extra stream");
      covered_vertices += unsigned(covered.size());
      covered_sections += unsigned(sections.size());
    }
    for (const auto &owner : controls) {
      const auto model = cache_for(owner.first).load_model(owner.second, false);
      const std::array<std::uint32_t, 6> defaults{0, 0, 0, 0x3f800000, 0x3f800000, 0x3f800000};
      if (model.mesh.vertices.empty()) throw std::runtime_error("plain PRT control is empty");
      for (const auto &v : model.mesh.vertices) {
        if (words(v) != defaults) throw std::runtime_error("absent extra streams changed plain PRT defaults");
        ++default_vertices;
      }
    }
    if (!train_black || !default_vertices) throw std::runtime_error("missing train/plain PRT controls");
    std::cout << "Literal model streams: 7 models/42 sections/23459 source vertices; "
              << expanded << " expanded and " << posed << " posed extra-stream vertices; "
              << train_black << " black train checks; " << default_vertices << " unchanged plain-PRT vertices; "
              << covered_vertices << " source vertices / " << covered_sections << " sections covered by six authored LODs; "
              << nonzero_radiance_components << " nonzero nonendpoint radiance components\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
