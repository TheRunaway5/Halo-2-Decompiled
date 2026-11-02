#include "halo2/native_campaign_ai_awareness_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> values){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:values){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4u];out+=digits[v&15u];}return out;}
struct Prefix {};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};mutable unsigned attempted_reads{};unsigned attempted_writes{},published{};
    mutable std::optional<std::pair<std::uint32_t,std::size_t>> failed_access;
    bool recording{},raw_stores{};mutable Json reads=Json::array();Json changes=Json::array();
    std::vector<SourcePhysicsContactRecordStore> stores;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing awareness storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){failed_access={{address,size}};throw std::runtime_error("Awareness storage extent differs");}return found;
    }
    void change_inputs(){for(const auto &change:changes)if(change.at("after_store")==published){const auto raw=bytes(change.at("bytes"));const auto at=change.at("address").get<std::uint32_t>();const auto found=locate(at,raw.size());std::copy(raw.begin(),raw.end(),banks.at(found->first).begin()+at-found->first);}++published;}
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read" && attempted_reads++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
        if(recording)reads.push_back(Json::array({address,out.size(),published,hex(out)}));
    }
    void write(std::uint32_t address,std::span<const std::byte> data)override {
        if(phase=="before_write" && attempted_writes++==ordinal)throw Prefix{};
        const auto found=locate(address,data.size());
        if(raw_stores)stores.push_back({0,address,{found->second.begin()+address-found->first,found->second.begin()+address-found->first+data.size()},{data.begin(),data.end()}});
        std::copy(data.begin(),data.end(),banks.at(found->first).begin()+address-found->first);change_inputs();
    }
};
struct HostEnvironment {
    alignas(16) std::array<std::byte,108> original{},before{},after{};
    std::uint32_t mxcsr;
    HostEnvironment():mxcsr(_mm_getcsr()) {
        asm volatile("fnsave %0;fninit;fld1;fldpi;fldln2":"=m"(original)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        asm volatile("fnsave %0;frstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    }
    bool unchanged(){
        asm volatile("fnsave %0;frstor %0":"=m"(after)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        // FIP/FOP/FDP can change across an unrelated scheduling interval on
        // this host. Keep strict live control/status/tag/value preservation;
        // record pointer metadata separately instead of attributing it to a
        // gameplay kernel. The independent no-engine probe retains evidence.
        for(auto offset:{0u,4u,8u})if(!std::equal(before.begin()+offset,before.begin()+offset+2u,after.begin()+offset))return false;
        return std::equal(before.begin()+28u,before.end(),after.begin()+28u) && mxcsr==_mm_getcsr();
    }
    bool pointer_metadata_changed()const{return !std::equal(before.begin()+12u,before.begin()+26u,after.begin()+12u);}
    ~HostEnvironment(){asm volatile("frstor %0"::"m"(original):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr);}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply unchanged physical-original awareness fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Awareness original image changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Awareness reference lacks physical unchanged authority");
    std::size_t count{},reads{},stores{},prefixes{},faults{},whole{},pointer_metadata_observations{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>() && row.at("live_x87_unchanged").get<bool>(),name+": original physical evidence missing");
        for(bool reading:{false,true}) {
            Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            memory.changes=row.value("input_changes",Json::array());const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading && memory.phase=="after_write")continue;
            memory.recording=reading;memory.raw_stores=!reading && (memory.phase=="before_read"||memory.phase=="before_write"||row.value("actual_write_fault",false));
            SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
            if(!reading && !memory.raw_stores)observe=[&](const auto &event){events.push_back(event);if(memory.phase=="after_write" && observed++==memory.ordinal)throw Prefix{};};
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};bool stopped{};HostEnvironment host;
            try {
                if(row.at("entry")==0x27d7d0u)native_campaign_ai_magically_see_tu15(memory,row.at("first_ai"),row.at("second_reference"),fp,row.at("live_x87").size(),observe);
                else {check(row.at("entry")==0x27da40u,"Unknown awareness reference entry");native_campaign_ai_magically_see_object_tu15(memory,row.at("first_ai"),row.at("second_reference"),fp,row.at("live_x87").size(),observe);}
            }catch(const Prefix&){stopped=true;}
            catch(const std::runtime_error &error) {
                if(!(row.value("actual_read_fault",false)||row.value("actual_write_fault",false))||!memory.failed_access)throw std::runtime_error(name+": "+error.what());
                check(memory.failed_access->first==row.at("boundary").at("address") && memory.failed_access->second==row.at("boundary").at("bytes"),name+": actual awareness fault operand differs");stopped=true;++faults;
            }
            const bool host_unchanged=host.unchanged();
            check(host_unchanged,name+": awareness leaked ambient FP/live x87 values before="+hex(host.before)+" after="+hex(host.after)+" MXCSR="+std::to_string(_mm_getcsr())+" expected="+std::to_string(host.mxcsr));
            pointer_metadata_observations+=host.pointer_metadata_changed();
            check(stopped==!row.at("completed").get<bool>(),name+": awareness completion differs");
            check(fp.control_word==row.at("after_fp")[0] && fp.status==row.at("after_fp")[1] && fp.mxcsr==row.at("after_fp")[2],name+": awareness FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained awareness bytes differ");
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes"),event.at("stores_before"),event.at("value")}));
                check(memory.reads==expected,name+": awareness ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());reads+=memory.reads.size();
            }else {
                if(memory.raw_stores)events=memory.stores;
                check(events.size()==row.at("writes").size(),name+": awareness store count differs");
                for(std::size_t n=0;n<events.size();++n){const auto &a=events[n];const auto &b=row.at("writes")[n];check((memory.raw_stores||a.instruction==b.at("instruction")) && a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": awareness ordered store differs");}stores+=events.size();
            }
            prefixes+=stopped;whole+=!stopped;
        }
        ++count;
    }
    {
        Memory memory;NativeTu15PhysicsFrameFp fp;bool occupied{},unmasked{};
        try{native_campaign_ai_magically_see_tu15(memory,0,0,fp,8);}catch(const std::invalid_argument&){occupied=true;}
        fp.mxcsr=0;try{native_campaign_ai_magically_see_object_tu15(memory,0,0,fp,0);}catch(const std::invalid_argument&){unmasked=true;}
        check(occupied&&unmasked&&memory.reads.empty()&&memory.stores.empty(),"Awareness FP admission did not reject before memory");
    }
    check(count && whole,"Awareness matrix is empty");
    std::cout<<count<<" physical original awareness cases, "<<reads<<" reads, "<<stores<<" stores, "<<prefixes<<" prefixes, "<<faults<<" actual faults and "<<whole<<" whole checks passed; "<<pointer_metadata_observations<<" scheduling-sensitive host pointer metadata observations outside live FP contract\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
