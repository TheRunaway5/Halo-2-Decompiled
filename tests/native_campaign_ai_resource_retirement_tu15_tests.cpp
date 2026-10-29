#include "halo2/native_campaign_ai_resource_retirement_tu15.hpp"
#include "halo2/source_runtime_host_physical_memory.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
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
    SourceRuntimeMemory bank;
    SourceRuntimeHostPhysicalMemory physical{bank};
    std::string phase;unsigned ordinal{},write_attempts{};mutable unsigned read_attempts{};
    mutable std::optional<std::pair<std::uint32_t,std::size_t>> failed_read;
    mutable Json events=Json::array();Json writes=Json::array(),services=Json::array();
    Memory() {check(physical.allocate({0x4000,0x1000000,0x1003fff,0x1000,4})==0x81000000u,"Real physical allocation identity differs");}
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read"&&read_attempts++==ordinal)throw Prefix{};
        try{bank.read(address,out);}catch(const std::out_of_range&){failed_read={{address,out.size()}};throw;}events.push_back(Json::array({"read",address,out.size()}));
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        if(phase=="before_write"&&write_attempts==ordinal)throw Prefix{};
        const auto before=hex(bank.view(address,values.size()));
        bank.write(address,values);
        writes.push_back(Json{{"address",address},{"before",before},{"written",hex(values)}});
        events.push_back(Json::array({"write",address,hex(values)}));
        if(phase=="after_write"&&write_attempts++==ordinal)throw Prefix{};
        if(phase!="after_write")++write_attempts;
    }
};
struct Protection final:NativeCampaignAiResourceProtectionTu15 {
    Memory &memory;
    explicit Protection(Memory &m):memory(m){}
    void protect_pages(std::uint32_t address,std::uint32_t bytes,std::uint32_t flags)const override {
        memory.services.push_back(Json::array({address,bytes,flags}));
        memory.events.push_back(Json::array({"protect",address,bytes,flags}));
        memory.physical.protect(address,bytes,flags);
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original resource-retirement fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Resource retirement original changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&
        !fixture.at("supplied_original_outcomes").get<bool>()&&fixture.at("platform_service")=="real-linux-i386-mprotect-stdcall-bridge","Resource reference lacks unchanged physical/platform authority");
    std::size_t count{},reads{},stores{},prefixes{},owner_boundaries{},protections{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>()&&row.at("live_x87_unchanged").get<bool>(),name+": original physical evidence missing");
        Memory memory;
        for(const auto &b:row.at("banks")) {
            const auto address=b.at("address").get<std::uint32_t>();const auto data=bytes(b.at("before"));
            if(address>=0x81000000u&&address<0x81004000u)memory.bank.write(address,data);
            else memory.bank.mount(address,data,SourceRuntimeMemory::Access::ReadWrite,"Declared original resource input");
        }
        if(row.at("initial_read_only").get<bool>())memory.physical.protect(0x81000001u,0x1fffu,2);
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        Protection protection(memory);
        const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
        bool stopped{};
        try {
            switch(row.at("entry").get<std::uint32_t>()) {
            case 0x130030:native_campaign_ai_resource_backing_retire_tu15(memory,row.at("user"),protection);break;
            case 0x140310:native_campaign_ai_resource_metadata_retire_tu15(memory,row.at("container"),row.at("datum"));break;
            case 0x12fd10:native_campaign_ai_unassociated_resource_retire_tu15(memory,row.at("datum"));break;
            default:throw std::runtime_error("Unknown resource retirement owner");
            }
        }catch(const Prefix &){stopped=true;}
        catch(const NativeCampaignAiResourceRetirementRequiredOwnerTu15 &required) {
            check(row.contains("required_owner")&&required.entry()==row.at("required_owner")&&
                required.identity()==row.at("boundary").at("identity"),name+": real required-owner boundary differs");stopped=true;++owner_boundaries;
        }catch(const std::out_of_range&) {
            if(!row.value("actual_read_fault",false)||!memory.failed_read)throw;
            check(memory.failed_read->first==row.at("boundary").at("address")&&memory.failed_read->second==row.at("boundary").at("bytes"),name+": actual missing-source operand differs");stopped=true;
        }
        std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
        check(actual_cw==host_cw&&actual_sw==host_sw&&_mm_getcsr()==host_mxcsr,name+": host FP leaked");
        check(stopped==!row.at("completed").get<bool>(),name+": resource completion differs");
        check(row.at("before_fp")==row.at("after_fp"),name+": original resource FP changed");
        for(const auto &b:row.at("banks"))check(hex(memory.bank.view(b.at("address"),b.at("after").get_ref<const std::string&>().size()/2))==b.at("after").get_ref<const std::string&>(),name+": retained resource bytes differ");
        check(memory.events==row.at("events"),name+": interleaved resource effects differ native="+memory.events.dump()+" original="+row.at("events").dump());
        check(memory.services==row.at("services"),name+": actual native platform protection differs");
        check(memory.writes.size()==row.at("writes").size(),name+": resource store count differs");
        for(std::size_t n=0;n<memory.writes.size();++n) {
            const auto &a=memory.writes[n];const auto &b=row.at("writes")[n];
            check(a.at("address")==b.at("address")&&a.at("before")==b.at("before")&&a.at("written")==b.at("written"),name+": ordered resource store differs");
        }
        const auto ownership=memory.physical.ownership_checkpoint();
        check(ownership.allocations.size()==1&&ownership.allocations[0].base==0x81000000u&&ownership.allocations[0].bytes==0x4000u,
            name+": original retained physical allocation lifetime differs");
        check(Json(ownership.allocations[0].read_only)==row.at("physical_read_only"),name+": actual original/native page permissions differ");
        reads+=row.at("reads").size();stores+=memory.writes.size();protections+=memory.services.size();prefixes+=stopped;++count;
    }
    check(count&&prefixes&&owner_boundaries&&protections,"Resource qualification matrix incomplete");
    std::cout<<count<<" physical original resource-retirement cases, "<<reads<<" ordered reads, "<<stores<<" stores, "<<protections<<" real page protections; lazy list/callback/pool semantics, fault prefixes and native physical lifetimes passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
