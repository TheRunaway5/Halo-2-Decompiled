#pragma once
#include "audio/native_tu15_sound_kernel.hpp"
#include "halo2/source_sound_cache_io_tu15.hpp"
#include "halo2/source_workshop_script_builtins_tu15.hpp"
#include <limits>
#include <map>

// Declared empty source pools use the exact ABI layouts exercised by
// oracle_sound_cache_pages_tu15.py. Every handle, allocation, binding, cache
// touch and completion below comes from the real native owners. MCC bytes
// come from MccSoundDatResourcesTu15, through the same I/O boundary as World.
// This component does not run World's pool initialization or HS scheduling.
namespace workshop_prediction_cold {
using namespace halo2;
using Json = nlohmann::json;
inline void need(bool value, const char* message) { if (!value) throw FormatError(message); }
struct Memory final : SourcePhysicsContactMemory {
    std::map<unsigned, std::vector<std::byte>> regions;
    void map(unsigned address, std::size_t size) {
        need(std::uint64_t(address) + size <= 0x100000000ull, "Cold fixture mapping wraps");
        for (const auto& [first, bytes] : regions)
            need(std::uint64_t(address) + size <= first || std::uint64_t(first) + bytes.size() <= address,
                 "Cold fixture mappings overlap");
        need(regions.emplace(address, std::vector<std::byte>(size)).second, "Cold mapping repeats");
    }
    std::span<const std::byte> view(unsigned address, std::size_t size) const {
        auto found = regions.upper_bound(address);
        if (found == regions.begin())
            throw FormatError("Cold native read lacks backing: address=" + std::to_string(address) +
                              " bytes=" + std::to_string(size));
        --found;
        if (std::uint64_t(address) + size > std::uint64_t(found->first) + found->second.size())
            throw FormatError("Cold native read escapes backing: address=" + std::to_string(address) +
                              " bytes=" + std::to_string(size) + " region=" + std::to_string(found->first));
        return std::span(found->second).subspan(address - found->first, size);
    }
    void read(unsigned address, std::span<std::byte> out) const override {
        const auto data = view(address, out.size());
        std::ranges::copy(data, out.begin());
    }
    void write(unsigned address, std::span<const std::byte> in) override {
        (void)view(address, in.size());
        auto found = regions.upper_bound(address); --found;
        std::ranges::copy(in, found->second.begin() + (address - found->first));
    }
    unsigned get(unsigned address, unsigned count = 4) const {
        unsigned result = 0;
        const auto bytes = view(address, count);
        for (unsigned n = 0; n < count; ++n) result |= std::to_integer<unsigned>(bytes[n]) << (n * 8);
        return result;
    }
    void put(unsigned address, unsigned value, unsigned count = 4) {
        std::array<std::byte, 4> bytes{};
        for (unsigned n = 0; n < count; ++n) bytes[n] = std::byte(value >> (n * 8));
        write(address, std::span(bytes).first(count));
    }
};
inline Json exercise(const SourceRuntimeMccPreparation& metadata,
                     const MccSoundDatResourcesTu15& resources, TagHandle loop, const Json& tracks) {
    constexpr unsigned allocator = 0x04000000, pool = 0x04100000, bits = 0x04200000,
        rows = 0x04300000, cache = 0x04400000, cache_bits = 0x04500000,
        cache_rows = 0x04600000, payload = 0x09000000, stack = 0x02018000;
    need(tracks.size() == 2, "Cold E3 fixture requires its two actual tracks");
    std::uint64_t payload_size = 0;
    for (const auto& track : tracks) payload_size += track.at("sector_bytes").get<unsigned>();
    need(payload_size && payload_size <= 64 * 1024 * 1024, "Cold prediction payload is unbounded");
    Memory initial;
    initial.map(0x02000000, 0x20000);
    initial.map(0x004f0000, 0x40000);
    initial.map(allocator, 0x700000);
    initial.map(payload, std::size_t(payload_size));
    for (const auto& region : metadata.delivery.tag_image().regions) {
        if (region.bytes.empty()) continue;
        initial.map(region.address, region.bytes.size());
        initial.write(region.address, region.bytes);
    }
    need(std::ranges::equal(initial.view(metadata.sound.address, metadata.sound.bytes.size()), metadata.sound.bytes),
         "Cold prediction lost its exact canonical sound bank");
    initial.put(0x4f0414, metadata.delivery.index().table);
    // Match source_bind_runtime_cache_sound_gestalt_tu15: the global points
    // to the authored canonical ugh! TAG root, not the allocation buffer's
    // first merged table (MccSoundRuntimePlan::address).
    const auto& index = metadata.delivery.index();
    const auto globals_row = index.table + (index.globals & 65535u) * 16u;
    need(initial.get(globals_row) == tag_groups::globals.value &&
         initial.get(globals_row + 4) == index.globals, "Cold globals directory identity differs");
    const auto globals = initial.get(globals_row + 8);
    need(initial.get(globals + 0xc0) == 1, "Cold fixture lacks its singular authored SoundGlobals");
    const auto gestalt = initial.get(initial.get(globals + 0xc4) + 0x20);
    need(gestalt != 0xffffffff && (gestalt & 65535u) < index.count, "Cold gestalt datum escapes its canonical index");
    const auto gestalt_row = index.table + (gestalt & 65535u) * 16u;
    need(initial.get(gestalt_row) == tag_groups::sound_cache_file_gestalt.value &&
         initial.get(gestalt_row + 4) == gestalt && initial.get(gestalt_row + 12) >= 0x58,
         "Cold authored gestalt salt/group/extent differs");
    const auto gestalt_root = initial.get(gestalt_row + 8);
    for (unsigned n = 0; n < metadata.sound.blocks.size(); ++n)
        need(initial.get(gestalt_root + n * 8) == unsigned(metadata.sound.blocks[n].count) &&
             initial.get(gestalt_root + n * 8 + 4) == metadata.sound.blocks[n].address,
             "Cold authored gestalt differs from its merged sound plan");
    initial.put(0x52d664, gestalt_root);
    initial.put(0x50ebdc, cache);
    initial.put(0x50ebe0, payload);
    initial.put(0x50ebe4, allocator);
    for (const auto address : {pool, cache}) {
        initial.put(address + 0x20, 64);
        initial.put(address + 0x24, address == pool ? 24 : 16);
        initial.put(address + 0x40, 0x8000, 2);
        initial.put(address + 0x44, address == pool ? rows : cache_rows);
        initial.put(address + 0x48, address == pool ? bits : cache_bits);
    }
    initial.put(allocator + 0x20, 0x21f3f0);
    initial.put(allocator + 0x24, 0x21f3b0);
    initial.put(allocator + 0x30, unsigned(payload_size / 512));
    initial.put(allocator + 0x34, 9);
    initial.put(allocator + 0x38, 0x12345678);
    initial.put(allocator + 0x3c, 0xffffffff);
    initial.put(allocator + 0x40, 0xffffffff);
    for (unsigned n = 0; n < 8; ++n) initial.put(allocator + 0x44 + n * 4, 0x7fffffff);
    initial.put(allocator + 0x64, pool);
    for (const auto& track : tracks)
        need(initial.get(track.at("native_descriptor").get<unsigned>() + 8) == 0xffffffff,
             "Cold prediction was given a resident chunk");
    const auto kernels = native_tu15_sound::resource_kernels();
    Json report{{"scope", "Native18D1D0/128A20/21F260/21F420 and original page/datum allocation over actual E3 canonical sound metadata and retained DAT bytes; declared empty pools and synchronous native resource policy"},
                {"passed", false}, {"actual_World_initialization", false}, {"native_HS_executed", false},
                {"complete_mod_support", false}};
    for (unsigned fault = 0; fault < 3; ++fault) {
        auto memory = initial;
        const auto first = tracks.front().at("native_descriptor").get<unsigned>();
        if (fault == 1) memory.put(first, memory.get(first) ^ 1u);
        if (fault == 2) memory.put(first + 4, memory.get(first + 4) ^ 0x40000000u);
        struct Receipt { unsigned descriptor, destination, bytes; };
        std::vector<Receipt> receipts;
        std::vector<SourceSoundCacheReadTu15> reads;
        unsigned request_calls = 0, allocation_calls = 0, io_calls = 0;
        const SourceSoundCacheIoTu15Services io{
            [&](const auto& request) {
                reads.push_back(request);
                return resources.read(request);
            },
            [&](unsigned descriptor, unsigned destination, unsigned bytes) {
                need(std::ranges::find(tracks, descriptor, [](const auto& row) {
                    return row.at("native_descriptor").template get<unsigned>();
                }) != tracks.end(), "Cold completion lost its actual track descriptor");
                need(!reads.empty() && reads.back().descriptor == descriptor && reads.back().bytes == bytes &&
                     std::ranges::equal(memory.view(destination, bytes), resources.read(reads.back())),
                     "Cold receipt preceded its actual retained encoded bytes");
                receipts.push_back({descriptor, destination, bytes});
                return unsigned(receipts.size() - 1); // Index of the actual retained receipt above.
            }};
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner = [&](auto& bank, const auto& call, auto& abi, const auto& observer) {
            if (call.instruction == 0x21f4bd) {
                ++io_calls;
                need(source_try_sound_cache_io_tu15(bank, call, abi, io), "Cold native I/O owner declined its source caller");
                return;
            }
            const auto found = std::ranges::find(kernels, call.target, &native_tu15_sound::Kernel::entry);
            need(found != kernels.end(), "Cold prediction reached an unqualified native callee");
            request_calls += call.target == 0x21f260;
            allocation_calls += call.target == 0x21f420;
            found->run(bank, abi, *abi.floating_point, observer, services);
        };
        const auto run = [&] {
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp[0] = loop.value; abi.gp[4] = stack;
            abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
            abi.x87_depth = 3; abi.x87[0] = .5L; abi.x87[1] = -1.L; abi.x87[2] = 2.L;
            memory.put(stack, 0x03000000);
            source_run_workshop_script_builtins_tu15(memory, 0x18d1d0, abi, *abi.floating_point, services);
            need(abi.gp[4] == stack + 4 && abi.x87_depth == 3 && abi.x87[0] == .5L &&
                 abi.x87[1] == -1.L && abi.x87[2] == 2.L, "Cold prediction lost caller stack or live FP values");
        };
        bool refused = false;
        try { run(); } catch (const FormatError&) { if (!fault) throw; refused = true; }
        if (fault) {
            need(refused && receipts.empty() && reads.size() == 1 && io_calls == 1 &&
                 memory.get(cache_rows + 2, 1) == 0 && memory.regions.at(payload) == initial.regions.at(payload),
                 "Invalid real resource request fabricated completion or payload");
            continue;
        }
        need(request_calls == 2 && allocation_calls == 2 && io_calls == 2 && receipts.size() == 2 &&
             memory.get(pool + 0x3c) == 2 && memory.get(cache + 0x3c) == 2,
             "Cold prediction did not make its two actual allocations and native reads");
        std::set<unsigned> selected;
        Json results = Json::array();
        for (const auto& track : tracks) {
            const auto descriptor = track.at("native_descriptor").get<unsigned>();
            selected.insert(descriptor);
            const auto handle = memory.get(descriptor + 8), index = handle & 65535;
            need(handle != 0xffffffff && index < 2 && memory.get(rows + index * 24, 2) == handle >> 16 &&
                 memory.get(cache_rows + index * 16, 2) == handle >> 16 &&
                 memory.get(cache_rows + index * 16 + 2, 1) == 1 &&
                 memory.get(cache_rows + index * 16 + 3, 1) == 1 &&
                 memory.get(cache_rows + index * 16 + 12) == descriptor,
                 "Cold prediction lost its genuine salted allocation, completion or usage flag");
            const auto receipt = std::ranges::find(receipts, descriptor, &Receipt::descriptor);
            need(receipt != receipts.end() && receipt->bytes == track.at("sector_bytes").get<unsigned>(),
                 "Cold track has no retained actual read");
            results.push_back({{"descriptor", descriptor}, {"handle", handle},
                {"destination", receipt->destination}, {"sector_bytes", receipt->bytes}});
        }
        for (const auto& chunk : metadata.sound.chunks)
            if (!selected.contains(chunk.native_address))
                need(std::ranges::equal(memory.view(chunk.native_address, 12), initial.view(chunk.native_address, 12)),
                     "Cold prediction changed an unselected chunk descriptor");
        const auto cache_before = memory.regions.at(allocator), payload_before = memory.regions.at(payload);
        run();
        need(request_calls == 4 && allocation_calls == 2 && io_calls == 2 && receipts.size() == 2 &&
             memory.regions.at(allocator) == cache_before && memory.regions.at(payload) == payload_before,
             "Resident prediction reallocated, reread or changed its retained resources");
        report.update({{"passed", true}, {"cold_cache_requests", 2}, {"cold_allocations", 2},
            {"actual_native_reads", 2}, {"retained_read_receipts", 2}, {"resident_replay_requests", 2},
            {"resident_replay_extra_reads", 0}, {"tracks", results}});
    }
    report["real_resource_identity_rejection_controls"] = 2;
    return report;
}
} // namespace workshop_prediction_cold
