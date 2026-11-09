#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/mcc_script_layout.hpp"
#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/mcc_sound_dat_resources_tu15.hpp"
#include "halo2/source_runtime_mcc_preparation.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
#ifdef HALO2_WORKSHOP_PREDICTION_COLD_CACHE
#include "workshop_prediction_cold_sound.hpp"
#endif

using namespace halo2;
using Json = nlohmann::json;
namespace {
void need(bool condition, const char* message) { if (!condition) throw FormatError(message); }
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> output{};
    unsigned length = 0;
    need(EVP_Digest(bytes.data(), bytes.size(), output.data(), &length, EVP_sha256(), nullptr) == 1 && length == 32,
         "Cannot authenticate prediction bytes");
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (unsigned n = 0; n < length; ++n) { result += digits[output[n] >> 4]; result += digits[output[n] & 15]; }
    return result;
}
template<class F> void refused(F&& action) {
    bool failed = false;
    try { action(); } catch (const FormatError&) { failed = true; }
    need(failed, "Changed prediction resource identity was accepted");
}
}

int main(int argc, char** argv) {
    Json report{{"scope", "Actual E3 canonical looping/sound/chunk ownership and immutable encoded DAT reads; no native HS, cache allocation, readiness, voices or mission admission"},
                {"passed", false}, {"complete_mod_support", false}};
    try {
        need(argc == 5, "Supply map, shared maps, prepared root and report");
        Cache cache(argv[1], argv[2], {}, argv[3]);
        need(cache.is_mcc() && cache.is_custom_mcc(), "Prediction delivery needs an actual prepared custom MCC Cache");
        const auto* scenario_tag = cache.find_tag(cache.scenario_tag());
        need(scenario_tag, "Prediction scenario is absent");
        const Bytes scenario(cache.read_tag_bytes(scenario_tag->address, 0x240));
        const auto nodes = cache.read_tag_bytes(scenario.u32(0x23c), std::size_t(scenario.u32(0x238)) * 20);
        const auto strings = cache.read_tag_bytes(scenario.u32(0x1b4), scenario.u32(0x1b0));
        const auto scripts = cache.read_tag_bytes(scenario.u32(0x1bc), std::size_t(scenario.u32(0x1b8)) * 0x28);
        const auto nodes_digest = digest(nodes);
        const auto layout = project_mcc_script_layout(cache, nodes, strings, scripts, {true});
        const NativeMccScriptBuiltinProfile owned(layout, strings, true);
        std::set<std::string> requirements;
        for (const auto& requirement : layout.requirements)
            if (!owned.owns(requirement)) requirements.insert(requirement.name);
        need(requirements.size() == 22 && requirements.contains("sound_looping_predict"),
             "Prediction inventory changed complete-E3 script readiness");
        need(!mcc_script_builtin_signature(607), "Unqualified looping prediction declaration was admitted");
        const auto resolve = [&](unsigned handle) {
            need(handle != null_datum && (handle & 65535) < nodes.size() / 20, "Prediction syntax index is invalid");
            const Bytes row(nodes.subspan(std::size_t(handle & 65535) * 20, 20));
            need(row.u16(0) == handle >> 16 && row.u16(6) != 0xbaba, "Prediction syntax salt is stale");
            return row;
        };
        std::vector<TagHandle> loops;
        for (unsigned n = 0; n < nodes.size() / 20; ++n) {
            const Bytes row(nodes.subspan(n * 20, 20));
            if (!row.u16(0) || row.u16(6) == 0xbaba || (row.u16(6) & 7) || row.u16(2) != 607) continue;
            const auto function = resolve(row.u32(16));
            const auto argument = resolve(function.u32(8));
            need(function.u16(4) == 2 && (function.u16(6) & 1) && argument.u16(4) == 35 &&
                 (argument.u16(6) & 7) == 1 && argument.u32(8) == null_datum,
                 "Looping prediction lost its authored literal operand");
            need(Bytes(strings).string(function.u32(12), strings.size() - function.u32(12)) == "sound_looping_predict",
                 "Looping prediction lost its source function name");
            loops.push_back({argument.u32(16)});
        }
        need(loops.size() == 1 && !loops.front().is_null(), "E3 prediction caller count differs");
        std::optional<MccSoundRuntimePlan> sound;
        unsigned captures = 0;
        auto planned_delivery = MccRuntimeCacheDelivery::plan(cache, 0x90000000, 0x60000000,
            [&](MccSoundRuntimePlan&& value) { ++captures; sound.emplace(std::move(value)); });
        need(planned_delivery.issues().empty() && captures == 1 && sound, "Prediction canonical metadata is incomplete");
        auto timing = build_mcc_sound_timing_reference(cache, *sound,
            [&](unsigned address, std::size_t bytes) { return planned_delivery.runtime_address(address, bytes); });
        SourceRuntimeMccPreparation metadata{0x90000000, 0x60000000,
            std::move(planned_delivery), std::move(*sound), std::move(timing)};
        const auto& delivery = metadata.delivery;
        auto image = delivery.tag_image();
        const auto read = [&](unsigned address, std::size_t bytes) {
            for (const auto& region : image.regions)
                if (address >= region.address && std::uint64_t(address) + bytes <= std::uint64_t(region.address) + region.bytes.size())
                    return Bytes(region.bytes).slice(address - region.address, bytes);
            throw FormatError("Prediction escaped its canonical metadata allocation");
        };
        const auto root = [&](TagHandle handle, TagGroup group) {
            const auto* tag = cache.query_tag({.handle = handle, .group = group, .require_address = true});
            need(tag && tag->handle() == handle, "Prediction lost its full-salt source tag");
            const Bytes entry(read(delivery.index().table + (handle.value & 65535) * 16, 16));
            need(entry.u32(0) == group.value && entry.u32(4) == handle.value && entry.u32(12) == tag->size &&
                 entry.u32(8) == delivery.runtime_address(tag->address, tag->size),
                 "Prediction canonical typed directory differs");
            return entry.u32(8);
        };
        const auto loop_handle = loops.front();
        const auto* loop_tag = cache.find_tag(loop_handle, tag_groups::sound_looping);
        need(loop_tag, "Prediction looping tag is absent");
        const auto source_root = cache.read_tag_bytes(loop_tag->address, 0x34);
        const auto source_digest = digest(source_root);
        const auto authored = cache.load_looping_sound(loop_handle.value);
        const Bytes canonical_loop(read(root(loop_handle, tag_groups::sound_looping), 0x2c));
        need(authored.tracks.size() == 2 && canonical_loop.u32(0x1c) == authored.tracks.size() &&
             canonical_loop.u32(0x20) == delivery.runtime_address(Bytes(source_root).u32(0x28), authored.tracks.size() * 0x58),
             "Prediction did not preserve the two authored looping tracks");
        const auto resources = MccSoundDatResourcesTu15::prepare(cache, metadata);
        resources->require_routes(metadata.sound);
        report["resource_routes"] = resources->routes();
        report["tracks"] = Json::array();
        unsigned controls = 0;
        const auto table_row = [&](unsigned bank, unsigned index, unsigned stride) {
            const auto block = metadata.sound.blocks.at(bank);
            need(block.count >= 0 && index < unsigned(block.count), "Prediction selector escapes its native sound bank");
            return Bytes(read(block.address + index * stride, stride));
        };
        for (unsigned track = 0; track < authored.tracks.size(); ++track) {
            const Bytes row(read(canonical_loop.u32(0x20) + track * 0x58, 0x58));
            const TagHandle selected{row.u32(0x20)};
            need(selected.value == authored.tracks[track].loop && !selected.is_null(), "Prediction selected another track sound");
            const Bytes definition(read(root(selected, tag_groups::sound), 20));
            const auto pitch = std::bit_cast<std::int16_t>(definition.u16(8));
            need(pitch >= 0 && std::to_integer<unsigned>(definition.slice(10, 1)[0]),
                 "Prediction has no original-compatible first pitch range");
            const auto range = table_row(4, unsigned(pitch), 12);
            const auto permutation = std::bit_cast<std::int16_t>(range.u16(8));
            need(permutation >= 0 && range.u16(10), "Prediction has no original-compatible first permutation");
            const auto permutation_row = table_row(5, unsigned(permutation), 16);
            const auto chunk_index = std::bit_cast<std::int16_t>(permutation_row.u16(12));
            need(chunk_index >= 0 && permutation_row.u16(14), "Prediction has no original-compatible first chunk");
            const auto chunk = table_row(8, unsigned(chunk_index), 12);
            const auto descriptor = metadata.sound.blocks[8].address + unsigned(chunk_index) * 12;
            need(chunk.u32(8) == null_datum, "Metadata prediction fabricated native cache residency");
            const auto route = std::ranges::find(metadata.sound.chunks, descriptor, &MccSoundRuntimeChunk::native_address);
            need(route != metadata.sound.chunks.end() && route->file_offset == chunk.u32(0) &&
                 route->encoded_bytes == (chunk.u32(4) & 0x3fffffffu), "Prediction selected another physical resource descriptor");
            const auto size = (route->encoded_bytes + 511u) & ~511u;
            const SourceSoundCacheReadTu15 request{descriptor, chunk.u32(0), chunk.u32(4), size};
            const auto sector = resources->read(request);
            std::optional<MccSoundResourceRead> first_encoded;
            const auto retained = cache.load_sound(selected.value, 0, 0, [&](const auto& encoded) {
                if (!first_encoded) first_encoded = encoded;
                return resources->read_encoded(cache, encoded);
            });
            const auto independent = cache.load_sound(selected.value, 0, 0);
            need(first_encoded && !retained.packet_chunks.empty() && retained.packet_chunks.front().bytes == route->encoded_bytes &&
                 retained.data == independent.data && retained.data.size() >= route->encoded_bytes && sector.size() == size &&
                 std::equal(sector.begin(), sector.begin() + route->encoded_bytes, retained.data.begin()),
                 "Native prediction sector differs from its actual retained and independent encoded owner");
            for (unsigned fault = 0; fault < 4; ++fault) {
                auto bad = request;
                if (fault == 0) bad.descriptor = null_datum;
                if (fault == 1) bad.encoded_offset ^= 1;
                if (fault == 2) bad.size_flags ^= 0x40000000;
                if (fault == 3) bad.bytes ^= 512;
                refused([&] { (void)resources->read(bad); });
                ++controls;
            }
            auto bad = *first_encoded;
            bad.sound.value ^= 0x10000;
            refused([&] { (void)resources->read_encoded(cache, bad); });
            ++controls;
            report["tracks"].push_back({{"index", track}, {"sound", selected.value}, {"name", independent.name},
                {"native_descriptor", descriptor}, {"source_descriptor", route->source_address},
                {"encoded_bytes", route->encoded_bytes}, {"sector_bytes", size},
                {"first_chunk_sha256", digest(std::span(sector).first(route->encoded_bytes))},
                {"permutation_bytes", retained.data.size()}, {"initial_cache_handle_NONE", true}});
        }
        need(digest(nodes) == nodes_digest && digest(cache.read_tag_bytes(loop_tag->address, 0x34)) == source_digest,
             "Prediction delivery modified authored script or looping metadata");
        resources->require_routes(metadata.sound);
#ifdef HALO2_WORKSHOP_PREDICTION_COLD_CACHE
        report["native_cold_prediction"] = workshop_prediction_cold::exercise(metadata, *resources, loop_handle, report.at("tracks"));
        need(report["native_cold_prediction"].at("passed").get<bool>(), "Actual native cold prediction failed");
#endif
        report.update({{"passed", true}, {"authored_prediction_callers", loops.size()},
            {"complete_script_requirements", requirements.size()}, {"negative_controls", controls},
            {"script_declaration_admitted", false}, {"native_HS_executed", false},
            {"native_cache_allocation_executed", false}, {"source_metadata_unchanged", true},
            {"decoded_PCM_created", false}, {"complete_mod_support", false}});
#ifdef HALO2_WORKSHOP_PREDICTION_COLD_CACHE
        report["scope"] = "Actual E3 canonical sound resources through complete native cold prediction/allocation/I/O and resident replay; declared empty source pools, no World initialization, HS scheduling, voices or mission admission";
        report["native_cache_allocation_executed"] = true;
#endif
    } catch (const std::exception& error) { report["error"] = error.what(); }
    if (argc == 5) std::ofstream(argv[4]) << report.dump(2) << '\n';
    std::cout << report.dump(2) << '\n';
    return report.at("passed").get<bool>() ? 0 : 1;
}
