#include "halo2/native_object_factory_placement_tu15.hpp"
#include "halo2/native_tu15_contact_pressure.hpp"
#include "halo2/native_tu15_memory_pressure.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
using Json=nlohmann::json;
using Bytes=std::vector<std::byte>;

Bytes decode(std::string_view text) {
    Bytes bytes;
    bytes.reserve(text.size()/2);
    for(std::size_t i=0;i<text.size();i+=2)
        bytes.push_back(std::byte(std::stoul(std::string(text.substr(i,2)),nullptr,16)));
    return bytes;
}

class Memory final:public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,Bytes> banks;
    mutable Json reads=Json::array();
    bool allow_pressure_guard=false;
    Json writes=Json::array();

    void read(std::uint32_t address,std::span<std::byte> output)const override {
        auto it=banks.upper_bound(address);
        if(it==banks.begin())throw std::runtime_error("Reclamation read has no declared bank");
        --it;
        const auto offset=address-it->first;
        if(std::uint64_t(offset)+output.size()>it->second.size())
            throw std::runtime_error("Reclamation read exceeds its declared bank");
        reads.push_back(Json::array({address,output.size()}));
        std::ranges::copy(std::span(it->second).subspan(offset,output.size()),output.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes) override {
        if(!allow_pressure_guard||address!=0x48b873u||bytes.size()!=1)
            throw std::runtime_error("Reclamation selection changed retained storage");
        auto bank=std::prev(banks.upper_bound(address));
        auto& guard=bank->second.at(address-bank->first);
        writes.push_back(Json::array({address,std::to_integer<unsigned>(guard),
            std::to_integer<unsigned>(bytes.front())}));
        guard=bytes.front();
    }
};

void verify_positive_boundary(std::string_view message,std::uint32_t selected) {
    char datum[9]{};
    std::snprintf(datum,sizeof(datum),"%08X",selected);
    if(selected==0xffffffffu||!message.starts_with(
            "Native memory pressure requires actual1CCCDC selected-object reclamation for datum ")||
       !message.ends_with(datum))
        throw std::runtime_error(std::string(message));
}

void compare(const Json& row) {
    Memory memory;
    for(const auto& bank:row.at("banks"))
        memory.banks.emplace(bank.at("address").get<std::uint32_t>(),
            decode(bank.at("before").get_ref<const std::string&>()));
    const auto& request=row.at("request");
    const auto protection=[&](std::uint32_t object) {
        return native_object_factory_tu15::object_delete_protected(memory,object);
    };
    const auto mode=request.at("mode").get<std::uint32_t>();
    const auto excluded=request.at("excluded").get<std::uint32_t>();
    const auto aggressive=request.at("aggressive").get<bool>();
    const auto recent=request.at("recent_cleanup").get<bool>();
    const auto name=row.at("name").get<std::string>();
    const auto selected=row.at("selected").get<std::uint32_t>();
    if(row.contains("body")) {
        const auto actual=native_tu15_reclamation_body_candidate(memory,
            {row.at("body").get<std::uint32_t>(),mode,excluded,aggressive,recent},protection);
        if(actual!=selected)throw std::runtime_error(name+" selected a different object datum");
    } else if(row.contains("pressure_boundary")) {
        memory.allow_pressure_guard=true;
        const auto& begin=row.at("before_fp");
        NativeTu15PhysicsFrameFp fp{begin[0],begin[1],begin[2]};
        NativeTu15MemoryPressureServices services;
        NativeTu15MemoryPressureStats stats;
        services.remove_candidate=[&](auto query,auto&) {
            return native_tu15_reclaim_memory_if_no_candidate(memory,query,&stats);
        };
        bool stopped=false;
        try { native_tu15_collect_memory_pressure(memory,fp,services,{},&stats); }
        catch(const std::runtime_error& error) {
            const std::string_view message=error.what();
            if(row.at("pressure_boundary")==0x1cb330u) {
                if(message!="Native memory pressure requires actual1CB330 cleanup owner")throw;
            } else verify_positive_boundary(message,selected);
            stopped=true;
        }
        if(!stopped||stats.candidate_calls!=1||stats.candidate_removals||stats.unbound_services!=1||
           memory.writes!=row.at("writes")||
           Json::array({fp.control_word,fp.status,fp.mxcsr})!=row.at("after_fp"))
            throw std::runtime_error(name+" pressure continuation differs");
    } else {
        bool stopped=false;
        try {
            if(native_tu15_reclaim_memory_if_no_candidate(memory,
                    {mode,excluded,request.at("final_pass").get<bool>(),aggressive,recent}))
                throw std::runtime_error(name+" manufactured a successful reclamation");
        } catch(const std::runtime_error& error) {
            verify_positive_boundary(error.what(),selected);
            stopped=true;
        }
        if(stopped!=(selected!=0xffffffffu))
            throw std::runtime_error(name+" selection continuation differs");
    }
    const auto& expected=row.at("reads");
    if(memory.reads!=expected) {
        std::size_t i=0;
        while(i<memory.reads.size()&&i<expected.size()&&memory.reads[i]==expected[i])++i;
        const auto actual=i<memory.reads.size()?memory.reads[i].dump():std::string("end");
        const auto original=i<expected.size()?expected[i].dump():std::string("end");
        throw std::runtime_error(name+" read "+std::to_string(i)+" native "+actual+" original "+original);
    }
}
}

int main(int argc,char** argv)try {
    if(argc!=2)throw std::runtime_error("A physical reclamation fixture is required");
    std::ifstream file(argv[1]);
    const auto fixture=Json::parse(file);
    if(fixture.at("image_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("The fixture belongs to a different original executable");
    for(const auto& row:fixture.at("cases"))compare(row);
    std::cout<<fixture.at("cases").size()<<" physical candidate cases matched\n";
    return 0;
} catch(const std::exception& error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}
