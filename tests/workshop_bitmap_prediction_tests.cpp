#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_campaign_runtime_tu15.hpp"
#include "halo2/source_workshop_script_builtins_tu15.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

using namespace halo2;
using Json = nlohmann::json;

namespace {
constexpr unsigned effect = 0x2aa200;
constexpr unsigned lookup = 0x12f800;
constexpr unsigned request = 0x12f910;
constexpr unsigned initialize = 0x212470;
constexpr unsigned schedule = 0x211b10;
constexpr unsigned evaluator = 0x2ac680;
constexpr unsigned parse = 0x212010;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Memory final : SourcePhysicsContactMemory {
    std::map<unsigned, std::byte> bytes;
    Json stores = Json::array();
    unsigned pc{};

    explicit Memory(const Json& banks) {
        for (const auto& bank : banks) {
            unsigned address = bank.at(0);
            const auto text = bank.at(1).get<std::string>();
            require(text.size() % 2 == 0, "Incomplete bitmap prediction bank");
            for (std::size_t index = 0; index < text.size(); index += 2)
                require(bytes.emplace(address++, std::byte(std::stoul(text.substr(index, 2), nullptr, 16))).second,
                        "Overlapping bitmap prediction banks");
        }
    }

    void read(unsigned address, std::span<std::byte> output) const override {
        for (auto& byte : output) byte = bytes.at(address++);
    }

    void write(unsigned address, std::span<const std::byte> input) override {
        require(input.size() <= 4, "Unexpected bitmap prediction store width");
        unsigned value{};
        for (std::size_t index = 0; index < input.size(); ++index) {
            bytes.at(address + index) = input[index];
            value |= std::to_integer<unsigned>(input[index]) << (index * 8);
        }
        stores.push_back({pc, address, input.size(), value});
    }
};

void compare(const Json& row, unsigned entry) {
    Memory memory(row.at("initial_banks"));
    const Memory expected(row.at("expected_banks"));
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = row.at("initial_gp").get<decltype(abi.gp)>();
    abi.xmm = row.at("initial_xmm").get<decltype(abi.xmm)>();
    abi.eflags = row.at("initial_flags");
    constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f, 0x1f80};
    abi.floating_point = fp;
    const auto initial = abi;
    std::vector<unsigned> trace, parser_results;
    unsigned lookups{}, requests{};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](unsigned pc) {
        memory.pc = pc;
        trace.push_back(pc);
    };
    services.required_owner = [&](auto& bank, const auto& call, auto& state, const auto& observer) {
        const auto controls = *state.floating_point;
        if ((call.instruction == 0x2aa26a && call.target == lookup && !call.tail) ||
            (call.instruction == 0x2aa279 && call.target == request && !call.tail)) {
            if (call.target == lookup) ++lookups;
            else ++requests;
            source_run_physics_live_support_tu15(bank, call.target, state, controls, services, observer);
        } else if (entry == schedule && source_workshop_script_builtins_tu15_owns_entry(call.target)) {
            source_run_workshop_script_builtins_tu15(bank, call.target, state, controls, services, observer);
        } else if (entry == schedule && source_campaign_runtime_tu15_owns_entry(call.target)) {
            source_run_campaign_runtime_tu15(bank, call.target, state, controls, services, observer);
        } else if (entry == schedule && call.target == parse && call.instruction == 0x2ac6a0 && !call.tail) {
            source_run_ai_frame_tu15(bank, call.target, state, controls, services, observer);
            parser_results.push_back(state.gp[0]);
        } else if (entry == schedule && source_ai_frame_tu15_owns_entry(call.target)) {
            source_run_ai_frame_tu15(bank, call.target, state, controls, services, observer);
        } else {
            throw std::runtime_error("Bitmap prediction reached an undeclared dependency " +
                                     std::to_string(call.target) + " at " + std::to_string(call.instruction));
        }
    };
    if (entry == schedule) {
        source_run_campaign_runtime_tu15(memory, initialize, abi, fp, services);
        require(abi.gp == row.at("initialized_gp").get<decltype(abi.gp)>(), "Script initializer GP/RET differs");
        abi.gp = initial.gp;
        abi.eflags = initial.eflags;
        source_run_campaign_runtime_tu15(memory, entry, abi, fp, services);
        require(parser_results == row.at("parser_results").get<std::vector<unsigned>>(),
                "Bitmap prediction deferred argument or parser result differs");
    } else {
        source_run_workshop_script_builtins_tu15(memory, entry, abi, fp, services);
    }
    require(abi.gp == row.at("expected_gp").get<decltype(abi.gp)>(), "Bitmap prediction register/return differs");
    require(((abi.eflags ^ row.at("expected_flags").get<unsigned>()) &
             row.at("defined_flags").get<unsigned>()) == 0, "Bitmap prediction defined flags differ");
    require(abi.xmm == row.at("expected_xmm").get<decltype(abi.xmm)>(), "Bitmap prediction SIMD differs");
    require(abi.x87_depth == 0 && abi.x87_status == 0 && !abi.active_mmx && abi.floating_point &&
            abi.floating_point->x87_control_word == fp.x87_control_word &&
            abi.floating_point->mxcsr == fp.mxcsr, "Bitmap prediction floating point state differs");
    require(memory.bytes == expected.bytes && memory.stores == row.at("stores"),
            "Bitmap prediction ordered stores or final memory differs");
    require(trace == row.at("trace").get<std::vector<unsigned>>() && lookups == row.at("lookups") &&
            requests == row.at("requests"), "Bitmap prediction instruction or cache call path differs");
}
}

int main(int argc, char** argv) try {
    require(argc == 2, "Supply the original bitmap prediction reference");
    std::ifstream input(argv[1]);
    const auto reference = Json::parse(input);
    const unsigned entry = reference.at("entry");
    require(reference.at("xbe_sha256") == "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
            (entry == effect || entry == schedule), "Bitmap prediction reference lost original authority");
    require(source_workshop_script_builtins_tu15_owns_entry(effect) &&
            source_workshop_script_builtins_tu15_owns_entry(evaluator) &&
            source_physics_live_support_tu15_owns_entry(lookup) &&
            source_physics_live_support_tu15_owns_entry(request), "Native bitmap prediction owners are missing");
    for (const auto& row : reference.at("cases")) {
        try {
            compare(row, entry);
        } catch (const std::exception& error) {
            throw std::runtime_error(row.at("name").get<std::string>() + ": " + error.what());
        }
    }
    std::cout << "Native bitmap prediction matched " << reference.at("cases").size() << " original cases\n";
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
