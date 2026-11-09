#include "rendering/mcc_cpu_bitmap_owner.hpp"
#include "rendering/source_texture_cache_io_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
using Json = nlohmann::json;

namespace {
void need(bool condition, const char* message) {
    if (!condition) throw FormatError(message);
}

unsigned word(const SourceRuntimeMemory& memory, unsigned address) {
    return Bytes(memory.view(address, 4)).u32(0);
}

struct WorldBank final : SourcePhysicsContactMemory {
    PhysicsLiveWorldTu15& world;
    explicit WorldBank(PhysicsLiveWorldTu15& owner) : world(owner) {}

    void read(unsigned address, std::span<std::byte> output) const override {
        world.memory().read(address, output);
    }

    void write(unsigned address, std::span<const std::byte> input) override {
        world.write_source_call_input(address, input);
    }

    void put(unsigned address, unsigned value) {
        std::array<std::byte, 4> input;
        for (unsigned n = 0; n < input.size(); ++n) input[n] = std::byte(value >> (n * 8));
        write(address, input);
    }
};

std::set<unsigned> predicted_bitmaps(const Cache& cache) {
    const auto* scenario = cache.find_tag(cache.scenario_tag());
    need(scenario, "E3 scenario is absent");
    const Bytes root(cache.read_tag_bytes(scenario->address, 0x240));
    const auto syntax = cache.read_tag_bytes(root.u32(0x23c), std::size_t(root.u32(0x238)) * 20);
    const auto node = [&](unsigned handle) {
        const auto row = Bytes(syntax).slice(std::size_t(handle & 65535) * 20, 20);
        need(Bytes(row).u16(0) == handle >> 16, "E3 syntax node lost its authored salt");
        return Bytes(row);
    };
    std::set<unsigned> result;
    for (std::size_t offset = 0; offset < syntax.size(); offset += 20) {
        const Bytes row(syntax.subspan(offset, 20));
        if (!row.u16(0) || (row.u16(6) & 7) || row.u16(2) != 120) continue;
        const auto argument = node(node(row.u32(16)).u32(8));
        need(argument.u16(4) == 39 && (argument.u16(6) & 7) == 1 && argument.u32(8) == null_datum,
             "E3 bitmap prediction is missing its authored literal argument");
        if (argument.u32(16) != null_datum) result.insert(argument.u32(16));
    }
    return result;
}

#if defined(HALO2_WORKSHOP_PREDICTION_WORLD)
// Keep the decoded E3 asset owners while isolating the unrelated mission
// commands. The production readiness gate still sees the full disk content.
struct ScriptFixture {
    std::vector<std::pair<std::span<std::byte>, std::array<std::byte, 4>>> fields;

    explicit ScriptFixture(Cache& cache) {
        const auto* scenario = cache.find_tag(cache.scenario_tag());
        need(scenario, "E3 World fixture has no scenario");
        const auto root = cache.read_tag_bytes(scenario->address, 0x240);
        for (const unsigned offset : {0x1b8u, 0x1c0u, 0x238u}) {
            const auto* field = root.data() + offset;
            auto bytes = std::span(const_cast<std::byte*>(field), 4);
            need(Bytes(bytes).u32(0) != 0, "E3 World fixture has no complete script data");
            fields.push_back({bytes, {bytes[0], bytes[1], bytes[2], bytes[3]}});
        }
        for (const auto& [bytes, saved] : fields) std::ranges::fill(bytes, std::byte{});
    }

