#include "halo2/source_campaign_audio_tu15.hpp"
#ifndef HALO2_DSP_IMAGE_ONLY_TEST
#include "halo2/source_vehicle_director_tu15.hpp"
#include "halo2/source_player_departure_tu15.hpp"
#include "halo2/source_input_startup_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#endif
#include "halo2/source_sound_dsp_image_tu15.hpp"
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

namespace {
using namespace halo2;
using Json = nlohmann::json;
void check(bool value, const std::string &message) {
    if (!value) throw std::runtime_error(message);
}
std::vector<std::byte> bytes(const std::string &text) {
    check(text.size() % 2 == 0, "Invalid original audio byte string");
    std::vector<std::byte> result;
    result.reserve(text.size() / 2);
    for (std::size_t i = 0; i < text.size(); i += 2)
        result.push_back(std::byte(std::stoul(text.substr(i, 2), nullptr, 16)));
    return result;
}
struct Memory : SourcePhysicsContactMemory {
    std::string label;
    bool hold_listener{};
    bool hold_classes{},hold_clock_gain{},hold_dialogue_routing{};
    std::map<std::uint32_t, std::vector<std::byte>> banks;
    auto locate(std::uint32_t address, std::size_t size) const {
        auto at = banks.upper_bound(address);
        check(at != banks.begin(), label + ": unowned fixture address " + std::to_string(address));
        --at;
        const auto offset = std::uint64_t(address) - at->first;
        check(offset <= at->second.size() && size <= at->second.size() - offset,
              label + ": access escapes declared bank at " + std::to_string(address));
        return at;
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        const auto at = locate(address, out.size());
        std::copy_n(at->second.begin() + (address - at->first), out.size(), out.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> input) override {
        const auto at = locate(address, input.size());
        if (hold_listener && address >= 0x2105088u && address + input.size() <= 0x21051a8u) return;
        if (hold_classes && address >= 0x2120000u && address + input.size() <= 0x2120360u) return;
        if (hold_clock_gain && address == 0x21051e4u) return;
        if (hold_dialogue_routing && address == 0x10e3208u) return;
        std::copy(input.begin(), input.end(), banks.at(at->first).begin() + (address - at->first));
    }
};
struct Boundary {};
Json read(const std::filesystem::path &path) {
    std::ifstream input(path);
    check(bool(input), "Original audio fixture unavailable");
    Json result; input >> result; return result;
}
}

int main(int argc, char **argv) try {
    check(argc == 2 || argc == 3, "Supply original audio fixture directory [--omit-listener|--omit-class-fades|--omit-clock-gain|--omit-dialogue-routing]");
    const auto root = std::filesystem::path(argv[1]);
    if (!std::filesystem::is_regular_file(root / "report.json")) return 77;
    const bool omit = argc == 3 && std::string(argv[2]) == "--omit-listener";
    const bool omit_classes=argc==3 && std::string(argv[2])=="--omit-class-fades";
    const bool omit_clock=argc==3 && std::string(argv[2])=="--omit-clock-gain";
    const bool omit_routing=argc==3 && std::string(argv[2])=="--omit-dialogue-routing";
    check(argc == 2 || omit || omit_classes || omit_clock || omit_routing, "Unknown audio negative control");
    const auto report = read(root / "report.json");
    const bool routing=report.at("suite")=="audio_routing";
    const bool dsp=report.at("suite")=="audio_dsp";
#ifdef HALO2_DSP_IMAGE_ONLY_TEST
    check(dsp,"Standalone DSP fixture requires the original DSP suite");
#endif
    check((report.at("suite") == "audio" || routing || dsp) && report.at("xbe_sha256") ==
              "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Audio fixture has another executable");
    check(report.at("instruction_patches").empty() && report.at("supplied_external_results").empty(),
          "Original audio fixture supplied instructions or external results");
    unsigned compared{}, writes{}, stopped{}, directors{}, stack_probes{}, clocks{};
    for (const auto &item : report.at("cases")) {
        const auto row = read(root / item.at("file").get<std::string>());
        const auto name = row.at("name").get<std::string>();
        for (bool observed : {false, true}) {
            Memory memory; memory.label = name; memory.hold_listener = omit;
            memory.hold_classes=omit_classes;memory.hold_clock_gain=omit_clock;
            memory.hold_dialogue_routing=omit_routing;
            for (const auto &[address, data] : row.at("before").items())
                memory.banks.emplace(std::stoul(address, nullptr, 16), bytes(data));
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp = row.at("gp").get<decltype(abi.gp)>();
            abi.xmm = row.at("xmm").get<decltype(abi.xmm)>(); abi.eflags = 2;
            std::vector<SourcePhysicsContactRecordStore> stores;
            std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> digest(nullptr,EVP_MD_CTX_free);
            unsigned store_count{};
            if(observed && row.contains("store_sha256")) {
                digest.reset(EVP_MD_CTX_new());
                check(digest && EVP_DigestInit_ex(digest.get(),EVP_sha256(),nullptr)==1,"Cannot initialize original store digest");
            }
            std::vector<std::uint32_t> boundaries;
            const auto external=row.value("external_targets",report.at("external_targets"))
                .get<std::set<std::uint32_t>>();
            SourcePhysicsWorldRuntimeServices services;
            // A fixture may deliberately bound a recovered callee too. Stop
            // before its first instruction, retaining its real caller frame.
            services.instruction=[&](std::uint32_t site) {
                if(external.contains(site)) {boundaries.push_back(site);throw Boundary{};}
            };
            services.required_owner = [&](auto &m, const auto &call, auto &a, const auto &o) {
#ifndef HALO2_DSP_IMAGE_ONLY_TEST
                if (call.target == 0x158290u) {
                    ++directors;
                    source_run_vehicle_director_tu15(m, call.target, a, {0x27f, 0x1f80}, services, o);
                    return;
                }
                if (call.target == 0x32a660u) {
                    ++stack_probes;
                    source_run_player_departure_tu15(m, call.target, a, {0x27f, 0x1f80}, services, o);
                    return;
                }
                if (call.target == 0x33b590u && !external.contains(call.target)) {
                    ++clocks;
                    source_run_input_startup_tu15(m, call.target, a, {0x27f, 0x1f80}, services, o);
                    return;
                }
                if (routing && source_physics_live_support_tu15_owns_entry(call.target)) {
                    source_run_physics_live_support_tu15(m,call.target,a,{0x27f,0x1f80},services,o);return;
                }
                if (routing && source_map_world_support_tu15_owns_entry(call.target)) {
                    source_run_map_world_support_tu15(m,call.target,a,{0x27f,0x1f80},services,o);return;
                }
                if (routing && source_scene_device_constructor_tu15_owns_entry(call.target)) {
                    source_run_scene_device_constructor_tu15(m,call.target,a,{0x27f,0x1f80},services,o);return;
                }
#endif
                boundaries.push_back(call.target); throw Boundary{};
            };
            SourcePhysicsContactRecordObserver observer;
            if (observed) observer = [&](const auto &store) {
                if(!digest) {stores.push_back(store);return;}
                std::array<unsigned char,12> header{};
                const std::array<std::uint32_t,3> words{store.instruction,store.address,std::uint32_t(store.before.size())};
                for(unsigned i=0;i<words.size();++i)for(unsigned j=0;j<4;++j)header[4*i+j]=static_cast<unsigned char>(words[i]>>(8*j));
                check(EVP_DigestUpdate(digest.get(),header.data(),header.size())==1 &&
                      EVP_DigestUpdate(digest.get(),store.before.data(),store.before.size())==1 &&
                      EVP_DigestUpdate(digest.get(),store.written.data(),store.written.size())==1,"Cannot retain original ordered store digest");
                ++store_count;
            };
            const auto entry = row.at("entry").get<std::uint32_t>();
#ifdef HALO2_DSP_IMAGE_ONLY_TEST
            check(source_sound_dsp_image_tu15_owns_entry(entry),"Missing complete original DSP image entry");
#else
            check(dsp ? source_sound_dsp_image_tu15_owns_entry(entry) : source_campaign_audio_tu15_owns_entry(entry), "Missing complete original audio entry");
#endif
            try {
                if(dsp)source_run_sound_dsp_image_tu15(memory,entry,abi,{0x27f,0x1f80},services,observer);
#ifndef HALO2_DSP_IMAGE_ONLY_TEST
                else source_run_campaign_audio_tu15(memory, entry, abi, {0x27f, 0x1f80}, services, observer);
#endif
            } catch (const Boundary &) {}
            check(boundaries == row.at("external_boundary").get<decltype(boundaries)>(), name + ": external boundary differs");
            for (const auto &[address, data] : row.at("after").items())
                check(memory.banks.at(std::stoul(address, nullptr, 16)) == bytes(data), name + ": retained bank differs at " + address);
            check(abi.gp == row.at("gp_after").get<decltype(abi.gp)>(), name + ": complete GP state differs");
            check(abi.xmm == row.at("xmm_after").get<decltype(abi.xmm)>(), name + ": complete XMM state differs");
            const auto flags_mask=row.value("defined_flags_mask",0xcc5u);
            check((abi.eflags & flags_mask) == (row.at("eflags_after").get<unsigned>() & flags_mask),
                  name + ": defined flags differ");
            if (observed) {
                if(digest) {
                    std::array<unsigned char,32> output{};unsigned size{};
                    check(EVP_DigestFinal_ex(digest.get(),output.data(),&size)==1 && size==output.size(),"Cannot finalize original ordered stores");
                    const auto expected=bytes(row.at("store_sha256"));
                    check(expected.size()==output.size() && std::equal(expected.begin(),expected.end(),reinterpret_cast<const std::byte *>(output.data())) &&
                          store_count==row.at("store_count"),name+": original ordered store digest differs");
                    writes+=store_count;
                } else {
                check(stores.size() == row.at("stores").size(), name + ": original store count differs");
                for (std::size_t i = 0; i < stores.size(); ++i) {
                    const auto &expected = row.at("stores").at(i); const auto &store = stores[i];
                    check(store.instruction == expected.at("pc") && store.address == expected.at("address") &&
                              store.before == bytes(expected.at("before")) && store.written == bytes(expected.at("written")),
                          name + ": store site/order/before/written bytes differ at " + std::to_string(i));
                }
                writes += unsigned(stores.size());
                }
            }
            if (!boundaries.empty()) ++stopped;
            ++compared;
        }
    }
    check(dsp ? compared>=32 && writes>5000 : routing ? compared>=200 && writes>5000 && stopped>=30 :
          compared >= 160 && writes > 6000 && stopped >= 30 && directors >= 200 && stack_probes >= 48,
          "Audio fixture lost source state/store/boundary/director coverage");
    check(compared==2*report.at("cases").size() && clocks==2*report.value("clock_cases",0u),
          "Audio fixture lost its declared case or original clock-reader coverage");
    std::cout << "PASS " << compared << " native audio comparisons, " << writes << " ordered stores, "
              << stopped << " external stops, " << directors << " actual director queries and "
              << stack_probes << " actual stack probes\n";
    std::cout << "Actual original clock reads=" << clocks << '\n';
    std::cout << "LIMIT declared offline caller states; native startup, SDK mixing, final volume and sample timing unverified\n";
} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
