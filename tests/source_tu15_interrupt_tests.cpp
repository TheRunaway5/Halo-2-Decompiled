#include "halo2/source_tu15_interrupt.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>
namespace {
using J=nlohmann::json;using M=halo2::SourceRuntimeMemory;using A=halo2::SourcePhysicsContactRuntimeAbi;
struct RequiredCallee {};
void check(bool v,const char *why) {if(!v)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &s) {
    check(!(s.size()&1),"Invalid byte capture");std::vector<std::byte> b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;
}
A state(const J &j) {
    A a{};a.gp=j.at("gp").get<decltype(a.gp)>();a.xmm=j.at("xmm").get<decltype(a.xmm)>();
    a.eflags=j.at("eflags");a.fs_base=j.at("fs_base");a.x87_status=j.at("x87_status");
    a.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{j.at("x87_control_word"),j.at("mxcsr")};return a;
}
void equal(const A &a,const J &j) {
    const auto b=state(j);
    check(a.gp==b.gp && a.xmm==b.xmm,"Original interrupt GP/XMM differs");
    check(((a.eflags^b.eflags)&0xec5)==0 && a.fs_base==b.fs_base && a.x87_depth==0 &&
        a.x87_status==b.x87_status && !a.active_mmx && a.floating_point &&
        a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
        a.floating_point->mxcsr==b.floating_point->mxcsr,"Original interrupt flags/FS/FP differs");
}
}
int main(int argc,char **argv) {
 try {
    check(argc==3,"Supply original interrupt fixture and owned kernel text");
    for(unsigned i=1;i<3;++i)if(!std::filesystem::exists(argv[i]))return 77;
    std::ifstream in(argv[1]);const auto f=J::parse(in);
    check(f.at("procedure_context_inputs").get<bool>() && f.at("procedure_clock_inputs").get<bool>() &&
        !f.at("captured_scheduler_state_used").get<bool>() && !f.at("callee_returns_supplied").get<bool>() &&
        f.at("code_patches").empty() && !f.at("live_physics_parity").get<bool>(),"Wrong interrupt evidence scope");
    std::size_t cases=0,instructions=0,writes=0,ports_total=0,timers=0;std::set<std::uint32_t> covered;
    for(const auto &row:f.at("cases")) {
        M memory;halo2::source_mount_tu15_timer_constants(memory,argv[2]);
        halo2::source_mount_tu15_interrupt_masks(memory,argv[2]);
        for(const auto &bank:row.at("initial_banks"))memory.mount(bank.at("address"),bytes(bank.at("hex")),
            M::Access::ReadWrite,"declared original procedure input, not a running scheduler snapshot");
        const halo2::SourceTu15TimerDataOwner data=[&](auto &bank,const auto request) {
            check(&bank==&memory,"Interrupt resource bank mismatch");memory.view(request.address,request.bytes);
        };
        for(const auto &phase:row.at("phases")) {
            auto abi=state(phase.at("before"));memory.write(abi.gp[4],bytes(phase.at("frame")));
            std::size_t trace=0,stores=0,ports=0;
            halo2::SourcePhysicsContactRuntimeServices base;
            base.instruction=[&](auto p) {
                check(trace<phase.at("instructions").size() && p==phase.at("instructions").at(trace++),"Original interrupt trace differs");covered.insert(p);
            };
            base.required_owner=[&](auto &bank,const auto &call,auto &,const auto &) {
                check(&bank==&memory && !phase.at("boundary").is_null() && call.target==phase.at("boundary").at("target") &&
                    call.instruction==phase.at("boundary").at("instruction") && call.tail==phase.at("boundary").at("tail"),"Wrong missing scheduler/dispatch boundary");
                throw RequiredCallee{};
            };
            const auto observe=[&](const auto &a) {
                check(stores<phase.at("stores").size(),"Unexpected interrupt store");const auto &b=phase.at("stores").at(stores++);
                check(a.instruction==b.at("instruction") && trace==b.at("trace_index") && a.address==b.at("address") &&
                    a.before==bytes(b.at("before")) && a.written==bytes(b.at("written")),"Original interrupt ordered store differs");
            };
            const halo2::SourceTu15InterruptPortOwner port=[&](auto a) {
                check(ports<phase.at("ports").size(),"Unexpected PIC output");const auto &b=phase.at("ports").at(ports++);
                check(a.instruction==b.at("instruction") && trace==b.at("trace_index") && a.port==b.at("port") && a.value==b.at("value"),"Original PIC output differs");
            };
            bool stopped=false;const auto entry=phase.at("entry").get<std::uint32_t>();
            try {
                if(entry==0x80018ba0||entry==0x80014368)
                    halo2::source_run_tu15_interrupt_runtime(memory,entry,abi,{0x27f,0x1f80},base,observe,data,port);
                else {
                    const auto joined=halo2::source_tu15_interrupt_services(base,{0x27f,0x1f80},data,port);
                    halo2::source_run_tu15_timer_runtime(memory,entry,abi,{0x27f,0x1f80},joined,observe,data);
                }
            } catch(const RequiredCallee &) {stopped=true;}
            check(stopped==!phase.at("boundary").is_null() && trace==phase.at("instructions").size() &&
                stores==phase.at("stores").size() && ports==phase.at("ports").size(),"Incomplete original interrupt procedure");
            try {equal(abi,phase.at("after"));} catch(...) {std::cerr<<"case "<<cases<<" entry "<<entry<<'\n';throw;}
            instructions+=trace;writes+=stores;ports_total+=ports;if(entry==0x800195d6&&!stopped)++timers;
        }
        for(const auto &bank:row.at("final_banks")) {
            const auto b=bytes(bank.at("hex"));check(std::ranges::equal(memory.view(bank.at("address"),b.size()),b),"Final interrupt bank differs");
        }
        ++cases;
    }
    check(covered==f.at("covered_instructions").get<std::set<std::uint32_t>>(),"Original interrupt coverage differs");
    // Missing context must stop before reading it; original CMP executes first.
    M memory;memory.mount(0x1000,std::vector<std::byte>(4096),M::Access::ReadWrite,"negative source stack");
    A abi{};abi.gp[4]=0x1800;abi.eflags=0x202;abi.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    bool missing=false;
    try {halo2::source_run_tu15_interrupt_runtime(memory,0x80018ba0,abi,{0x27f,0x1f80},{});}
    catch(const halo2::SourceTu15TimerDataUnavailable &e) {missing=e.request.instruction==0x80018ba0&&e.request.address==0x80035c08&&e.request.bytes==4;}
    check(missing && abi.gp[4]==0x1800,"Missing context owner was substituted");
    std::cout<<"PASS original interrupt lowering: "<<cases<<" cases, "<<instructions<<" instructions, "<<writes<<" stores, "
        <<ports_total<<" PIC outputs, "<<timers<<" connected timer returns; context/clock inputs remain procedure-only\n";
 } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