    ~ScriptFixture() {
        for (const auto& [bytes, saved] : fields) std::ranges::copy(saved, bytes.begin());
    }
};

void test_world(Cache& cache, Json& report, const std::filesystem::path& saves) {
    const auto bitmaps = predicted_bitmaps(cache);
    need(bitmaps.size() == 9, "E3 predicted bitmap catalogue changed");
    const ScriptFixture fixture(cache);
    report["phase"] = "construct E3 World";
    PhysicsLiveWorldTu15 world(cache, saves);
    world.initialize_map();
    world.configure_campaign_session(1, 1, 30, 0x12345678);
    const auto first_job = world.authority_checkpoint().next_resource_read;
    std::map<unsigned, unsigned> descriptors;
    const auto loaded = [&](unsigned bitmap, unsigned row, const auto& checkpoint) {
        const auto& memory = world.memory();
        const auto read = [&](unsigned address) { return word(memory, address); };
        const auto pool = read(0x4f2d24);
        const auto token = read(row + 0x28);
        need(token != null_datum && (token & 0xffffu) < read(pool + 0x38), "E3 bitmap has no live cache datum");
        const auto tracker = read(pool + 0x44) + (token & 0xffffu) * 0x28;
        const Bytes state(memory.view(tracker, 0x28));
        need(state.u16(0) == (token >> 16) && state.u32(0x10) == row && state.u32(4) == 0 &&
             state.slice(3, 1)[0] == std::byte{1}, "E3 cache tracker lost its full datum, row or readiness");
        const auto job = state.u32(0xc);
        const auto receipt = checkpoint.resource_reads.find(job);
        need(receipt != checkpoint.resource_reads.end(), "E3 texture tracker has no real World read receipt");
        const auto partitions = private_cpu_bitmap::PredictionPartitions::load(cache, {bitmap}, 0);
        const auto partition = partitions.descriptor(0);
        const auto size = (partition->payload_bytes + 511u) & ~511u;
        const auto backing = read(row + 0x40);
        need(receipt->second.descriptor == row && receipt->second.destination == backing && receipt->second.bytes == size,
             "E3 World receipt differs from the actual texture cache read");
        need(std::ranges::equal(partitions.read_sectors(0, partition->encoded_offset, size), memory.view(backing, size)),
             "E3 World retained different authored payload bytes");
        return job;
    };

    report["phase"] = "cold E3 World requests";
    for (const auto bitmap : bitmaps) {
        world.predict_mcc_bitmap_frame({bitmap}, 0);
        world.predict_mcc_bitmap_frame({bitmap}, 0);
    }
    const auto saved = world.authority_checkpoint();
    need(saved.next_resource_read == first_job + bitmaps.size(), "Cold or resident E3 request count differs");
    for (const auto& region : saved.bank.regions) {
        if (region.owner != "MCC authored bitmap prediction row") continue;
        const auto bitmap = word(world.memory(), region.address + 0x4c);
        need(bitmaps.contains(bitmap) && descriptors.emplace(bitmap, region.address).second,
             "World checkpoint captured a different bitmap descriptor catalogue");
        const auto job = loaded(bitmap, region.address, saved);
        report["reads"].push_back({{"bitmap", bitmap}, {"row", region.address}, {"job", job}});
    }
    need(descriptors.size() == bitmaps.size(), "World checkpoint omitted an E3 bitmap row");

    report["phase"] = "E3 World authority restore";
    const auto [bitmap, row] = *descriptors.begin();
    const auto revision = world.authority_backing_revision();
    {
        auto corrupt = saved;
        auto altered = std::ranges::find(corrupt.bank.regions, row, &SourceRuntimeMemory::CheckpointRegion::address);
        need(altered != corrupt.bank.regions.end(), "Missing E3 descriptor in corruption control");
        altered->bytes[0x1c] ^= std::byte{1};
        bool rejected = false;
        try { (void)world.prepare_authority(corrupt); }
        catch (const FormatError& error) { rejected = std::string_view(error.what()).find("authored descriptor") != std::string_view::npos; }
        need(rejected && world.authority_backing_revision() == revision, "Altered bitmap descriptor did not fail atomically");
    }
    auto prepared = world.prepare_authority(saved);
    need(world.commit_authority(std::move(prepared)) && world.authority_backing_revision() != revision,
         "Native E3 World did not commit its complete authority checkpoint");
    for (const auto& [handle, descriptor] : descriptors) {
        world.predict_mcc_bitmap_frame({handle}, 0);
        loaded(handle, descriptor, saved);
    }
    need(world.authority_checkpoint().next_resource_read == saved.next_resource_read,
         "A restored resident E3 bitmap started another read");

    report["phase"] = "E3 World evict and reload";
    auto evict = world.prepare_source_call();
    evict.gp[3] = word(world.memory(), row + 0x28);
    evict.gp[7] = word(world.memory(), 0x4f2d34);
    need(word(world.memory(), evict.gp[7] + 0x20) == 0x12fd10, "E3 texture page cache lost its native retirement owner");
    world.run_actual(0x140310, evict);
    need(word(world.memory(), row + 0x28) == null_datum && word(world.memory(), row + 0x40) == 0,
         "E3 native eviction left a live texture descriptor binding");
    world.predict_mcc_bitmap_frame({bitmap}, 0);
    const auto reloaded = world.authority_checkpoint();
    need(reloaded.next_resource_read == saved.next_resource_read + 1 &&
         loaded(bitmap, row, reloaded) == saved.next_resource_read,
         "E3 eviction did not create one real read receipt on the restored descriptor");
    report["restored_bindings"] = descriptors.size();
    report["reloaded_job"] = saved.next_resource_read;
    report["phase"] = "complete";
}
#endif
}

