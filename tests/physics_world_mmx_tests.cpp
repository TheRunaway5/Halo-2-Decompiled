#include "halo2/physics_world_runtime.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using J=nlohmann::json;
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &text) {
    check(text.size()%2==0,"Invalid original MMX fixture bytes");std::vector<std::byte> result;
    for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
struct Memory : halo2::SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto at(std::uint32_t p)const {
        auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing original MMX bank");--i;
        check(p-i->first<i->second.size(),"Missing original MMX bank extent");return i;
    }
    void read(std::uint32_t p,std::span<std::byte> data)const override {
        for(auto &v:data){const auto i=at(p);v=i->second[p++-i->first];}
    }
    void write(std::uint32_t p,std::span<const std::byte> data)override {
        for(auto v:data){const auto i=at(p);banks.at(i->first)[p++-i->first]=v;}
    }
};
halo2::SourcePhysicsContactRuntimeAbi abi(const J &j) {
    halo2::SourcePhysicsContactRuntimeAbi a{};a.gp=j.at("gp").get<decltype(a.gp)>();
    a.eflags=j.at("eflags");a.fs_base=j.at("fs_base");a.x87_status=j.at("x87_status");
    a.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{j.at("x87_control_word"),j.at("mxcsr")};
    a.x87_depth=j.at("x87_values").size();check(a.x87_depth<=8&&sizeof(long double)>=10,"Unsupported original MMX/x87 value backend");
    for(unsigned i=0;i<a.x87_depth;++i){const auto raw=bytes(j.at("x87_values")[i]);check(raw.size()==10,"Invalid original extended value");std::memcpy(&a.x87[i],raw.data(),10);}
    for(unsigned n=0;n<8;++n){const auto s=j.at("xmm")[n].get<std::string>();for(unsigned k=0;k<4;++k)a.xmm[n][k]=std::stoul(s.substr(24-k*8,8),nullptr,16);}
    if(j.contains("active_mmx")) {
        halo2::SourcePhysicsMmxAlias alias{};alias.known_mask=j.at("active_mmx").at("known_mask");
        for(unsigned i=0;i<8;++i){const auto raw=bytes(j.at("active_mmx").at("physical")[i]);check(raw.size()==10,"Invalid original physical MMX alias");std::copy(raw.begin(),raw.end(),alias.physical[i].begin());}
        a.active_mmx=alias;
    }
    return a;
}
struct Stop {};
void check_host_mmx_alias() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    alignas(16) std::array<std::byte,512> saved,active,empty;
    const std::array<std::uint64_t,8> values{0,1,0xffffu,0x8000800080008000ull,
        0xffffffffffffffffull,0x7fff8000ffff0000ull,0x123456789abcdef0ull,0xfedcba9876543210ull};
    // Independently sample the CPU's physical upper words and abridged tags.
    // Restore the entire host FP/SSE context before returning to C++.
    asm volatile("fxsave %0\n\tfninit\n\tmovq 0(%3),%%mm0\n\tmovq 8(%3),%%mm1\n\tmovq 16(%3),%%mm2\n\tmovq 24(%3),%%mm3\n\tmovq 32(%3),%%mm4\n\tmovq 40(%3),%%mm5\n\tmovq 48(%3),%%mm6\n\tmovq 56(%3),%%mm7\n\tfxsave %1\n\temms\n\tfxsave %2\n\tfxrstor %0"
        :"=m"(saved),"=m"(active),"=m"(empty):"r"(values.data())
        :"memory","mm0","mm1","mm2","mm3","mm4","mm5","mm6","mm7");
    check(active[4]==std::byte{255}&&empty[4]==std::byte{0},"Host MMX valid/empty tags differ from Intel SDM");
    check((std::to_integer<unsigned>(active[3])&0x38u)==0&&(std::to_integer<unsigned>(empty[3])&0x38u)==0,"Host MMX TOP differs from Intel SDM");
    for(unsigned i=0;i<8;++i) {
        std::uint64_t actual=0;std::memcpy(&actual,active.data()+32+i*16,8);
        check(actual==values[i]&&active[40+i*16]==std::byte{255}&&active[41+i*16]==std::byte{255},"Host physical MMX payload/upper words differ from Intel SDM");
    }
