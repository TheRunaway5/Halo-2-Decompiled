#include "halo2/native_tu15_startup_platform.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using J=nlohmann::json;using M=halo2::SourceRuntimeMemory;using A=halo2::SourcePhysicsContactRuntimeAbi;
struct TimerRequired {};
void check(bool v,const char *why) { if(!v)throw std::runtime_error(why); }
std::vector<std::byte> bytes(const std::string &s) {
    check(!(s.size()&1),"Invalid byte input");std::vector<std::byte> b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return b;
}
void put(M &m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4> b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));m.write(p,b);
}
A state(const J &j) {
    A a{};a.gp=j.at("gp").get<decltype(a.gp)>();a.xmm=j.at("xmm").get<decltype(a.xmm)>();
    a.eflags=j.at("eflags");a.fs_base=j.at("fs_base");a.x87_status=j.at("x87_status");
    a.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{j.at("x87_control_word"),j.at("mxcsr")};return a;
}
void cpu_equal(const A &a,const J &j,std::uint32_t mask=0xcc5) {
    const auto b=state(j);
    check(a.gp==b.gp && a.xmm==b.xmm && !((a.eflags^b.eflags)&mask) && a.fs_base==b.fs_base &&
        a.x87_depth==0 && a.x87_status==b.x87_status && !a.active_mmx && a.floating_point &&
        a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
        a.floating_point->mxcsr==b.floating_point->mxcsr,"Native startup GP/flags/XMM/FP state differs");
}
void change_equal(const halo2::SourcePhysicsContactRecordStore &a,const J &b) {
    check(a.address==b.at("address") && a.before==bytes(b.at("before")) &&
        a.written==bytes(b.at("written")),"Native startup ordered store differs");
}
void bank_equal(M &m,std::uint32_t p,const std::vector<std::byte> &b) {
    check(std::ranges::equal(m.view(p,b.size()),b),"Native startup retained bank differs");
}
void call_cases(const J &f,const J &row,const halo2::NativeTu15StartupPlatform &owner) {
    const auto stack=f.at("stack_address").get<std::uint32_t>();
    for(const auto &event:row.at("native_calls")) {
        M m;m.mount(stack,bytes(f.at("initial_stack_hex")),M::Access::ReadWrite,"fresh native call stack");
        auto a=state(event.at("before"));const auto sp=a.gp[4],bp=a.gp[5];
        const auto frame=bytes(event.at("frame"));m.write(sp,frame);
        if(event.at("target")==0x80000018u) {
            m.write(bp+8,std::span(frame).subspan(4));
            for(const auto &s:event.at("stores"))m.write(s.at("address"),bytes(s.at("before")));
        }
        const auto initial=std::vector<std::byte>(m.view(stack,0x10000).begin(),m.view(stack,0x10000).end());
        const halo2::SourcePhysicsContactRuntimeCall call{event.at("instruction"),event.at("target"),event.at("target")==0x80000018u};
        std::size_t writes=0;
        check(owner.try_call(m,call,a,[&](const auto &s) {
            check(writes<event.at("stores").size(),"Extra native setting store");
            check(s.instruction==call.instruction,"Native setting store lost its caller");
            change_equal(s,event.at("stores").at(writes++));
        }),"Native startup call was not handled");
        check(writes==event.at("stores").size(),"Missing native setting store");cpu_equal(a,event.at("after"),0xffffffffu);
        for(unsigned fault=0;fault<4;++fault) {
            m.write(stack,initial);a=state(event.at("before"));auto bad=call;
            if(fault==0)bad.tail=!bad.tail;
            if(fault==1)put(m,sp,0xbad00bad);
            if(fault==2) {
                if(call.tail)put(m,sp+4,18);else {put(m,sp+4,0xc000009c);a.gp[0]=0xc000009c;}
            }
            if(fault==3) {
                if(call.tail)put(m,sp+16,8);else put(m,sp+4,1);
            }
            const auto before=a;const auto bank=std::vector<std::byte>(m.view(stack,0x10000).begin(),m.view(stack,0x10000).end());
            bool rejected=false;try {owner.try_call(m,bad,a);}catch(const std::runtime_error &) {rejected=true;}
            check(rejected && a.gp==before.gp && a.eflags==before.eflags,"Invalid native startup frame admitted");bank_equal(m,stack,bank);
        }
        M readonly;readonly.mount(stack,initial,M::Access::ReadOnly,"read-only native output");a=state(event.at("before"));
        if(call.tail) {
            bool rejected=false;try {owner.try_call(readonly,call,a);}catch(const std::runtime_error &) {rejected=true;}
            check(rejected,"Read-only native setting outputs admitted");cpu_equal(a,event.at("before"),0xffffffffu);bank_equal(readonly,stack,initial);
        }
        m.write(stack,initial);a=state(event.at("before"));auto unknown=call;unknown.instruction^=1;
        check(!owner.try_call(m,unknown,a),"Unknown native startup call consumed");cpu_equal(a,event.at("before"),0xffffffffu);bank_equal(m,stack,initial);
    }
}
}
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply native startup comparison fixture and owned TU1.5 title image");
        for(unsigned i=1;i<3;++i)if(!std::filesystem::exists(argv[i]))return 77;
#if !defined(__linux__)
        return 77;
