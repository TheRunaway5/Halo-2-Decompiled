#include "halo2/effect_light.hpp"
#include "halo2/pstencil/projected_constants_2003.hpp"
#include "halo2/vertex_program.hpp"
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>

using namespace halo2;
using Json = nlohmann::json;
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw FormatError(message);
}
Json read_json(const std::filesystem::path &path) {
  std::ifstream file(path);
  Json result;
  file >> result;
  return result;
}
std::vector<std::byte> unhex(std::string_view text) {
  require(text.size() % 2 == 0, "Invalid retained program hex");
  std::vector<std::byte> result;
  for (unsigned i = 0; i < text.size(); i += 2)
    result.push_back(
        std::byte(std::stoul(std::string(text.substr(i, 2)), nullptr, 16)));
  return result;
}
std::vector<std::byte> read_binary(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  require(bool(file), "Current authored vertex program missing");
  std::vector<std::byte> result(std::size_t(file.tellg()));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(result.data()),
            std::streamsize(result.size()));
  return result;
}
Vec3 vec3(const Json &row) {
  return {row[0].get<float>(), row[1].get<float>(), row[2].get<float>()};
}
void equal(float actual, float expected, const char *message) {
  if (actual != expected) {
    std::cerr << message << ": " << actual << " != " << expected << '\n';
    throw FormatError(message);
  }
}
} // namespace
int main(int argc, char **argv) try {
  require(argc == 4, "Supply projected oracle, original vertex audit and "
                     "current sphere-input directory");
  const auto reference = read_json(argv[1]), audit = read_json(argv[2]);
  const auto current =
      read_json(std::filesystem::path(argv[3]) / "inputs.json");
  const auto program = [&](unsigned index) {
    for (const auto &p : audit["programs"])
      if (p["index"] == index)
        return decode_vertex_program(unhex(p["bytes"].get<std::string>()));
    throw FormatError("Missing original projected vertex program");
  };
  const auto utility = program(0x2f), dependent = program(0x25);
  std::mt19937 random(0x200325);
  std::uniform_real_distribution<float> value(-20, 20);
  unsigned scalars = 0, vertices = 0;
  for (const auto &row : reference["fixtures"]) {
    const auto shape = row["shape"].get<std::array<std::uint32_t, 31>>();
    const auto frame = row["frame"].get<std::array<float, 16>>();
    const auto prior = row["previous"].get<std::array<ShaderVector, 6>>();
    const auto result = pstencil::projected_constants_2003(
        shape, frame, row["falloff"], row["constant20_w"], prior);
    const auto bits = row["constants_bits"].get<std::array<unsigned, 24>>();
    for (unsigned n = 0; n < 24; ++n) {
      require(std::bit_cast<unsigned>(result[n / 4][n % 4]) == bits[n],
              "Original projected constant bits differ");
      ++scalars;
    }
    for (const auto &pass : current["passes"]) {
      const auto path = std::filesystem::path(argv[3]) /
                        ("current-vs-" +
                         std::to_string(pass["pass"].get<unsigned>()) + ".bin");
      const auto now_vertex = decode_vertex_program(read_binary(path));
      VertexProgramState old{}, now{};
      for (unsigned n = 0; n < 4; ++n)
        old.constants[n][n] = now.constants[n][n] = 1;
      old.attributes[15] = {0, .5f, 1, 42}; // Actual35E81..35E94 defaults.
      old.attributes[9] = {0, 0, 0, 1};     // Original18087F static gel path.
      old.constants[4] =
          now.constants[7] = {value(random), value(random), value(random), 1};
      old.constants[5][3] = .5f;
      for (unsigned n = 0; n < 16; ++n)
        now.constants[n + 18] =
            pass["program_constants"][1][n].get<ShaderVector>();
      for (unsigned n = 0; n < 6; ++n)
        old.constants[n + 15] = now.constants[n + 31] = result[n];
      now.constants[172] = {1, 1, 0, 0};
      const ShaderVector position{value(random), value(random), value(random),
                                  1};
      old.attributes[0] = now.attributes[0] = position;
      now.attributes[1] = {value(random), value(random), 0, 1};
      ShaderVector mapped{0, 0, 0, 1};
      for (unsigned c = 0; c < 2; ++c) {
        const float base = now.attributes[1][c] * now.constants[18][c] +
                           now.constants[18][c + 2];
        mapped[c] = base * now.constants[20][c] + now.constants[20][c + 2];
      }
      old.attributes[3] = mapped;
      for (unsigned axis = 0; axis < 3; ++axis) {
        ShaderVector tangent{};
        tangent[axis] = (vertices & 1) ? -1.f : 1.f;
        tangent[3] = 1;
        old.attributes[axis + 4] = now.attributes[axis + 2] = tangent;
      }
      execute_vertex_program(utility, old);
      execute_vertex_program(now_vertex, now);
      for (unsigned c = 0; c < 3; ++c) {
        equal(old.outputs[10][c], now.outputs[10][c],
              "Projected light tangent input differs");
        equal(old.outputs[11][c], now.outputs[12][c],
              "Projected eye tangent input differs");
      }
      for (unsigned c = 0; c < 2; ++c)
        equal(old.outputs[9][c], now.outputs[11][c],
              "Projected bump UV differs");
      ShaderVector offset{};
      for (unsigned c = 0; c < 3; ++c)
        offset[c] = result[1][c] - position[c];
      const auto dot = [&](const ShaderVector &constant) {
        return (offset[0] * constant[0] + offset[1] * constant[1]) +
               offset[2] * constant[2];
      };
      const float distance = dot(result[4]);
      for (unsigned c = 0; c < 3; ++c)
        equal(old.outputs[12][c], distance + result[5][2],
              "Original2F falloff XYZ differs");
      equal(old.outputs[12][3], distance + result[4][3],
            "Original2F falloff Q differs");
      VertexProgramState final{};
      final.attributes[0] = position;
      final.attributes[9] = {0, 0, 0, 1};
      final.attributes[15] = {0, .5f, 1, 42};
      for (unsigned n = 0; n < 6; ++n)
        final.constants[n + 15] = result[n];
      execute_vertex_program(dependent, final);
      equal(final.outputs[12][0], dot(result[2]) + result[2][3],
            "Original25 gel S differs");
      equal(final.outputs[12][1], dot(result[3]) + result[3][3],
            "Original25 gel T differs");
      equal(final.outputs[12][3], distance * result[0][3] + (1.f - result[0][3]),
            "Original25 gel Q differs");
      ++vertices;
    }
  }
  std::cout << scalars << " exact original constant scalars; " << vertices
            << " actual current/original projected vertex cases\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