#else
    throw std::runtime_error("Independent host MMX probe unavailable");
#endif
}
}

int main(int argc,char **argv) {try {
    check(argc==2,"Supply independent owned MMX fixture");std::ifstream input(argv[1]);check(bool(input),"Original MMX fixture missing");
    const auto fixture=J::parse(input);check_host_mmx_alias();std::size_t total=0,partial=0,writes=0,instructions=0;
    for(const auto &c:fixture.at("cases")) {
        Memory memory;
        for(const auto &b:c.at("initial_banks"))memory.banks.emplace(b.at("address").get<std::uint32_t>(),bytes(b.at("hex")));
        auto state=abi(c.at("entry_abi"));std::vector<std::uint32_t> trace;std::vector<halo2::SourcePhysicsContactRecordStore> stores;
        halo2::SourcePhysicsWorldRuntimeServices services;const auto stop=c.at("stop_before").get<std::uint32_t>();
        services.instruction=[&](auto p){if(stop&&p==stop)throw Stop{};trace.push_back(p);};
        bool stopped=false;
        try {halo2::source_run_physics_world_runtime(memory,c.at("canonical_entry"),state,
            c.at("image")=="1.5"?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11,
            *state.floating_point,services,[&](const auto &store){stores.push_back(store);});}
        catch(const Stop &){stopped=true;}
        check(stopped==!c.at("complete").get<bool>(),"Original MMX completion differs");
        check(trace==c.at("instructions").get<std::vector<std::uint32_t>>(),"Original MMX instruction path differs");
        check(stores.size()==c.at("writes").size(),"Original MMX store count differs");
        for(std::size_t i=0;i<stores.size();++i) {
            const auto &a=stores[i];const auto &b=c.at("writes")[i];
            check(a.instruction==b.at("instruction")&&a.address==b.at("address")&&a.before==bytes(b.at("before"))&&a.written==bytes(b.at("written")),"Original MMX ordered store differs");
        }
        const auto expected=abi(c.at("exit"));
        check(state.gp==expected.gp&&state.xmm==expected.xmm&&state.fs_base==expected.fs_base,"Original MMX GP/SSE/FS state differs");
        check((state.eflags&0xcd5u)==(expected.eflags&0xcd5u),"Original MMX defined flags differ");
        check(state.x87_depth==expected.x87_depth&&(state.x87_status&0x7d3fu)==(expected.x87_status&0x7d3fu),"Original MMX x87 tag/TOP/status differs");
        check(state.floating_point->x87_control_word==expected.floating_point->x87_control_word&&state.floating_point->mxcsr==expected.floating_point->mxcsr,"Original MMX FP controls differ");
        check(bool(state.active_mmx)==bool(expected.active_mmx),"Original active MMX ownership differs");
        if(expected.active_mmx) {
            check(state.active_mmx->known_mask==expected.active_mmx->known_mask,"Original physical MMX known state differs");
            for(unsigned i=0;i<8;++i) {
                const auto &actual=state.active_mmx->physical[i],&wanted=expected.active_mmx->physical[i];
                check(std::equal(actual.begin(),actual.begin()+8,wanted.begin()),"Original physical MMX low64 payload differs");
                // Unicorn2.1.4 retains the incoming physical upper words. Use
                // the documented ISA and independent CPU probe for those bits.
                check(actual[8]==std::byte{255}&&actual[9]==std::byte{255},"Native MMX upper words differ from Intel SDM and host probe");
            }
            ++partial;
        }
        for(const auto &b:c.at("final_banks")) {
            const auto wanted=bytes(b.at("hex"));std::vector<std::byte> actual(wanted.size());memory.read(b.at("address"),actual);check(actual==wanted,"Original MMX retained memory differs");
        }
        ++total;writes+=stores.size();instructions+=trace.size();
    }
    check(total==240&&partial==48,"Incomplete original MMX coverage");
    std::cout<<"PASS original world MMX: "<<total<<" cases, "<<partial<<" active physical-alias prefixes, "<<instructions<<" instructions, "<<writes<<" ordered writes\n";
    return 0;
} catch(const std::exception &e){std::cerr<<"FAIL original world MMX: "<<e.what()<<'\n';return 1;}}
