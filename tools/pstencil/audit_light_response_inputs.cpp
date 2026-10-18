#include "halo2/cache.hpp"
#include "halo2/effect_light.hpp"
#include "halo2/pstencil/material.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using J = nlohmann::json;
int main(int argc, char **argv) try {
    if (argc != 3)
        throw FormatError("Supply current map and new evidence directory");
    Cache cache(argv[1]);
    const std::filesystem::path out = argv[2];
    if (std::filesystem::exists(out))
        throw FormatError("Refusing to replace retained evidence");
    std::filesystem::create_directories(out);
    const auto at = std::find_if(cache.tags().begin(), cache.tags().end(), [](auto &t) {
        return t.name == "objects\\characters\\masterchief\\shaders\\masterchief" && t.group == "shad";
    });
    if (at == cache.tags().end())
        throw FormatError("Current Chief armor shader missing");
    auto authored = cache.load_material(at->datum);
    EffectLightUniformInput in;
    in.position = {1, 2, 3};
    in.forward = {1, 0, 0};
    in.up = {0, 0, 1};
    in.left = {0, 1, 0};
    in.radius = 4;
    in.diffuse = {.2, .5, .8};
    in.specular = {.3, .2, .7};
    in.brightness = 1;
    const auto u = source_effect_light_uniforms(in);
    J rows = J::array();
    for (unsigned i = 0; i < authored.passes.size(); ++i) {
        const auto &p = authored.passes[i];
        if (!p.vertex_shader.ends_with("\\stencil_light_specular_tangentspace_sphere"))
            continue;
        auto m = pstencil::bind_light_pass(cache, authored, i, u);
        if (!m || !m->vertex_bank)
            throw FormatError("Current sphere specular inputs did not bind");
        std::ofstream v(out / ("current-vs-" + std::to_string(i) + ".bin"), std::ios::binary);
        v.write(reinterpret_cast<const char *>(m->vertex_program.data()), m->vertex_program.size());
        if (!v)
            throw FormatError("Cannot retain current vertex program");
        J textures = J::array();
        unsigned stage{};
        for (const auto *t : {&m->diffuse, &m->detail, &m->bump, &m->auxiliary}) {
            const auto filename = "texture-" + std::to_string(i) + "-" + std::to_string(stage++) + ".rgba";
            std::ofstream f(out / filename, std::ios::binary);
            f.write(reinterpret_cast<const char *>(t->pixels.data()), t->pixels.size());
            if (!f)
                throw FormatError("Cannot retain current texture");
            J mips = J::array();
            for (const auto &q : t->mips)
                mips.push_back({q.width, q.height, q.depth, q.offset, q.size});
            textures.push_back({{"file", filename},
                                {"width", t->width},
                                {"height", t->height},
                                {"depth", t->depth},
                                {"layers", t->layers},
                                {"format", t->format},
                                {"mips", mips}});
        }
        rows.push_back({{"pass", i},
                        {"vertex_shader", p.vertex_shader},
                        {"program", m->forward_program.words},
                        {"textures", textures},
                        {"constants", m->vertex_bank->constants},
                        {"program_constants", m->program_constants},
                        {"texture_states", J::array()}});
    }
    const auto attenuation = cache.load_effect_light_attenuation_bitmap();
    std::ofstream volume(out / "current-attenuation.rgba", std::ios::binary);
    volume.write(reinterpret_cast<const char *>(attenuation.pixels.data()), attenuation.pixels.size());
    if (!volume || rows.empty())
        throw FormatError("Current attenuation image or sphere pass missing");
    J mips = J::array();
    for (const auto &m : attenuation.mips)
        mips.push_back({m.width, m.height, m.depth, m.offset, m.size});
    std::ofstream(out / "inputs.json")
        << J{{"map", argv[1]},
             {"uniforms", u.constants},
             {"passes", rows},
             {"attenuation",
              {{"width", attenuation.width},
               {"height", attenuation.height},
               {"depth", attenuation.depth},
               {"layers", attenuation.layers},
               {"format", attenuation.format},
               {"mips", mips},
               {"bytes", attenuation.pixels.size()}}},
             {"scope", "Read-only prepared current sphere inputs and complete selected attenuation image; no "
                       "original GPU frame claim"}}
               .dump(2)
        << '\n';
    std::cout << rows.size() << " prepared current specular passes\n";
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}
