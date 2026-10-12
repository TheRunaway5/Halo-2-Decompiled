#include "halo2/source_tu15_timer.hpp"
#include "halo2/source_runtime_image.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>

namespace {
using J=nlohmann::json;
using M=halo2::SourceRuntimeMemory;
using A=halo2::SourcePhysicsContactRuntimeAbi;
struct SettingsOwnerRequired {};
void check(bool value,const std::string &why) { if(!value)throw std::runtime_error(why); }
std::vector<std::byte> bytes(const std::string &s) {
    check(!(s.size()&1),"Invalid byte capture");std::vector<std::byte> b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return b;
}
void put(M &m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4> b;
    for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));m.write(p,b);
}
A state(const J &j) {
    A a{};a.gp=j.at("gp").get<decltype(a.gp)>();a.xmm=j.at("xmm").get<decltype(a.xmm)>();
    a.eflags=j.at("eflags");a.fs_base=j.at("fs_base");a.x87_status=j.at("x87_status");
    a.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{j.at("x87_control_word"),j.at("mxcsr")};return a;
}
void cpu_equal(const A &a,const J &j,std::uint32_t mask) {
    const auto b=state(j);
    check(a.gp==b.gp && a.xmm==b.xmm,"TU1.5 pending GP/XMM ABI differs");
    // TEST leaves AF undefined; the earlier missing-owner boundary follows SUB
    // and also checks its defined AF. Each caller supplies the exact mask.
    check(((a.eflags^b.eflags)&mask)==0 && a.fs_base==b.fs_base &&
        a.x87_depth==0 && a.x87_status==b.x87_status && !a.active_mmx && a.floating_point &&
        a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
        a.floating_point->mxcsr==b.floating_point->mxcsr,"TU1.5 retained flags/FS/FP ABI differs");
}
void change_equal(const halo2::SourcePhysicsContactRecordStore &a,const J &b) {
    check(a.address==b.at("address") && a.before==bytes(b.at("before")) &&
        a.written==bytes(b.at("written")),"Independent primary record store differs");
}
void same_bank(M &m,std::uint32_t p,const std::vector<std::byte> &b) {
    check(std::ranges::equal(m.view(p,b.size()),b),"Retained source memory differs");
}
std::uint32_t word(std::span<const std::byte> b,std::size_t p) {
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[p+i])<<(i*8);
    return value;
}
}
int main(int argc,char **argv) {
 try {
    check(argc==3,"Supply original timer procedure fixture and owned kernel text");
    for(unsigned i=1;i<3;++i)if(!std::filesystem::exists(argv[i]))return 77;
    std::ifstream in(argv[1]);const auto f=J::parse(in);
    check(f.at("original_producers_executed").get<bool>() && f.at("procedure_clock_inputs").get<bool>() &&
        !f.at("captured_scheduler_state_used").get<bool>() && !f.at("live_timer_scheduling").get<bool>() && f.at("code_patches").empty(),"Wrong original timer scope");
    std::size_t cases=0,instructions=0,writes=0;std::set<std::uint32_t> covered;
    for(const auto &row:f.at("cases")) {
        M memory;halo2::source_mount_tu15_timer_constants(memory,argv[2]);
        for(const auto &bank:f.at("initial_banks"))memory.mount(bank.at("address"),bytes(bank.at("hex")),M::Access::ReadWrite,"fresh declared raw constructor backing");
        memory.mount(0x8003a880,bytes(row.at("clocks_hex")),M::Access::ReadOnly,"declared stable procedure clock inputs");
        const halo2::SourceTu15TimerDataOwner data=[&](auto &bank,const auto request) {
            check(&bank==&memory && ((0x80010cc8<=request.address && std::uint64_t(request.address)+request.bytes<=0x80010cd1)||
                (0x8003a880<=request.address && std::uint64_t(request.address)+request.bytes<=0x8003a89c)),"Unexpected timer procedure data owner");
            memory.view(request.address,request.bytes);
        };
        for(const auto &phase:row.at("phases")) {
            auto abi=state(phase.at("before"));memory.write(abi.gp[4],bytes(phase.at("frame")));
            std::size_t trace=0,stores=0;halo2::SourcePhysicsContactRuntimeServices services;
            services.instruction=[&](auto p) {check(trace<phase.at("instructions").size() && p==phase.at("instructions").at(trace++),"Original timer procedure trace differs");covered.insert(p);};
            services.required_owner=[](auto &,const auto &,auto &,const auto &) {throw std::runtime_error("Unexpected timer procedure callee");};
            const auto observe=[&](const auto &a) {
                check(stores<phase.at("stores").size(),"Unexpected timer procedure store");const auto &b=phase.at("stores").at(stores++);
                check(a.instruction==b.at("instruction") && trace==b.at("trace_index"),"Timer procedure store order differs");change_equal(a,b);
            };
            halo2::source_run_tu15_timer_runtime(memory,phase.at("entry"),abi,{0x27f,0x1f80},services,observe,data);
            check(trace==phase.at("instructions").size() && stores==phase.at("stores").size(),"Incomplete timer constructor/procedure");
            try {cpu_equal(abi,phase.at("after"),0xcc5);} catch(...) {
                std::cerr<<"case "<<cases<<" entry "<<phase.at("entry")<<" actual GP "<<J(abi.gp)
                    <<" expected "<<phase.at("after").at("gp")<<'\n';throw;
            }
            instructions+=trace;writes+=stores;
        }
        for(const auto &bank:row.at("final_banks"))same_bank(memory,bank.at("address"),bytes(bank.at("hex")));
        ++cases;
    }
    check(covered==f.at("covered_instructions").get<std::set<std::uint32_t>>(),"Original timer procedure coverage differs");
    std::cout<<"PASS original timer procedures: "<<cases<<" fresh cases, "<<instructions<<" instructions, "<<writes<<" stores, "<<covered.size()<<" sites; live clock/context ownership remains required\n";
 } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
