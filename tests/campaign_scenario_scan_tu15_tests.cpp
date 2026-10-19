#include "halo2/source_campaign_activation_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <xmmintrin.h>

namespace {
using namespace halo2;
using Json = nlohmann::json;
void check(bool value, const std::string &why) {
    if (!value) throw std::runtime_error(why);
}
std::vector<std::byte> bytes(const std::string &hex) {
    check(hex.size()%2 == 0, "Odd fixture hex");
    std::vector<std::byte> result;
    for (std::size_t n=0; n<hex.size(); n+=2)
        result.push_back(std::byte(std::stoul(hex.substr(n, 2), nullptr, 16)));
    return result;
}
Json read(const std::filesystem::path &path) {
    std::ifstream input(path);
    check(bool(input), "Required original fixture unavailable");
    Json result;
    input >> result;
    return result;
}
struct Memory : SourcePhysicsContactMemory {
    std::map<std::uint32_t, std::vector<std::byte>> banks;
    auto locate(std::uint32_t at, std::size_t size) const {
        auto it=banks.upper_bound(at);
        check(it != banks.begin(), "Undeclared source bank");
        --it;
        const auto offset=std::uint64_t(at)-it->first;
        check(offset <= it->second.size() && size <= it->second.size()-offset, "Source bank extent escaped");
        return it;
    }
    void read(std::uint32_t at, std::span<std::byte> out) const override {
        const auto it=locate(at, out.size());
        std::copy_n(it->second.begin()+(at-it->first), out.size(), out.begin());
    }
    void write(std::uint32_t at, std::span<const std::byte> in) override {
        const auto it=locate(at, in.size());
        std::copy(in.begin(), in.end(), banks.at(it->first).begin()+(at-it->first));
    }
    std::uint32_t word(std::uint32_t at) const {
        std::array<std::byte,4> data;
        read(at, data);
        std::uint32_t value{};
        for (unsigned n=0; n<4; ++n) value |= std::to_integer<std::uint32_t>(data[n])<<(8*n);
        return value;
    }
};
void abi_equal(const SourcePhysicsWorldRuntimeAbi &state, const Json &expected) {
    check(state.gp == expected.at("gp").get<decltype(state.gp)>(), "General registers differ");
    check(state.xmm == expected.at("xmm").get<decltype(state.xmm)>(), "XMM registers differ");
    check((state.eflags&0xcc5u) == (expected.at("eflags").get<unsigned>()&0xcc5u), "Defined flags differ");
    check(state.fs_base==0 && state.x87_depth==0 && state.x87_status==0 && !state.active_mmx,
          "Retained empty FP/MMX/FS state differs");
    check(state.floating_point && state.floating_point->x87_control_word==0x27f &&
          state.floating_point->mxcsr==0x1f80, "Source FP controls differ");
}
struct MissingOwner { std::uint32_t target; };
struct Call { std::uint32_t instruction, returned, cleanup; };
const std::map<std::uint32_t, Call> calls{
    {0xd7f30,{0xd7b11,0xd7b16,0}}, {0xc1790,{0xd7b25,0xd7b2a,0}},
    {0xc1120,{0xd7b48,0xd7b4d,20}}, {0xc13b0,{0xd7b4d,0xd7b52,0}}};
}