int main(int argc, char** argv) {
#if defined(HALO2_WORKSHOP_PREDICTION_WORLD)
    if (argc != 5) return 1;
    Json report{{"scope", "E3 campaign asset World with isolated decoded script tables, production texture I/O and authority"},
        {"passed", false}, {"reads", Json::array()}};
    try {
        Cache cache(argv[1], argv[2], {}, argv[3]);
        need(cache.is_custom_mcc(), "E3 World test requires a Workshop cache");
        const auto* scratch = std::getenv("HALO2_TEST_SCRATCH");
        need(scratch, "E3 World test requires owned scratch");
        test_world(cache, report, std::filesystem::path(scratch) / "saves");
        need(predicted_bitmaps(cache).size() == 9, "Decoded script fixture did not restore its authored data");
        report["passed"] = true;
    } catch (const std::exception& error) {
        report["error"] = error.what();
    }
#else
    if (argc != 6) return 1;
    Json report{{"scope", "Native cold texture allocator over World-started stock MCC cache pools and actual E3 payloads"},
        {"passed", false}, {"reads", Json::array()}};
    try {
        const Cache mod(argv[1], argv[2], {}, argv[3]);
        const Cache stock(argv[5]);
        need(mod.is_custom_mcc() && stock.is_mcc(), "Cold prediction fixture requires authored MCC content");
        const auto* scratch = std::getenv("HALO2_TEST_SCRATCH");
        need(scratch, "Cold prediction test requires owned scratch");
        PhysicsLiveWorldTu15 world(stock, std::filesystem::path(scratch) / "saves");
        world.initialize_map();
        world.configure_session(source_game_variant_default(0), 1, 30, 0x12345678);
        auto& memory = world.memory();
        need(word(memory, 0x4f2d24) && word(memory, 0x4f2d34) && word(memory, 0x4f2d30),
             "World startup did not construct and activate the texture cache");

        constexpr unsigned extent = 4096;
        const auto address = memory.find_unowned_range(0xb0000000, 0xb1000000, extent, extent);
        need(address.has_value(), "No address space for cold texture rows");
        const auto fixture = *address;
        memory.mount(fixture, std::vector<std::byte>(extent), SourceRuntimeMemory::Access::ReadWrite,
                     "explicit MCC bitmap cold test descriptor rows");
        WorldBank bank(world);
        private_cpu_bitmap::PredictionRows prediction_rows;
        const auto bitmaps = predicted_bitmaps(mod);
        need(bitmaps.size() == 9, "E3 predicted bitmap catalogue changed");
        unsigned jobs = 0, rejected_transfers = 0, evicted = 0;
        std::map<unsigned, unsigned> descriptors;
        for (const auto handle : bitmaps) {
            need(mod.bitmap_count(handle) == 1, "E3 cold bitmap frame count changed");
            const auto texture = private_cpu_bitmap::PredictionPartitions::load(mod, {handle}, 0);
            const auto resource = private_cpu_bitmap::load(mod, {handle}, 0, true);
            const auto route = texture.descriptor(0);
            need(route.has_value() && !texture.descriptor(1) && !texture.descriptor(2),
                 "E3 bitmap needs an unexpected partition");
            const auto descriptor = prediction_rows.bind(memory, mod, {handle}, 0);
            descriptors.emplace(handle, descriptor);
            const auto pending = fixture + 0xf00, failed = pending + 4;
            need(word(memory, descriptor + 0x4c) == handle && word(memory, descriptor + 0x28) == null_datum,
                 "Projected source descriptor lost its full-salt owner or starts resident");
            const auto sectors = (route->payload_bytes + 511u) & ~511u;
            unsigned read_count = 0, tracker = 0, backing = 0;
            const auto run = [&] {
                bank.put(pending, 0);
                bank.put(failed, 0);
                auto abi = world.prepare_source_call(std::array{descriptor, 0u, 0u, pending, failed});
                abi.gp[0] = 0; // Original mode 2 selects partition 0.
                abi.gp[1] = 1; // Permit allocation.
                abi.gp[3] = 2; // Original mode 2 priority.
                const auto stack = abi.gp[4];
                SourcePhysicsWorldRuntimeServices services;
                services.required_owner = [&](auto& active, const auto& call, auto& state, const auto& observer) {
                    if (call.instruction != 0x1301b0 || call.target != 0x1a3b70 || call.tail) {
                        world.run_actual(call.target, state, observer);
                        return;
                    }
                    tracker = state.gp[6];
                    backing = word(memory, state.gp[4] + 8);
                    const auto ready = [&] { return memory.view(tracker + 3, 1)[0] != std::byte{}; };
                    need(!ready() && backing >= 0x80000000, "Texture allocation was ready before reading");
                    if (!jobs) {
                        const auto before = memory.view(backing, sectors);
                        const std::vector saved(before.begin(), before.end());
                        const auto registers = state.gp;
                        bool rejected = false;
                        try {
                            source_try_texture_cache_io_tu15(active, bank, call, state,
                                [&](const auto&) { return std::vector<std::byte>(sectors - 1); },
                                [&](const auto&) { return ++jobs; });
                        } catch (const FormatError&) {
                            rejected = true;
                        }
                        need(rejected && jobs == 0 && !ready() && registers == state.gp &&
                             std::ranges::equal(saved, memory.view(backing, sectors)),
                             "Incomplete transfer changed backing, receipt, readiness or ABI");
                        ++rejected_transfers;
                    }
                    need(source_try_texture_cache_io_tu15(active, bank, call, state,
                        [&](const SourceTextureCacheReadTu15& read) {
                            need(read.descriptor == descriptor && read.partition == 0 &&
                                 read.encoded_offset == route->encoded_offset && read.bytes == sectors &&
                                 read.destination == backing, "Texture read differs from the authored E3 route");
                            auto bytes = prediction_rows.read(memory, mod, read.descriptor,
                                read.partition, read.encoded_offset, read.bytes);
                            need(bytes.has_value(), "World bitmap owner did not recognize its own row");
                            return std::move(*bytes);
                        },
                        [&](const SourceTextureCacheReadTu15&) {
                            need(!ready() && std::ranges::equal(resource.encoded,
                                memory.view(backing, resource.encoded.size())),
                                "Texture job preceded its actual payload or readiness was published early");
                            return ++jobs;
                        }) && ready(), "Texture host I/O did not complete the native request");
                    ++read_count;
                };
                source_run_physics_live_support_tu15(bank, 0x12f4a0, abi, *abi.floating_point, services);
                need(abi.gp[4] == stack + 24 && word(memory, pending) == 0 && word(memory, failed) == 0,
                     "Cold texture worker did not complete its original call frame");
                return abi.gp[0];
            };
            const auto header = run();
            need(read_count == 1 && header == tracker + 0x14 &&
                 word(memory, descriptor + 0x28) != null_datum && word(memory, tracker + 0xc) == jobs &&
                 word(memory, descriptor + 0x4c) == handle,
                 "Cold native bitmap did not allocate and publish its original ready header");
            need(run() == header && read_count == 1, "Ready native bitmap started another resource read");
            // There are nine authored cold requests. The original worker
            // throttles after ten reads in a frame, leaving one extra read.
            const bool exercise_eviction = handle == *bitmaps.begin();
            if (exercise_eviction) {
                auto evict = world.prepare_source_call();
                evict.gp[3] = word(memory, descriptor + 0x28);
                evict.gp[7] = word(memory, 0x4f2d34);
                need(word(memory, evict.gp[7] + 0x20) == 0x12fd10,
                     "Texture page cache lost its original retirement callback");
                world.run_actual(0x140310, evict);
                need(word(memory, descriptor + 0x28) == null_datum && word(memory, descriptor + 0x40) == 0 &&
                     word(memory, descriptor + 0x54) == 0 && word(memory, descriptor + 0x4c) == handle,
                     "Original eviction did not clear the live binding and preserve its map owner");
                need(prediction_rows.bind(memory, mod, {handle}, 0) == descriptor,
                     "Evicted bitmap lost its map-scoped descriptor address");
                const auto reloaded = run();
                need(reloaded == tracker + 0x14 && read_count == 2 && word(memory, tracker + 0xc) == jobs,
                     "Evicted bitmap did not issue a new real cache read");
                ++evicted;
            }
            report["reads"].push_back({{"handle", handle}, {"partition", 0}, {"bytes", sectors},
                {"descriptor", descriptor}, {"tracking_row", tracker}, {"backing", backing},
                {"reused", true}, {"reloaded_after_eviction", exercise_eviction}});
        }
        need(jobs == bitmaps.size() + 1 && word(memory, 0x4f2d54) == jobs &&
             evicted == 1 && rejected_transfers == 1, "Cold texture coverage differs");
        auto restored = private_cpu_bitmap::PredictionRows::restore(memory, mod);
        for (const auto& [handle, descriptor] : descriptors)
            need(restored.bind(memory, mod, {handle}, 0) == descriptor,
                 "Restore failed to retain an already resident texture tracker address");
        const auto row = descriptors.begin()->second;
        const auto authored_offset = word(memory, row + 0x1c);
        bank.put(row + 0x1c, authored_offset ^ 1u);
        bool rejected = false;
        try {
            (void)private_cpu_bitmap::PredictionRows::restore(memory, mod);
        } catch (const FormatError&) {
            rejected = true;
        }
        bank.put(row + 0x1c, authored_offset);
        need(rejected, "Restore accepted an altered authored texture offset");
        report["restored_bindings"] = descriptors.size();
        report["evicted_and_reloaded"] = evicted;
        report["rejected_altered_descriptor"] = rejected;
        report["rejected_incomplete_transfers"] = rejected_transfers;
        report["passed"] = true;
    } catch (const std::exception& error) {
        report["error"] = error.what();
    }
#endif
    std::ofstream(argv[4]) << report.dump(2) << '\n';
    std::cout << report.dump(2) << '\n';
    return report.at("passed").get<bool>() ? 0 : 1;
}
