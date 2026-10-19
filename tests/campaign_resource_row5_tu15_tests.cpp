#include "halo2/source_campaign_activation_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <xmmintrin.h>

namespace {
using namespace halo2;
using Json = nlohmann::json;
void check(bool value, const std::string &why) {
    if (!value) throw std::runtime_error(why);
}
std::vector<std::byte> bytes(const std::string &text) {
    check(text.size() % 2 == 0, "Odd fixture hex");
    std::vector<std::byte> output;
    for (std::size_t n = 0; n < text.size(); n += 2)
        output.push_back(std::byte(std::stoul(text.substr(n, 2), nullptr, 16)));
    return output;
}
Json read(const std::filesystem::path &path) {
    std::ifstream input(path);
    check(bool(input), "Required present fixture unavailable");
    Json value;
    input >> value;
    return value;
}
struct Memory : SourcePhysicsContactMemory {
    std::map<std::uint32_t, std::vector<std::byte>> banks;
    auto locate(std::uint32_t address, std::size_t size) const {
        auto it = banks.upper_bound(address);
        check(it != banks.begin(), "Undeclared bank");
        --it;
        const auto offset = std::uint64_t(address) - it->first;
        check(offset <= it->second.size() && size <= it->second.size() - offset,
              "Declared bank extent escaped");
        return it;
    }
    void read(std::uint32_t address, std::span<std::byte> value) const override {
        const auto it = locate(address, value.size());
        std::copy_n(it->second.begin() + (address-it->first), value.size(), value.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> value) override {
        const auto it = locate(address, value.size());
        std::copy(value.begin(), value.end(), banks.at(it->first).begin() + (address-it->first));
    }
};
void abi_equal(const SourcePhysicsWorldRuntimeAbi &state, const Json &expected) {
    check(state.gp == expected.at("gp").get<decltype(state.gp)>(), "General registers differ");
    check(state.xmm == expected.at("xmm").get<decltype(state.xmm)>(), "XMM registers differ");
    check((state.eflags & 0xcc5u) == (expected.at("eflags").get<unsigned>() & 0xcc5u),
          "Defined flags differ");
    check(state.fs_base == 0 && state.x87_depth == 0 && state.x87_status == 0 && !state.active_mmx,
          "Retained empty FP/MMX/FS state differs");
    check(state.floating_point && state.floating_point->x87_control_word == 0x27f &&
          state.floating_point->mxcsr == 0x1f80, "Source FP controls differ");
}
}

int main(int argc, char **argv) try {
    check(argc == 2 || (argc == 3 && std::string(argv[2]) == "--baseline-missing"),
          "Supply fixture directory [--baseline-missing]");
    const bool baseline = argc == 3;
    const std::filesystem::path root(argv[1]);
    if (!std::filesystem::is_regular_file(root / "report.json")) return 77;
    const auto report = read(root / "report.json");
    check(report.at("status") == "PASS_UNCHANGED_ORIGINAL_ROW5_RELEASE_NO_EXTERNAL_FIXTURES" &&
          report.at("suite") == "campaign-resource-row5-release" && report.at("entry") == 0x2255f0 &&
          report.at("xbe_sha256") == "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
          report.at("raw_body_hex") == "a1fcb8480085c0740ac705fcb8480000000000c3",
          "Wrong original owner/authority");
    check(report.at("external_fixture_owners").empty() && report.at("tested_owner_instruction_patches").empty() &&
          report.at("startup_resource_lifecycle_proven") == false, "Invalid original fixture scope");
    const std::set<std::uint32_t> pcs{0x2255f0, 0x2255f5, 0x2255f7, 0x2255f9, 0x225603};
    check(report.at("static_original_PCs").get<std::set<std::uint32_t>>() == pcs &&
          report.at("cases").size() == 16, "Original static/case coverage differs");
    unsigned comparisons = 0, stores_count = 0, branch_taken = 0, branch_clear = 0;
    for (std::uint32_t word : {0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 0x12345678u, 0xaaaaaaaau, 0x55555555u}) {
        for (unsigned repetition = 0; repetition < 2; ++repetition) {
            std::ostringstream name;
            name << "word-" << std::hex << std::setfill('0') << std::setw(8) << word
                 << "-call-" << std::dec << repetition+1;
            const auto &item = report.at("cases").at(comparisons / 2);
            check(item.at("name") == name.str() && item.at("file") == name.str()+".json",
                  "Original case name/order differs");
            const auto row = read(root / (name.str()+".json"));
            check(row.at("entry") == 0x2255f0 && row.at("initial_word") == word &&
                  row.at("repetition") == repetition && row.at("name") == name.str() &&
                  row.at("final_pc") == 0x300f000, "Wrong original case/return");
            for (bool observed : {false, true}) {
                Memory memory;
                for (const auto &[address, data] : row.at("before").items())
                    memory.banks.emplace(std::stoul(address, nullptr, 16), bytes(data));
                check(memory.banks.size() == 2 && memory.banks.at(0x48b8e8).size() == 64 &&
                      memory.banks.at(0x4000000).size() == 0x1000, "Wrong declared memory banks");
                const auto before = memory.banks;
                SourcePhysicsWorldRuntimeAbi state{};
                state.gp = row.at("gp").get<decltype(state.gp)>();
                state.xmm = row.at("xmm").get<decltype(state.xmm)>();
                state.eflags = row.at("eflags");
                state.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
                SourcePhysicsWorldRuntimeServices services;
                std::vector<std::uint32_t> trace;
                std::vector<SourcePhysicsContactRecordStore> stores;
                services.instruction = [&](std::uint32_t pc) {
                    check(pcs.contains(pc), "Executed unexpected owner");
                    trace.push_back(pc);
                };
                services.required_owner = [&](auto &, const auto &, auto &, const auto &) {
                    throw std::runtime_error("Unexpected external owner");
                };
                SourcePhysicsContactRecordObserver observer;
                if (observed) observer = [&](const auto &store) { stores.push_back(store); };
                if (baseline) {
                    check(!source_campaign_activation_tu15_owns_entry(0x2255f0), "Baseline owns new entry");
                    bool rejected = false;
                    try { source_run_campaign_activation_tu15(memory, 0x2255f0, state, {0x27f, 0x1f80}, services, observer); }
                    catch (const std::runtime_error &error) {
                        rejected = std::string(error.what()) == "Required original contact entry unavailable";
                    }
                    check(rejected && trace.empty() && stores.empty() && memory.banks == before,
                          "Baseline rejection mutated memory or executed source");
                    abi_equal(state, Json{{"gp", row.at("gp")}, {"xmm", row.at("xmm")}, {"eflags", row.at("eflags")}});
                    std::cout << "PASS baseline rejects missing2255F0 before execution\n";
                    return 0;
                }
                check(source_campaign_activation_tu15_owns_entry(0x2255f0), "Missing original2255F0 owner");
                std::uint16_t before_cw;
                asm volatile("fnstcw %0" : "=m"(before_cw));
                const auto before_mxcsr = _mm_getcsr();
                source_run_campaign_activation_tu15(memory, 0x2255f0, state, {0x27f, 0x1f80}, services, observer);
                std::uint16_t after_cw;
                asm volatile("fnstcw %0" : "=m"(after_cw));
                check(before_cw == after_cw && before_mxcsr == _mm_getcsr(), "Host FP controls escaped");
                abi_equal(state, row.at("final"));
                for (const auto &[address, data] : row.at("after").items())
                    check(memory.banks.at(std::stoul(address, nullptr, 16)) == bytes(data), "Complete original bank differs");
                check(trace == row.at("trace").get<std::vector<std::uint32_t>>(), "Dynamic original PC sequence differs");
                const bool clears = word != 0 && repetition == 0;
                check((std::find(trace.begin(), trace.end(), 0x2255f9) != trace.end()) == clears,
                      "Conditional clear branch differs");
                if (observed) {
                    check(stores.size() == row.at("stores").size() && stores.size() == unsigned(clears),
                          "Original store count differs");
                    for (std::size_t n = 0; n < stores.size(); ++n) {
                        const auto &actual = stores[n];
                        const auto &expected = row.at("stores").at(n);
                        check(actual.instruction == expected.at("pc") && actual.address == expected.at("address") &&
                              actual.before == bytes(expected.at("before")) && actual.written == bytes(expected.at("written")),
                              "Ordered original store differs");
                    }
                    stores_count += stores.size();
                }
                if (clears) ++branch_clear; else ++branch_taken;
                ++comparisons;
            }
        }
    }
    check(comparisons == 32 && stores_count == 7 && branch_clear == 14 && branch_taken == 18,
          "Original differential branch coverage lost");
    std::cout << "PASS32 original/native row5 comparisons,7 ordered stores,14 clear and18 skip paths\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
