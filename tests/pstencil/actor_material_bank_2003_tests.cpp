#include "halo2/native_fragment_kernel.hpp"
#include "halo2/pstencil/light_response_2003.hpp"
#include "halo2/pstencil/light_response_material.hpp"
#include "halo2/pstencil/material.hpp"
#include "halo2/pstencil/normal_accumulation_2003.hpp"
#include "halo2/pstencil/reference_light_materials.hpp"
#include "halo2/pstencil/specular_mask.hpp"
#include "../scalar_combiners.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>

using namespace halo2;
using Json = nlohmann::json;
namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void word(EffectLightDefinition& definition, unsigned offset, unsigned value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; ++i) definition.bytes[offset + i] = std::byte(value >> (i * 8));
}

bool actor_or_weapon(std::string_view name) {
    return name.starts_with("objects\\characters\\") || name.starts_with("objects\\weapons\\");
}

void verify_coverage(const Material& source, const Material& mask) {
    const auto& original = source.passes.at(mask.forward_pass_index).program.words;
    const auto& projected = mask.forward_program.words;
    std::mt19937 random(2003);
    std::uniform_real_distribution<float> component(0, 1);
    for (unsigned n = 0; n < 128; ++n) {
        std::array<ShaderVector, 4> samples;
        ShaderVector v0, v1;
        for (auto& texture : samples) for (auto& channel : texture) channel = component(random);
        for (auto* color : {&v0, &v1}) for (auto& channel : *color) channel = component(random);
        const auto full = halo2_test::scalar_combiners(original, samples, v0, v1);
        samples[3] = {};
        const auto sliced = halo2_test::scalar_combiners(projected, samples, v0, v1);
        require(full[3] == sliced[3], "Cutout texture pass changed the authored coverage alpha");
    }
}

