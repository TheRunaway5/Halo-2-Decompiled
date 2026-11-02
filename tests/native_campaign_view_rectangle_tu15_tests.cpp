#include "halo2/native_campaign_view_rectangle_tu15.hpp"
#include "halo2/native_campaign_view_rectangle_tu15_binding.hpp"
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
    bool recording{},raw_stores{};mutable Json reads=Json::array(),accesses=Json::array();
    std::vector<SourcePhysicsContactRecordStore> stores;
    Json changes=Json::array();unsigned published{};
    void change_inputs(){for(const auto &change:changes)if(change.at("after_store")==published){const auto raw=bytes(change.at("bytes"));const auto at=change.at("address").get<std::uint32_t>();const auto found=locate(at,raw.size());std::copy(raw.begin(),raw.end(),banks.at(found->first).begin()+at-found->first);}++published;}
    std::map<std::uint32_t,std::vector<std::byte>>::const_iterator locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing view rectangle storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){failed_access={{address,size}};throw std::runtime_error("Render camera packet storage extent differs");}return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());
        if(recording){reads.push_back(Json::array({address,out.size()}));accesses.push_back(Json::array({"read",address,out.size()}));}
        std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,values.size());if(recording)accesses.push_back(Json::array({"write",address,values.size()}));
        if(raw_stores)stores.push_back({0,address,
            {found->second.begin()+address-found->first,found->second.begin()+address-found->first+values.size()},
            {values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),banks.at(found->first).begin()+address-found->first);change_inputs();
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original view rectangle fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","View rectangle original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() &&
        !fixture.at("supplied_original_outcomes").get<bool>(),"View rectangle lacks unchanged physical authority");
    std::size_t count{},reads{},stores{},prefixes{},faults{},bindings{},values{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>() && row.at("live_x87_unchanged").get<bool>(),name+": original evidence missing");
        for(unsigned transport=0;transport<4;++transport)for(bool reading:{false,true}) {
            const bool binding=transport==1||transport==2,value=transport==3;
            if(value&&!row.at("value_profile").get<bool>())continue;
            Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            memory.changes=row.at("input_changes");const auto sp=row.at("gp")[4].get<std::uint32_t>();
            memory.banks.emplace(sp,std::vector<std::byte>{std::byte{0},std::byte{0},std::byte{0},std::byte{3}});
            const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading&&memory.phase=="after_write")continue;
            memory.recording=reading;memory.raw_stores=!reading&&(memory.phase=="before_read"||memory.phase=="before_write"||row.value("actual_write_fault",false));
            SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
            if(!reading&&!memory.raw_stores)observe=[&](const auto &event){events.push_back(event);if(memory.phase=="after_write"&&observed++==memory.ordinal)throw Prefix{};};
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202u;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];const auto before=abi;
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
            bool stopped{};std::optional<NativeCampaignViewRectangleTu15> result;
            try {
                if(binding){native_campaign_view_rectangle_run_tu15(memory,0x2f950,transport==1?0x2b8b4u:0x13c8au,abi,observe);++bindings;}
                else if(value){result=native_campaign_view_rectangle_value_tu15(memory,row.at("camera"),fp,observe);++values;}
                else check(native_campaign_view_rectangle_tu15(memory,row.at("camera"),row.at("destination"),fp,observe),name+": result differs");
            }catch(const Prefix &){stopped=true;}
            catch(const std::runtime_error &error) {
                if(!(row.value("actual_read_fault",false)||row.value("actual_write_fault",false))||!memory.failed_access)throw std::runtime_error(name+": "+error.what());
                check(memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),name+": actual rectangle fault differs");stopped=true;++faults;
            }
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(actual_cw==host_cw&&actual_sw==host_sw&&_mm_getcsr()==host_mxcsr,name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": rectangle completion differs");
            if(!stopped)check(row.at("result_low8")==1,name+": original AL differs");
            if(binding) {
                fp.mxcsr=abi.floating_point->mxcsr;
                for(unsigned k=0;k<8;++k)if(k!=4)check(abi.gp[k]==before.gp[k],name+": dead GP reconstructed");
                check(abi.gp[4]==sp+(stopped?0:4),name+": binding stack differs");
                check(abi.eflags==before.eflags&&abi.x87_status==before.x87_status&&abi.floating_point->x87_control_word==before.floating_point->x87_control_word,name+": unrelated ABI changed");
            }
            check(fp.mxcsr==row.at("after_fp")[2],name+": rectangle FP differs native="+std::to_string(fp.mxcsr)+" original="+row.at("after_fp").dump());
            const auto destination=row.at("destination").get<std::uint32_t>();
            const auto private_output=[&](std::uint32_t p,std::size_t n){return value&&p>=destination&&std::uint64_t(p-destination)+n<=16;};
            for(const auto &bank:row.at("banks")) {
                auto expected=bytes(bank.at("after")),actual=memory.banks.at(bank.at("address"));const auto first=bank.at("address").get<std::uint32_t>();
                if(value)for(std::size_t k=0;k<expected.size();++k)if(private_output(first+k,1)) {
                    const auto offset=first+k-destination;check(result&&std::byte(result->bounds[offset/4]>>(8u*(offset%4)))==expected[k],name+": native private rectangle differs");actual[k]=expected[k];
                }
                check(actual==expected,name+": retained rectangle bytes differ");
            }
            if(reading) {
                Json expected=Json::array(),accesses=Json::array();
                for(const auto &event:row.at("reads"))if(!private_output(event.at("address"),event.at("bytes")))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                for(const auto &event:row.at("accesses"))if(!private_output(event[1],event[2]))accesses.push_back(event);
                if(binding&&!stopped){expected.push_back(Json::array({sp,4}));accesses.push_back(Json::array({"read",sp,4}));}
                check(memory.reads==expected,name+": rectangle ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());
                check(memory.accesses==accesses,name+": rectangle interleaving differs");reads+=memory.reads.size();
            } else {
                if(memory.raw_stores)events=memory.stores;std::vector<Json> expected;
                for(const auto &event:row.at("writes"))if(!private_output(event.at("address"),event.at("written").get<std::string>().size()/2))expected.push_back(event);
                check(events.size()==expected.size(),name+": rectangle store count differs");
                for(std::size_t k=0;k<events.size();++k) {
                    const auto &a=events[k];const auto &b=expected[k];
                    check((memory.raw_stores||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+": rectangle ordered store differs");
                }stores+=events.size();
            }prefixes+=stopped;
        }++count;
    }
    check(count&&prefixes&&bindings&&faults&&values,"Rectangle matrix incomplete");
    {
        Memory memory;SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x40000000;abi.eflags=0x602;bool unknown{},direction{};
        try{native_campaign_view_rectangle_run_tu15(memory,0x2f950,0,abi);}catch(const std::invalid_argument&){unknown=true;}
        try{native_campaign_view_rectangle_run_tu15(memory,0x2f950,0x2b8b4,abi);}catch(const std::invalid_argument&){direction=true;}
        check(unknown&&direction&&memory.reads.empty()&&memory.stores.empty(),"Rectangle admission touched storage");
    }
    {
        const auto &row=fixture.at("cases")[0];Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};const auto sp=abi.gp[4];bool failed{};
        try{native_campaign_view_rectangle_run_tu15(memory,0x2f950,0x13c8a,abi);}catch(const std::runtime_error&){failed=true;}
        check(failed&&memory.failed_access==std::pair<std::uint32_t,std::size_t>{sp,4}&&abi.gp[4]==sp,"Rectangle RET failure lost committed output");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),"Rectangle RET failure rolled back state");
    }
    std::cout<<count<<" physical rectangle cases, "<<reads<<" reads, "<<stores<<" stores, "<<prefixes<<" prefixes, "<<bindings<<" caller transports and "<<values<<" native values passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
