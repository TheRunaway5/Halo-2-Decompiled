#include "gameplay/multiplayer/source_objective_native_tu15_internal.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
using Json=nlohmann::json;
namespace {
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    void set(std::uint32_t at,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(8*n));
    }
    void read(std::uint32_t at,std::span<std::byte> to)const override {
        for(unsigned n=0;n<to.size();++n)to[n]=bytes.at(at+n);
    }
    void write(std::uint32_t at,std::span<const std::byte> from)override {
        for(unsigned n=0;n<from.size();++n)bytes.at(at+n)=from[n];
    }
};
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
}
int main(int argc,char** argv)try {
    need(argc==2,"Usage: objective_native_tu15_tests ORIGINAL_FIXTURE");
    Json fixture;std::ifstream(argv[1])>>fixture;
    need(fixture.at("schema")=="H2_NATIVE_OBJECTIVE_RULE_WINDOWS_V1" &&
        fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("supplied_service_results").empty(),"Unauthenticated native objective reference");
    unsigned count{};
    for(const auto& row:fixture.at("cases")) {
        Memory memory;
        memory.set(0x4f3218u,0x4000000u);memory.set(0x51df54u,0x4002000u);
        memory.set(0x4000244u,row.at("seconds").get<std::uint32_t>(),2);
        memory.set(0x4002002u,row.at("rate").get<unsigned>(),2);
        auto gp=row.at("gp").get<std::array<std::uint32_t,8>>();
        const auto at=0x4004000u+0x1a0u+gp[7]*2u;
        memory.set(at,row.at("progress").get<std::uint32_t>(),2);
        SourcePhysicsWorldRuntimeServices services;
        Json writes=Json::array();
        SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore& store) {
            std::uint32_t value{};
            for(unsigned n=0;n<store.written.size();++n)value|=std::to_integer<unsigned>(store.written[n])<<(8*n);
            writes.push_back({store.instruction,store.address,store.written.size(),value});
        };
        physics_world_runtime_detail::WorldState state{memory,observe,services,SourcePhysicsContactImage::MapPack11};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=gp;abi.eflags=row.at("eflags");
        state.load(abi);
        const bool arm=row.at("kind")=="arm";
        need(arm?source_objective_native_tu15_detail::arm(state):source_objective_native_tu15_detail::decay(state),
            "Native objective kernel declined a declared input");
        const auto out=state.save();
        need(out.gp==row.at("expected_gp").get<decltype(out.gp)>(),"Native objective kernel changed original ABI");
        need((out.eflags&0x8d5u)==row.at("expected_eflags"),"Native objective kernel changed original flags");
        need(writes==row.at("writes"),"Native objective kernel changed ordered original publication");
        const auto value=std::uint16_t(std::to_integer<unsigned>(memory.bytes.at(at))|
            (std::to_integer<unsigned>(memory.bytes.at(at+1))<<8));
        need(std::bit_cast<std::int16_t>(value)==row.at("expected_progress"),"Native objective kernel changed signed16 progress");
        ++count;
    }
    need(count==986,"Native objective rule matrix lost cases");
    std::cout<<"Native objective kernels match "<<count<<" unchanged original windows, including neutral row8, zero-time arming, signed widths and interrupted decay.\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
