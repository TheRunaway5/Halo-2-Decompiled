#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <source_location>

namespace {
void check(bool value,
           std::source_location at = std::source_location::current()) {
  if (!value)
    throw std::runtime_error("shader animation assertion at " +
                             std::to_string(at.line()));
}
template <class T> void write(std::ofstream &file, const T &v) {
  file.write(reinterpret_cast<const char *>(&v), sizeof(v));
}
} // namespace
int main(int argc, char **argv) {
  try {
    // Original 1B0A0 wraps before flooring. These endpoints include the
    // one-image Lockout gravity lift and the weapon's ten number images.
    check(halo2::bitmap_frame_index(1.f,1)==0);
    check(halo2::bitmap_frame_index(1.999f,1)==0);
    check(halo2::bitmap_frame_index(10.25f,10)==0);
    check(halo2::bitmap_frame_index(19.99f,10)==9);
    check(halo2::bitmap_frame_index(-.00001f,10)==0);
    using namespace halo2;
    if (argc < 2)
      throw FormatError("shader animation tests require extracted maps");
    Cache cache(std::filesystem::path(argv[1]) / "mainmenu.map");
    auto carrier = cache.load_model(0xe1d50060, true);
    const auto functions = cache.load_object_functions(0xe1d50060);
    const auto tables = cache.shader_function_tables();
    check(functions.size() == 1 && functions[0].output == 0x070008e1 &&
          functions[0].scale == 0x110008e2);
    auto function = [&](float seconds, std::uint32_t id) {
      return evaluate_object_function(
                 functions, *tables, id, seconds,
                 [](std::uint32_t name) -> std::optional<float> {
                   if (name == 0x110008e2)
                     return 1.f;
                   return {};
                 })
          .value;
    };
    check(function(0, 0x110008e2) == 1 && function(0, 0) == 1);
    check(function(0, 0x070008e1) > 0 &&
          function(5, 0x070008e1) != function(0, 0x070008e1));
    std::ofstream output;
    if (argc > 2) {
      output.open(argv[2], std::ios::binary);
      check(bool(output));
    }
    std::uint32_t cases = 0;
    for (auto &m : carrier.mesh.materials) {
      const bool plasma = m.name.ends_with("\\antigrav_effects") ||
                          m.name.ends_with("\\antigrav_effects2");
      if (!plasma && !m.name.ends_with("\\capital_ship_c"))
        continue;
      check(m.forward_pass && (!plasma || (m.vertex_program.size() == 676 &&
                                           m.forward_blend == 3)));
      const auto &pass = m.passes.at(m.forward_pass_index);
      check(!m.animation.overlays.empty() && pass.implementation == 0);
      const auto tag =
          std::find_if(cache.tags().begin(), cache.tags().end(),
                       [&](const auto &t) { return t.name == m.name; });
      check(tag != cache.tags().end());
      for (unsigned tick = 0; tick < 3000; tick += 13) {
        const double seconds = double(tick) / 30;
        const float object_seconds = float(tick) * (1.f / 30.f);
        const auto color = function(object_seconds, 0x070008e1);
        check(animate_forward_material(m, seconds, [&](std::uint32_t id) {
          return function(object_seconds, id);
        }));
        if (output) {
          write(output, tag->datum);
          write(output, pass.template_pass);
          write(output, pass.implementation);
          write(output, seconds);
          write(output, object_seconds);
          write(output, color);
          write(output, pass.program.words);
          write(output, pass.constants);
          write(output, m.forward_program.words);
          write(output, m.program_constants);
        }
        ++cases;
      }
      if (m.name.ends_with("\\antigrav_effects")) {
        check(m.forward_program.words[54] == 0x8420 &&
              m.forward_program.words[53] == 0x11107);
        check(m.program_constants[1][2][0] == 1.5f &&
              m.program_constants[1][3][1] == 1.5f);
        check((m.forward_program.words[13] & 0xffffff) != 0 &&
              (m.forward_program.words[16] & 0xffffff) != 0);
      } else if (m.name.ends_with("\\antigrav_effects2")) {
        check((m.forward_program.words[10] >> 24) == 255);
        check(m.program_constants[1][2][0] == 2 &&
              m.program_constants[1][3][1] == 6);
      }
    }
    check(cases == 693);
    // World owners share authored materials but require independent animated
    // constants. An on/off/on sequence must not mutate the resident pass or
    // leave the next owner's sword blade combiner disabled.
    Cache lockout(std::filesystem::path(argv[1])/"lockout.map");
    std::vector<Material> sword_materials;
    for(const auto datum:{0xe2de0169u,0xe2e80173u})sword_materials.push_back(lockout.load_material(datum));unsigned blades{};
    for(const auto &m:sword_materials) {
      if(!m.name.ends_with("\\energy_blade_top") && !m.name.ends_with("\\energy_blade_bottom"))continue;
      const auto &pass=m.passes.at(m.forward_pass_index);
      const auto evaluate=[&](float on) {
        auto program=pass.program;auto constants=pass.constants;
        check(animate_forward_constants(m.animation,pass,m.shader_switches,2,
            [&](std::uint32_t id){return id==0x09000568u?on:0.f;},program,constants));
        return std::pair{program.words,constants};
      };
      const auto on=evaluate(1),off=evaluate(0),again=evaluate(1);
      check(on.first[43]!=0 && off.first[43]==0 && on==again);++blades;
    }
    check(blades==2);
    // Control is object scoped, and disabling it removes both the color
    // function's scale and the secondary layer's opacity.
    check(!evaluate_object_function(
               functions, *tables, 0x070008e1, 5,
               [](std::uint32_t name) -> std::optional<float> {
                 if (name == 0x110008e2)
                   return 0.f;
                 return {};
               })
               .value);
    if (output) {
      output.close();
      check(bool(output));
    }
    std::cout << "Retail carrier shader passes: " << cases
              << " animated samples passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
