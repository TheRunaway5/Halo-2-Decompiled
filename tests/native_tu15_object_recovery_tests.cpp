#include "halo2/native_tu15_object_recovery.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(const std::string &value) {
    if(value.size()%2)throw std::runtime_error("Odd recovery fixture hex length");
    std::vector<std::byte> bytes;
    bytes.reserve(value.size()/2);
    for(std::size_t i=0;i<value.size();i+=2)
        bytes.push_back(std::byte(std::stoul(value.substr(i,2),nullptr,16)));
    return bytes;
}
std::uint32_t word(std::span<const std::byte> bytes) {
    std::uint32_t value{};
    for(unsigned i=0;i<bytes.size();++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8u*i);
    return value;
}
struct Memory final:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;bool writable;};
    std::vector<Bank> banks;
    std::size_t find(std::uint32_t at,std::size_t count) const {
        for(std::size_t i=0;i<banks.size();++i) {
            const auto &bank=banks[i];
            if(at>=bank.address && std::uint64_t(at-bank.address)+count<=bank.bytes.size())return i;
        }
        throw std::runtime_error("Recovery input escaped its declared bank");
    }
    void read(std::uint32_t at,std::span<std::byte> output) const override {
        const auto &bank=banks[find(at,output.size())];
        std::ranges::copy(std::span(bank.bytes).subspan(at-bank.address,output.size()),output.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> input) override {
        auto &bank=banks[find(at,input.size())];
        if(!bank.writable)throw std::runtime_error("Recovery store escaped its stack owner");
        std::ranges::copy(input,bank.bytes.begin()+(at-bank.address));
    }
};
}

int main(int argc,char **argv) try {
    if(argc!=2)throw std::runtime_error("Expected pinned original recovery fixture");
    std::ifstream file(argv[1]);
    const auto fixture=Json::parse(file);
    if(fixture.at("image_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Unexpected original recovery image");
    unsigned count{};
    for(const auto &row:fixture.at("cases")) {
        Memory bank;
        for(const auto &region:row.at("banks"))
            bank.banks.push_back({region.at(0).get<std::uint32_t>(),unhex(region.at(1).get<std::string>()),
                                  region.at(2).get<bool>()});
        const auto before=bank.banks.back().bytes;
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp=row.at("gp").get<decltype(abi.gp)>();
        abi.eflags=row.at("eflags").get<std::uint32_t>();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{.x87_control_word=0x27f,.mxcsr=0x1f80};
        const auto initial=abi;
        const bool consumer=row.at("consumer").get<bool>();
        SourcePhysicsContactRuntimeCall call=consumer?
            SourcePhysicsContactRuntimeCall{0xc14de,0xc12f0,false}:
            SourcePhysicsContactRuntimeCall{0xc14b8,0xc1270,false};
        if(row.at("invalid")==true)call.tail=true;
        Json stores=Json::array();
        bool refused=false;
        try {
            const SourcePhysicsContactRecordObserver observer=[&](const auto &store) {
                stores.push_back(Json::array({store.instruction,store.address,word(store.before),word(store.written)}));
            };
            if(consumer)native_tu15_finish_empty_object_recovery(bank,call,abi,observer);
            else native_tu15_collect_object_recovery_candidates(bank,call,abi,observer);
        } catch(const std::runtime_error &e) {
            const std::string_view error=e.what();
            if(error!="C1270 requires its authenticated object recovery caller" &&
               error!="C12F0 requires its authenticated object recovery caller" &&
               error!="C12F0 positive list requires native object eligibility and cleanup")throw;
            refused=true;
        }
        if(abi.gp!=row.at("after_gp").get<decltype(abi.gp)>() ||
           (abi.eflags&0xcd5u)!=row.at("after_flags").get<std::uint32_t>() ||
           stores!=row.at("stores") ||
           bank.banks.back().bytes!=unhex(row.at("after").get<std::string>()) ||
           refused!=row.at("refused").get<bool>() || abi.x87_depth!=initial.x87_depth ||
           abi.floating_point->mxcsr!=initial.floating_point->mxcsr ||
           abi.floating_point->x87_control_word!=initial.floating_point->x87_control_word)
            throw std::runtime_error("Original recovery mismatch: "+row.at("name").get<std::string>());
        if(row.at("invalid")==true && bank.banks.back().bytes!=before)
            throw std::runtime_error("Refused recovery mutated its stack");
        ++count;
    }
    if(count<12)throw std::runtime_error("Missing recovery original coverage");
    std::cout<<count<<" original recovery profiles passed\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';return 1;
}
