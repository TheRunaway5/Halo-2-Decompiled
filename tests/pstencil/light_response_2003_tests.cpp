#include "../gpu_compute.hpp"
#include "../scalar_combiners.hpp"
#include "halo2/cache.hpp"
#include "halo2/pstencil/light_response_2003.hpp"
#include <cfenv>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>

using namespace halo2;
using Json = nlohmann::json;
namespace {
struct Fixture {
    std::array<std::uint32_t, 60> utility, dependent;
    std::array<ShaderVector, 4> samples;
    ShaderVector mapped_texture, gel, diffuse_material, specular_material;
};
static_assert(sizeof(Fixture) == 608);
void require(bool value, const char *why) {
    if (!value)
        throw FormatError(why);
}
PixelProgram utility(const Json &in) {
    return pstencil::light_utility_2003(in["context_7438a8"], in["context_u176"], in["context_u178"],
                                        in["global_48f346"], in["builtin"] == 10);
}
PixelProgram dependent(const Json &in, const std::array<std::uint32_t, 60> &words) {
    return pstencil::dependent_light_2003(in["context_7438a8"], in["context_u0dc"], in["global_7439dc"],
                                          words[11], words[13]);
}
} // namespace
int main(int argc, char **argv) try {
    require(argc == 3 || argc == 4,
            "Supply original constructor reference, GPU compute shader and optional RGB oracle");
    require(std::fegetround() == FE_TONEAREST,
            "Intermediate fixture requires explicit nearest-even host rounding");
    Json original;
    std::ifstream(argv[1]) >> original;
    require(original["original_sha256"] == "5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18",
            "Unrecognized original fixture image");
    std::vector<const Json *> utilities, dependents;
    for (const auto &row : original["fixtures"]) {
        const auto words = row["words"].template get<std::array<std::uint32_t, 60>>();
        const auto &in = row["inputs"];
        const bool final = in["builtin"] == 7;
        const auto program = final ? dependent(in, words) : utility(in);
        require(program.words == words, "Native constructor table differs from unchanged original output");
        (final ? dependents : utilities).push_back(&row);
    }
    require(utilities.size() == 512 && dependents.size() == 48,
            "Original complete constructor coverage missing");
    const auto packed_diffuse = pstencil::pack_light_rgb_2003(
        original["declared_colors"]["007439bc"].template get<std::array<float, 3>>());
    const auto packed_specular = pstencil::pack_light_rgb_2003(
        original["declared_colors"]["007439cc"].template get<std::array<float, 3>>());
    for (const auto *row : dependents)
        require((*row)["words"][11] == packed_diffuse && (*row)["words"][13] == packed_specular,
                "Native RGB pack differs from complete original constructor");
    unsigned rgb_cases{};
    if (argc == 4) {
        Json rgb;
        std::ifstream(argv[3]) >> rgb;
        require(rgb["original_sha256"] == original["original_sha256"] && rgb["fpcw"] == "023f" &&
                    rgb["color_packer_entry"] == "0002f380",
                "RGB oracle provenance differs");
        for (const auto &row : rgb["color_fixtures"]) {
            require(pstencil::pack_light_rgb_2003(row["rgb"].template get<std::array<float, 3>>()) ==
                        row["packed"],
                    "Native light RGB differs from unchanged original FIST rounding");
            ++rgb_cases;
        }
        require(rgb_cases >= 1000, "Original RGB boundary corpus missing");
    }
    std::mt19937 random(0x20036);
    std::uniform_real_distribution<float> unorm(0, 1);
    std::vector<Fixture> fixtures;
    std::vector<ShaderVector> expected;
    for (unsigned row = 0; row < original["fixtures"].size(); ++row)
        for (unsigned sample = 0; sample < 8; ++sample) {
            const auto &selected = original["fixtures"][row];
            const bool final = selected["inputs"]["builtin"] == 7;
            const auto &u = final ? *utilities[(row * 7u + sample) % utilities.size()] : selected;
            const auto &d = final ? selected : *dependents[(row * 3u + sample) % dependents.size()];
            const auto dependent_words = d["words"].template get<std::array<std::uint32_t, 60>>();
            Fixture fixture{utility(u["inputs"]).words, dependent(d["inputs"], dependent_words).words};
            for (auto &vector : fixture.samples)
                for (float &v : vector)
                    v = unorm(random);
            for (auto *vector : {&fixture.mapped_texture, &fixture.gel, &fixture.diffuse_material,
                                 &fixture.specular_material})
                for (float &v : *vector)
                    v = unorm(random);
            if (sample < 2)
                for (auto &vector : fixture.samples)
                    vector.fill(float(sample));
            if (sample == 2)
                fixture.samples[0] = fixture.samples[1] = fixture.samples[2] = {.5f, .5f, 1, 1};
            const ShaderVector color_default{0, 0, 0, 1};
            const auto raw =
                halo2_test::scalar_combiners(fixture.utility, fixture.samples, color_default, color_default);
            expected.push_back(raw);
            ShaderVector stored{};
            for (unsigned c = 0; c < 4; ++c)
                stored[c] = std::nearbyint(raw[c] * 255.f) / 255.f;
            const std::array<ShaderVector, 4> dependent_samples{fixture.mapped_texture, stored,
                                                                ShaderVector{}, fixture.gel};
            expected.push_back(halo2_test::scalar_combiners(
                fixture.dependent, dependent_samples, fixture.diffuse_material, fixture.specular_material));
            fixtures.push_back(fixture);
        }
    const auto actual = halo2_test::Compute<Fixture, 2>{argv[2]}.run(fixtures);
    require(actual.size() == expected.size(), "Incomplete GPU light-response output");
    float maximum{};
    for (unsigned i = 0; i < actual.size(); ++i)
        for (unsigned c = 0; c < 4; ++c) {
            const float error = std::abs(actual[i][c] - expected[i][c]);
            maximum = std::max(maximum, error);
            if (!std::isfinite(actual[i][c]) || error > 2e-5f) {
                std::cerr << "Fixture " << i / 2 << " phase " << i % 2 << " channel " << c << ": "
                          << actual[i][c] << " != " << expected[i][c] << '\n';
                throw FormatError("GPU recovered light response differs from independent scalar evaluation");
            }
        }
    unsigned rejected{};
    for (auto call : {0u, 1u, 2u, 3u})
        try {
            if (call == 0)
                pstencil::light_utility_2003(4, 0, 0, 0, false);
            if (call == 1)
                pstencil::light_utility_2003(0, 4, 0, 0, false);
            if (call == 2)
                pstencil::dependent_light_2003(0, 4, 0, 0, 0);
            if (call == 3)
                pstencil::dependent_light_2003(0, 0, 3, 0, 0);
        } catch (const FormatError &) {
            ++rejected;
        }
    require(rejected == 4, "Unknown light-response selectors were accepted");
    std::cout << "560 complete original program matches; " << rgb_cases << " original RGB cases; "
              << fixtures.size() << " two-stage GPU fixtures (" << actual.size()
              << " stage results), maximum channel error " << maximum << "\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
