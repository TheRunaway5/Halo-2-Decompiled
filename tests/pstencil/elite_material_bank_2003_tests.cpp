#include "halo2/pstencil/material.hpp"
#include "halo2/pstencil/light_response_2003.hpp"
#include "halo2/pstencil/reference_light_materials.hpp"
#include "halo2/pstencil/light_response_material.hpp"
#include "halo2/pstencil/normal_accumulation_2003.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include <iostream>

using namespace halo2;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void word(EffectLightDefinition& definition, unsigned offset, unsigned value, unsigned count) {
    for (unsigned i = 0; i < count; ++i) definition.bytes[offset+i] = std::byte((value >> (i*8)) & 255);
}
}
int main(int argc, char** argv) try {
    require(argc == 2, "Supply the prepared MCC opening cache");
    Cache cache(argv[1]);
    require(cache.is_mcc(), "Elite integration requires current prepared MCC assets");
    Mesh mesh;
    pstencil::MaterialBank bank(cache, mesh);
    // Declared completed inputs at the material-bank seam, C31..39. Light
    // admission/constant construction have separate original-code tests.
    EffectLightUniforms uniforms{};
    uniforms.constants[0] = {1,2,3,1}; uniforms.constants[1] = {1,2,3,0};
    uniforms.constants[2] = {0,-.25f,0,0}; uniforms.constants[3] = {0,0,-.25f,0};
    uniforms.constants[4] = {-.25f,0,0,-.125f};
    uniforms.constants[6] = {.2f,.4f,.8f,0}; uniforms.constants[7] = {.7f,.3f,.1f,0};
    unsigned materials{}, armor_materials{}, cases{}, unlit{}, teeth_cases{};
    for (const auto& tag : cache.tags()) {
        if (tag.group != "shad" || !tag.name.starts_with("objects\\characters\\elite\\shaders\\")) continue;
        const auto* reference = pstencil::reference_light_material(tag.name);
        if (!reference) continue;
        auto authored = cache.load_material(tag.datum);
        ++materials;
        unsigned material_cases{};
        for (unsigned lod = 0; lod < authored.lods.size(); ++lod) {
            authored.selected_shader_lod = lod;
            for (const unsigned geometry : {0u, 2u}) for (const unsigned contrast : {0u, 3u}) {
                EffectLightDefinition definition;
                word(definition, 4, geometry, 2);
                word(definition, 0x7c, ~0u, 4); word(definition, 0xe0, ~0u, 4);
                word(definition, 0x8e, geometry ? 1 : 0, 2);
                word(definition, 0x8a, contrast, 2); word(definition, 0x8c, contrast, 2);
                const auto light = geometry * 4 + contrast;
                bank.preload(authored, light, definition);
                const auto draws = bank.complete(authored.source_shader_datum, lod, light, uniforms);
                if (!(authored.lods.at(lod).available_layers & (1u << 13))) {
                    require(!draws || draws->empty(), "An authored LOD without a specular layer gained a highlight");
                    ++unlit;
                    continue;
                }
                if (!draws || draws->size() != 3) {
                    std::cerr << tag.name << " lod=" << lod << " geometry=" << geometry << " contrast=" << contrast
                              << " draws=" << (draws ? draws->size() : 0) << '\n';
                    if (draws) for (const auto& draw : *draws) {
                        const auto& resident = mesh.materials.at(draw.material_slot);
                        std::cerr << "blend=" << resident.forward_blend << " bump=" << resident.bump.format
                                  << " depth=" << resident.bump.depth << " layers=" << resident.bump.layers
                                  << " detail-layers=" << resident.detail.layers << '\n';
                    }
                }
                require(draws && draws->size() == 3, "Elite did not use the recovered normal/utility/dependent chain");
                const auto& utility = draws->at(1);
                require(mesh.materials.at(utility.material_slot).forward_blend == pstencil::normal_light_utility_blend_2003,
                        "Elite lost its native normal-space utility pipeline");
                require(utility.program.words == pstencil::light_utility_2003(geometry, contrast, contrast, 0, 9).words,
                        "Actual Elite material completion lost builtin9");
                require(utility.constants[0][10] == reference->v10 && utility.constants[0][11] == reference->v11,
                        "Actual Elite draw lost the original material facing/grazing colors");
                require(!native_fragment_kernel_path("pstencil-normal-utility", utility.program.words).empty(),
                        "Actual Elite draw has no exact native fragment identity");
                require(!native_fragment_kernel_path("pstencil-light-utility", utility.program.words).empty(),
                        "Elite tangent-space control has no exact native fragment identity");
                ++cases;
                ++material_cases;
            }
        }
        require(material_cases >= 4, "Elite material lacks positive sphere/projected shiny coverage");
        if (tag.name == "objects\\characters\\elite\\shaders\\teeth") teeth_cases=material_cases;
        else ++armor_materials;
    }
    std::cout << materials << " prepared MCC Elite materials; " << cases
              << " material-bank completions with original shiny kernels/colors and native routing; "
              << unlit << " authored unlit controls and " << teeth_cases << " teeth responses\n";
    require(materials == 6 && armor_materials == 5 && unlit >= 16 && teeth_cases == 4,
            "Incomplete Elite armor, cutout or unlit controls");
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