Json audit(const std::filesystem::path& path, unsigned& recovered, unsigned& adapted) {
    Cache cache(path);
    require(cache.is_mcc(), "Actor lighting integration needs a prepared MCC map");
    Mesh mesh;
    pstencil::MaterialBank bank(cache, mesh);
    EffectLightUniforms uniforms{};
    uniforms.constants[0] = {1, 2, 3, 1};
    uniforms.constants[1] = {1, 2, 3, 0};
    uniforms.constants[2] = {0, -.25f, 0, 0};
    uniforms.constants[3] = {0, 0, -.25f, 0};
    uniforms.constants[4] = {-.25f, 0, 0, -.125f};
    uniforms.constants[6] = {.2f, .4f, .8f, 0};
    uniforms.constants[7] = {.7f, .3f, .1f, 0};

    Json materials = Json::object();
    for (const auto& tag : cache.tags()) {
        if (tag.group != "shad" || !tag.address || !actor_or_weapon(tag.name)) continue;
        auto& row = materials[tag.name];
        const auto* historical = pstencil::reference_light_material(tag.name);
        row["reference"] = historical != nullptr;
        Material authored;
        try {
            authored = cache.load_material(tag.datum);
        } catch (const FormatError& error) {
            row["load_boundary"] = error.what();
            continue;
        }
        row["specular_type"] = authored.shader_switches[4];
        row["template"] = authored.template_name;
        row["resident_passes"] = authored.passes.size();
        if (authored.passes.empty()) continue;
        std::array<unsigned,2> coverage{};
        const unsigned builtin = historical ? historical->utility_builtin
            : authored.shader_switches[4] == 2 ? 10u : authored.shader_switches[4] == 3 ? 9u : 6u;
        for (unsigned lod = 0; lod < authored.lods.size(); ++lod) {
            authored.selected_shader_lod = lod;
            const auto& selected_lod = authored.lods[lod];
            if (lod == 0 && !(selected_lod.available_layers & 2) &&
                (selected_lod.available_layers & 8) && (selected_lod.available_layers & (1u << 13))) {
                const auto range = authored.layers.at(selected_lod.layers.first() + 3);
                for (const auto& pass : authored.passes) {
                    if (pass.implementation != 2 || pass.template_pass < range.first() ||
                        pass.template_pass - range.first() >= range.count()) continue;
                    auto& info = row["combined"];
                    info["pass"] = pass.name;
                    info["vertex"] = pass.vertex_shader;
                    info["words"] = pass.program.words;
                    for (const auto& binding : pass.textures) {
                        std::string bitmap;
                        for (const auto& remap : pass.bitmap_remaps) {
                            if ((remap.destination & 255) != binding.parameter ||
                                remap.source >= authored.bitmaps.size()) continue;
                            const auto datum = authored.bitmaps[remap.source].datum;
                            for (const auto& bitm : cache.tags())
                                if (bitm.datum == datum) { bitmap = bitm.name; break; }
                        }
                        info["textures"].push_back({binding.stage, binding.external, bitmap});
                    }
                    for (const auto& remap : pass.pixel_externs)
                        info["externs"].push_back({remap.destination, remap.source, remap.component});
                }
            }
            for (const unsigned geometry : {0u, 2u}) {
                EffectLightDefinition light;
                word(light, 4, geometry, 2);
                word(light, 0x7c, ~0u, 4);
                word(light, 0xe0, ~0u, 4);
                word(light, 0x8e, geometry ? 1 : 0, 2);
                const auto id = geometry + 1;
                try {
                    bank.preload(authored, id, light);
                    const auto draws = bank.complete(tag.datum, lod, id, uniforms);
                    if (!(authored.lods[lod].available_layers & (1u << 13))) {
                        require(!draws || draws->empty(), "A material without layer 13 gained highlights");
                        continue;
                    }
                    if (!draws || draws->size() < 3) {
                        row["other_lit_cases"] = row.value("other_lit_cases", 0u) + 1;
                        continue;
                    }
                    const auto& utility = draws->at(1);
                    require(draws->at(0).program.words == pstencil::normal_accumulation_2003().words &&
                            !native_fragment_kernel_path("pstencil-normal", draws->at(0).program.words).empty(),
                            "Material normal accumulation has no native fragment kernel");
                    require(mesh.materials.at(utility.material_slot).forward_blend ==
                            pstencil::normal_light_utility_blend_2003, "Unexpected utility pass");
                    require(utility.program.words == pstencil::light_utility_2003(geometry, 0, 0, 0, builtin).words,
                            "Material selected the wrong light utility program");
                    require(!native_fragment_kernel_path("pstencil-normal-utility", utility.program.words).empty(),
                            "Material utility program has no native fragment kernel");
                    require(!native_fragment_kernel_path("pstencil-dependent-light", draws->at(2).program.words).empty(),
                            "Material dependent light has no native fragment kernel");
                    const auto mask = std::ranges::find_if(mesh.materials, [&](const auto& material) {
                        return material.source_shader_datum == tag.datum && pstencil::texture_mask_pass(material) &&
                               (material.forward_vertex == 3 || material.selected_shader_lod == lod);
                    });
                    require(mask != mesh.materials.end(), "Native material lacks a texture target producer");
                    if (mask->forward_vertex == 6) {
                        require(!native_fragment_kernel_path("pstencil-texture", mask->forward_program.words).empty(),
                                "Combined texture producer has no native fragment kernel");
                        row["combined_native"] = true;
                        if (mask->forward_raster.alpha_test && geometry == 0) {
                            verify_coverage(authored, *mask);
                            row["cutout_native"] = true;
                        }
                        if (mask->forward_program.words[34] == 0x29200901u) {
                            auto controls = mask->forward_program.words;
                            controls[10] = 0xff4080c0u;
                            const std::array<ShaderVector, 4> samples{{
                                {1, 1, 1, .7f}, {0, .5f, 1, .4f}, {}, {}
                            }};
                            const auto factor = halo2_test::scalar_combiners(controls, samples, {}, {});
                            require(factor[0] == 1 && std::abs(factor[1] - (.5f + .5f * 128 / 255)) < 1.e-6f &&
                                    std::abs(factor[2] - 192.f / 255) < 1.e-6f,
                                    "Combined texture target changed the authored color mask");
                        } else {
                            const std::array<ShaderVector, 4> samples{{
                                {1, 1, 1, .7f}, {.5f, .5f, .5f, .4f}, {0, 0, 0, .6f}, {}
                            }};
                            const auto factor = halo2_test::scalar_combiners(mask->forward_program.words,
                                samples, {}, {});
                            require(factor[0] == 1 && factor[1] == 1 && factor[2] == 1,
                                    "Plain combined texture target darkens base/detail RGB");
                        }
                    }
                    if (historical) {
                        require(utility.constants[0][10] == historical->v10 &&
                                utility.constants[0][11] == historical->v11,
                                "Material did not use the recovered colors");
                        ++recovered;
                    } else {
                        const auto range = authored.layers.at(authored.lods[lod].layers.first() + 13);
                        const auto pass = std::ranges::find_if(authored.passes, [&](const auto& candidate) {
                            return candidate.implementation == 0 && candidate.template_pass >= range.first() &&
                                   candidate.template_pass - range.first() < range.count();
                        });
                        require(pass != authored.passes.end() &&
                                utility.constants[0][10] == pass->constants[0][10] &&
                                utility.constants[0][11] == pass->constants[0][11],
                                "Later adaptation changed authored facing/grazing colors");
                        ++adapted;
                    }
                    row["native_cases"] = row.value("native_cases", 0u) + 1;
                    ++coverage[geometry != 0];
                } catch (const FormatError& error) {
                    row["binding_boundary"] = error.what();
                }
            }
        }
        if (coverage[0] || coverage[1]) row["native_geometries"] = coverage;
    }
    return materials;
}
}

