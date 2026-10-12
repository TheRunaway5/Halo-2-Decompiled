#include "halo2/weapon_error.hpp"
#include "halo2/original_data.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
std::uint32_t read_bits(std::istream &input) {
  std::uint32_t value;
  if (!(input >> std::hex >> value))
    throw std::runtime_error("truncated original weapon fixture");
  return value;
}
float read_float(std::istream &input) {
  return std::bit_cast<float>(read_bits(input));
}
void match(float actual, std::uint32_t expected, const std::string &label) {
  if (std::bit_cast<std::uint32_t>(actual) != expected) {
    std::ostringstream message;
    message << label << ": actual " << std::hex
            << std::bit_cast<std::uint32_t>(actual) << ", original " << expected;
    throw std::runtime_error(message.str());
  }
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 3)
      throw std::runtime_error("supply original weapon error fixtures and your local executable");
    if (!std::filesystem::exists(argv[2]))
      return 77;
    const auto original = halo2::OriginalData::load_xbe(argv[2]);
    std::ifstream file(argv[1]);
    if (!file)
      throw std::runtime_error("cannot open original weapon error fixtures");
    std::string line;
    unsigned count = 0;
    while (std::getline(file, line)) {
      if (line.empty() || line.front() == '#')
        continue;
      std::istringstream input(line);
      std::string name;
      input >> name;
      const float error = read_float(input), recoil = read_float(input),
                  dt = read_float(input), blend = read_float(input);
      std::array<float, 12> d;
      for (auto &value : d)
        value = read_float(input);
      const halo2::WeaponErrorDefinition definition{
          d[0], d[1], d[2], d[3], d[4], d[5],
          d[6], d[7], d[8], d[9], d[10], d[11]};
      for (const bool firing : {true, false}) {
        halo2::WeaponErrorState state{error, recoil};
        halo2::weapon_error_step(state, definition, dt, blend, firing);
        const auto label = name + (firing ? " firing" : " idle");
        match(state.error, read_bits(input), label + " error");
        match(state.recoil, read_bits(input), label + " recoil");
      }
      const auto angles = halo2::weapon_error_angles(definition, error, blend);
      match(angles.minimum, read_bits(input), name + " minimum angle");
      match(angles.maximum, read_bits(input), name + " maximum angle");
      const halo2::Vec3 forward{read_float(input), read_float(input), read_float(input)};
      const std::array expected_direction{read_bits(input), read_bits(input), read_bits(input)};
      auto seed = read_bits(input);
      const auto expected_seed = read_bits(input);
      std::string extra;
      if (input >> extra)
        throw std::runtime_error("unexpected fields in original weapon fixture");
      const auto result = halo2::weapon_random_deviation(forward, angles, seed,
                                                       original->weapon_directions);
      match(result.x, expected_direction[0], name + " direction x");
      match(result.y, expected_direction[1], name + " direction y");
      match(result.z, expected_direction[2], name + " direction z");
      if (seed != expected_seed)
        throw std::runtime_error(name + " original seed progression mismatch");
      ++count;
    }
    if (count != 70)
      throw std::runtime_error("expected all 70 original weapon error fixtures");
    std::cout << count << " original weapon error fixtures match exactly\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
