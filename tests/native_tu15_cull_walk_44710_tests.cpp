#include "halo2/native_tu15_cull_walk_44710.hpp"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;using J=nlohmann::json;
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};std::vector<Bank> banks;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(const auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+out.size()<=b.bytes.size()) {
            std::copy_n(b.bytes.begin()+at-b.address,out.size(),out.begin());return;}
        throw std::runtime_error("Unmapped native44710 read at "+std::to_string(at));
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+in.size()<=b.bytes.size()) {
            std::copy(in.begin(),in.end(),b.bytes.begin()+at-b.address);return;}
        throw std::runtime_error("Unmapped native44710 write");
    }
    void put(std::uint32_t at,std::uint32_t value) {
        std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));write(at,b);
    }
};
void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
void controls() {
    constexpr auto ctx=0x1000000u,other=0x1003000u,units=0x4c6958u;
    Memory m;m.banks={{0x4c6000u,std::vector<std::byte>(8192)},{0x4cd000u,std::vector<std::byte>(4096)},
        {ctx,std::vector<std::byte>(0x6000)}};
    m.put(0x4cd3a0u,ctx);m.put(0x4cd39cu,3);m.put(units,0x48);m.put(units+0x18,0xffff);
    m.put(units+32,0x40);m.put(units+32+0x18,0xabcdffff);m.put(units+64,0x40);
    const auto row=ctx+0xa50u; // signed association -1
    m.put(row,3);m.put(row+4,1);m.put(row+8,0);m.put(row+12,0);
    NativeTu15PhysicsFrameFp fp{0x27f,0,0x1f80};std::vector<std::array<std::uint32_t,2>> calls;
    NativeTu15CullWalkServices services;
    services.frustum=[&](auto unit,auto entry,auto&) {
        calls.push_back({unit,entry});
        if(calls.size()==1) {
            m.put(row,0);m.put(row+4,4);m.put(row+8,8); // first word remains pinned
            m.put(units,0x49); // flag1 must not cancel the remaining association calls
            m.put(0x4cd39cu,0);m.put(ctx+0x2a78,1); // loop bounds remain pinned
            m.put(0x4cd3a0u,other); // per-unit query mode still uses original context
        }
        if(unit==1)m.put(units+64,1); // future unit eligibility remains live
    };
    native_tu15_cull_walk_44710(m,fp,services);
    check(calls==std::vector<std::array<std::uint32_t,2>>{{0,0},{0,1},{0,34},{0,67},{1,65535}},
        "Native44710 lost pinned/lazy association, live eligibility or source loop order");
    // Nonzero mode preserves full ESI in service arguments; addressing and
    // termination consume only its signed/unsigned low half respectively.
    m.put(0x4cd3a0u,ctx);m.put(0x4cd39cu,0xbeef0001u);m.put(0x4cd398u,0x12340003);
    m.put(ctx+0x2a7c,2);m.put(units+32,0x40);m.put(units+64,0x40);calls.clear();
    services.frustum=[&](auto unit,auto entry,auto&){calls.push_back({unit,entry});};
    native_tu15_cull_walk_44710(m,fp,services);
    check(calls==std::vector<std::array<std::uint32_t,2>>{{0xbeef0001u,65535},{0xbeef0002u,0}},
        "Native44710 narrowed the full service unit ordinal or rejected non-sphere mode2");
    m.put(0x4cd39cu,0);m.put(ctx+0x2a78,0);native_tu15_cull_walk_44710(m,fp,{});
}
J run(const J& row) {
    Memory m;
    for(const auto& bank:row.at("banks")) {
        const auto s=bank.at("before").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<s.size();i+=2)bytes.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
        m.banks.push_back({bank.at("address"),std::move(bytes)});
    }
    const auto& controls=row.at("before_fp");NativeTu15PhysicsFrameFp fp{controls[0],controls[1],controls[2]};
    J calls=J::array();NativeTu15CullWalkServices services;
    services.frustum=[&](auto unit,auto entry,auto&){calls.push_back(J{{"kind","frustum"},{"unit",unit},{"entry",entry}});};
    services.sphere=[&](auto unit,const auto& center,auto radius,auto&){calls.push_back(J{{"kind","sphere"},{"unit",unit},{"center",center},{"radius",radius}});};
    native_tu15_cull_walk_44710(m,fp,services);
    return {{"calls",calls},{"fp",{fp.control_word,fp.status,fp.mxcsr}}};
}
}
int main()try {
    controls();
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
