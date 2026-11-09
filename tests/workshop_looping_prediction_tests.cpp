#include "halo2/source_workshop_script_builtins_tu15.hpp"
#include "audio/native_tu15_sound_kernel.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

using namespace halo2;
using Json = nlohmann::json;

namespace {
void need(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::byte> unhex(const std::string& text) {
    need(text.size() % 2 == 0, "Incomplete original prediction bytes");
    std::vector<std::byte> result;
    for (std::size_t offset = 0; offset < text.size(); offset += 2)
        result.push_back(std::byte(std::stoul(text.substr(offset, 2), nullptr, 16)));
    return result;
}

struct Memory final : SourcePhysicsContactMemory {
    unsigned base;
    std::vector<std::byte> data;
    std::map<unsigned, std::vector<std::byte>> globals;

    Memory(unsigned address, unsigned size) : base(address), data(size) {}

    std::span<const std::byte> bytes(unsigned address, std::size_t size) const {
        if (address >= base && std::uint64_t(address) + size <= std::uint64_t(base) + data.size())
            return std::span(data).subspan(address - base, size);
        for (const auto& [first, values] : globals)
            if (address >= first && std::uint64_t(address) + size <= std::uint64_t(first) + values.size())
                return std::span(values).subspan(address - first, size);
        throw std::runtime_error("Prediction escaped its original fixture owner");
    }

    void read(unsigned address, std::span<std::byte> out) const override {
        const auto input = bytes(address, out.size());
        std::ranges::copy(input, out.begin());
    }

    void write(unsigned address, std::span<const std::byte> input) override {
        (void)bytes(address, input.size());
        if (address >= base && std::uint64_t(address) + input.size() <= std::uint64_t(base) + data.size()) {
            std::ranges::copy(input, data.begin() + (address - base));
            return;
        }
        for (auto& [first, values] : globals)
            if (address >= first && std::uint64_t(address) + input.size() <= std::uint64_t(first) + values.size()) {
                std::ranges::copy(input, values.begin() + (address - first));
                return;
            }
        throw std::runtime_error("Prediction store escaped its original fixture owner");
    }

    void patches(const Json& items) {
        for (const auto& item : items)
            write(base + item.at(0).get<unsigned>(), unhex(item.at(1)));
    }
};
}

int main(int argc, char** argv) try {
    need(argc == 2, "Supply unchanged original looping prediction reference");
    std::ifstream input(argv[1]);
    const auto reference = Json::parse(input);
    need(reference.at("xbe_sha256") == "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
         !reference.at("original_code_patched").get<bool>() &&
         !reference.at("original_calls_intercepted").get<bool>() &&
         !reference.at("supplied_callee_results").get<bool>() && reference.at("entry") == 0x18d1d0 &&
         reference.at("cases").size() == 11, "Prediction reference lost its original execution authority");
    need(source_workshop_script_builtins_tu15_owns_entry(0x18d1d0) &&
         source_workshop_script_builtins_tu15_owns_entry(0x128a20),
         "Complete native looping prediction owners are missing");
    const auto kernels = native_tu15_sound::resource_kernels();
    const auto request = std::ranges::find(kernels, 0x21f260u, &native_tu15_sound::Kernel::entry);
    need(request != kernels.end(), "Production native21F260 cache request is missing");
    unsigned cases = 0, requests = 0;
    for (const auto& row : reference.at("cases")) {
        try {
            Memory memory(reference.at("data_base"), reference.at("data_bytes"));
            for (const auto& global : reference.at("globals"))
                memory.globals.emplace(global.at(0).get<unsigned>(), unhex(global.at(1)));
            auto expected = memory;
            memory.patches(row.at("initial_patches"));
            expected.patches(row.at("expected_patches"));
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp = row.at("initial_gp").get<decltype(abi.gp)>();
            abi.xmm = row.at("initial_xmm").get<decltype(abi.xmm)>();
            abi.eflags = row.at("initial_flags");
            abi.x87_depth = 3;
            abi.x87_status = 0x1800;
            abi.x87[0] = .5L;
            abi.x87[1] = -1.L;
            abi.x87[2] = 2.L;
            abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
            const auto before = abi;
            unsigned calls = 0;
            std::vector<unsigned> trace;
            SourcePhysicsWorldRuntimeServices services;
            services.instruction = [&](unsigned pc) { trace.push_back(pc); };
            services.required_owner = [&](auto& bank, const auto& call, auto& state, const auto& observer) {
                need(call.instruction == 0x128a78 && call.target == 0x21f260 && !call.tail,
                     "Prediction reached an unqualified allocation or streaming owner");
                ++calls;
                request->run(bank, state, *state.floating_point, observer, services);
            };
            source_run_workshop_script_builtins_tu15(memory, 0x18d1d0, abi, {0x27f, 0x1f80}, services);
            need(abi.gp == row.at("expected_gp").get<decltype(abi.gp)>(), "Original prediction GP/RET differs");
            need(((abi.eflags ^ row.at("expected_flags").get<unsigned>()) &
                  row.at("defined_flags_mask").get<unsigned>()) == 0, "Original prediction defined flags differ");
            if (memory.data != expected.data || memory.globals != expected.globals) {
                Json differences = Json::array();
                for (unsigned offset = 0; offset < memory.data.size() && differences.size() < 16; ++offset)
                    if (memory.data[offset] != expected.data[offset])
                        differences.push_back({{"address", memory.base + offset},
                            {"native", std::to_integer<unsigned>(memory.data[offset])},
                            {"original", std::to_integer<unsigned>(expected.data[offset])}});
                throw std::runtime_error("Original prediction complete bank/stack differences: " + differences.dump());
            }
            need(trace == row.at("generated_effect_trace").get<std::vector<unsigned>>() &&
                 calls == row.at("genuine_request_calls"), "Original prediction path or cache call count differs");
            need(abi.xmm == before.xmm && abi.x87_depth == before.x87_depth && abi.x87_status == before.x87_status &&
                 abi.x87[0] == before.x87[0] && abi.x87[1] == before.x87[1] && abi.x87[2] == before.x87[2] &&
                 abi.floating_point && abi.floating_point->x87_control_word == 0x27f &&
                 abi.floating_point->mxcsr == 0x1f80 && !abi.active_mmx,
                 "Prediction changed live caller FP/SIMD state");
            ++cases;
            requests += calls;
        } catch (const std::exception& error) {
            throw std::runtime_error(row.at("name").get<std::string>() + ": " + error.what());
        }
    }
    need(cases == 11 && requests == 86, "Original prediction comparison is incomplete");
    std::cout << "PASS native looping prediction:11 cases,56 complete effect sites,86 production cache calls,"
                 " complete banks/stack/GP/defined flags and live FP/SIMD\n";
    std::cout << "LIMIT cold allocation/streaming, MCC projection, HS caller composition and mission flow remain open\n";
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
