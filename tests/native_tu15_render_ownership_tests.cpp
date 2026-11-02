#include "halo2/native_tu15_render_object_ownership.hpp"
#include "halo2/native_tu15_render_object_ownership_binding.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "rendering/models/native_tu15_render_object_ownership_bridge.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <xmmintrin.h>

namespace {
using namespace halo2;
using Json = nlohmann::json;
void check(bool ok, const char* message) {if (!ok) throw std::runtime_error(message);}
std::vector<std::byte> decode(const std::string& text) {
    std::vector<std::byte> bytes;
    for (std::size_t n = 0; n < text.size(); n += 2)
        bytes.push_back(std::byte(std::stoul(text.substr(n, 2), nullptr, 16)));
    return bytes;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    for (auto byte : bytes) {const auto n = std::to_integer<unsigned>(byte); out += digits[n >> 4]; out += digits[n & 15];}
    return out;
}
class Memory final : public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t, std::vector<std::byte>> banks;
    mutable Json reads = Json::array();
    mutable std::optional<std::array<std::uint32_t, 2>> failed;
    std::optional<unsigned> reject_read;
    mutable unsigned data_reads{};
    bool recording = true;
    explicit Memory(const Json& row) {
        for (const auto& bank : row.at("banks")) banks.emplace(bank.at("address"), decode(bank.at("bytes")));
        if (row.contains("reject_read")) reject_read = row.at("reject_read");
    }
    std::span<std::byte> view(std::uint32_t address, std::size_t bytes) {
        auto at = banks.upper_bound(address);
        if (at == banks.begin()) throw std::runtime_error("Missing ownership bank");
        --at;
        if (std::uint64_t(address - at->first) + bytes > at->second.size())
            throw std::runtime_error("Truncated ownership bank");
        return std::span(at->second).subspan(address - at->first, bytes);
    }
    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        if (recording) reads.push_back(Json::array({address, bytes.size()}));
        if (!(address >= 0x62000000u && address < 0x62001000u) && reject_read && data_reads++ == *reject_read) {
            failed = {{address, std::uint32_t(bytes.size())}};
            throw std::runtime_error("Declared ownership read boundary");
        }
        try {
            const auto source = const_cast<Memory*>(this)->view(address, bytes.size());
            std::ranges::copy(source, bytes.begin());
        } catch (...) {failed = {{address, std::uint32_t(bytes.size())}}; throw;}
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        std::ranges::copy(bytes, view(address, bytes.size()).begin());
    }
    Json images() const {
        Json result = Json::array();
        for (const auto& [at, bytes] : banks) result.push_back({{"address", at}, {"bytes", hex(bytes)}});
        return result;
    }
};
SourcePhysicsWorldRuntimeAbi caller(const Json& row) {
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = row.at("gp").get<decltype(abi.gp)>(); abi.eflags = row.value("eflags", 0x602u);
    for (unsigned n = 0; n < 8; ++n)
        for (unsigned lane = 0; lane < 4; ++lane) abi.xmm[n][lane] = 0x7fc12000u + n * 4 + lane;
    abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27fu, row.value("mxcsr", 0x1f80u)};
    abi.x87_depth = 2; abi.x87[0] = 0.25L; abi.x87[1] = -1.75L;
    return abi;
}
Json run(const Json& row) {
    const auto entry = row.at("entry").get<std::uint32_t>();
    Memory memory(row); auto abi = caller(row); const auto initial = abi;
    // ABI admission must precede all source reads and stack pushes.
    for (unsigned condition = 0; condition < 5; ++condition) {
        Memory rejected(row); auto invalid = initial;
        if (condition == 0) invalid.active_mmx.emplace();
        if (condition == 1) invalid.x87_depth = 9;
        if (condition == 2) invalid.floating_point->x87_control_word &= ~1u;
        if (condition == 3) invalid.floating_point->mxcsr &= ~0x80u;
        if (condition == 4) invalid.floating_point->mxcsr |= 0x10000u;
        bool failed{};
        try {native_tu15_render_object_ownership_run(rejected, entry, invalid);}
        catch (const std::invalid_argument&) {failed = true;}
        check(failed && rejected.reads.empty() && rejected.images() == memory.images() && invalid.gp == initial.gp,
            "Unsupported ownership ABI touched source state before rejection");
    }
    const auto host_mxcsr = _mm_getcsr(); std::uint16_t cw{}, sw{};
    asm volatile("fnstcw %0\n\tfnstsw %1" : "=m"(cw), "=m"(sw));
    bool stopped{};
    try {native_tu15_render_object_ownership_run(memory, entry, abi);}
    catch (const std::exception&) {if (!memory.failed) throw; stopped = true;}
    std::uint16_t actual_cw{}, actual_sw{};
    asm volatile("fnstcw %0\n\tfnstsw %1" : "=m"(actual_cw), "=m"(actual_sw));
    check(host_mxcsr == _mm_getcsr() && cw == actual_cw && sw == actual_sw,
        "Ownership binding leaked its selected FP environment");
    check(abi.x87 == initial.x87 && abi.x87_depth == initial.x87_depth && abi.x87_status == initial.x87_status,
        "Ownership binding changed the live x87 stack");
    Json out{{"stopped", stopped}, {"gp", abi.gp}, {"eflags", abi.eflags & 0x8d5u},
        {"xmm", abi.xmm}, {"mxcsr", abi.floating_point->mxcsr},
        {"reads", memory.reads}, {"after", memory.images()}, {"host_fp_restored", true}};
    out["failed"] = memory.failed ? Json(*memory.failed) : Json(nullptr);
    Memory observed(row); observed.recording = false; auto second = caller(row);
    Json stores = Json::array(); bool second_stopped{};
    try {
        native_tu15_render_object_ownership_run(observed, entry, second, [&](const auto& store) {
            stores.push_back({{"pc", store.instruction}, {"address", store.address},
                {"before", hex(store.before)}, {"after", hex(store.written)}});
        });
    } catch (const std::exception&) {if (!observed.failed) throw; second_stopped = true;}
    check(second_stopped == stopped && observed.images() == memory.images(),
        "Ownership observers changed the actual store/read prefix");
    out["stores"] = stores;
    // Every generated family retains the real WorldState save/load boundary.
    // Exercise that transport with live x87/XMM state and observed stores.
    Memory bridged(row); bridged.recording = false; auto third = caller(row);
    Json bridged_stores = Json::array(); bool third_stopped{};
    {
        using namespace physics_world_runtime_detail;
        FpScope scope(*third.floating_point);
        SourcePhysicsWorldRuntimeServices services;
        SourcePhysicsContactRecordObserver observer = [&](const auto& store) {
            bridged_stores.push_back({{"pc", store.instruction}, {"address", store.address},
                {"before", hex(store.before)}, {"after", hex(store.written)}});
        };
        WorldState state{bridged, observer, services, SourcePhysicsContactImage::TitleUpdate15};
        state.load(third);
        try {native_tu15_render_object_ownership_bridge(state, entry);}
        catch (const std::exception&) {if (!bridged.failed) throw; third_stopped = true;}
        third = state.save();
    }
    check(third_stopped == stopped && third.gp == abi.gp && third.xmm == abi.xmm &&
        (third.eflags & 0x8d5u) == (abi.eflags & 0x8d5u) && third.x87 == abi.x87 &&
        third.x87_depth == abi.x87_depth && third.x87_status == abi.x87_status &&
        third.floating_point->mxcsr == abi.floating_point->mxcsr &&
        bridged.images() == memory.images() && bridged_stores == stores,
        "Generated ownership bridge changed the complete ABI/store prefix");
    // The original WorldState permits an inactive x87 stack aliased by MMX.
    // These leaves never touch either register file, so preserve valid aliases.
    if (!stopped) {
        Memory aliased(row); auto mmx = initial; mmx.x87_depth = 0; mmx.x87_status = 0;
        mmx.active_mmx.emplace(); mmx.active_mmx->known_mask = 255;
        mmx.active_mmx->physical[3].fill(std::byte{0xa5});
        const auto preserved = mmx.active_mmx;
        native_tu15_render_object_ownership_run(aliased, entry, mmx);
        check(mmx.active_mmx->physical == preserved->physical && mmx.active_mmx->known_mask == preserved->known_mask &&
            !mmx.x87_depth && !mmx.x87_status && mmx.gp == abi.gp && aliased.images() == memory.images(),
            "Ownership kernel changed a valid MMX alias");
    }
    out["generated_bridge_verified"] = true;
    // The semantic API consumes no source stack. Its data reads and result
    // must still agree with the unchanged original complete function.
    Memory semantic(row); semantic.recording = true;
    auto fp = NativeTu15PhysicsFrameFp{0x27fu, 0, row.value("mxcsr", 0x1f80u)};
    std::uint32_t value{}; bool semantic_stopped{};
    try {
        switch (entry) {
            case 0xbcfb0u: value = native_tu15_render_root_owner(semantic, initial.gp[1]); break;
            case 0xbac70u: value = native_tu15_render_attachment_marker(semantic, initial.gp[1],
                std::bit_cast<std::int16_t>(std::uint16_t(initial.gp[0]))); break;
            case 0x158290u: value = native_tu15_render_view_category(semantic, initial.gp[0], fp); break;
            case 0x3e9b0u: value = native_tu15_render_owner_has_view_priority(semantic, initial.gp[6], fp); break;
            default: throw std::runtime_error("Unexpected ownership test entry");
        }
    } catch (const std::exception&) {if (!semantic.failed) throw; semantic_stopped = true;}
    Json data_reads = Json::array();
    for (const auto& access : memory.reads) {
        const auto address = access[0].get<std::uint32_t>();
        if (!(address >= 0x62000000u && address < 0x62001000u)) data_reads.push_back(access);
    }
    check(semantic_stopped == stopped && semantic.reads == data_reads && fp.mxcsr == abi.floating_point->mxcsr,
        "Semantic ownership API changed original data reads or SSE status");
    if (!stopped) check(value == (entry == 0x3e9b0u ? abi.gp[0] & 255u : abi.gp[0]),
        "Semantic ownership result differs from its source transport");
    out["semantic_result"] = value;
    return out;
}
}
int main() try {
    for (std::string line; std::getline(std::cin, line);) {
        if (line.empty()) continue;
        std::cout << run(Json::parse(line)).dump() << '\n' << std::flush;
    }
    return 0;
} catch (const std::exception& error) {std::cerr << error.what() << '\n'; return 1;}
