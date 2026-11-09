#include "halo2/source_workshop_script_builtins_tu15.hpp"
#include "halo2/source_campaign_runtime_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "audio/native_tu15_sound_kernel.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

using namespace halo2;
using Json = nlohmann::json;
namespace {
void need(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
std::vector<std::byte> unhex(const std::string& text) {
    need(text.size() % 2 == 0, "Incomplete original HS prediction bytes");
    std::vector<std::byte> result;
    for (std::size_t offset = 0; offset < text.size(); offset += 2)
        result.push_back(std::byte(std::stoul(text.substr(offset, 2), nullptr, 16)));
    return result;
}
struct Memory final : SourcePhysicsContactMemory {
    std::map<unsigned, std::array<std::byte, 4096>> pages;
    void read(unsigned address, std::span<std::byte> output) const override {
        for (auto& value : output) {
            const auto page = pages.find(address & ~4095u);
            need(page != pages.end(), "HS prediction read escaped its complete original page owners");
            value = page->second[address++ & 4095u];
        }
    }
    void write(unsigned address, std::span<const std::byte> input) override {
        for (auto value : input) {
            const auto page = pages.find(address & ~4095u);
            need(page != pages.end(), "HS prediction write escaped its complete original page owners");
            page->second[address++ & 4095u] = value;
        }
    }
};
}

int main(int argc, char** argv) try {
    need(argc == 2, "Supply complete original HS prediction reference");
    std::ifstream input(argv[1]);
    const auto reference = Json::parse(input);
    need(reference.at("xbe_sha256") == "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
         !reference.at("original_code_patched").get<bool>() &&
         !reference.at("original_calls_intercepted").get<bool>() &&
         !reference.at("supplied_callee_results").get<bool>() && reference.at("cases").size() == 11,
         "HS prediction reference lost its complete unchanged original ownership");
    std::vector<std::array<std::byte, 4096>> images;
    for (const auto& encoded : reference.at("page_images")) {
        const auto decoded = unhex(encoded);
        need(decoded.size() == 4096, "HS prediction page image is incomplete");
        std::array<std::byte, 4096> page;
        std::ranges::copy(decoded, page.begin());
        images.push_back(page);
    }
    const auto kernels = native_tu15_sound::resource_kernels();
    const auto request = std::ranges::find(kernels, 0x21f260u, &native_tu15_sound::Kernel::entry);
    need(request != kernels.end(), "HS prediction has no real native cache request");
    unsigned completed = 0, request_count = 0, parser_count = 0, deferred_count = 0;
    for (const auto& row : reference.at("cases")) {
        try {
            Memory memory;
            for (const auto& page : row.at("initial_pages")) {
                const auto address = page.at(0).get<unsigned>(), index = page.at(1).get<unsigned>();
                need(address % 4096 == 0 && index < images.size() &&
                     memory.pages.emplace(address, images[index]).second, "HS prediction page identity repeats or escapes its bank");
            }
            auto expected = memory;
            for (const auto& patch : row.at("expected_page_changes"))
                expected.write(patch.at(0), unhex(patch.at(1)));
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp = row.at("initial_cpu").at("gp").get<decltype(abi.gp)>();
            abi.xmm = row.at("initial_cpu").at("xmm").get<decltype(abi.xmm)>();
            abi.eflags = row.at("initial_cpu").at("flags");
            abi.x87_depth = 3;
            abi.x87_status = 0x1800;
            abi.x87[0] = .5L;
            abi.x87[1] = -1.L;
            abi.x87[2] = 2.L;
            abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
            const auto before = abi;
            unsigned calls = 0;
            std::vector<unsigned> trace, parsed;
            SourcePhysicsWorldRuntimeServices services;
            services.instruction = [&](unsigned pc) { trace.push_back(pc); };
            services.required_owner = [&](auto& bank, const auto& call, auto& state, const auto& observer) {
                if (call.target == 0x21f260u) {
                    need(call.instruction == 0x128a78u && !call.tail, "HS prediction changed the real cache call ABI");
                    ++calls;
                    request->run(bank, state, *state.floating_point, observer, services);
                } else if (source_workshop_script_builtins_tu15_owns_entry(call.target)) {
                    source_run_workshop_script_builtins_tu15(bank, call.target, state, {0x27f, 0x1f80}, services, observer);
                } else if (call.target == 0x212010u) {
                    source_run_ai_frame_tu15(bank, call.target, state, {0x27f, 0x1f80}, services, observer);
                    parsed.push_back(state.gp[0]);
                } else if (source_campaign_runtime_tu15_owns_entry(call.target)) {
                    source_run_campaign_runtime_tu15(bank, call.target, state, {0x27f, 0x1f80}, services, observer);
                } else if (source_ai_frame_tu15_owns_entry(call.target)) {
                    source_run_ai_frame_tu15(bank, call.target, state, {0x27f, 0x1f80}, services, observer);
                } else {
                    throw std::runtime_error("HS prediction reached an unowned native dependency: " + std::to_string(call.target));
                }
            };
            source_run_campaign_runtime_tu15(memory, 0x212470u, abi, {0x27f, 0x1f80}, services);
            need(abi.gp == row.at("initializer_cpu").at("gp").get<decltype(abi.gp)>(),
                 "HS prediction scalar initializer GP/RET differs");
            abi.gp = before.gp;
            abi.eflags = before.eflags;
            source_run_campaign_runtime_tu15(memory, 0x211b10u, abi, {0x27f, 0x1f80}, services);
            need(abi.gp == row.at("expected_cpu").at("gp").get<decltype(abi.gp)>(),
                 "HS prediction complete dispatcher/parser/effect/completion GP differs");
            need(((abi.eflags ^ row.at("expected_cpu").at("flags").get<unsigned>()) & 0x8c5u) == 0,
                 "HS prediction defined arithmetic flags differ");
            need(memory.pages == expected.pages, "HS prediction complete original ordinary/stack/metadata pages differ");
            // The hand-authored production cache kernel publishes its actual
            // semantic stores, but has no generated instruction callback.
            // Compare every generated HS/wrapper/effect instruction exactly.
            std::vector<unsigned> expected_trace;
            for (unsigned pc : row.at("trace").get<std::vector<unsigned>>())
                if (pc < 0x21f260u || pc > 0x21f35cu) expected_trace.push_back(pc);
            need(trace == expected_trace && calls == row.at("expected_cache_requests") &&
                 parsed == row.at("parser_results").get<std::vector<unsigned>>(),
                 "HS prediction original native path, deferred arguments or cache calls differ");
            need(abi.xmm == before.xmm && abi.x87_depth == before.x87_depth && abi.x87_status == before.x87_status &&
                 abi.x87[0] == before.x87[0] && abi.x87[1] == before.x87[1] && abi.x87[2] == before.x87[2] &&
                 abi.floating_point && abi.floating_point->x87_control_word == 0x27f &&
                 abi.floating_point->mxcsr == 0x1f80 && !abi.active_mmx,
                 "HS prediction changed live FP/SIMD state");
            ++completed;
            request_count += calls;
            parser_count += parsed.size();
            deferred_count += std::ranges::count(parsed, 0u);
        } catch (const std::exception& error) {
            throw std::runtime_error(row.at("name").get<std::string>() + ": " + error.what());
        }
    }
    need(completed == 11 && request_count == 86 && parser_count == 22 && deferred_count == 11,
         "HS prediction comparison is incomplete");
    std::cout << "PASS complete native HS looping prediction:11 cases,22 actual parsed/deferred wrappers,86 real cache requests,"
                 " full original pages/stack/GP/defined flags/FP/SIMD and generated instruction paths\n";
    std::cout << "LIMIT actual World/MCC resources/cold streaming/mission acceptance remain open\n";
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
