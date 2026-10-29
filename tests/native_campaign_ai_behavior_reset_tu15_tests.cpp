#include "halo2/native_campaign_ai_behavior_reset_tu15_binding.hpp"
#include "halo2/native_campaign_ai_behavior_reset_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool value,const std::string &message) {
    if(!value)throw std::runtime_error(message);
}
std::vector<std::byte> bytes(const std::string &text) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<text.size();n+=2)
        out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> data) {
    constexpr char digits[]="0123456789abcdef";
    std::string out;
    for(const auto value:data) {
        const auto n=std::to_integer<unsigned>(value);
        out+=digits[n>>4];out+=digits[n&15];
    }
    return out;
}
struct Prefix {};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;
    unsigned ordinal{};
    mutable unsigned read_attempts{};
    unsigned write_attempts{};
    bool log_reads{},log_writes{};
    std::uint32_t return_address{};
    mutable unsigned returns_read{};
    mutable Json reads=Json::array();
    std::vector<SourcePhysicsContactRecordStore> writes;
    auto locate(std::uint32_t at,std::size_t size)const {
        auto found=banks.upper_bound(at);
        check(found!=banks.begin(),"Missing cinematic lifecycle storage");--found;
        check(std::uint64_t(at-found->first)+size<=found->second.size(),
              "Cinematic lifecycle storage extent differs");
        return found;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        if(at==return_address) {
            ++returns_read;
            const auto found=locate(at,out.size());
            std::copy_n(found->second.begin()+at-found->first,out.size(),out.begin());return;
        }
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(at,out.size());
        if(log_reads)reads.push_back(Json::array({at,out.size()}));
        std::copy_n(found->second.begin()+at-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> input)override {
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(at,input.size());
        if(log_writes)writes.push_back({0,at,
            {found->second.begin()+at-found->first,found->second.begin()+at-found->first+input.size()},
            {input.begin(),input.end()}});
        std::copy(input.begin(),input.end(),banks.at(found->first).begin()+at-found->first);
    }
};
struct Arguments final:NativeCampaignAiBehaviorResetArgumentsTu15 {
    Memory &memory;std::uint32_t sp;
    Arguments(Memory &m,std::uint32_t s):memory(m),sp(s){}
    std::uint32_t get(unsigned offset)const {
        std::array<std::byte,4> raw;memory.read(sp+offset,raw);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);return value;
    }
    std::uint32_t actor()const override{return get(4);}
    std::uint32_t action()const override{return get(8);}
};
} // namespace