int main(int argc, char **argv) try {
    check(argc==2 || (argc==3 && std::string(argv[2])=="--baseline-missing"),
          "Supply original fixture directory [--baseline-missing]");
    const bool baseline=argc==3;
    const std::filesystem::path root(argv[1]);
    if (!std::filesystem::is_regular_file(root/"report.json")) return 77;
    const auto report=read(root/"report.json");
    check(report.at("status")=="PASS_UNCHANGED_ORIGINAL_SCENARIO_SCAN_WITH_DECLARED_EXTERNAL_FIXTURES" &&
          report.at("suite")=="campaign-scenario-scan" && report.at("entry")==0xd7aa0 &&
          report.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Wrong original scan authority");
    check(report.at("tested_owner_instruction_patches").empty() && report.at("full_startup_proven")==false,
          "Invalid external-fixture scope");
    const auto pcs=report.at("static_original_PCs").get<std::set<std::uint32_t>>();
    check(pcs.size()==72 && pcs.contains(0xd7aa0) && pcs.contains(0xd7b7c) && report.at("cases").size()==17,
          "Wrong original static/case coverage");
    unsigned comparisons{}, external_calls{}, stops{}, factories{}, stores_count{};
    std::set<std::uint32_t> covered;
    for (const auto &item : report.at("cases")) {
        const auto name=item.at("name").get<std::string>();
        check(item.at("file")==name+".json" && name.find('/')==std::string::npos, "Fixture path changed");
        const auto row=read(root/(name+".json"));
        check(row.at("entry")==0xd7aa0 && row.at("name")==name, "Wrong fixture identity");
        for (bool observed : {false, true}) {
            Memory memory;
            for (const auto &[at, data] : row.at("before").items())
                memory.banks.emplace(std::stoul(at,nullptr,16), bytes(data.get<std::string>()));
            const auto before=memory.banks;
            SourcePhysicsWorldRuntimeAbi state{};
            state.gp=row.at("gp").get<decltype(state.gp)>();
            state.xmm=row.at("xmm").get<decltype(state.xmm)>();
            state.eflags=row.at("eflags");
            state.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            std::vector<std::uint32_t> trace;
            std::vector<SourcePhysicsContactRecordStore> stores;
            SourcePhysicsContactRecordObserver observer;
            if (observed) observer=[&](const auto &store){ stores.push_back(store); };
            SourcePhysicsWorldRuntimeServices services;
            services.instruction=[&](std::uint32_t pc){
                check(pcs.contains(pc), "Executed unexpected scan instruction");
                trace.push_back(pc);
            };
            std::size_t call_index{};
            bool stopped{};
            services.required_owner=[&](auto &bank, const auto &call, auto &abi, const auto &) {
                check(&bank==&memory && !call.tail && calls.contains(call.target), "Wrong external owner");
                check(call_index < row.at("calls").size(), "Unexpected extra scan callee");
                const auto &expected=row.at("calls").at(call_index++);
                const auto &site=calls.at(call.target);
                check(call.target==expected.at("target") && call.instruction==site.instruction &&
                      memory.word(abi.gp[4])==site.returned, "Scan call/return chronology differs");
                abi_equal(abi, expected.at("abi"));
                const auto stack=bytes(expected.at("abi").at("stack"));
                std::vector<std::byte> actual_stack(stack.size());
                memory.read(abi.gp[4], actual_stack);
                check(actual_stack==stack, "Original scan stack arguments differ");
                if (call.target==0xc1120) {
                    const auto type=memory.word(abi.gp[4]+4), ordinal=memory.word(abi.gp[4]+8);
                    check(type<13 && ordinal<3 && memory.word(abi.gp[4]+16)==1 &&
                          memory.word(abi.gp[4]+20)==1, "Automatic birth lost original type/ordinal/1/1 operands");
                    ++factories;
                }
                if (expected.at("unavailable")) throw MissingOwner{call.target};
                const auto &fixture=row.at("fixtures").at(std::to_string(call.target));
                check(fixture.at("cleanup")==site.cleanup, "External test RET cleanup differs");
                std::vector<std::byte> code{std::byte{0xb8}};
                const auto result=fixture.at("result").template get<std::uint32_t>();
                for (unsigned n=0;n<4;++n) code.push_back(std::byte(result>>(8*n)));
                if (site.cleanup) {
                    code.push_back(std::byte{0xc2}); code.push_back(std::byte(site.cleanup)); code.push_back(std::byte{});
                } else code.push_back(std::byte{0xc3});
                check(code==bytes(fixture.at("code_hex")), "External fixture code changed");
                abi.gp[0]=result;
                abi.gp[4]+=4+site.cleanup;
            };
            if (baseline) {
                check(!source_campaign_activation_tu15_owns_entry(0xd7aa0), "Baseline already owns scan");
                bool rejected{};
                try { source_run_campaign_activation_tu15(memory,0xd7aa0,state,{0x27f,0x1f80},services,observer); }
                catch (const std::runtime_error &error) {
                    rejected=std::string(error.what())=="Required original contact entry unavailable";
                }
                check(rejected && trace.empty() && stores.empty() && memory.banks==before, "Missing owner executed or mutated state");
                abi_equal(state, row);
                std::cout << "PASS baseline rejects missing original D7AA0 before execution\n";
                return 0;
            }
            check(source_campaign_activation_tu15_owns_entry(0xd7aa0), "Missing original automatic scan");
            std::uint16_t before_cw{},after_cw{};
            asm volatile("fnstcw %0" : "=m"(before_cw));
            const auto before_mxcsr=_mm_getcsr();
            try { source_run_campaign_activation_tu15(memory,0xd7aa0,state,{0x27f,0x1f80},services,observer); }
            catch (const MissingOwner &missing) {
                check(row.at("unavailable")==missing.target, "Wrong missing-owner boundary");
                stopped=true;
            }
            asm volatile("fnstcw %0" : "=m"(after_cw));
            check(before_cw==after_cw && before_mxcsr==_mm_getcsr(), "Host FP state escaped");
            check(stopped==!row.at("unavailable").is_null() && call_index==row.at("calls").size(), "Wrong scan stop/call count");
            abi_equal(state, row.at("final"));
            for (const auto &[at,data] : row.at("after").items())
                check(memory.banks.at(std::stoul(at,nullptr,16))==bytes(data), "Complete scan memory bank differs");
            check(trace==row.at("trace").get<std::vector<std::uint32_t>>(), "Original instruction order differs");
            if (observed) {
                check(stores.size()==row.at("stores").size(), "Original scan store count differs");
                for (std::size_t n=0;n<stores.size();++n) {
                    const auto &a=stores[n]; const auto &e=row.at("stores").at(n);
                    check(a.instruction==e.at("pc") && a.address==e.at("address") &&
                          a.before==bytes(e.at("before")) && a.written==bytes(e.at("written")), "Ordered original scan store differs");
                }
                stores_count+=stores.size();
            }
            covered.insert(trace.begin(),trace.end());
            external_calls+=call_index; stops+=stopped; ++comparisons;
        }
    }
    check(comparisons==34 && external_calls==208 && stops==8 && factories>0 && covered==pcs,
          "Original differential branch coverage lost");
    std::cout << "PASS34 original/native automatic-scan comparisons,208 declared external calls,8 stops,"
              << stores_count << " ordered stores; all72 original instructions\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
