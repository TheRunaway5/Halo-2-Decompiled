#include "halo2/pstencil/light_response_2003.hpp"
#include "halo2/pstencil/reference_light_materials.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}
int main(int argc, char** argv) try {
    require(argc == 3, "Supply original utility constructions and Elite resident bindings");
    if (!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))
        return 77;
    nlohmann::json utilities, materials;
    std::ifstream(argv[1]) >> utilities;
    std::ifstream(argv[2]) >> materials;
    require(utilities["original_sha256"] == "5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18" &&
            utilities["instruction_patches"].empty() && utilities["replaced_services"].empty(),
            "Original constructor provenance differs");
    unsigned programs{}, bindings{};
    for (const auto& row : utilities["fixtures"]) {
        if (row["builtin_response"] != 9) continue;
        const auto program = pstencil::light_utility_2003(row["space"], row["normal_selector"],
            row["specular_selector"], row["color_shift"], 9u);
        require(program.words == row["program_words"].get<std::array<std::uint32_t, 60>>(),
                "Elite shiny builtin9 was collapsed into an ordinary/dull utility kernel");
        ++programs;
    }
    require(programs == 192, "Incomplete original shiny constructor coverage");
    for (const auto& row : materials["materials"]) {
        const auto* native = pstencil::reference_light_material(row["shader"].get<std::string>());
        require(native, "Elite material is missing its exact original direct-light binding");
        const auto& original = row["parameters"];
        require(native->utility_builtin == 9 && native->utility_builtin == original["utility_builtin"] &&
                native->v10 == original["v10"].get<ShaderVector>() &&
                native->v11 == original["v11"].get<ShaderVector>(),
                "Elite facing/grazing colors or utility selection differ from original resident bytes");
        require(!pstencil::reference_light_material(row["shader"].get<std::string>() + "_unproven"),
                "Elite binding guessed a material alias");
        ++bindings;
    }
    require(bindings == 6, "Incomplete original Elite material coverage");
    std::cout << programs << " original shiny constructors; " << bindings
              << " exact Elite direct-light bindings and alias rejection controls\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