int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original cinematic lifecycle fixture");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Different cinematic lifecycle original image");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),
          "Cinematic reference supplied instructions or results");
    std::size_t count{},read_count{},store_count{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>(),"Screen transport input lacks physical original evidence");
        for(bool binding:{false,true})for(bool reading:{false,true}) {
            Memory memory;
            for(const auto &bank:row.at("banks"))
                memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp=row.at("gp").get<decltype(abi.gp)>();
            abi.eflags=0x602;abi.fs_base=0x87650000;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            for(unsigned i=0;i<8;++i) {
                abi.x87[i]=i+0.25L;
                for(unsigned j=0;j<4;++j)abi.xmm[i][j]=0x3f000000u+i*4u+j;
            }
            const auto initial=abi;Arguments arguments(memory,abi.gp[4]);
            memory.return_address=abi.gp[4];
            memory.banks.emplace(abi.gp[4],bytes("78563412"));
            const auto fault=row.value("fault",Json::object());
            memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading && memory.phase=="after_write")continue;
            memory.log_reads=reading;
            // A raw recording avoids adding observer reads to fault-prefix
            // runs; the separate observed path compares original store sites.
            memory.log_writes=!reading && (memory.phase=="before_read" || memory.phase=="before_write");
            std::vector<SourcePhysicsContactRecordStore> stores;
            SourcePhysicsContactRecordObserver observer;
            unsigned observed{};
            if(!reading && !memory.log_writes)observer=[&](const auto &event) {
                stores.push_back(event);
                if(memory.phase=="after_write" && observed++==memory.ordinal)throw Prefix{};
            };
            const auto mxcsr=_mm_getcsr();std::uint16_t control{},status{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(control),"=m"(status));
            bool stopped{};
            try {
                if(binding)native_campaign_ai_behavior_reset_tu15_run(memory,row.at("entry"),abi,observer);
                else check(native_campaign_ai_behavior_reset_tu15(memory,arguments,observer),name+": typed reset did not succeed");
            }catch(const Prefix &){stopped=true;}
            std::uint16_t after_control{},after_status{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_control),"=m"(after_status));
            check(control==after_control && status==after_status && mxcsr==_mm_getcsr(),name+": native FP state changed");
            check(stopped==!row.at("completed").get<bool>(),name+": original completion differs");
            auto expected=initial.gp;
            if(!stopped) {
                check(row.at("boundary_gp").at(4)==initial.gp[4]+12u,name+": physical original argument stack contract differs");
                check((row.at("boundary_gp").at(0).get<std::uint32_t>()&0xffu)==1u,name+": original boolAL differs");
                if(binding){expected[4]+=12u;expected[0]=(expected[0]&0xffffff00u)|1u;}
                for(auto reg:{3u,5u,6u,7u})check(initial.gp[reg]==row.at("boundary_gp").at(reg),
                    name+": physical original callee-preserved contract differs");
            }
            check(abi.gp==expected,name+": void transport invented volatile outputs or changed preserved registers");
            check(memory.returns_read==unsigned(binding&&!stopped),name+": transport did not consume the actual return word exactly once");
            check(abi.xmm==initial.xmm && abi.x87==initial.x87 && abi.eflags==initial.eflags &&
                abi.fs_base==initial.fs_base && abi.x87_status==initial.x87_status &&
                abi.x87_depth==initial.x87_depth && !abi.active_mmx && abi.floating_point &&
                abi.floating_point->x87_control_word==initial.floating_point->x87_control_word &&
                abi.floating_point->mxcsr==initial.floating_point->mxcsr,
                name+": native transport touched flags/SIMD/x87/FP/FS");
            if(reading) {
                Json expected=Json::array();
                for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                check(memory.reads==expected,name+": original ordered reads differ");read_count+=memory.reads.size();
            }else {
                if(memory.log_writes)stores=memory.writes;
                check(stores.size()==row.at("writes").size(),name+": original store count differs");
                for(std::size_t n=0;n<stores.size();++n) {
                    const auto &actual=stores[n];const auto &expected=row.at("writes")[n];
                    check((memory.log_writes || actual.instruction==expected.at("instruction")) &&
                          actual.address==expected.at("address") && hex(actual.before)==expected.at("before").get<std::string>() &&
                          hex(actual.written)==expected.at("written").get<std::string>(),name+": original ordered store differs");
                }
                store_count+=stores.size();
            }
            for(const auto &bank:row.at("banks"))
                check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": original retained bank differs");
        }
        ++count;
    }
    // An unavailable RET word is a real failure after kernel publications.
    // No successful return/stack word is supplied by the transport.
    for(auto entry:{0x1c9ce0u}) {
        const auto &row=*std::find_if(fixture.at("cases").begin(),fixture.at("cases").end(),
            [&](const auto &candidate){return candidate.at("entry")==entry &&
                candidate.at("completed").template get<bool>() && !candidate.at("writes").empty();});
        Memory memory;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.eflags=0x202;
        const auto initial=abi.gp;memory.return_address=abi.gp[4];
        bool rejected=false;
        try{native_campaign_ai_behavior_reset_tu15_run(memory,entry,abi);}catch(const std::exception &){rejected=true;}
        check(rejected && abi.gp==initial && memory.returns_read==1,"Transport fabricated missing return backing");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),
            "Missing RET rejected before preceding semantic publications");
    }
    Memory absent;SourcePhysicsWorldRuntimeAbi refused{};bool rejected=false;
    try{native_campaign_ai_behavior_reset_tu15_run(absent,0x1bae40,refused);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && !native_campaign_ai_behavior_reset_tu15_owns_entry(0x1bae40),"Behavior reset claimed another unconverted entry");
    check(native_campaign_ai_behavior_reset_activity_tu15()>0,"No actual native behavior-init activity");
    std::cout<<count<<" physical-original behavior-init cases/prefixes through semantic and ABI entry; "<<read_count<<" ordered reads, "<<store_count<<" ordered stores, boolAL/preserved state/FP plus missingRET passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
