#include "halo2/effect_light.hpp"
#include "halo2/vertex_program.hpp"
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
        result.push_back(std::byte(std::stoul(std::string(text.substr(i, 2)), nullptr, 16)));
    return result;
}
std::vector<std::byte> read_binary(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    require(bool(file), "Current authored vertex program missing");
    std::vector<std::byte> result(std::size_t(file.tellg()));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(result.data()), std::streamsize(result.size()));
    return result;
}
Vec3 vec3(const Json &row) { return {row[0].get<float>(), row[1].get<float>(), row[2].get<float>()}; }
void equal(float actual, float expected, const char *message) {
    if (actual != expected) {
        std::cerr << message << ": " << actual << " != " << expected << '\n';
        throw FormatError(message);
    }
}
} // namespace
int main(int argc, char **argv) try {
    require(argc == 4, "Supply original light-input audit, current directory and original sphere constants");
    const auto reference = read_json(argv[1]),
               current = read_json(std::filesystem::path(argv[2]) / "inputs.json"),
               sphere = read_json(argv[3]);
    require(reference["attenuation"]["same_pixels"] && reference["attenuation"]["compatible_metadata"],
            "Actual current attenuation differs from original");
    require(reference["selector"]["fixtures"].size() == 12, "Original vertex selection coverage missing");
    const auto &programs = reference["programs"];
    const auto original =
        std::find_if(programs.begin(), programs.end(), [](const auto &row) { return row["index"] == 0x36; });
    require(original != programs.end(), "Actual original tangent sphere utility program absent");
    const auto old_vertex = decode_vertex_program(unhex((*original)["bytes"].template get<std::string>()));
    std::mt19937 random(0x2003);
    std::uniform_real_distribution<float> value(-20, 20);
    unsigned constants{}, vertices{}, preserved{}, unmapped{};
    for (const auto &fixture : sphere["fixtures"]) {
        EffectLightUniformInput input;
        input.position = vec3(fixture["position"]);
        input.forward = vec3(fixture["forward"]);
        input.up = vec3(fixture["up"]);
        input.left = vec3(fixture["left"]);
        input.radius = fixture["radius"];
        input.diffuse = vec3(fixture["diffuse"]);
        input.specular = vec3(fixture["specular"]);
        input.brightness = fixture["brightness"];
        for (unsigned i = 0; i < 8; ++i)
            input.previous[i] = fixture["previous"][i].template get<ShaderVector>();
        const auto bank = source_effect_light_uniforms(input);
        require(bank.color_shift == fixture["color_shift"],
                "Current light color-shift differs from original");
        for (unsigned lane = 0; lane < 32; ++lane) {
            const float old = fixture["constants"][lane / 4][lane % 4];
            const auto written = fixture["written_lanes"].template get<std::vector<unsigned>>();
            const bool was_written = std::find(written.begin(), written.end(), lane) != written.end();
            if (!was_written) {
                equal(old, input.previous[lane / 4][lane % 4], "Original caller lane was not preserved");
                ++preserved;
            }
            // Original C20.xy preserve caller state; current static sphere
            // clears C36.xy. Original C21.w writes its global zero, while the
            // current routine carries C37.w. These lanes are separate owners;
            // none is read by original utility36 or the final RGB packer.
            if (lane == 20 || lane == 21 || lane == 27) {
                ++unmapped;
                continue;
            }
            equal(bank.constants[lane / 4][lane % 4], old, "Mapped sphere constant differs");
            ++constants;
        }
        for (const auto &pass : current["passes"]) {
            const auto code =
                read_binary(std::filesystem::path(argv[2]) /
                            ("current-vs-" + std::to_string(pass["pass"].template get<unsigned>()) + ".bin"));
            const auto now_vertex = decode_vertex_program(code);
            VertexProgramState before{}, now{};
            for (unsigned i = 0; i < 4; ++i) {
                before.constants[i][i] = now.constants[i][i] = 1;
                before.constants[58][i] = now.constants[10][i] = 1;
            }
            const ShaderVector eye{value(random), value(random), value(random), 1};
            before.constants[4] = now.constants[7] = eye;
            before.constants[5][3] = .5f;
            now.constants[172] = {1, 1, 0, 0};
            const auto &authored = pass["program_constants"][1];
            for (unsigned i = 0; i < 16; ++i)
                now.constants[i + 18] = authored[i].template get<ShaderVector>();
            for (unsigned i = 0; i < 9; ++i)
                now.constants[i + 31] = bank.constants[i];
            for (unsigned i = 0; i < 8; ++i)
                before.constants[i + 15] = fixture["constants"][i].template get<ShaderVector>();
            const ShaderVector position{value(random), value(random), value(random), 1};
            before.attributes[0] = now.attributes[0] = position;
            now.attributes[1] = {value(random), value(random), 0, 1};
            // The current material retains its actual C18/C20 bump UV
            // remaps. Original36 receives a ready UV in V3; adapt that input
            // explicitly instead of claiming both asset layouts are equal.
            ShaderVector mapped_uv{};
            for (unsigned c = 0; c < 2; ++c) {
                const float base = now.attributes[1][c] * now.constants[18][c] + now.constants[18][c + 2];
                mapped_uv[c] = base * now.constants[20][c] + now.constants[20][c + 2];
            }
            mapped_uv[3] = 1;
            before.attributes[3] = mapped_uv;
            for (unsigned axis = 0; axis < 3; ++axis) {
                ShaderVector tangent{};
                tangent[axis] = (vertices & 1) ? -1.f : 1.f;
                tangent[3] = 1;
                before.attributes[axis + 4] = now.attributes[axis + 2] = tangent;
            }
            execute_vertex_program(old_vertex, before);
            execute_vertex_program(now_vertex, now);
            for (unsigned c = 0; c < 3; ++c) {
                equal(before.outputs[10][c], now.outputs[10][c],
                      "Original/current light tangent direction differs");
                equal(before.outputs[11][c], now.outputs[12][c],
                      "Original/current eye tangent direction differs");
                const float distance = bank.constants[0][c] - position[c];
                const float falloff = distance * (-bank.constants[4][3]) + .5f;
                equal(before.outputs[12][c], falloff, "Original sphere falloff coordinate differs");
            }
            for (unsigned c = 0; c < 2; ++c) {
                equal(now.outputs[11][c], mapped_uv[c], "Current authored bump UV remap differs");
                equal(before.outputs[9][c], mapped_uv[c], "Adapted original bump UV differs");
            }
            ++vertices;
        }
    }
    std::cout << constants << " mapped constants, " << vertices << " actual current/original vertex cases, "
              << preserved << " original carried lanes; " << unmapped << " explicitly separate lane cases\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
