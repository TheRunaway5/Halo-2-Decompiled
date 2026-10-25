#include "gameplay/objects/source_difficulty_parameter_native_tu15_internal.hpp"
#include "difficulty_literal_routes.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace {
using Json = nlohmann::json;
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
struct Memory final : SourcePhysicsContactMemory {
    std::map<std::uint32_t, std::byte> bytes;
    mutable Json reads = Json::array();
    std::uint32_t pc{};
    struct Fault { std::uint32_t instruction{}, address{}; std::size_t size{}; };
    void read(std::uint32_t address, std::span<std::byte> output) const override {
        for (std::size_t i = 0; i < output.size(); ++i)
            if (!bytes.contains(address + std::uint32_t(i))) throw Fault{pc, address, output.size()};
        std::string hex;
        constexpr char digits[] = "0123456789abcdef";
        for (std::size_t i = 0; i < output.size(); ++i) {
            output[i] = bytes.at(address + std::uint32_t(i));
            const auto value = std::to_integer<unsigned>(output[i]);
            hex += digits[value >> 4]; hex += digits[value & 15];
        }
        reads.push_back({{"instruction", pc}, {"address", address}, {"bytes", hex}});
    }
    void write(std::uint32_t, std::span<const std::byte>) override {
        throw std::runtime_error("Read-only difficulty selection emitted a store");
    }
};
}

int main(int argc, char** argv) try {
    check(argc == 2, "Supply independent unchanged-original difficulty cases");
    std::ifstream file(argv[1]); const auto fixture = Json::parse(file);
    check(fixture.at("original_xbe_sha256") ==
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Difficulty authority changed");
    std::size_t comparisons{}, total_reads{};
    for (const auto& row : fixture.at("cases")) {
        for (unsigned route = 0; route < 5; ++route) {
            if (row.at("entry") == 0x1f19b0u && route == 3) continue;
            try {
                Memory memory;
                for (const auto& bank : row.at("banks")) {
                    const auto address = bank[0].get<std::uint32_t>();
                    const auto hex = bank[1].get<std::string>();
                    for (std::size_t i = 0; i < hex.size() / 2; ++i)
                        memory.bytes[address + std::uint32_t(i)] =
                            std::byte(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
                }
                if (!row.at("fault_page").is_null()) {
                    const auto page = row.at("fault_page").get<std::uint32_t>();
                    for (auto it = memory.bytes.lower_bound(page); it != memory.bytes.end() && it->first < page + 4096u;)
                        it = memory.bytes.erase(it);
                }
                const auto before = memory.bytes;
                SourcePhysicsContactRuntimeAbi abi{};
                abi.gp = row.at("before").at("gp").get<decltype(abi.gp)>();
                abi.xmm = row.at("before").at("xmm").get<decltype(abi.xmm)>();
                abi.eflags = row.at("before").at("eflags");
                abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
                Json trace = Json::array();
                SourcePhysicsWorldRuntimeServices services{};
                services.instruction = [&](std::uint32_t pc) {
                    if (route > 1) pc = fixture.at("canonical_observation_map").at(std::to_string(pc));
                    memory.pc = pc; trace.push_back(pc);
                };
                const O observer = [](const auto&) {
                    throw std::runtime_error("Difficulty adapter emitted a store");
                };
                WorldState state(memory, observer, services, SourcePhysicsContactImage::TitleUpdate15);
                state.load(abi);
                const auto entry = row.at("entry").get<std::uint32_t>();
                Json fault = nullptr;
                const auto activity = source_difficulty_parameter_native_activity();
                try {
                    if (route == 0) {
                        auto site = [&](std::uint32_t, std::uint32_t actual) { services.instruction(actual); };
                        if (entry == 0x1f1950u) difficulty_parameter_native_tu15(state, site);
                        else current_difficulty_parameter_native_tu15(state, site);
                    } else difficulty_literal_routes::run(route, entry, state);
                } catch (const Memory::Fault& missing) {
                    fault = {{"instruction", missing.instruction}, {"address", missing.address}, {"size", missing.size}};
                }
                abi = state.save();
                const auto after = source_difficulty_parameter_native_activity();
                bool native_selected = route == 0;
#ifdef HALO2_DIFFICULTY_INSTALLED_ROUTES
                native_selected = true;
#endif
                if (native_selected) {
                    check(after.current_difficulty_calls == activity.current_difficulty_calls +
                        (entry == 0x1f19b0u), "Native difficulty wrapper was not selected");
                    check(after.parameter_calls == activity.parameter_calls +
                        (entry == 0x1f1950u || std::any_of(trace.begin(), trace.end(),
                            [](const auto& pc) { return pc == 0x1f1950u; })),
                        "Native difficulty parameter was not selected");
                } else check(after.parameter_calls == activity.parameter_calls &&
                    after.current_difficulty_calls == activity.current_difficulty_calls,
                    "Retained literal route entered the replacement kernel");
                check(fault == row.at("fault"), "Difficulty missing-owner boundary differs");
                check(trace == row.at("instructions"), "Difficulty instruction/fault boundaries differ");
                check(memory.reads == row.at("reads"), "Difficulty ordered reads differ");
                check(abi.gp == row.at("after").at("gp").get<decltype(abi.gp)>(), "Difficulty GP state differs");
                check(abi.eflags == row.at("after").at("eflags"), "Difficulty integer flags differ");
                check(abi.xmm == row.at("after").at("xmm").get<decltype(abi.xmm)>(), "Difficulty SSE payload differs");
                check(abi.x87_depth == 0 && abi.x87_status == 0 && !abi.active_mmx && abi.fs_base == 0 &&
                    abi.floating_point->x87_control_word == 0x27f && abi.floating_point->mxcsr == 0x1f80,
                    "Difficulty FP/control state changed");
                check(memory.bytes == before, "Difficulty full memory changed");
                ++comparisons; total_reads += memory.reads.size();
            } catch (const std::exception& error) {
                std::cerr << "case=" << row.at("id") << " route=" << route << ' ';
                throw;
            }
        }
    }
    std::cout << "PASS complete original difficulty comparisons=" << comparisons
        << " ordered_reads=" << total_reads << '\n';
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n'; return 1;
}
