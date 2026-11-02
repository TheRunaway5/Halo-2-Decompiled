#include "halo2/native_campaign_ai_awareness_tu15.hpp"
#include "halo2/native_campaign_ai_awareness_tu15_binding.hpp"
#include <cstring>
#include <limits>
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
    std::uint32_t private_frame{};bool reject_return{};mutable Json caller_reads=Json::array();
    bool recording{},raw_stores{};mutable Json reads=Json::array();Json changes=Json::array();
    std::vector<SourcePhysicsContactRecordStore> stores;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing awareness storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){failed_access={{address,size}};throw std::runtime_error("Awareness storage extent differs");}return found;
    }
    void change_inputs(){for(const auto &change:changes)if(change.at("after_store")==published){const auto raw=bytes(change.at("bytes"));const auto at=change.at("address").get<std::uint32_t>();const auto found=locate(at,raw.size());std::copy(raw.begin(),raw.end(),banks.at(found->first).begin()+at-found->first);}++published;}
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        const bool private_input=private_frame && address>=private_frame && std::uint64_t(address-private_frame)+out.size()<=12u;
        if(private_input){caller_reads.push_back(Json::array({address,out.size()}));if(reject_return&&address==private_frame){failed_access={{address,out.size()}};throw std::runtime_error("Rejected actual awareness RET read");}}
        else if(phase=="before_read" && attempted_reads++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
        if(recording && !private_input)reads.push_back(Json::array({address,out.size(),published,hex(out)}));
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
    static_assert(std::numeric_limits<long double>::digits==64 && std::numeric_limits<long double>::max_exponent==16384);
    check(argc==2,"Supply independent original awareness fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline" && fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Awareness binding reference authority differs");
    std::size_t calls{},reads{},stores{},prefixes{},faults{},whole{};const auto before_calls=native_campaign_ai_awareness_calls_tu15();
    const auto run=[&](const Json &row,bool reading,bool return_fault,bool wrong_return) {
        const auto name=row.at("name").get<std::string>();const auto entry=row.at("entry").get<unsigned>();const bool ai=entry==0x27d7d0u;
        check(ai||entry==0x27da40u,"Unknown awareness binding reference root");check(row.at("physical_verified").get<bool>()&&row.at("live_x87_unchanged").get<bool>(),name+": missing physical evidence");
        Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.changes=row.value("input_changes",Json::array());const auto sp=row.at("gp")[4].get<std::uint32_t>();memory.private_frame=sp;memory.reject_return=return_fault;
        const auto actual_return=ai?0x2af0f5u:0x2af144u;std::array<std::uint32_t,3> words{wrong_return?0x71000000u:actual_return,row.at("first_ai"),row.at("second_reference")};
        if(!ai)words[1]=row.at("second_reference");std::vector<std::byte> frame(ai?12:8);
        for(unsigned n=0;n<frame.size();++n)frame[n]=std::byte(words[n/4u]>>(8u*(n%4u)));memory.banks.emplace(sp,frame);
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        memory.recording=reading;memory.raw_stores=!reading&&(memory.phase=="before_read"||memory.phase=="before_write"||row.value("actual_write_fault",false));
        SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
        if(!reading&&!memory.raw_stores)observe=[&](const auto &event){events.push_back(event);if(memory.phase=="after_write"&&observed++==memory.ordinal)throw Prefix{};};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x206;abi.fs_base=0x60200000u;
        abi.gp[1]=row.at("second_reference");if(ai){abi.gp[2]=row.at("first_ai");abi.gp[0]=sp+0x1200u;}else abi.gp[0]=row.at("first_ai");
        std::vector<std::byte> arguments(8);const std::array<std::uint32_t,2> inputs{row.at("first_ai"),row.at("second_reference")};
        for(unsigned n=0;n<8;++n)arguments[n]=std::byte(inputs[n/4u]>>(8u*(n%4u)));memory.banks.emplace(sp+0x1200u,arguments);
        abi.gp[6]=0xabcd0003u;for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x9a000000u+n*0x101u+k;
        std::memset(abi.x87.data(),0xa5,sizeof(abi.x87));abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);check(value.size()==10,"Reference x87 width differs");std::memcpy(&abi.x87[n],value.data(),10);}
        std::array<std::byte,sizeof(abi.x87)> old_x87;std::memcpy(old_x87.data(),abi.x87.data(),old_x87.size());
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
        const auto old_gp=abi.gp;const auto old_xmm=abi.xmm;const auto old_depth=abi.x87_depth,old_flags=abi.eflags,old_fs=abi.fs_base;
        const SourcePhysicsContactRuntimeCall call{ai?0x2af0f0u:0x2af13fu,entry,false};
        check(native_campaign_ai_awareness_tu15_owns_entry(entry)&&native_campaign_ai_awareness_tu15_owns_call(call),"Awareness binding rejected genuine caller metadata");
        bool stopped{},ret_required{};HostEnvironment host;
        try{native_campaign_ai_awareness_tu15_run(memory,call,abi,observe);}
        catch(const Prefix&){stopped=true;}
        catch(const NativeCampaignAiAwarenessReturnRequiredTu15 &error){check(wrong_return&&error.target()==0x71000000u,"Unexpected awareness return owner");stopped=ret_required=true;}
        catch(const std::runtime_error &error){
            if(return_fault){check(memory.failed_access==std::pair<std::uint32_t,std::size_t>{sp,4},"Awareness RET failed at wrong operand");stopped=true;}
            else {if(!(row.value("actual_read_fault",false)||row.value("actual_write_fault",false))||!memory.failed_access)throw std::runtime_error(name+": "+error.what());check(memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),name+": actual awareness binding fault operand differs");stopped=true;++faults;}
        }
        ++calls;check(host.unchanged(),name+": awareness binding leaked ambient live FP");
        check(stopped==(!row.at("completed").get<bool>()||return_fault||wrong_return),name+": awareness binding completion differs");check(ret_required==wrong_return,"Awareness binding supplied an unknown RET continuation");
        for(unsigned n=0;n<8;++n)if(n!=4)check(abi.gp[n]==old_gp[n],name+": awareness binding fabricated GP state");
        check(abi.gp[4]==sp+(stopped?0u:(ai?12u:8u)),name+": awareness binding stack differs");
        check(abi.xmm==old_xmm&&abi.eflags==old_flags&&abi.fs_base==old_fs&&abi.x87_depth==old_depth&&!abi.active_mmx,name+": awareness binding changed unrelated ABI state");
        check(std::memcmp(old_x87.data(),abi.x87.data(),old_x87.size())==0,name+": awareness binding changed retained x87 payloads or padding");
        check(abi.floating_point->x87_control_word==row.at("after_fp")[0]&&abi.x87_status==row.at("after_fp")[1]&&abi.floating_point->mxcsr==row.at("after_fp")[2],name+": awareness binding FP publication differs");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained awareness binding bytes differ");
        check(memory.banks.at(sp)==frame,"Awareness binding changed its immutable private caller words");
        check(memory.banks.at(sp+0x1200u)==arguments,"Awareness binding changed its completed HS argument array");
        const Json wanted_caller=row.at("completed").get<bool>()?Json::array({Json::array({sp,4})}):Json::array();
        check(memory.caller_reads==wanted_caller,name+": awareness binding peeked arguments or omitted actual RET read");
        if(reading){Json expected=Json::array();for(const auto &e:row.at("reads"))expected.push_back(Json::array({e.at("address"),e.at("bytes"),e.at("stores_before"),e.at("value")}));check(memory.reads==expected,name+": awareness binding ordered reads differ");reads+=memory.reads.size();}
        else {if(memory.raw_stores)events=memory.stores;check(events.size()==row.at("writes").size(),name+": awareness binding store count differs");for(std::size_t n=0;n<events.size();++n){const auto &a=events[n];const auto &b=row.at("writes")[n];check((memory.raw_stores||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+": awareness binding ordered store differs");}stores+=events.size();}
        prefixes+=stopped;whole+=!stopped;
    };
    for(const auto &row:fixture.at("cases"))for(bool reading:{false,true}){if(reading&&row.value("fault",Json::object()).value("phase",std::string{})=="after_write")continue;run(row,reading,false,false);}
    const auto selected=std::find_if(fixture.at("cases").begin(),fixture.at("cases").end(),[](const auto &r){return r.at("completed").template get<bool>()&&!r.at("writes").empty();});check(selected!=fixture.at("cases").end(),"Missing mutating awareness RET control");
    run(*selected,true,true,false);run(*selected,true,false,true);
    const auto check_rejection=[&](SourcePhysicsContactRuntimeCall call,SourcePhysicsWorldRuntimeAbi abi){Memory memory;memory.recording=true;const auto count=native_campaign_ai_awareness_calls_tu15();bool rejected{};try{native_campaign_ai_awareness_tu15_run(memory,call,abi);}catch(const std::invalid_argument&){rejected=true;}check(rejected&&memory.reads.empty()&&memory.stores.empty()&&count==native_campaign_ai_awareness_calls_tu15(),"Awareness binding admission touched source memory/counter or supplied fallback");};
    SourcePhysicsWorldRuntimeAbi rejected{};rejected.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f80u};
    const SourcePhysicsContactRuntimeCall valid{0x2af0f0u,0x27d7d0u,false};
    check_rejection({0,valid.target,false},rejected);check_rejection({valid.instruction,0,false},rejected);check_rejection({valid.instruction,valid.target,true},rejected);check_rejection({0x2af13fu,valid.target,false},rejected);
    auto variant=rejected;variant.floating_point.reset();check_rejection(valid,variant);variant=rejected;variant.x87_depth=8;check_rejection(valid,variant);variant=rejected;variant.active_mmx=SourcePhysicsMmxAlias{};check_rejection(valid,variant);
    variant=rejected;variant.floating_point->mxcsr=0;check_rejection(valid,variant);variant=rejected;variant.floating_point->x87_control_word=0;check_rejection(valid,variant);variant=rejected;variant.x87_status=0x80;check_rejection(valid,variant);
    check(native_campaign_ai_awareness_calls_tu15()==before_calls+calls,"Awareness binding activity counter differs");check(calls&&whole&&prefixes&&faults,"Awareness binding matrix is incomplete");
    std::cout<<calls<<" genuine awareness caller transports, "<<reads<<" ordered reads, "<<stores<<" stores, "<<prefixes<<" partial stops, "<<faults<<" source faults and "<<whole<<" completions passed; actual x87 payloads and active FP retained, no argument prepeek\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
