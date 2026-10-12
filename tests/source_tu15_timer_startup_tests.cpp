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
    check(argc==5,"Supply TU1.5 settings fixture, owned TU1.5 XBE and actual EEPROM");
    for(unsigned i=1;i<5;++i)if(!std::filesystem::exists(argv[i]))return 77;
#if !defined(__linux__)
    return 77;
#endif
    std::ifstream in(argv[1]);const auto f=J::parse(in);
    check(f.at("parity_target")=="English TU1.5 only" && f.at("nonvolatile_query_executed").get<bool>() &&
        !f.at("older_image_executed").get<bool>() && f.at("settings_wrapper_completed").get<bool>() && f.at("status_converter_executed").get<bool>() &&
        !f.at("timer_armed").get<bool>() && !f.at("process_heap_installed").get<bool>() &&
        !f.at("live_physics_parity").get<bool>(),"Wrong settings evidence scope");
    const auto tables=halo2::SourceTu15StatusTables::load(argv[4]);
    const auto actual=halo2::SourceTu15UserSettings::load(argv[3]);
    halo2::SourceRuntimeKernelHardwareInfo hardware{};
    const auto &record=f.at("kernel_input").at("record");hardware.address=record.at("address");
    const auto hardware_bytes=bytes(record.at("hex"));check(hardware_bytes.size()==8,"Wrong hardware extent");
    std::copy(hardware_bytes.begin(),hardware_bytes.end(),hardware.bytes.begin());
    hardware.owner=f.at("kernel_input").at("reference");
    std::map<std::uint32_t,std::uint32_t> canonical;
    for(auto it=f.at("source_instructions").begin();it!=f.at("source_instructions").end();++it)
        canonical.emplace(std::stoul(it.key(),nullptr,16),it.value().at("canonical"));
    std::size_t cases=0,instructions=0,source_writes=0,query_writes=0;std::set<std::uint32_t> statuses;
    for(const auto &row:f.at("cases")) {
        auto memory=halo2::load_source_runtime_image(argv[2],halo2::SourcePhysicsContactImage::TitleUpdate15,hardware);
        tables.mount(memory);
        memory.mount(f.at("stack_address"),bytes(f.at("initial_stack_hex")),M::Access::ReadWrite,"declared patterned source stack");
        memory.mount(0,std::vector<std::byte>(0x1000),M::Access::ReadWrite,"declared standalone exception chain");
        put(memory,0,0xffffffff);put(memory,row.at("stack_pointer"),0x3000000);
        const auto input=bytes(row.at("user_section").at("hex"));check(input.size()==96,"Wrong UserSettings extent");
        if(row.at("user_section").at("kind")=="actual_owned")
            check(std::ranges::equal(actual.bytes(),input),"Owned EEPROM changed since independent query");
        std::array<std::byte,96> snapshot;std::copy(input.begin(),input.end(),snapshot.begin());
        const halo2::SourceTu15UserSettings settings{snapshot,"actual or explicitly damaged immutable EEPROM input"};
        halo2::SourceRuntimeHostVirtualMemory vm(memory);halo2::PhysicsPlatformLocks locks;
        halo2::SourceProcessShutdownRegistry registry(memory,f.at("primary_registry_owner").at("head"));
        auto abi=state(row.at("before"));std::size_t trace=0,stores=0,records=0,registrations=0,queries=0,kernel_trace=0,kernel_stores=0;
        std::vector<J> expected_records,expected_registry,expected_query;
        for(const auto &call:row.at("platform_calls"))for(const auto &change:call.at("stores")) {
            auto value=change;value["canonical_instruction"]=call.at("canonical_instruction");
            (call.at("target")==0x8000002f?expected_registry:call.at("target")==0x80000018?expected_query:expected_records).push_back(value);
        }
        halo2::SourcePhysicsContactRuntimeServices services;
        services.instruction=[&](auto p) {
            check(trace<row.at("instructions").size() && p==canonical.at(row.at("instructions").at(trace++)),"Fresh settings source trace differs");
        };
        services.required_owner=[&](auto &,const auto &,auto &,const auto &) {
            throw std::runtime_error("Unexpected timer prerequisite callee boundary");
        };
        const auto kernel_instruction=[&](auto p) {
            check(kernel_trace<row.at("kernel_instructions").size() && p==row.at("kernel_instructions").at(kernel_trace++),"Original kernel301 trace differs");
        };
        const auto kernel_observe=[&](const auto &a) {
            check(kernel_stores<row.at("kernel_stores").size(),"Unexpected kernel301 store");
            const auto &b=row.at("kernel_stores").at(kernel_stores++);
            check(a.instruction==b.at("instruction") && trace==b.at("trace_index") && kernel_trace==b.at("kernel_trace_index"),"Kernel301 store order differs");change_equal(a,b);
        };
        const auto observe=[&](const auto &a) {
            check(stores<row.at("stores").size(),"Unexpected original settings source write");
            const auto &b=row.at("stores").at(stores++);
            check(a.instruction==b.at("canonical_instruction") && trace==b.at("trace_index"),"Settings source write order differs");change_equal(a,b);
        };
        const auto record_observe=[&](const auto &a) {
            check(records<expected_records.size(),"Unexpected startup record write");const auto &b=expected_records.at(records++);
            check(a.instruction==b.at("canonical_instruction") && trace==b.at("trace_index"),"Startup record write order differs");change_equal(a,b);
        };
        const auto query_observe=[&](const auto &a) {
            check(queries<expected_query.size(),"Unexpected EEPROM query write");const auto &b=expected_query.at(queries++);
            check(a.instruction==0x2e101c && trace==b.at("trace_index"),"EEPROM query write order differs");change_equal(a,b);
        };
        const auto shutdown_observe=[&](const auto &a) {
            check(registrations<expected_registry.size(),"Unexpected shutdown registry write");const auto &b=expected_registry.at(registrations++);
            check(trace==b.at("trace_index") && a.address==b.at("address") && a.before==bytes(b.at("before")) &&
                a.written==bytes(b.at("written")),"Shutdown registry write differs");
        };
        const halo2::PhysicsPlatformLockServices forbidden{
            []()->std::uint32_t {throw std::runtime_error("Unexpected thread identity query");},
            [](auto){throw std::runtime_error("Unexpected wait");},[](auto){throw std::runtime_error("Unexpected wake");}};
        bool stopped=false;
        try {halo2::source_run_tu15_process_startup_with_timer(memory,row.at("canonical_entry"),abi,{0x27f,0x1f80},
            services,vm,locks,forbidden,registry,settings,tables,observe,record_observe,shutdown_observe,query_observe,
            halo2::source_tu15_hardware_import_owner(hardware),kernel_instruction,kernel_observe);}
        catch(const halo2::SourceTu15TimerDataUnavailable &e) {
            check(e.request.instruction==row.at("boundary").at("instruction") && e.request.address==row.at("boundary").at("address") &&
                e.request.bytes==1,"Wrong original timer resource boundary");
            same_bank(memory,abi.gp[4],bytes(row.at("boundary").at("frame")));
            statuses.insert(row.at("kernel_calls").at(0).at("before").at("gp").at(0).get<std::uint32_t>());stopped=true;
        }
        check(stopped && trace==row.at("instructions").size() && stores==row.at("stores").size() &&
            records==expected_records.size() && queries==expected_query.size() && registrations==expected_registry.size(),"Settings did not retain its exact status boundary");
        check(kernel_trace==row.at("kernel_instructions").size() && kernel_stores==row.at("kernel_stores").size(),"Kernel301 did not complete exact original body");
        cpu_equal(abi,row.at("after"),0xcc5);
        for(const auto &bank:row.at("final_banks"))same_bank(memory,bank.at("address"),bytes(bank.at("hex")));
        if(cases==0) {
            bool rejected=false;
            try {(void)halo2::SourceTu15UserSettings(snapshot,"");}
            catch(const std::invalid_argument &) {rejected=true;}
            check(rejected,"Unnamed EEPROM resource admitted");
            const auto &query=*std::find_if(row.at("platform_calls").begin(),row.at("platform_calls").end(),[](const auto &call) {
                return call.at("target")==0x80000018;
            });
            const auto frame=bytes(query.at("frame"));const auto sp=query.at("frame_address").get<std::uint32_t>();
            const auto before=state(query.at("before_abi"));
            std::array<std::uint32_t,3> output{word(frame,8),word(frame,12),word(frame,20)};
            std::array<std::vector<std::byte>,3> retained;
            for(unsigned i=0;i<3;++i) {auto b=memory.view(output[i],4);retained[i]={b.begin(),b.end()};}
            for(unsigned fault=0;fault<7;++fault) {
                memory.write(sp,frame);abi=before;
                halo2::SourcePhysicsContactRuntimeCall call{0x2e101c,0x80000018,true};
                if(fault==0)call.tail=false;
                if(fault==1)put(memory,sp,0xbad00bad);
                if(fault==2)put(memory,sp+4,18);
                if(fault==3)put(memory,sp+16,3);
                if(fault==4)put(memory,sp+8,output[0]+4);
                if(fault==5)abi.gp[4]+=4;
                if(fault==6)abi.gp[5]+=4;
                const auto pending=abi;rejected=false;
                try {halo2::source_try_tu15_settings_query(memory,call,abi,settings);}
                catch(const std::exception &) {rejected=true;}
                check(rejected && abi.gp==pending.gp && abi.xmm==pending.xmm && abi.eflags==pending.eflags,"Invalid settings frame changed CPU state");
                for(unsigned i=0;i<3;++i)same_bank(memory,output[i],retained[i]);
            }
            memory.write(sp,frame);abi=before;
            check(!halo2::source_try_tu15_settings_query(memory,{0x2e101d,0x80000018,true},abi,settings) &&
                abi.gp==before.gp,"Unowned settings caller changed state");
            M readonly;const auto p=row.at("final_banks").at(0).at("address").get<std::uint32_t>();
            auto bank=memory.view(p,0x140);readonly.mount(p,bank,M::Access::ReadOnly,"actual readonly caller bank");abi=before;rejected=false;
            try {halo2::source_try_tu15_settings_query(readonly,{0x2e101c,0x80000018,true},abi,settings);}
            catch(const std::exception &) {rejected=true;}
            check(rejected && abi.gp==before.gp,"Unavailable writable settings output admitted");
            same_bank(readonly,p,{bank.begin(),bank.end()});
        }
        ++cases;instructions+=trace;source_writes+=stores;query_writes+=queries;
    }
    check(statuses==std::set<std::uint32_t>{0,0xc000009c},"Actual and checksum-damaged query paths were not both exercised");
    std::cout<<"PASS TU1.5 startup timer: "<<cases<<" fresh prefixes, "<<instructions<<" source instructions, "
        <<source_writes<<" source writes, "<<query_writes<<" independent primary query writes; connected original timer149/150; actual kernel context data remains required\n";
 } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
