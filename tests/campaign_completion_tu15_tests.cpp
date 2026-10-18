#include "halo2/source_campaign_completion_tu15.hpp"
#include "halo2/source_campaign_activation_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool value,const std::string &message) {
    if(!value)throw std::runtime_error(message);
}
std::vector<std::byte> bytes(const std::string &text) {
    check(text.size()%2==0,"Invalid original completion byte string");
    std::vector<std::byte> result;result.reserve(text.size()/2);
    for(std::size_t i=0;i<text.size();i+=2)
        result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
struct Memory:SourcePhysicsContactMemory {
    std::string label;
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto at=banks.upper_bound(address);
        check(at!=banks.begin(),label+": unowned caller fixture address "+std::to_string(address));--at;
        const auto offset=std::uint64_t(address)-at->first;
        check(offset<=at->second.size() && size<=at->second.size()-offset,
              label+": caller fixture access escapes a declared bank at "+std::to_string(address)+" size "+std::to_string(size));
        return at;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        const auto at=locate(address,out.size());
        std::copy_n(at->second.begin()+(address-at->first),out.size(),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        const auto at=locate(address,input.size());
        std::copy(input.begin(),input.end(),banks.at(at->first).begin()+(address-at->first));
    }
};
struct Boundary {};
Json read(const std::filesystem::path &path) {
    std::ifstream input(path);check(bool(input),"Original completion fixture is unavailable");
    Json value;input>>value;return value;
}
}
int main(int argc,char **argv)try {
    if(argc!=2 && argc!=3)throw std::runtime_error("Supply source caller fixture directory [--omit-options|--omit-bsp-bind]");
    const auto root=std::filesystem::path(argv[1]);
    if(!std::filesystem::is_regular_file(root/"report.json"))return 77;
    const auto report=read(root/"report.json");
    const auto suite=report.value("suite",std::string("completion"));
    check(suite=="completion" || suite=="activation","Unknown source caller fixture suite");
    const bool activation=suite=="activation";
    const bool omit=argc==3 && std::string(argv[2])==(activation?"--omit-bsp-bind":"--omit-options");
    check(argc==2 || omit,"Unknown source caller negative control");
    check(report.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Completion fixture has another executable");
    check(report.at("instruction_patches").empty() && report.at("supplied_external_results").empty(),
          "Completion fixture supplied original instructions or external results");
    unsigned compared{},stopped{},writes{};
    for(const auto &item:report.at("cases")) {
        const auto row=read(root/item.at("file").get<std::string>());
        const auto name=row.at("name").get<std::string>();
        // Run both observed and unobserved store paths against the same bytes.
        for(bool observed:{false,true}) {
            Memory memory;
            memory.label=name;
            for(const auto &[address,data]:row.at("before").items())
                memory.banks.emplace(std::stoul(address,nullptr,16),bytes(data));
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp=row.at("gp").get<decltype(abi.gp)>();
            abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();abi.eflags=2;
            std::vector<SourcePhysicsContactRecordStore> stores;
            std::vector<std::uint32_t> boundaries;
            SourcePhysicsWorldRuntimeServices services;
            services.required_owner=[&](auto &,const auto &call,auto &,const auto &) {
                boundaries.push_back(call.target);throw Boundary{};
            };
            SourcePhysicsContactRecordObserver observer;
            if(observed)observer=[&](const auto &store){stores.push_back(store);};
            const auto entry=row.at("entry").get<std::uint32_t>();
            check(activation?source_campaign_activation_tu15_owns_entry(entry):source_campaign_completion_tu15_owns_entry(entry),
                  "Missing recovered source caller entry");
            try {
                if(!(omit && entry==(activation?0x13a820u:0x154f80u))) {
                    if(activation)source_run_campaign_activation_tu15(memory,entry,abi,{0x27f,0x1f80},services,observer);
                    else source_run_campaign_completion_tu15(memory,entry,abi,{0x27f,0x1f80},services,observer);
                }
            }catch(const Boundary &) {}
            check(boundaries==row.at("external_boundary").get<decltype(boundaries)>(),name+": external owner boundary differs");
            for(const auto &[address,data]:row.at("after").items())
                check(memory.banks.at(std::stoul(address,nullptr,16))==bytes(data),name+": retained bytes differ");
            check(abi.gp==row.at("gp_after").get<decltype(abi.gp)>(),name+": complete GP caller state differs");
            check(abi.xmm==row.at("xmm_after").get<decltype(abi.xmm)>(),name+": complete XMM caller state differs");
            // AF is undefined after the original logical instructions. Check
            // the defined arithmetic flags, including parity, carry and OF.
            check((abi.eflags&0xcc5u)==(row.at("eflags_after").get<unsigned>()&0xcc5u),name+": defined caller flags or direction flag differ");
            if(observed) {
                check(stores.size()==row.at("stores").size(),name+": original store count differs");
                for(std::size_t i=0;i<stores.size();++i) {
                    const auto &expected=row.at("stores").at(i);const auto &store=stores[i];
                    check(store.instruction==expected.at("pc") && store.address==expected.at("address") &&
                          store.before==bytes(expected.at("before")) && store.written==bytes(expected.at("written")),
                          name+": original store site/order/before/written bytes differ");
                }
                writes+=unsigned(stores.size());
            }
            if(!boundaries.empty())++stopped;
            ++compared;
        }
    }
    check(compared>=(activation?140u:94u) && stopped>=10 && writes>1000,"Source caller fixture lost its state/store/boundary coverage");
    std::cout<<"PASS "<<compared<<" native "<<suite<<" comparisons, "<<writes<<" ordered stores, "
        <<stopped<<" explicit external-owner stops; complete retained banks and caller registers\n";
    std::cout<<"LIMIT declared caller states; full activation, GPU/platform calls, profile persistence and actual level loading remain unverified\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
