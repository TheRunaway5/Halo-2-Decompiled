#include "rendering/mcc_cpu_bitmap_owner.hpp"
#include "halo2/content.hpp"
#include "halo2/mcc_cache.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
using Json = nlohmann::json;
namespace {
void need(bool value, const char* message) { if (!value) throw FormatError(message); }
template<class F> void refused(F&& action) {
    bool failed = false;
    try { action(); } catch (const FormatError&) { failed = true; }
    need(failed, "Prediction bitmap negative control was admitted");
}
}
int main(int argc, char** argv) {
    Json report{{"scope", "Actual authored E3 bitmap prediction resource owners; no HS, source cache allocation, CPU sampling or full mission acceptance"},
                {"passed", false}, {"complete_mod_support", false}};
    try {
        need(argc == 5, "Supply map, shared maps, prepared root and report");
        Cache cache(argv[1], argv[2], {}, argv[3]);
        need(cache.is_mcc() && cache.is_custom_mcc(), "Bitmap prediction requires an actual prepared custom MCC Cache");
        const auto* scenario_tag = cache.find_tag(cache.scenario_tag());
        need(scenario_tag, "Bitmap prediction scenario is absent");
        const Bytes scenario(cache.read_tag_bytes(scenario_tag->address, 0x240));
        const auto nodes = cache.read_tag_bytes(scenario.u32(0x23c), std::size_t(scenario.u32(0x238)) * 20);
        const auto resolve = [&](unsigned handle) {
            need(handle != null_datum && (handle & 65535) < nodes.size() / 20, "Bitmap prediction syntax index is invalid");
            const Bytes row(nodes.subspan(std::size_t(handle & 65535) * 20, 20));
            need(row.u16(0) == handle >> 16 && row.u16(6) != 0xbaba, "Bitmap prediction syntax salt is stale");
            return row;
        };
        std::set<unsigned> bitmaps;
        unsigned calls = 0;
        for (unsigned n = 0; n < nodes.size() / 20; ++n) {
            const Bytes row(nodes.subspan(n * 20, 20));
            if (!row.u16(0) || row.u16(6) == 0xbaba || (row.u16(6) & 7) || row.u16(2) != 120) continue;
            const auto function = resolve(row.u32(16)), argument = resolve(function.u32(8));
            need(argument.u16(4) == 39 && (argument.u16(6) & 7) == 1 && argument.u32(8) == null_datum,
                 "Bitmap prediction lost its authored literal argument");
            ++calls;
            if (argument.u32(16) != null_datum) bitmaps.insert(argument.u32(16));
        }
        need(calls == 11 && bitmaps.size() == 9, "Bitmap prediction authored caller/resource count differs");
        report["resources"] = Json::array();
        unsigned frames = 0, raw_prediction_frames = 0, controls = 0;
        unsigned partition_count = 0, route_controls = 0;
        for (unsigned handle : bitmaps) {
            const auto* tag = cache.query_tag({.handle = TagHandle{handle}, .group = tag_groups::bitmap, .require_address = true});
            need(tag && tag->handle().value == handle, "Bitmap prediction lost a full-salt typed resource");
            const auto bank = cache.mcc_resource_bank_path({handle}, MccResourceBank::Textures);
            refused([&] { (void)private_cpu_bitmap::PredictionPartitions::load(cache, {handle ^ 0x10000u}, 0); });
            refused([&] { (void)private_cpu_bitmap::PredictionPartitions::load(cache, {handle}, cache.bitmap_count(handle)); });
            route_controls += 2;
            for (unsigned frame = 0; frame < cache.bitmap_count(handle); ++frame) {
                const auto resource = cache.load_bitmap_resource(handle, frame);
                const auto texture = cache.load_bitmap(handle, frame);
                const auto owner = private_cpu_bitmap::load(cache, {handle}, frame, true);
                need(owner.bitmap.handle().value == handle && owner.frame == frame &&
                     owner.encoded == resource.levels[0] && owner.metadata.format == resource.format,
                     "Bitmap prediction changed its actual encoded image owner");
                const auto partitions = private_cpu_bitmap::PredictionPartitions::load(cache, {handle}, frame);
                const Bytes row(std::span(owner.rows).subspan(std::size_t(frame) * 168, 168));
                Json routes = Json::array();
                for (unsigned level = 0; level < resource.levels.size(); ++level) {
                    const auto descriptor = partitions.descriptor(level);
                    const auto stored = row.u32(80 + level * 4);
                    const auto offset = row.u32(56 + level * 4);
                    if (!stored) {
                        need(!descriptor && resource.levels[level].empty(), "Absent MCC partition became a cache route");
                        refused([&] { (void)partitions.read_sectors(level, offset, 512); });
                        ++route_controls;
                        continue;
                    }
                    const auto payload = read_mcc_texture_partition(bank, offset, stored);
                    need(descriptor && descriptor->encoded_offset == offset && descriptor->stored_bytes == stored &&
                         descriptor->payload_bytes == payload.size() && payload == resource.levels[level],
                         "Prediction descriptor differs from its exact texture-bank partition");
                    const auto sector_bytes = (descriptor->payload_bytes + 511u) & ~511u;
                    const auto sectors = partitions.read_sectors(level, offset, sector_bytes);
                    need(sectors.size() == sector_bytes && std::ranges::equal(payload, std::span(sectors).first(payload.size())) &&
                         std::ranges::all_of(std::span(sectors).subspan(payload.size()), [](auto byte) { return byte == std::byte{}; }),
                         "Bitmap prediction sectors differ from their payload and transfer padding");
                    refused([&] { (void)partitions.read_sectors(level, offset ^ 1u, sector_bytes); });
                    refused([&] { (void)partitions.read_sectors(level, offset, sector_bytes + 512); });
                    route_controls += 2;
                    ++partition_count;
                    routes.push_back({{"level", level}, {"offset", offset}, {"stored_bytes", stored},
                        {"payload_bytes", payload.size()}, {"sector_bytes", sector_bytes}});
                }
                refused([&] { (void)partitions.descriptor(3); });
                ++route_controls;
                if (resource.format == 1) {
                    ++raw_prediction_frames;
                    const auto check = [&](auto rows, auto input, auto decoded) {
                        return private_cpu_bitmap::project(*tag, frame, owner.root, owner.source_first,
                                                          rows, input, decoded, true);
                    };
                    refused([&] { (void)private_cpu_bitmap::load(cache, {handle}, frame, false); });
                    auto short_resource = resource;
                    short_resource.levels[0].pop_back();
                    refused([&] { (void)check(owner.rows, short_resource, texture); });
                    auto padded_resource = resource;
                    padded_resource.levels[0].push_back(std::byte{1});
                    refused([&] { (void)check(owner.rows, padded_resource, texture); });
                    auto decoded = texture;
                    decoded.pixels[0] ^= std::byte{1};
                    refused([&] { (void)check(owner.rows, resource, decoded); });
                    for (unsigned bit : {8u, 16u}) {
                        auto rows = owner.rows;
                        rows[frame * 168 + 14] |= std::byte(bit);
                        refused([&] { (void)check(rows, resource, texture); });
                    }
                    controls += 6;
                }
                report["resources"].push_back({{"handle", handle}, {"name", tag->name}, {"frame", frame},
                    {"format", resource.format}, {"encoded_bytes", owner.encoded.size()},
                    {"bank", bank.lexically_relative(argv[3]).generic_string()}, {"partitions", std::move(routes)}});
                ++frames;
            }
        }
        need(frames == 9 && raw_prediction_frames == 1 && controls == 6 && partition_count >= frames &&
             route_controls == bitmaps.size() * 2 + frames * 4 + partition_count,
             "Bitmap prediction format/resource coverage is incomplete");
        report.update({{"passed", true}, {"authored_callers", calls}, {"distinct_bitmaps", bitmaps.size()},
            {"resource_frames", frames}, {"new_raw_resource_frames", raw_prediction_frames},
            {"negative_controls", controls}, {"source_cache_readiness_supplied", false},
            {"bound_partitions", partition_count}, {"partition_negative_controls", route_controls},
            {"CPU_sampling_qualified", false}, {"HS_executed", false}, {"script_declaration_admitted", false}});
    } catch (const std::exception& error) { report["error"] = error.what(); }
    if (argc == 5) std::ofstream(argv[4]) << report.dump(2) << '\n';
    std::cout << report.dump(2) << '\n';
    return report.at("passed").get<bool>() ? 0 : 1;
}
