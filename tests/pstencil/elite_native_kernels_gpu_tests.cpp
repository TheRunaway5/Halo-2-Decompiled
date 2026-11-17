#include "../gpu_compute.hpp"
#include "../scalar_combiners.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>

using namespace halo2;
namespace {
struct Fixture {
    std::array<std::uint32_t, 60> program;
    std::array<ShaderVector, 4> samples;
    ShaderVector facing, grazing;
};
static_assert(sizeof(Fixture) == 336);
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}
int main(int argc, char** argv) try {
    require(argc == 2, "Supply native numerical kernel manifest");
    nlohmann::json modules;
    std::ifstream(argv[1]) >> modules;
    require(modules.size() == 64, "Incomplete native shiny selector domain");
    std::mt19937 random(0x2003e117);
    std::uniform_real_distribution<float> unorm(0, 1);
    float maximum{};
    unsigned cases{};
    for (const auto& module : modules) {
        std::vector<Fixture> fixtures;
        std::vector<ShaderVector> expected;
        for (unsigned sample = 0; sample < 256; ++sample) {
            Fixture fixture{module["controls"].get<std::array<std::uint32_t, 60>>()};
            for (unsigned i = 0; i < 60; ++i)
                if ((i >= 10 && i < 26) || i == 43 || i == 44) fixture.program[i] = random();
            for (auto& vector : fixture.samples)
                for (auto& value : vector) value = unorm(random);
            for (auto* vector : {&fixture.facing, &fixture.grazing})
                for (auto& value : *vector) value = unorm(random);
            if (sample < 4) fixture.samples[sample] = {0, .5f, 1, float(sample) / 3};
            expected.push_back(halo2_test::scalar_combiners(fixture.program, fixture.samples,
                                                          fixture.facing, fixture.grazing));
            fixtures.push_back(fixture);
        }
        const auto path = module["path"].get<std::string>();
        const auto actual = halo2_test::Compute<Fixture, 1>(path.c_str()).run(fixtures);
        require(actual.size() == expected.size(), "Incomplete native GPU output");
        for (unsigned i = 0; i < actual.size(); ++i)
            for (unsigned channel = 0; channel < 4; ++channel) {
                const auto error = std::abs(actual[i][channel] - expected[i][channel]);
                maximum = std::max(maximum, error);
                require(std::isfinite(actual[i][channel]) && error < 2e-5f,
                        "Native shiny GPU arithmetic differs from independent original combiner evaluation");
            }
        cases += fixtures.size();
    }
    std::cout << cases << " native shiny GPU/scalar fixtures across 64 complete identities; max error "
              << maximum << '\n';
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
