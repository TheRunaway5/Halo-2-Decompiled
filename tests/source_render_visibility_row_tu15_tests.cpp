#include "halo2/source_map_world_support_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
using Json = nlohmann::json;
namespace {
constexpr std::uint32_t bank=0x2100000,stack=0x2110000,stop=0x3000000;
std::vector<std::byte> bytes(const std::string &text) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<text.size();n+=2)
        out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> values) {
    constexpr char digits[]="0123456789abcdef";
    std::string out;
    for(auto value:values){const auto n=std::to_integer<unsigned>(value);out+=digits[n>>4];out+=digits[n&15];}
    return out;
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto at(std::uint32_t address)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin())throw std::runtime_error("Unowned visibility-row memory");
        --found;
        if(address-found->first>=found->second.size())throw std::runtime_error("Visibility-row memory extent exceeded");
        return found;
    }
    void read(std::uint32_t address,std::span<std::byte> values)const override {
        for(auto &value:values){auto found=at(address);value=found->second[address++-found->first];}
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        for(auto value:values){auto found=at(address);banks.at(found->first)[address++-found->first]=value;}
    }
    void put(std::uint32_t address,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> raw;
        for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(8*n));
        write(address,std::span(raw).first(size));
    }
};
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply original visibility-row fixture");
    std::ifstream file(argv[1]);check(bool(file),"Original visibility-row fixture absent");
    unsigned cases{};std::string line;
    while(std::getline(file,line)) {
        const auto row=Json::parse(line);Memory memory;
        memory.banks[bank].resize(0x3000);
        for(unsigned n=0;n<0x3000;++n)memory.banks.at(bank)[n]=std::byte((n*73+17)&255);
        memory.banks[stack-0x1000].resize(0x2000);
        memory.put(bank+0xa5e,row.at("cursor"),2);memory.put(stack,stop);memory.put(stack+4,row.at("bit"));
        auto expected=memory;
        for(const auto &write:row.at("original").at("writes"))expected.write(write[1],bytes(write[3]));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();abi.eflags=2;
        SourcePhysicsWorldRuntimeServices services;Json trace=Json::array(),stores=Json::array();
        services.instruction=[&](auto pc){trace.push_back(pc);};
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Unexpected visibility-row callee");};
        source_run_map_world_support_tu15(memory,0x136e10,abi,{0x27f,0x1f80},services,
            [&](const auto &write){stores.push_back(Json::array({write.instruction,write.address,hex(write.before),hex(write.written)}));});
        const auto &original=row.at("original");
        check(Json(abi.gp)==original.at("gp") && abi.eflags==original.at("eflags"),"Original visibility-row GP/flags mismatch");
        check(Json(abi.xmm)==row.at("xmm"),"Visibility-row helper changed untouched SIMD input");
        check(trace==original.at("instructions") && stores==original.at("writes"),"Original visibility-row instruction/store mismatch");
        check(memory.banks==expected.banks,"Original complete visibility/stack bank mismatch");++cases;
    }
    check(cases==1024,"Incomplete original visibility-row fixture");
    std::cout<<"PASS 1024 original visibility-row calls: full GP/SIMD/flags, instruction paths, ordered writes and retained banks\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
