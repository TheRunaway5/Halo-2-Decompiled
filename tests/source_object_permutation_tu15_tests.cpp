#include "halo2/source_object_functions_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

namespace {
using namespace halo2;
using Json = nlohmann::json;

void check(bool yes, const char* reason) {
    if (!yes) throw std::runtime_error(reason);
}

std::vector<std::byte> bytes(const std::string& text) {
    check(text.size() % 2 == 0, "Incomplete original byte encoding");
    std::vector<std::byte> result;
    for (std::size_t n = 0; n < text.size(); n += 2)
        result.push_back(std::byte(std::stoul(text.substr(n, 2), nullptr, 16)));
    return result;
}

struct Memory final : SourcePhysicsContactMemory {
    std::map<std::uint32_t, std::vector<std::byte>> banks;
    auto find(std::uint32_t address) const {
        auto owner = banks.upper_bound(address);
        check(owner != banks.begin(), "Missing original permutation memory owner");
        --owner;
        check(address - owner->first < owner->second.size(), "Original permutation memory extent differs");
        return owner;
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        for (auto& value : out) {
            const auto owner = find(address);
            value = owner->second[address++ - owner->first];
        }
    }
    void write(std::uint32_t address, std::span<const std::byte> in) override {
        for (auto value : in) {
            const auto owner = find(address);
            check(owner->first == 0x60000000, "Lookup/no-model path wrote outside its original stack");
            banks.at(owner->first)[address++ - owner->first] = value;
        }
    }
};

void check_original_source(const Json& source) {
    check(source.size() == 3, "Incomplete original permutation source roster");
    check(source.at("170030").at("instructions") == 27 &&
          source.at("170030").at("bytes") ==
          "83c8ff83f9ff743985ff74358b1514044f0081e1ffff0000c1e1048b4c1108568b717033d285f67e178b49748d6424003939740a4283c1103bd67cf45ec38bc25ec3",
          "Wrong complete original region lookup instructions");
    check(source.at("170080").at("instructions") == 32 &&
          source.at("170080").at("bytes") ==
          "83c8ff83faff744983f9ff744485ff744081e2ffff0000568b3514044f00c1e2048b5432088b7274c1e10403ce8b710833d285f67e1a8b490c8da424000000003939740a4283c1083bd67cf45ec38bc25ec3",
          "Wrong complete original permutation lookup instructions");
    check(source.at("bc440").at("instructions") == 93 &&
          source.at("bc440").at("bytes") ==
          "83ec208b4424248b0dd0cb4e008b51448b0d14044f0025ffff00008d04408d048289442404558b68088b450025ffff0000c1e0048b4408088b483883f9ff894424100f84dc0000005356578b7c2438e89c3b0b000fbf8d180100000fbfb51a01000089442418b867666666f7e98b442414c1fa028bdac1eb1f03da03f58bcb8bd1c1e902897424108d7c2420f3a58bca83e103f3a480780306751966837d1aff7412f6852c0100000275098b442434e84c1e110033ed85db7e528b4c24108d7459023b6c241874088b44243885c075348b54241c8b7c243c8b52388bcde85e3b0b0083f8ff750485ff75198b4c2410880429c646feffc646ff00c60600c74602ffffffff4583c6083beb7cb68b7424348bc6e8091b0c00568d442424e87f1b11005f5e5b5d83c420c20c00",
          "Wrong complete original object permutation instructions");
}
}

