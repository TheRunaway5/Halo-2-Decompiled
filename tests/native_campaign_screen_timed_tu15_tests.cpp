#include "halo2/native_campaign_screen_timed_tu15.hpp"
#include "halo2/native_campaign_screen_timed_tu15_binding.hpp"
#include "halo2/source_campaign_screen_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <cstring>
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
struct Arguments final:NativeCampaignScreenDofArgumentsTu15 {
    Memory &memory;std::uint32_t sp;
    Arguments(Memory &m,std::uint32_t s):memory(m),sp(s){}
    std::uint32_t word(unsigned index)const override {
        std::array<std::byte,4> raw;memory.read(sp+4u+index*4u,raw);std::uint32_t value{};
        for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(raw[i])<<(i*8u);return value;
    }
};
} // namespace

int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original timed screen fixture");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
          "Different cinematic lifecycle original image");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),
          "Cinematic reference supplied instructions or results");
    std::size_t count{},read_count{},store_count{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>(),"Screen transport input lacks physical original evidence");
        for(unsigned route=0;route<3;++route)for(bool reading:{false,true}) {
            const bool binding=route!=0;
            Memory memory;
            for(const auto &bank:row.at("banks"))
                memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            const auto gp=row.at("gp").get<std::array<std::uint32_t,8>>();
            const auto entry=row.at("entry").get<std::uint32_t>();
            NativeCampaignScreenTimedFpTu15 fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            Arguments arguments(memory,gp[4]);
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=gp;abi.eflags=0x202;abi.fs_base=0x87650000;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
            for(const auto &raw:row.at("live_x87")){const auto value=bytes(raw);std::memcpy(&abi.x87[abi.x87_depth++],value.data(),10);}
            for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)abi.xmm[i][j]=0x3f000000u+i*4u+j;
            abi.xmm[0][0]=row.at("focus");const auto initial_abi=abi;
            if(binding){memory.return_address=gp[4];memory.banks.emplace(gp[4],bytes("78563412"));}

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
                if(route==2)source_run_campaign_screen_tu15(memory,entry,abi,{fp.control_word,fp.mxcsr},{},observer);
                else if(binding)native_campaign_screen_timed_tu15_run(memory,entry,abi,observer);
                else if(entry==0x1fc10)native_campaign_screen_dof_setup_tu15(memory,row.at("focus"),arguments,fp,observer);
                else {
                    const auto result=native_campaign_screen_timed_update_tu15(memory,fp,observer);
                    check(result.value_or(gp[0])==row.at("boundary_gp").at(0),name+": interface result differs");
                }
            }catch(const Prefix &){stopped=true;}
            std::uint16_t after_control{},after_status{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_control),"=m"(after_status));
            check(control==after_control && status==after_status && mxcsr==_mm_getcsr(),name+": native FP state changed");
            check(stopped==!row.at("completed").get<bool>(),name+": original completion differs");
            if(!stopped)for(const auto index:{3u,5u,6u,7u})check(gp[index]==row.at("boundary_gp")[index],name+": original callee-preserved contract differs");
            const auto expected_fp=row.at("after_fp").get<std::array<std::uint32_t,3>>();
            if(binding) {
                auto expected=gp;if(!stopped){expected[4]+=entry==0x1fc10?28u:4u;if(entry==0x1fd00)expected[0]=row.at("boundary_gp")[0];}
                check(abi.gp==expected && memory.returns_read==unsigned(!stopped),name+": native public argument/return/SP contract differs");
                check(abi.xmm==initial_abi.xmm && !std::memcmp(abi.x87.data(),initial_abi.x87.data(),sizeof(abi.x87)) &&
                    abi.x87_depth==initial_abi.x87_depth && abi.eflags==initial_abi.eflags && abi.fs_base==initial_abi.fs_base && !abi.active_mmx,
                    name+": transport changed dead registers or caller live values");
                fp={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
            }

            check(fp.control_word==expected_fp[0] && (binding?(fp.status&~0x3800u)==(expected_fp[1]&~0x3800u):fp.status==expected_fp[1]) && fp.mxcsr==expected_fp[2],
                name+": original FP status differs (native "+std::to_string(fp.status)+"/"+std::to_string(fp.mxcsr)+
                ", original "+std::to_string(expected_fp[1])+"/"+std::to_string(expected_fp[2])+")");
            if(reading) {
                Json expected=Json::array();
                for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                if(memory.reads!=expected){std::cerr<<"actual "<<memory.reads.dump()<<"\nexpected "<<expected.dump()<<"\n";}
                check(memory.reads==expected,name+": original ordered reads differ");read_count+=memory.reads.size();
            }else {
                if(memory.log_writes)stores=memory.writes;
                check(stores.size()==row.at("writes").size(),name+": original store count differs "+std::to_string(stores.size())+" vs "+std::to_string(row.at("writes").size()));
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
    for(const auto entry:{0x1fc10u,0x1fd00u}) {
        const auto &row=*std::find_if(fixture.at("cases").begin(),fixture.at("cases").end(),[&](const auto &r){return r.at("entry")==entry && r.at("completed").template get<bool>() && !r.at("writes").empty();});
        Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.eflags=0x202;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.xmm[0][0]=row.at("focus");
        const auto initial=abi.gp;memory.return_address=abi.gp[4];bool rejected{};
        try{native_campaign_screen_timed_tu15_run(memory,entry,abi);}catch(const std::exception &){rejected=true;}
        check(rejected && abi.gp==initial && memory.returns_read==1,"Transport fabricated missing RET");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),"Missing RET skipped preceding publications");
    }
    for(unsigned refusal=0;refusal<6;++refusal){
        Memory empty;SourcePhysicsWorldRuntimeAbi abi{};abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        if(refusal==0)abi.eflags|=0x400u;if(refusal==1)abi.x87_depth=6;if(refusal==2)abi.floating_point.reset();
        if(refusal==3)abi.active_mmx=SourcePhysicsMmxAlias{};if(refusal==4)abi.floating_point->x87_control_word&=~1u;
        bool rejected{};try{native_campaign_screen_timed_tu15_run(empty,refusal==5?0x1fcb0u:0x1fd00u,abi);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Nonordinary/unknown timed caller admitted or fell back");
    }
    std::cout<<count<<" physical-original timed cases/prefixes through semantic core, native transport and compatibility API; "<<read_count<<" ordered reads, "<<store_count<<" ordered stores, FP/retained banks plus2 RET faults passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
