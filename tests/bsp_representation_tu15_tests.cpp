#include "halo2/bsp_lighting.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

namespace {
void check(bool yes, const char* reason) {
    if (!yes) throw std::runtime_error(reason);
}
template<class Function> void rejects(Function function) {
    bool rejected = false;
    try { function(); } catch (const halo2::FormatError&) { rejected = true; }
    check(rejected, "Nonrepresentable MCC value was admitted");
}
}

int main(int argc, char** argv) try {
    using namespace halo2;
    if (argc != 2 || !std::filesystem::exists(argv[1])) return 77;
    nlohmann::json fixture;
    std::ifstream(argv[1]) >> fixture;
    check(fixture.at("original_sha256") == "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Wrong original packing reference");
    const auto& vector_source = fixture.at("source").at("183af0");
    check(vector_source.front().at("address") == 0x183af0 && vector_source.front().at("bytes") == "51" &&
          vector_source.back().at("address") == 0x183c01 && vector_source.back().at("bytes") == "c3",
          "Wrong original signed vector packer entry/return");
    const auto& uv_source = fixture.at("source").at("d5300");
    check(uv_source.front().at("address") == 0xd5300 && uv_source.back().at("address") == 0xd54c3 &&
          uv_source.back().at("bytes") == "c20800", "Wrong original SHORT2 decoder entry/return");
    check(fixture.at("incident").size() == 12093 && fixture.at("lightmap_uv").size() == 28836 &&
          fixture.at("original_short2_midpoint_bits").size() == 65536, "Incomplete source representation fixture");
    for (const auto& record : fixture.at("incident")) {
        const auto& bits = record.at("bits");
        const Vec3 value{std::bit_cast<float>(bits.at(0).get<unsigned>()),
                         std::bit_cast<float>(bits.at(1).get<unsigned>()),
                         std::bit_cast<float>(bits.at(2).get<unsigned>())};
        check(record.at("original_word") == record.at("xbox_word"), "Original CPU result differs from authored Xbox incident");
        check(pack_bsp_incident_tu15(value) == record.at("original_word"), "Canonical incident differs from genuine original packer");
    }
    for (unsigned n = 0; n < 65536; ++n) {
        const auto value = std::bit_cast<float>(fixture.at("original_short2_midpoint_bits").at(n).get<unsigned>());
        check(pack_bsp_lightmap_uv_tu15(value) == std::int32_t(n)-32768,
              "SHORT2 inverse differs from actual original midpoint decoder");
    }
    for (const auto& record : fixture.at("lightmap_uv"))
        check(pack_bsp_lightmap_uv_tu15(std::bit_cast<float>(record.at("bits").get<unsigned>())) == record.at("xbox_short"),
              "Canonical MCC UV differs from paired authored Xbox SHORT2");
    for (float value : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
                        -std::numeric_limits<float>::infinity()}) {
        rejects([&] { (void)pack_bsp_lightmap_uv_tu15(value); });
        rejects([&] { (void)pack_bsp_incident_tu15({value, 0, 0}); });
        rejects([&] { (void)pack_bsp_incident_tu15({0, value, 0}); });
        rejects([&] { (void)pack_bsp_incident_tu15({0, 0, value}); });
    }
    rejects([] { (void)pack_bsp_lightmap_uv_tu15(std::nextafter(1.f, 2.f)); });
    rejects([] { (void)pack_bsp_lightmap_uv_tu15(std::nextafter(-1.f, -2.f)); });
    check(pack_bsp_lightmap_uv_tu15(-1.f) == -32768 && pack_bsp_lightmap_uv_tu15(1.f) == 32767,
          "SHORT2 signed endpoints differ");
    std::cout << "PASS 12093 genuine vector packs, 65536 original SHORT2 decoder inverses, 28836 paired UVs and14 rejection guards\n";
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
