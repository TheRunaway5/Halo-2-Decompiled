#include "halo2/source_sound_gain_envelope_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

using namespace halo2;

int main(int argc, char** argv) try {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    using Json = nlohmann::json;
    if (argc != 3) throw std::runtime_error("Supply the original driver and gain publication fixtures");
    const auto fixture = Json::parse(std::ifstream(argv[1]));
    const auto publication = Json::parse(std::ifstream(argv[2]));
    if (fixture.at("original_xbe_sha256") != "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
        fixture.at("source_start") != 0x2266dau || fixture.at("source_stop") != 0x226b46u ||
        fixture.at("executed_instructions").size() != 593 || fixture.at("cases").size() != 872 ||
        !fixture.at("instruction_patches").empty() || !fixture.at("supplied_callee_results").empty())
        throw std::runtime_error("Original combined-driver fixture scope differs");
    if (publication.at("original_xbe_sha256") != fixture.at("original_xbe_sha256") ||
        !publication.at("source_instruction_patches").empty() ||
        !publication.at("substituted_calls").empty() || publication.at("combined").size() != 120)
        throw std::runtime_error("Original combined gain publication fixture scope differs");

    unsigned index = 0, composed = 0;
    for (const auto& row : fixture.at("cases")) {
        const SoundDriverFloatingPointContext fp{row.at("control_word"),row.at("mxcsr")};
        const auto expected = row.at("packet").get<std::array<std::uint32_t,10>>();
        const auto original_gains = row.at("channel_gains").get<std::array<std::uint32_t,4>>();
        auto gains = original_gains;
        std::uint16_t before, after;
        asm volatile("fnstcw %0" : "=m"(before));
        const auto csr = _mm_getcsr();
        if (row.contains("publication_index")) {
            const auto& source = publication.at("combined").at(composed);
            if (row.at("publication_index") != composed || source.at("expected") != row.at("channel_gains") ||
                source.at("fp_control") != row.at("control_word") || source.at("mxcsr") != row.at("mxcsr"))
                throw std::runtime_error("Driver case lost its original gain publication input");
            const auto& m = source.at("metadata");
            const SourceSoundGainEnvelopeTu15 metadata{
                m.at("class_lower"),m.at("class_upper"),m.at("scale_lower"),m.at("scale_upper")};
            gains = source_loop_combined_gain_tu15(source.at("amplitudes").get<std::array<std::uint32_t,4>>(),
                metadata,source.at("control"),source.at("parent_gain"),source.at("ducked_class_gain"),fp);
            ++composed;
        }
        const auto packet = source_combined_driver_packet_tu15(row.at("driver_flags"),row.at("voice_gain"),gains,fp);
        if (packet.flags != expected[0] || packet.common_gain != expected[1] ||
            packet.voice_gain != expected[2] ||
            packet.channel_gains != std::array{expected[6],expected[7],expected[8],expected[9]})
            throw std::runtime_error("Original combined packet differs at case " + std::to_string(index) +
                ": common gain " + std::to_string(packet.common_gain) + " vs " + std::to_string(expected[1]));
        const SourceCombinedDriverRoutingTu15 routing{
            row.at("category"),row.at("plugin_gain"),row.at("mode"),row.at("auxiliary"),row.at("layout")};
        const auto routes = source_combined_driver_routes_tu15(packet,routing,fp);
        const auto& expected_routes = row.at("routes");
        if (routes.count != expected_routes.size())
            throw std::runtime_error("Original combined route count differs at case " + std::to_string(index));
        for (unsigned n=0;n<routes.count;++n) {
            const auto pair = expected_routes[n].get<std::array<std::uint32_t,2>>();
            if (routes.routes[n] != SourceCombinedMixBinTu15{pair[0],std::bit_cast<std::int32_t>(pair[1])})
                throw std::runtime_error("Original combined route differs at case " + std::to_string(index) +
                    ", route " + std::to_string(n));
        }
        asm volatile("fnstcw %0" : "=m"(after));
        if (before != after || csr != _mm_getcsr())
            throw std::runtime_error("Combined driver leaked the caller's floating-point state");
        ++index;
    }
    if (composed != 120) throw std::runtime_error("Missing original gain-publication cases");
    bool rejected = false;
    try {
        (void)source_combined_driver_packet_tu15(0,0,{},{0x027f,0x1f80});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected) throw std::runtime_error("Ordinary voice entered the combined driver branch");
    const SoundDriverFloatingPointContext fp{0x027f,0x1f80};
    const auto packet = source_combined_driver_packet_tu15(0x20u,0,{0,0,0,0},fp);
    for (const auto routing : {SourceCombinedDriverRoutingTu15{0,0,0,32,0},
                               SourceCombinedDriverRoutingTu15{1,0,1,29,0}}) {
        rejected = false;
        try {
            (void)source_combined_driver_routes_tu15(packet,routing,fp);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        if (!rejected) throw std::runtime_error("Invalid combined driver route list was admitted");
    }
    std::cout << "PASS " << index << " original combined driver packets and ordered route lists, including "
              << composed << " gain publications\n";
    return 0;
#else
    (void)argc;
    (void)argv;
    return 77;
#endif
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