int main(int argc, char** argv) try {
    require(argc >= 3, "Supply a report path and prepared MCC maps");
    Json maps = Json::object();
    unsigned recovered = 0, adapted = 0;
    for (int i = 2; i < argc; ++i) {
        const std::filesystem::path path(argv[i]);
        maps[path.stem().string()] = audit(path, recovered, adapted);
    }
    std::ofstream report(argv[1]);
    report << Json{{"maps", maps}, {"recovered_cases", recovered}, {"adapted_cases", adapted}}.dump() << '\n';
    std::cout << recovered << " recovered and " << adapted << " adapted native material cases\n";
    require(recovered && adapted, "Missing recovered or later material coverage");
    const auto check = [&](const char* map, const char* name, bool historical, bool combined, bool cutout=false) {
        const auto& row = maps.at(map).at(name);
        if (!row.contains("native_geometries") || row["native_geometries"][0] == 0 ||
            row["native_geometries"][1] == 0 || row["reference"] != historical ||
            row.value("combined_native", false) != combined || row.value("cutout_native", false) != cutout)
            throw std::runtime_error(std::string("Missing sphere/projected native coverage: ") + name);
    };
    check("07a_highcharity", "objects\\characters\\hunter\\shaders\\hunter_torso", true, true);
    check("07a_highcharity", "objects\\characters\\grunt\\shaders\\grunt_arms", true, true);
    check("07a_highcharity", "objects\\characters\\grunt\\shaders\\grunt_head", true, true);
    check("07a_highcharity", "objects\\characters\\jackal\\shaders\\jackal_head", true, true);
    check("07a_highcharity", "objects\\characters\\brute\\shaders\\brute_head", true, true);
    check("07a_highcharity", "objects\\characters\\brute\\shaders\\brute", false, true);
    check("08b_deltacontrol", "objects\\characters\\brute\\shaders\\tartarus_shoulder_armor", false, true);
    check("07a_highcharity", "objects\\characters\\prophet\\shaders\\truth_head", false, true);
    check("07a_highcharity", "objects\\characters\\flood_infection\\shaders\\flood_infection", true, true, true);
    check("06a_sentinelwalls", "objects\\characters\\floodcombat_elite\\shaders\\flood_fronds", true, true, true);
    check("06a_sentinelwalls", "objects\\characters\\sentinel_enforcer\\shaders\\sentinel_enforcer", false, true);
    check("07a_highcharity", "objects\\weapons\\rifle\\plasma_rifle\\shaders\\plasma_rifle_main", true, true);
    check("coagulation", "objects\\characters\\masterchief\\shaders\\masterchief", true, false);
    check("coagulation", "objects\\characters\\masterchief\\shaders\\masterchief_visor", true, false);
    check("coagulation", "objects\\characters\\masterchief\\fp\\shaders\\fp_arms", true, false);
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
