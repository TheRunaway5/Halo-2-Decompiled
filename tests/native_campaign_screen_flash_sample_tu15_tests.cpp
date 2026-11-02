#include "halo2/native_campaign_screen_flash_sample_tu15.hpp"
#include "halo2/native_campaign_screen_flash_sample_tu15_binding.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &text) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> values) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto value:values){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;
}
struct Prefix {};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    mutable std::optional<std::pair<std::uint32_t,std::size_t>> failed_access;
    bool recording{},raw_stores{};mutable Json reads=Json::array();
    std::vector<SourcePhysicsContactRecordStore> stores;
    Json changes=Json::array();unsigned published{};
    void change_inputs(){for(const auto &change:changes)if(change.at("after_store")==published){const auto raw=bytes(change.at("bytes"));const auto at=change.at("address").get<std::uint32_t>();const auto found=locate(at,raw.size());std::copy(raw.begin(),raw.end(),banks.at(found->first).begin()+at-found->first);}++published;}
    std::map<std::uint32_t,std::vector<std::byte>>::const_iterator locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing screen flash sample storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){failed_access={{address,size}};throw std::runtime_error("Screen flash sample storage extent differs");}return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());
        if(recording)reads.push_back(Json::array({address,out.size()}));
        std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,values.size());
        if(raw_stores)stores.push_back({0,address,
            {found->second.begin()+address-found->first,found->second.begin()+address-found->first+values.size()},
            {values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),banks.at(found->first).begin()+address-found->first);change_inputs();
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original screen flash sample fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Screen flash sample original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() &&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Screen flash sample reference lacks unchanged physical authority");
    std::size_t count{},reads{},stores{},prefixes{},faults{},bindings{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();const auto entry=row.at("entry").get<unsigned>();
        check(row.at("physical_verified").get<bool>() && row.at("live_x87_unchanged").get<bool>(),name+": original physical evidence missing");
        for(unsigned transport=0;transport<2;++transport)for(bool reading:{false,true}) {
            const bool binding=transport!=0;
            Memory memory;
            for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            memory.changes=row.at("input_changes");const auto sp=row.at("gp")[4].get<std::uint32_t>();
            std::vector<std::byte> stack(12);const std::array<std::uint32_t,3> words{0x71000000u,row.at("matrix"),row.at("source")};
            for(unsigned n=0;n<3;++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(words[n]>>(k*8u));
            memory.banks.emplace(sp,stack);
            const auto fault=row.value("fault",Json::object());
            memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading && memory.phase=="after_write")continue;
            memory.recording=reading;
            memory.raw_stores=!reading && (memory.phase=="before_read" || memory.phase=="before_write" || row.value("actual_write_fault",false));
            SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
            if(!reading && !memory.raw_stores)observe=[&](const auto &event) {
                events.push_back(event);if(memory.phase=="after_write" && observed++==memory.ordinal)throw Prefix{};
            };
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202u;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
            abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
            const auto original_abi=abi;
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
            bool stopped{};
            try {
                if(binding){native_campaign_screen_flash_sample_tu15_run(memory,entry,0x2b8ecu,abi,observe);++bindings;}
                else native_campaign_screen_flash_sample_tu15(memory,row.at("user"),row.at("destination"),fp,observe);
            }catch(const NativeCampaignScreenFlashRequiredTu15 &required) {
                check(row.at("boundary").at("phase")=="required_owner"&&required.target()==row.at("boundary").at("instruction"),name+": real CRT owner boundary differs");stopped=true;
            }catch(const Prefix &){stopped=true;}
            catch(const std::runtime_error &error) {
                if(!(row.value("actual_read_fault",false)||row.value("actual_write_fault",false))||!memory.failed_access)throw std::runtime_error(name+": "+error.what());
                check(memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),name+": actual screen flash sample fault operand differs");stopped=true;++faults;
            }
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(actual_cw==host_cw && actual_sw==host_sw && _mm_getcsr()==host_mxcsr,name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": screen flash sample completion differs");
            if(binding) {
                fp.mxcsr=abi.floating_point->mxcsr;fp.control_word=abi.floating_point->x87_control_word;fp.status=abi.x87_status;
                for(unsigned n=0;n<8;++n)if(n!=4)check(abi.gp[n]==original_abi.gp[n],name+": binding fabricated dead GP state");
                check(abi.gp[4]==sp+(stopped?0:4),name+": binding stack differs");
                check(abi.eflags==original_abi.eflags && abi.x87_depth==original_abi.x87_depth,name+": binding changed unrelated ABI state");
            }
            check(fp.mxcsr==row.at("after_fp")[2] && fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1],name+": screen flash sample FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained screen flash sample bytes differ");
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                if(binding && !stopped)expected.push_back(Json::array({sp,4}));
                check(memory.reads==expected,name+": screen flash sample ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());reads+=memory.reads.size();
            }else {
                if(memory.raw_stores)events=memory.stores;
                check(events.size()==row.at("writes").size(),name+": screen flash sample store count differs");
                for(std::size_t n=0;n<events.size();++n) {
                    const auto &a=events[n];const auto &b=row.at("writes")[n];
                    check((memory.raw_stores || a.instruction==b.at("instruction")) && a.address==b.at("address") &&
                        hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": screen flash sample ordered store differs");
                }
                stores+=events.size();
            }
            prefixes+=stopped;
        }
        ++count;
    }
    check(count && prefixes && bindings && faults,"Screen flash sample qualification matrix incomplete");
    {
        Memory memory;SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x40000000;abi.eflags=0x602;
        bool unknown{},direction{};try{native_campaign_screen_flash_sample_tu15_run(memory,0,0,abi);}catch(const std::invalid_argument&){unknown=true;}
        try{native_campaign_screen_flash_sample_tu15_run(memory,0x156e40,0x2b8ec,abi);}catch(const std::invalid_argument&){direction=true;}
        check(unknown&&direction&&memory.reads.empty()&&memory.stores.empty(),"Flash binding failed before-touch admission");
    }
    {
        // A genuine unavailable RET word occurs after the command's state
        // publications. Transport cannot roll them back or advance the SP.
        const auto &row=fixture.at("cases").at(0);Memory memory;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202u;
        const auto sp=abi.gp[4];bool failed{};memory.banks.emplace(sp+4u,std::vector<std::byte>(4));
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
        try{native_campaign_screen_flash_sample_tu15_run(memory,0x156e40,0x2b8ec,abi);}catch(const std::runtime_error&){failed=true;}
        check(failed&&memory.failed_access==std::pair<std::uint32_t,std::size_t>{sp,4}&&abi.gp[4]==sp,"Flash binding lost actual RET failure");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),"RET failure rolled back flash state");
    }
    std::cout<<count<<" physical original screen flash sample cases, "<<reads<<" reads, "<<stores<<" stores, "<<prefixes<<" prefixes and "<<bindings<<" completed native ABI transports passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
