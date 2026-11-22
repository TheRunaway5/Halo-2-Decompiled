#include "halo2/native_tu15_memory_pressure.hpp"
#include "halo2/native_tu15_retained_counter_update.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
using Json=nlohmann::json;
using Bytes=std::vector<std::byte>;
constexpr std::uint32_t iterator=0x62000200u;

Bytes decode(std::string_view text) {
    Bytes bytes;
    for(std::size_t i=0;i<text.size();i+=2)
        bytes.push_back(std::byte(std::stoul(std::string(text.substr(i,2)),nullptr,16)));
    return bytes;
}

std::string hex(std::span<const std::byte> bytes) {
    static constexpr std::string_view digits="0123456789abcdef";
    std::string result;
    for(const auto byte:bytes) {
        const auto value=std::to_integer<unsigned>(byte);
        result+=digits[value>>4];
        result+=digits[value&15u];
    }
    return result;
}

class Memory final:public SourcePhysicsContactMemory,public NativeTu15RetainedCounterStorage {
public:
    std::map<std::uint32_t,Bytes> banks;
    Json iterator_writes=Json::array();
    mutable Json dirty_reads=Json::array();
    mutable bool recording_dirty_return{};

    static auto get(auto& banks,std::uint32_t address,std::size_t bytes) {
        auto bank=banks.upper_bound(address);
        if(bank==banks.begin())throw std::runtime_error("Cleanup operand has no declared bank");
        --bank;
        const auto offset=address-bank->first;
        if(std::uint64_t(offset)+bytes>bank->second.size())
            throw std::runtime_error("Cleanup operand exceeds its declared bank");
        return std::span(bank->second).subspan(offset,bytes);
    }

    void read(std::uint32_t address,std::span<std::byte> output)const override {
        if(address==0x56dfccu) {
            dirty_reads.push_back(Json::array());
            recording_dirty_return=true;
        }
        if(recording_dirty_return) {
            dirty_reads.back().push_back(Json::array({address,output.size()}));
            if(address==0x52be44u)recording_dirty_return=false;
        }
        std::ranges::copy(get(banks,address,output.size()),output.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes) override {
        const auto target=get(banks,address,bytes.size());
        if(address>=iterator&&address<iterator+16u)
            iterator_writes.push_back(Json::array({address-iterator,hex(bytes)}));
        std::ranges::copy(bytes,target.begin());
    }
    NativeTu15RetainedCounterUpdate adjust_counter(std::uint32_t address,unsigned width,
                                                    std::uint32_t delta) override {
        std::array<std::byte,4> old{};
        read(address,std::span(old).first(width));
        const auto result=native_tu15_adjust_counter_value(old,width,delta);
        write(address,std::span(result.written).first(width));
        return result;
    }
};

void compare(const Json& row) {
    Memory memory;
    for(const auto& bank:row.at("banks"))
        memory.banks.emplace(bank.at("address").get<std::uint32_t>(),
            decode(bank.at("before").get_ref<const std::string&>()));
    const auto& initial=row.at("before_fp");
    NativeTu15PhysicsFrameFp fp{initial[0],initial[1],initial[2]};
    Json writes=Json::array();
    const SourcePhysicsContactRecordObserver observe=[&](const auto& event){
        writes.push_back({{"instruction",event.instruction},{"address",event.address},
                          {"before",hex(event.before)},{"written",hex(event.written)}});
    };
    NativeTu15PhysicsFrameMemory storage(memory,observe);
    NativeTu15MemoryPressureServices services;
    NativeTu15MemoryPressureStats stats;
    const auto depth=row.at("x87_depth").get<unsigned>();
    services.begin_cleanup=[&](auto& context){storage.begin_memory_pressure_cleanup(iterator,context,depth);};
    services.remove_candidate=[&](auto request,auto&){
        return native_tu15_reclaim_memory_if_no_candidate(memory,request,&stats);
    };
    const auto name=row.at("name").get<std::string>();
    const auto compose=row.at("compose").get<bool>();
    try {
        if(compose)native_tu15_collect_memory_pressure(memory,fp,services,observe,&stats);
        else storage.begin_memory_pressure_cleanup(iterator,fp,depth);
        throw std::runtime_error("Cleanup unexpectedly returned");
    } catch(const std::runtime_error& error) {
        if(error.what()!=row.at("message").get<std::string>())
            throw std::runtime_error(name+" unexpected cleanup handoff: "+error.what());
    }
    const auto compare_trace=[&](std::string_view field,const Json& actual){
        const auto& original=row.at(field);
        if(actual!=original)throw std::runtime_error(name+" "+std::string(field)+" differs. Native "+
                                                     actual.dump()+", original "+original.dump());
    };
    compare_trace("writes",writes);
    compare_trace("iterator_writes",memory.iterator_writes);
    compare_trace("dirty_reads",memory.dirty_reads);
    compare_trace("after_fp",Json::array({fp.control_word,fp.status,fp.mxcsr}));
    if(compose&&(stats.candidate_calls!=1||stats.candidate_removals||stats.cleanup_batches))
        throw std::runtime_error(name+" caller trace differs");
    for(const auto& bank:row.at("banks")) {
        const auto address=bank.at("address").get<std::uint32_t>();
        if(bank.contains("after")&&hex(memory.banks.at(address))!=bank.at("after").get<std::string>())
            throw std::runtime_error(name+" final retained bank differs");
    }
}
}

int main(int argc,char** argv)try {
    if(argc!=2)throw std::runtime_error("A physical cleanup-prefix fixture is required");
    std::ifstream file(argv[1]);
    const auto fixture=Json::parse(file);
    if(fixture.at("image_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Cleanup-prefix fixture uses a different original image");
    for(const auto& row:fixture.at("cases"))compare(row);
    std::cout<<fixture.at("cases").size()<<" physical Havok cleanup prefix cases matched\n";
    return 0;
} catch(const std::exception& error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}
