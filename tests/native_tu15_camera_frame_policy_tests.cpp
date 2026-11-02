#include "halo2/native_tu15_camera_frame_policy.hpp"
#include "halo2/native_tu15_camera_frame_policy_binding.hpp"
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
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing camera frame policy storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){failed_access={{address,size}};throw std::runtime_error("Camera policy storage extent differs");}return found;
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
    check(argc==2,"Supply independent original frame-policy fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Policy original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Policy lacks unchanged physical authority");
    std::size_t count{},read_count{},store_count{},prefixes{},faults{},bindings{},actual_callers{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();const auto owner=row.at("owner").get<std::uint32_t>();
        check(row.at("physical_verified").get<bool>()&&row.at("live_x87_unchanged").get<bool>(),name+": original evidence missing");
        const auto real_caller=row.at("caller").get<std::uint32_t>();
        const std::vector<std::uint32_t> callers=owner==0x2c470u?std::vector<std::uint32_t>{0x229b9eu}:std::vector<std::uint32_t>{0x14d67u,0x14d9au,0x198a8u};
        for(unsigned transport=0;transport<=callers.size();++transport)for(bool reading:{false,true}) {
            const bool binding=transport!=0;const auto caller=binding?callers[transport-1]:0u;
            if(real_caller&&binding&&real_caller!=caller)continue;
            Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            memory.changes=row.at("input_changes");
            auto sp=row.at("gp")[4].get<std::uint32_t>();if(real_caller)sp-=4u;
            const auto return_address=real_caller?real_caller+5u:0x3000000u;
            std::vector<std::byte> return_word(4);for(unsigned n=0;n<4;++n)return_word[n]=std::byte(return_address>>(n*8u));memory.banks.emplace(sp,return_word);
            const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading&&memory.phase=="after_write")continue;
            memory.recording=reading;memory.raw_stores=!reading&&(memory.phase=="before_read"||memory.phase=="before_write"||row.value("actual_write_fault",false));
            SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
            if(!reading&&!memory.raw_stores)observe=[&](const auto &event){events.push_back(event);if(memory.phase=="after_write"&&observed++==memory.ordinal)throw Prefix{};};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.gp[4]=sp;
            for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x6b000000u+row.at("id").get<unsigned>()*37u+n*4u+k;
            abi.eflags=0x602u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
            abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<8;++n)abi.x87[n]=n+0.25L;const auto before=abi;
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
            bool stopped{};std::uint32_t result{};
            try {
                if(binding){native_tu15_camera_frame_policy_run(memory,owner,caller,abi,observe);result=abi.gp[0];++bindings;}
                else if(owner==0x2c470u)native_tu15_camera_frame_policy(memory,row.at("gp")[0],observe);
                else result=native_tu15_camera_target_query(memory);
            }catch(const Prefix &){stopped=true;}
            catch(const std::runtime_error &error) {
                if(!(row.value("actual_read_fault",false)||row.value("actual_write_fault",false))||!memory.failed_access)throw std::runtime_error(name+": "+error.what());
                check(memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),name+": policy actual fault differs");stopped=true;++faults;
            }
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(actual_cw==host_cw&&actual_sw==host_sw&&_mm_getcsr()==host_mxcsr,name+": policy host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": policy completion differs");
            if(!stopped&&owner==0x25940u)check(result==row.at("result_eax"),name+": policy target result differs");
            if(binding) {
                for(unsigned k=0;k<8;++k)if(k!=4&&(k!=0||owner==0x2c470u||stopped))check(abi.gp[k]==before.gp[k],name+": dead policy GP reconstructed");
                check(abi.gp[4]==sp+(stopped?0:4),name+": policy binding stack differs");
                check(abi.xmm==before.xmm&&abi.eflags==before.eflags&&abi.x87==before.x87&&abi.x87_depth==before.x87_depth&&abi.x87_status==before.x87_status&&
                    abi.floating_point->mxcsr==before.floating_point->mxcsr&&abi.floating_point->x87_control_word==before.floating_point->x87_control_word,name+": unrelated policy ABI changed");
            }
            check(row.at("before_fp")==row.at("after_fp"),name+": original policy FP changed");
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": policy retained bytes differ");
            if(reading) {
                Json expected=Json::array(),accesses=row.at("accesses");for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                if(binding&&!stopped){expected.push_back(Json::array({sp,4}));accesses.push_back(Json::array({"read",sp,4}));}
                check(memory.reads==expected,name+": policy ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());
                check(memory.accesses==accesses,name+": policy interleaving differs");read_count+=memory.reads.size();
            } else {
                if(memory.raw_stores)events=memory.stores;const auto &expected=row.at("writes");check(events.size()==expected.size(),name+": policy store count differs");
                for(std::size_t k=0;k<events.size();++k) {
                    const auto &a=events[k];const auto &b=expected[k];check((memory.raw_stores||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&
                        hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+": policy ordered store differs");
                }store_count+=events.size();
            }
            prefixes+=stopped;
        }++count;if(real_caller)++actual_callers;
    }
    check(count&&prefixes&&bindings&&faults&&actual_callers==20,"Policy matrix incomplete");
    {
        Memory memory;SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x40000000;bool capture{},unknown{};
        try{native_tu15_camera_frame_policy_run(memory,0x25940,0x258f7,abi);}catch(const std::invalid_argument&){capture=true;}
        try{native_tu15_camera_frame_policy_run(memory,0x2c470,0,abi);}catch(const std::invalid_argument&){unknown=true;}
        check(capture&&unknown&&memory.reads.empty()&&memory.stores.empty(),"Policy admission touched storage");
    }
    {
        const auto &row=fixture.at("cases")[0];Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();const auto sp=abi.gp[4];bool failed{};
        try{native_tu15_camera_frame_policy_run(memory,0x2c470,0x229b9e,abi);}catch(const std::runtime_error&){failed=true;}
        check(failed&&memory.failed_access==std::pair<std::uint32_t,std::size_t>{sp,4}&&abi.gp[4]==sp,"Policy RET failure lost committed output");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),"Policy RET failure rolled back state");
    }
    std::cout<<count<<" physical policy cases, "<<read_count<<" reads, "<<store_count<<" stores, "<<prefixes<<" prefixes, "<<bindings<<" transports and "<<actual_callers<<" actual call profiles passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