int main(int argc, char** argv) try {
    if (argc != 2 || !std::filesystem::exists(argv[1])) return 77;
    Json fixture;
    std::ifstream(argv[1]) >> fixture;
    check(fixture.at("reference") == "English TU1.5 only" &&
          fixture.at("original_sha256") == "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Wrong original permutation reference");
    check_original_source(fixture.at("source"));
    for (auto entry : {0x170030u, 0x170080u, 0xbc440u})
        check(source_object_functions_tu15_owns_entry(entry), "Missing complete original permutation routine owner");
    std::map<std::uint32_t, unsigned> cases;
    std::set<std::string> names;
    for (const auto& capture : fixture.at("cases")) {
        const auto name = capture.at("name").get<std::string>();
        check(names.insert(name).second, "Duplicate original permutation case");
        try {
            Memory memory;
            for (const auto& region : capture.at("regions")) {
                const auto address = region.at("address").get<std::uint32_t>();
                check(memory.banks.emplace(address, bytes(region.at("before"))).second, "Duplicate namespace memory owner");
            }
            const auto& before = capture.at("input");
            const auto& expected = capture.at("original");
            const auto entry = capture.at("entry").get<std::uint32_t>();
            check(entry == 0x170030 || entry == 0x170080 || entry == 0xbc440, "Unowned fixture entry");
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp = before.at("gp").get<decltype(abi.gp)>();
            abi.xmm = before.at("xmm").get<decltype(abi.xmm)>();
            abi.eflags = before.at("eflags");
            abi.fs_base = before.at("fs_base");
            abi.x87_status = before.at("x87_status");
            abi.x87_depth = before.at("x87_values").size();
            for (unsigned n = 0; n < abi.x87_depth; ++n) {
                const auto raw = bytes(before.at("x87_values").at(n));
                check(raw.size() == 10, "Original live x87 encoding differs");
                std::memcpy(&abi.x87[n], raw.data(), 10);
            }
            abi.floating_point = SourcePhysicsFactoryFloatingPoint{
                before.at("x87_control_word").get<std::uint16_t>(), before.at("mxcsr").get<unsigned>()};
            std::vector<unsigned> trace;
            SourcePhysicsWorldRuntimeServices services;
            services.instruction = [&](auto pc) { trace.push_back(pc); };
            services.required_owner = [](auto&, const auto&, auto&, const auto&) {
                throw std::runtime_error("Lookup/no-model path unexpectedly called an external owner");
            };
            unsigned stores = 0;
            source_run_object_functions_tu15(memory, entry, abi, *abi.floating_point, services,
                [&](const auto& write) {
                    check(write.address >= 0x60000000 && write.address + write.written.size() <= 0x60002000,
                          "Lookup/no-model observer saw a nonstack store");
                    ++stores;
                });
            check(abi.gp == expected.at("gp").get<decltype(abi.gp)>(), "Original GP registers/RET stack differ");
            check(abi.gp[0] == capture.at("expected_eax"), "Original lookup result differs");
            check(abi.xmm == expected.at("xmm").get<decltype(abi.xmm)>(), "Original SSE state differs");
            const auto mask = capture.at("eflags_mask").get<unsigned>();
            check(mask == 0xfffffdff, "Unexpected original integer-flag exclusion");
            check((abi.eflags & mask) == (expected.at("eflags").get<unsigned>() & mask), "Original integer flags differ");
            check(abi.fs_base == before.at("fs_base").get<unsigned>(), "Original FS base changed");
            check(abi.x87_depth == expected.at("x87_values").size() &&
                  abi.x87_status == expected.at("x87_status").get<std::uint16_t>(), "Original x87 depth/status differ");
            for (unsigned n = 0; n < abi.x87_depth; ++n) {
                const auto raw = bytes(expected.at("x87_values").at(n));
                check(!std::memcmp(&abi.x87[n], raw.data(), 10), "Original live x87 value changed");
            }
            check(abi.floating_point &&
                  abi.floating_point->x87_control_word == expected.at("x87_control_word").get<std::uint16_t>() &&
                  abi.floating_point->mxcsr == expected.at("mxcsr").get<unsigned>(), "Original FP controls differ");
            check(trace == capture.at("instructions").get<decltype(trace)>(), "Original complete instruction trace differs");
            check(stores == (entry == 0xbc440 ? 3u :
                  std::count(trace.begin(), trace.end(), entry == 0x170030 ? 0x17004fu : 0x170097u)),
                  "Original stack-store count differs");
            for (const auto& region : capture.at("regions"))
                check(memory.banks.at(region.at("address").get<unsigned>()) == bytes(region.at("after")),
                      "Original full namespace/stack memory differs");
            ++cases[entry];
        } catch (const std::exception& error) {
            throw std::runtime_error(name + ": " + error.what());
        }
    }
    check(cases[0x170030] == 15 && cases[0x170080] == 16 && cases[0xbc440] == 4,
          "Incomplete original permutation fixture roster");
    std::cout << "PASS 35 original permutation cases: full GP/SSE/x87/flags/FP controls, instruction paths and namespace/stack bytes\n";
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
