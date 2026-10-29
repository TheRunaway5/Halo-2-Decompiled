#include "halo2/native_campaign_camera_reset_tu15.hpp"
#include "halo2/native_campaign_camera_reset_tu15_binding.hpp"
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
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing camera reset storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){failed_access={{address,size}};throw std::runtime_error("Camera reset storage extent differs");}return found;
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
    check(argc==2,"Supply independent original camera reset fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Camera reset original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() &&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Camera reset reference lacks unchanged physical authority");
    std::size_t count{},reads{},stores{},prefixes{},faults{},bindings{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();const auto entry=row.at("entry").get<unsigned>();
        check(row.at("physical_verified").get<bool>() && row.at("live_x87_unchanged").get<bool>(),name+": original physical evidence missing");
        for(bool binding:{false,true})for(bool reading:{false,true}) {
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
            
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202u;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
            abi.x87_status=row.at("before_fp")[1];
            const auto original_abi=abi;
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
            bool stopped{};
            try {
                if(binding){native_campaign_camera_reset_tu15_run(memory,entry,abi,observe);++bindings;}
                else native_campaign_camera_director_reset_tu15(memory,observe);
            }catch(const NativeCampaignCameraResetRequiredOwnerTu15 &required) {
                check(row.contains("required_owner")&&required.owner()==row.at("required_owner")&&required.record()==row.at("required_record")&&
                    std::uint32_t(required.view())==row.at("required_view"),name+": genuine required controller owner differs");stopped=true;
            }catch(const Prefix &){stopped=true;}
            catch(const std::runtime_error &error) {
                if(!(row.value("actual_read_fault",false)||row.value("actual_write_fault",false))||!memory.failed_access)
                    throw std::runtime_error(name+": "+error.what()+" address="+(memory.failed_access?std::to_string(memory.failed_access->first):"none"));
                check(memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),name+": actual camera reset fault operand differs");stopped=true;++faults;
            }
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(actual_cw==host_cw && actual_sw==host_sw && _mm_getcsr()==host_mxcsr,name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": camera reset completion differs");
            if(binding) {
                for(unsigned n=0;n<8;++n)if(n!=4)check(abi.gp[n]==original_abi.gp[n],name+": binding fabricated dead GP state");
                check(abi.gp[4]==sp+(stopped?0:4),name+": binding stack differs");
                check(abi.eflags==original_abi.eflags && abi.x87_status==original_abi.x87_status &&
                    abi.floating_point->x87_control_word==original_abi.floating_point->x87_control_word,name+": binding changed unrelated ABI state");
            }
            check(row.at("before_fp")==row.at("after_fp"),name+": original startup changed FP");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained camera reset bytes differ");
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                if(binding && !stopped)expected.push_back(Json::array({sp,4}));
                check(memory.reads==expected,name+": camera reset ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());reads+=memory.reads.size();
            }else {
                if(memory.raw_stores)events=memory.stores;
                check(events.size()==row.at("writes").size(),name+": camera reset store count differs");
                for(std::size_t n=0;n<events.size();++n) {
                    const auto &a=events[n];const auto &b=row.at("writes")[n];
                    check((memory.raw_stores || a.instruction==b.at("instruction")) && a.address==b.at("address") &&
                        hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": camera reset ordered store differs");
                }
                stores+=events.size();
            }
            prefixes+=stopped;
        }
        ++count;
    }
    check(count && prefixes && bindings && faults,"Camera reset qualification matrix incomplete");
    {
        Memory memory;SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x40000000;abi.eflags=0x602;
        bool unknown{},direction{};try{native_campaign_camera_reset_tu15_run(memory,0,abi);}catch(const std::invalid_argument&){unknown=true;}
        try{native_campaign_camera_reset_tu15_run(memory,0x157eb0,abi);}catch(const std::invalid_argument&){direction=true;}
        check(unknown&&direction&&memory.reads.empty()&&memory.stores.empty(),"Camera binding failed before-touch admission");
    }
    {
        // A genuine unavailable RET word occurs after the command's state
        // publications. Transport cannot roll them back or advance the SP.
        const auto &row=*std::find_if(fixture.at("cases").begin(),fixture.at("cases").end(),[](const auto &value){return value.at("entry")==0x157eb0u&&value.at("completed").template get<bool>();});Memory memory;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202u;
        const auto sp=abi.gp[4];bool failed{};
        try{native_campaign_camera_reset_tu15_run(memory,0x157eb0,abi);}catch(const std::runtime_error&){failed=true;}
        check(failed&&memory.failed_access==std::pair<std::uint32_t,std::size_t>{sp,4}&&abi.gp[4]==sp,"Camera binding lost actual RET failure");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),"RET failure rolled back camera state");
    }
    std::cout<<count<<" physical original camera reset cases, "<<reads<<" reads, "<<stores<<" stores, "<<prefixes<<" prefixes and "<<bindings<<" completed native ABI transports passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