#endif
        std::ifstream input(argv[1]);const auto f=J::parse(input);
        const auto &env=f.at("native_environment");
        check(!env.at("bios_input_used").get<bool>() && !env.at("eeprom_input_used").get<bool>() &&
            !env.at("kernel_tables_used").get<bool>() && !f.at("timer_armed").get<bool>() &&
            !f.at("process_heap_installed").get<bool>() && !f.at("live_physics_parity").get<bool>(),"Wrong native platform scope");
        std::map<std::uint32_t,std::uint32_t> canonical;
        for(auto it=f.at("source_instructions").begin();it!=f.at("source_instructions").end();++it)
            canonical.emplace(std::stoul(it.key(),nullptr,16),it.value().at("canonical"));
        std::size_t cases=0,instructions=0,source_writes=0,native_queries=0;
        for(const auto &row:f.at("cases")) {
            halo2::NativeTu15StartupPlatform owner({row.at("automatic_power_down")});call_cases(f,row,owner);
            auto m=owner.load_image(argv[2]);
            m.mount(f.at("stack_address"),bytes(f.at("initial_stack_hex")),M::Access::ReadWrite,"fresh patterned source stack");
            m.mount(0,std::vector<std::byte>(0x1000),M::Access::ReadWrite,"standalone FS exception-chain input");
            put(m,0,0xffffffff);put(m,row.at("stack_pointer"),0x3000000);
            halo2::SourceRuntimeHostVirtualMemory vm(m);halo2::PhysicsPlatformLocks locks;
            halo2::SourceProcessShutdownRegistry registry(m,f.at("primary_registry_owner").at("head"));
            auto a=state(row.at("before"));std::size_t trace=0,stores=0,records=0,registrations=0,queries=0;
            std::vector<J> expected_records,expected_registry,expected_query;
            for(const auto &call:row.at("platform_calls"))for(const auto &s:call.at("stores")) {
                auto v=s;v["canonical_instruction"]=call.at("canonical_instruction");
                (call.at("target")==0x8000002f?expected_registry:expected_records).push_back(v);
            }
            for(const auto &call:row.at("native_calls"))for(const auto &s:call.at("stores")) {
                auto v=s;v["trace_index"]=call.at("trace_index");expected_query.push_back(v);
            }
            halo2::SourcePhysicsContactRuntimeServices services;
            services.instruction=[&](auto p) {
                check(trace<row.at("instructions").size() && p==canonical.at(row.at("instructions").at(trace++)),"Native startup original trace differs");
            };
            services.required_owner=[&](auto &bank,const auto &call,auto &cpu,const auto &) {
                check(&bank==&m && call.instruction==0x2dc5e6u && call.target==0x80000095u && !call.tail,
                    "Unexpected native startup required owner");
                check(cpu.gp[4]==row.at("boundary").at("frame_address"),"Native startup pending timer stack differs");
                bank_equal(m,cpu.gp[4],bytes(row.at("boundary").at("frame")));throw TimerRequired{};
            };
            const auto source=[&](const auto &s) {
                check(stores<row.at("stores").size(),"Extra source startup store");const auto &b=row.at("stores").at(stores++);
                check(s.instruction==b.at("canonical_instruction") && trace==b.at("trace_index"),"Source startup store attribution differs");change_equal(s,b);
            };
            const auto record=[&](const auto &s) {
                check(records<expected_records.size(),"Extra record store");const auto &b=expected_records.at(records++);
                check(s.instruction==b.at("canonical_instruction") && trace==b.at("trace_index"),"Record store attribution differs");change_equal(s,b);
            };
            const auto shutdown=[&](const auto &s) {
                check(registrations<expected_registry.size(),"Extra shutdown store");const auto &b=expected_registry.at(registrations++);
                check(trace==b.at("trace_index") && s.address==b.at("address") && s.before==bytes(b.at("before")) &&
                    s.written==bytes(b.at("written")),"Shutdown store differs");
            };
            const auto query=[&](const auto &s) {
                check(queries<expected_query.size(),"Extra native query store");const auto &b=expected_query.at(queries++);
                check(s.instruction==b.at("instruction") && trace==b.at("trace_index"),"Native query store attribution differs");change_equal(s,b);
            };
            const halo2::PhysicsPlatformLockServices forbidden{
                []()->std::uint32_t {throw std::runtime_error("Unexpected startup thread request");},
                [](auto){throw std::runtime_error("Unexpected startup wait");},[](auto){throw std::runtime_error("Unexpected startup wake");}};
            bool stopped=false;try {owner.run_startup(m,row.at("canonical_entry"),a,{0x27f,0x1f80},services,vm,locks,forbidden,registry,source,record,shutdown,query);}
            catch(const TimerRequired &) {stopped=true;}
            check(stopped && trace==row.at("instructions").size() && stores==row.at("stores").size() && records==expected_records.size() &&
                registrations==expected_registry.size() && queries==expected_query.size(),"Native startup missed its exact required timer frontier");
            cpu_equal(a,row.at("after"));for(const auto &bank:row.at("final_banks"))bank_equal(m,bank.at("address"),bytes(bank.at("hex")));
            ++cases;instructions+=trace;source_writes+=stores;++native_queries;
        }
        std::cout<<"PASS native desktop TU1.5 startup without BIOS/EEPROM/tables: "<<cases<<" fresh prefixes, "<<instructions<<" original instructions, "
            <<source_writes<<" source stores, "<<native_queries<<" native settings queries; genuine timer owner remains required\n";
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
