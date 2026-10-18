#include "halo2/source_campaign_activation_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <xmmintrin.h>
namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool v,const std::string &why){if(!v)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string&s){check(s.size()%2==0,"Odd hex fixture");std::vector<std::byte> out;for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return out;}
Json read(const std::filesystem::path&p){std::ifstream f(p);check(bool(f),"Required present fixture is unavailable");Json j;f>>j;return j;}
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>> banks;
 auto locate(std::uint32_t a,std::size_t n)const{auto it=banks.upper_bound(a);check(it!=banks.begin(),"Undeclared fixture owner");--it;const auto o=std::uint64_t(a)-it->first;check(o<=it->second.size()&&n<=it->second.size()-o,"Fixture owner extent escaped");return it;}
 void read(std::uint32_t a,std::span<std::byte>v)const override{auto it=locate(a,v.size());std::copy_n(it->second.begin()+(a-it->first),v.size(),v.begin());}
 void write(std::uint32_t a,std::span<const std::byte>v)override{auto it=locate(a,v.size());std::copy(v.begin(),v.end(),banks.at(it->first).begin()+(a-it->first));}
 std::uint32_t word(std::uint32_t a)const{std::array<std::byte,4>b;read(a,b);std::uint32_t v=0;for(unsigned n=0;n<4;++n)v|=std::to_integer<std::uint32_t>(b[n])<<(8*n);return v;}
};
struct MissingCallback {std::uint32_t target;};
void abi_equal(const SourcePhysicsWorldRuntimeAbi&a,const Json&j,const std::string&why){
 check(a.gp==j.at("gp").get<decltype(a.gp)>(),why+" GP");check(a.xmm==j.at("xmm").get<decltype(a.xmm)>(),why+" XMM");check((a.eflags&0xcc5u)==(j.at("eflags").get<unsigned>()&0xcc5u),why+" defined flags");
 check(a.fs_base==0 && a.x87_depth==0 && a.x87_status==0 && !a.active_mmx,why+" retained empty FP/MMX/FS state");
 check(a.floating_point && a.floating_point->x87_control_word==0x27f && a.floating_point->mxcsr==0x1f80,why+" source FP controls");
}
std::vector<std::byte> fixture_code(unsigned id,std::uint32_t canary,bool acquire){
 std::vector<std::byte>b{std::byte{0xc6},std::byte{0x05}};
 const auto word=[&](std::uint32_t v){for(unsigned k=0;k<4;++k)b.push_back(std::byte(v>>(8*k)));};word(canary);b.push_back(std::byte(id+1));
 for(auto pair:{std::pair{0xb8u,0xa0000000u},std::pair{0xb9u,0xc0000000u},std::pair{0xbau,0xd0000000u}}){b.push_back(std::byte(pair.first));word(pair.second|id);}if(acquire){b.push_back(std::byte{0xc2});b.push_back(std::byte{4});b.push_back(std::byte{});}else b.push_back(std::byte{0xc3});return b;
}
}
int main(int argc, char **argv) try {
    check(argc == 2 || argc == 3, "Supply fixture directory [--baseline-missing]");
    const auto root = std::filesystem::path(argv[1]);
    const bool missing_mode = argc == 3;
    check(!missing_mode || std::string(argv[2]) == "--baseline-missing", "Unknown control");
    if (!std::filesystem::is_regular_file(root / "report.json")) return 77;
    const auto report = read(root / "report.json");
    check(report.at("status") == "PASS_UNCHANGED_ORIGINAL_RESOURCE_STAGE_WITH_DECLARED_EXTERNAL_FIXTURES" &&
          report.at("suite") == "campaign-resource-stage", "Invalid original fixture status/suite");
    check(report.at("xbe_sha256") == "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
          report.at("entry") == 0x191cf0, "Wrong original body");
    check(report.at("tested_owner_instruction_patches").empty() &&
          report.at("full_startup_proven") == false, "Invalid tested-owner scope");
    const std::vector<std::string> names{
        "same-positive", "same-zero", "same-negative", "new-from-zero", "new-from-negative",
        "replace-positive", "replace-positive-empty", "replace-positive-sparse", "duplicate-target",
        "missing-selector", "missing-release", "missing-acquire"};
    check(report.at("cases").size() == names.size(), "Exact twelve original cases required");
    std::set<std::uint32_t> pcs;
    for (auto v : report.at("static_original_PCs")) pcs.insert(v.get<std::uint32_t>());
    check(pcs.size() == 45 && pcs.contains(0x191cf4) && pcs.contains(0x191d87), "Wrong static original coverage");
    unsigned comparisons = 0, selectors = 0, callbacks = 0, stops = 0, total_stores = 0;
    for (std::size_t index = 0; index < names.size(); ++index) {
        const auto &item = report.at("cases").at(index);
        check(item.at("name") == names[index] && item.at("file") == names[index] + ".json",
              "Fixture name/order/path changed");
        const auto row = read(root / (names[index] + ".json"));
        check(row.at("name") == names[index] && row.at("entry") == 0x191cf0, "Wrong case owner");
        for (bool observed : {false, true}) {
            Memory memory;
            for (const auto &[at, data] : row.at("before").items())
                memory.banks.emplace(std::stoul(at, nullptr, 16), bytes(data));
            const auto pre = memory.banks;
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp = row.at("gp").get<decltype(abi.gp)>();
            abi.xmm = row.at("xmm").get<decltype(abi.xmm)>();
            abi.eflags = row.at("eflags");
            abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
            const auto before = abi;
            std::vector<SourcePhysicsContactRecordStore> stores;
            std::vector<std::uint32_t> trace;
            std::size_t call_index = 0;
            bool stopped = false;
            SourcePhysicsContactRecordObserver observer;
            if (observed) observer = [&](const auto &store) { stores.push_back(store); };
            SourcePhysicsWorldRuntimeServices services;
            services.instruction = [&](std::uint32_t pc) {
                check(pcs.contains(pc), "Unexpected native instruction owner");
                trace.push_back(pc);
            };
            services.required_owner = [&](auto &bank, const auto &call, auto &state, const auto &) {
                check(&bank == &memory && !call.tail, "Wrong external owner ABI");
                check(call_index < row.at("calls").size(), "Unexpected external owner");
                const auto &expected = row.at("calls").at(call_index++);
                check(call.target == expected.at("target"), "External target/order differs");
                abi_equal(state, expected.at("abi"), "External boundary");
                const auto role = expected.at("role").template get<std::string>();
                const auto site = role == "selector" ? 0x191cf4u : role == "release" ? 0x191d1au : 0x191d7bu;
                const auto ret = role == "selector" ? 0x191cf9u : role == "release" ? 0x191d1cu : 0x191d7du;
                check((role == "selector" || role == "release" || role == "acquire") &&
                      call.instruction == site && memory.word(state.gp[4]) == ret &&
                      memory.word(state.gp[4]) == expected.at("abi").at("stack_return"),
                      "External return/site differs");
                if (role == "acquire")
                    check(memory.word(state.gp[4] + 4) == row.at("declared_selector"),
                          "Acquire did not receive the selected stage on its real stack");
                const Json *fixture = nullptr;
                for (const auto &v : row.at("declared_callbacks")) if (v.at("entry") == call.target) {
                    check(!fixture, "Duplicate callback fixture");
                    fixture = &v;
                }
                if (!fixture) throw MissingCallback{call.target};
                check(fixture->at("role") == role, "Fixture role differs");
                if (role == "selector") {
                    const auto selected = fixture->at("selected").template get<std::uint32_t>();
                    std::vector<std::byte> program{std::byte{0xb8}};
                    for (unsigned n = 0; n < 4; ++n) program.push_back(std::byte(selected >> (8*n)));
                    program.push_back(std::byte{0xc3});
                    check(call.target == 0x191ca0 && selected == row.at("declared_selector") &&
                          bytes(fixture->at("code_hex")) == program, "Undeclared test selector program");
                    // This constant belongs to the explicitly declared external test owner.
                    // The production selector and its source dependencies are not executed here.
                    state.gp[0] = selected;
                    state.gp[4] += 4;
                    ++selectors;
                    return;
                }
                const auto id = fixture->at("id").template get<unsigned>();
                const auto at = fixture->at("canary_address").template get<std::uint32_t>();
                const bool acquire = role == "acquire";
                check(id < 16 && (id >= 8) == acquire && at == 0x3100000u + id &&
                      fixture->at("canary_value") == id+1 && call.target == 0x3001000u + id*0x40u &&
                      bytes(fixture->at("code_hex")) == fixture_code(id, at, acquire),
                      "Resource fixture escaped declared canary/volatile-register/RET program");
                const std::array value{std::byte(id+1)};
                memory.write(at, value);
                state.gp[0] = 0xa0000000u | id;
                state.gp[1] = 0xc0000000u | id;
                state.gp[2] = 0xd0000000u | id;
                state.gp[4] += acquire ? 8 : 4;
                ++callbacks;
            };
            if (missing_mode) {
                check(!source_campaign_activation_tu15_owns_entry(0x191cf0), "Baseline unexpectedly owns resource stage");
                bool rejected = false;
                try { source_run_campaign_activation_tu15(memory, 0x191cf0, abi, {0x27f, 0x1f80}, services, observer); }
                catch (const std::runtime_error &error) {
                    rejected = std::string(error.what()) == "Required original contact entry unavailable";
                }
                check(rejected && memory.banks == pre && trace.empty() && !call_index && stores.empty(),
                      "Unsupported baseline mutated fixture");
                abi_equal(abi, Json{{"gp", before.gp}, {"xmm", before.xmm}, {"eflags", before.eflags}}, "Baseline rejection");
                std::cout << "PASS baseline rejects missing191CF0 before source stores\n";
                return 0;
            }
            check(source_campaign_activation_tu15_owns_entry(0x191cf0), "Missing recovered original191CF0 owner");
            std::uint16_t host_cw;
            asm volatile("fnstcw %0" : "=m"(host_cw));
            const auto host_mxcsr = _mm_getcsr();
            try { source_run_campaign_activation_tu15(memory, 0x191cf0, abi, {0x27f, 0x1f80}, services, observer); }
            catch (const MissingCallback &error) {
                check(row.at("stopped").size() == 1 && row.at("stopped").at(0).at("target") == error.target,
                      "Wrong first owner failure retained");
                stopped = true;
                ++stops;
            }
            std::uint16_t after_cw;
            asm volatile("fnstcw %0" : "=m"(after_cw));
            check(host_cw == after_cw && host_mxcsr == _mm_getcsr(), "Host FP controls escaped source call");
            check(stopped == !row.at("stopped").empty() && call_index == row.at("calls").size(),
                  "Missing external owner stop/order differs");
            for (const auto &[at, data] : row.at("after").items())
                check(memory.banks.at(std::stoul(at, nullptr, 16)) == bytes(data), "Complete source bank differs");
            abi_equal(abi, row.at("final"), "Final or failed caller");
            std::vector<std::uint32_t> expected_trace;
            for (auto pc : row.at("trace")) if (pcs.contains(pc.get<std::uint32_t>()))
                expected_trace.push_back(pc.get<std::uint32_t>());
            check(trace == expected_trace, "Original dynamic source PC sequence differs");
            if (observed) {
                check(stores.size() == row.at("stores").size(), "Source store count differs");
                for (std::size_t n = 0; n < stores.size(); ++n) {
                    const auto &s = stores[n];
                    const auto &e = row.at("stores").at(n);
                    check(s.instruction == e.at("pc") && s.address == e.at("address") &&
                          s.before == bytes(e.at("before")) && s.written == bytes(e.at("written")),
                          "Ordered source store PC/before/written differs");
                }
                total_stores += stores.size();
            }
            ++comparisons;
        }
    }
    check(comparisons == 24 && selectors == 22 && callbacks == 102 && stops == 6 && total_stores > 20,
          "Resource-stage differential coverage lost");
    std::cout << "PASS " << comparisons << " original/native cases, " << selectors << " declared selectors, "
              << callbacks << " declared resource callbacks, " << stops << " preserved missing-owner stops, "
              << total_stores << " ordered original stores\n"
              << "LIMIT external owners are fixtures; no actual resource allocation, readiness, full startup, World or GPU proof\n";
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
