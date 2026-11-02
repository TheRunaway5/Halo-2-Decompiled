#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/native_campaign_speech_smoothing_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <xmmintrin.h>
using namespace halo2;using J=nlohmann::json;
namespace {
void check(bool v,const std::string &why){if(!v)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &s){std::vector<std::byte> v;for(std::size_t n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte> data){constexpr char digits[]="0123456789abcdef";std::string s;for(auto b:data){const auto n=std::to_integer<unsigned>(b);s+=digits[n>>4];s+=digits[n&15];}return s;}
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory:SourcePhysicsContactMemory {
    mutable std::map<std::uint32_t,std::vector<std::byte>> banks;
    J timeline=J::array();mutable J applied=J::array(),reads=J::array(),writes=J::array();
    mutable std::map<std::uint32_t,unsigned> occurrences;
    std::string phase;unsigned ordinal{};mutable unsigned read_count{};unsigned write_count{};
    explicit Memory(const J &row){for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));timeline=row.at("input_timeline");const auto f=row.value("fault",J::object());phase=f.value("phase","");ordinal=f.value("ordinal",0u);}
    auto locate(std::uint32_t address,std::size_t width)const {
        auto i=banks.upper_bound(address);if(i==banks.begin())throw Missing{address,width};--i;
        if(std::uint64_t(address-i->first)+width>i->second.size())throw Missing{address,width};return i;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read"&&read_count==ordinal)throw Prefix{};
        const auto i=locate(address,out.size());std::copy_n(i->second.begin()+address-i->first,out.size(),out.begin());
        reads.push_back(J::array({address,out.size()}));++read_count;const auto occurrence=++occurrences[address];
        for(const auto &change:timeline)if(change.at("on_read").at("address")==address&&change.at("on_read").at("occurrence")==occurrence){
            for(const auto &edit:change.at("writes")){const auto data=bytes(edit.at("bytes"));const auto target=edit.at("address").get<unsigned>();const auto region=locate(target,data.size());std::copy(data.begin(),data.end(),region->second.begin()+target-region->first);}applied.push_back(change);
        }
    }
    void write(std::uint32_t address,std::span<const std::byte> data)override {
        if(phase=="before_write"&&write_count==ordinal)throw Prefix{};
        const auto i=locate(address,data.size());auto start=i->second.begin()+address-i->first;
        writes.push_back(J{{"instruction",0x18f804u},{"address",address},{"before",hex(std::span<const std::byte>(&*start,data.size()))},{"written",hex(data)}});
        std::copy(data.begin(),data.end(),start);const auto n=write_count++;
        if(phase=="after_write"&&n==ordinal)throw Prefix{};
    }
};
SourcePhysicsContactRuntimeAbi caller(const J &row) {
    SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();
    for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x6b000000u+row.at("id").get<unsigned>()*37u+n*4u+k;
    abi.xmm[0]=row.at("input_xmm0").get<std::array<std::uint32_t,4>>();
    abi.eflags=0x2d7;abi.fs_base=0x514000u;abi.x87_status=row.at("before_fp")[1];
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    if(row.at("active_mmx").get<bool>()){
        SourcePhysicsMmxAlias mmx{};mmx.known_mask=0xff;
        for(unsigned n=0;n<8;++n){const auto data=bytes(row.at("live_x87")[n]);std::copy(data.begin(),data.end(),mmx.physical[n].begin());}abi.active_mmx=mmx;
    }else{abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto data=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],data.data(),10);}}
    return abi;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply original smoothing fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("backend")=="physical-unchanged-i386-offline","Original smoothing authority differs");
    check(fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Original smoothing supplied an outcome");
    check(native_tu15_sound_manager_owns_entry(0x18f7a0u)&&!native_tu15_sound_manager_calls_out(0x18f7a0u),"Actual shadow unit registry lacks complete smoothing owner");
    check(native_tu15_sound_manager_owns_entry(0x21f860u),"Real unit registry lost envelope owner");
    unsigned checked{},observed{},late_returns{},mmx{},full_x87{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>()&&row.at("entry_live_x87_preserved").get<bool>(),name+": original live state missing");
        for(bool observing:std::array{false,true}) {
            const auto fault=row.value("fault",J::object());const auto phase=fault.value("phase",std::string{});
            // Diagnostic before-bytes belong only to this second observer pass.
            // Retain exact original read boundaries in the first pass and raw
            // before-read/write prefixes. Observer exceptions own after-write.
            if(observing&&(phase=="before_read"||phase=="before_write"))continue;
            Memory memory(row);if(observing&&phase=="after_write")memory.phase.clear();
            auto abi=caller(row);const auto initial=abi;const auto policy=abi.floating_point.value();
            J events=J::array();SourcePhysicsContactRecordObserver observer;
            if(observing)observer=[&](const auto &e){events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});if(phase=="after_write"&&events.size()-1==fault.at("ordinal"))throw Prefix{};};
            const auto host_mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            bool stopped{};SourcePhysicsContactRuntimeServices services;services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Smoothing requested a supplied dependency");};
            try{native_tu15_sound_manager_run(memory,0x18f7a0u,abi,policy,observer,services);}
            catch(const Prefix &){stopped=true;}
            catch(const Missing &e){stopped=true;check(row.value("expected_fault",false),name+": unexpected missing source storage");bool matched{};for(const auto &operand:row.at("boundary").at("operands"))matched|=operand.at("address")==e.address&&operand.at("bytes")==e.width;check(matched,name+": source fault provenance differs");}
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(cw==after_cw&&sw==after_sw&&host_mxcsr==_mm_getcsr(),name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": completion differs");
            check(J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})==row.at("after_fp"),name+": FP differs");
            check(abi.gp[4]==initial.gp[4]+(stopped?0u:4u),name+": return stack differs");
            for(unsigned n=0;n<8;++n)if(n!=4)check(abi.gp[n]==initial.gp[n],name+": opaque or retired GP changed");
            if(!stopped)for(unsigned n:{3u,5u,6u,7u})check(abi.gp[n]==row.at("boundary_gp")[n],name+": preserved original GP differs");
            check(abi.xmm[0]==row.at("boundary_xmm0").get<std::array<std::uint32_t,4>>(),name+": delta SIMD lanes differ native="+J(abi.xmm[0]).dump()+" original="+row.at("boundary_xmm0").dump());
            check(abi.xmm[1]==row.at("boundary_xmm1").get<std::array<std::uint32_t,4>>(),name+": level SIMD lanes differ native="+J(abi.xmm[1]).dump()+" original="+row.at("boundary_xmm1").dump());
            for(unsigned n=2;n<8;++n)check(abi.xmm[n]==initial.xmm[n],name+": preserved SIMD differs");
            check(abi.x87_depth==initial.x87_depth,name+": x87 depth differs");for(unsigned n=0;n<abi.x87_depth;++n)check(!std::memcmp(&abi.x87[n],&initial.x87[n],10),name+": x87 quantity differs");
            check(bool(abi.active_mmx)==bool(initial.active_mmx),name+": MMX ownership differs");if(abi.active_mmx)check(abi.active_mmx->physical==initial.active_mmx->physical&&abi.active_mmx->known_mask==initial.active_mmx->known_mask,name+": MMX values differ");
            check(abi.eflags==initial.eflags&&abi.fs_base==initial.fs_base,name+": opaque policy differs");
            for(const auto &bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": retained bytes differ");
            check(memory.applied==row.at("applied_input_timeline"),name+": timeline differs");
            check(memory.writes==row.at("writes"),name+": actual stores differ");
            if(observing){check(events==row.at("writes"),name+": observer stores differ");observed+=events.size();}
            else{J expected=J::array();for(const auto &r:row.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==expected,name+": ordered reads differ");}
        }
        if(!row.at("boundary").is_null()&&row.at("boundary").at("instruction")==0x18f809u)++late_returns;
        if(row.at("active_mmx").get<bool>())++mmx;else if(row.at("live_x87").size()==8)++full_x87;++checked;
    }
    check(checked==218&&late_returns==4&&observed&&mmx&&full_x87,"Smoothing manager profile incomplete");
    std::cout<<checked<<" original smoothing cases, "<<observed<<" observer stores, "<<late_returns<<" late returns, "<<mmx<<" MMX and "<<full_x87<<" full x87 profiles passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
