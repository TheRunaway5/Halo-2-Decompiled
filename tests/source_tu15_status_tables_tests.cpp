#include "halo2/source_tu15_status.hpp"
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
    check(argc==3,"Supply original kernel301 fixture and owned table capture");
    for(unsigned i=1;i<3;++i)if(!std::filesystem::exists(argv[i]))return 77;
    std::ifstream in(argv[1]);const auto f=J::parse(in);
    check(f.at("original_code_executed").get<bool>() && f.at("code_patches").empty() &&
        !f.at("native_expected_data_used").get<bool>() && !f.at("physical_xbox_proof").get<bool>(),"Wrong kernel301 evidence scope");
    const auto tables=halo2::SourceTu15StatusTables::load(argv[2]);
    std::size_t cases=0,instructions=0,writes=0;std::set<std::uint32_t> covered;
    for(const auto &row:f.at("cases")) {
        M memory;tables.mount(memory);
        memory.mount(f.at("stack_address"),bytes(f.at("initial_stack_hex")),M::Access::ReadWrite,"fresh patterned kernel301 source stack");
        const auto sp=row.at("stack_pointer").get<std::uint32_t>();put(memory,sp,0x3000000);put(memory,sp+4,row.at("status"));
        auto abi=state(row.at("before"));std::size_t trace=0,stores=0;
        halo2::SourcePhysicsContactRuntimeServices services;
        services.instruction=[&](auto p) {
            check(trace<row.at("instructions").size() && p==row.at("instructions").at(trace++),"Complete kernel301 trace differs");covered.insert(p);
        };
        const auto observe=[&](const auto &a) {
            check(stores<row.at("stores").size(),"Unexpected kernel301 store");const auto &b=row.at("stores").at(stores++);
            check(a.instruction==b.at("instruction") && trace==b.at("trace_index"),"Complete kernel301 write order differs");change_equal(a,b);
        };
        halo2::source_run_tu15_status_runtime(memory,abi,{0x27f,0x1f80},services,observe);
        check(trace==row.at("instructions").size() && stores==row.at("stores").size(),"Kernel301 returned before complete source trace");
        cpu_equal(abi,row.at("after"),0xcc5);same_bank(memory,row.at("final_bank").at("address"),bytes(row.at("final_bank").at("hex")));
        tables.validate(memory);++cases;instructions+=trace;writes+=stores;
    }
    const auto expected=f.at("covered_instructions").get<std::set<std::uint32_t>>();check(covered==expected,"Kernel301 coverage differs");
    // Reject lost table ownership and invalid source caller frames before a
    // converter writes its stack or changes the CPU bank.
    M m;std::vector<std::byte> stack(0x1000,std::byte{0xa5});m.mount(0x2000,stack,M::Access::ReadWrite,"negative caller input");
    A a{};a.gp[4]=0x2800;a.gp[0]=0xc000009c;put(m,0x2800,0x2e1000);put(m,0x2804,a.gp[0]);
    const auto before=a;bool rejected=false;
    try {halo2::source_try_tu15_status_conversion(m,{0x2e0ffa,0x8000012d,false},a,{0x27f,0x1f80},tables);}
    catch(const std::exception &) {rejected=true;}
    check(rejected && a.gp==before.gp && a.eflags==before.eflags,"Missing table owner changed CPU");
    tables.mount(m);const auto input=std::vector<std::byte>(m.view(0x2000,0x1000).begin(),m.view(0x2000,0x1000).end());
    for(unsigned fault=0;fault<3;++fault) {
        a=before;put(m,0x2800,fault==1?0xbad00bad:0x2e1000);put(m,0x2804,fault==2?0:a.gp[0]);
        const auto retained=std::vector<std::byte>(m.view(0x2000,0x1000).begin(),m.view(0x2000,0x1000).end());rejected=false;
        try {halo2::source_try_tu15_status_conversion(m,{0x2e0ffa,0x8000012d,fault==0},a,{0x27f,0x1f80},tables);}
        catch(const std::exception &) {rejected=true;}
        check(rejected && a.gp==before.gp && a.eflags==before.eflags,"Invalid kernel301 frame changed CPU");same_bank(m,0x2000,retained);
    }
    check(!halo2::source_try_tu15_status_conversion(m,{0x2e0ffb,0x8000012d,false},a,{0x27f,0x1f80},tables),"Unowned kernel301 caller admitted");
    std::cout<<"PASS owned kernel301 tables: "<<cases<<" cases, "<<instructions<<" original instructions, "<<writes<<" writes, "<<covered.size()<<" sites; modified BIOS resource scope only\n";
 } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
