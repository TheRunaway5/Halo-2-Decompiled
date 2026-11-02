#include "halo2/native_campaign_speech_envelope_tu15.hpp"
#include "halo2/native_tu15_sound_manager.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto value:data){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;}
#include "native_campaign_speech_envelope_memory.hpp"
SourcePhysicsContactRuntimeAbi caller(const Json &row) {
    SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();
    for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x6b000000u+row.at("id").get<unsigned>()*37u+n*4u+k;
    abi.eflags=0x2d7;abi.fs_base=0x514000u;abi.x87_status=row.at("before_fp")[1];
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    if(row.at("active_mmx").get<bool>()) {
        SourcePhysicsMmxAlias mmx{};mmx.known_mask=0xff;
        for(unsigned n=0;n<8;++n){const auto value=bytes(row.at("live_x87")[n]);std::copy(value.begin(),value.end(),mmx.physical[n].begin());}
        abi.active_mmx=mmx;abi.x87_depth=0;
    }else{
        abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}
    }
    return abi;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original speech-envelope fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Speech-envelope original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Speech-envelope reference supplied an outcome");
    check(native_tu15_sound_manager_owns_entry(0x21f860u) && !native_tu15_sound_manager_calls_out(0x21f860u),"Speech-envelope actual registry missing or incomplete");
    std::size_t count{},reads{},faults{},pending_count{},late_returns{},mmx_cases{},full_stack_cases{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>() && row.at("entry_live_x87_preserved").get<bool>(),name+": original live state unqualified");
        check(row.at("writes").empty() && row.at("pending_extended").empty(),name+": original envelope published retained storage or changed x87 depth");
        Memory memory(row);const auto fault=row.value("fault",Json::object());memory.prefix=fault.value("phase",std::string{})=="before_read";memory.ordinal=fault.value("ordinal",0u);
        auto abi=caller(row);const auto initial=abi;const auto policy=abi.floating_point.value();
        const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        bool stopped{};SourcePhysicsContactRuntimeServices services;
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Speech envelope requested a supplied dependency outcome");};
        const auto missing=[&](const Missing &value){++faults;check(row.value("expected_fault",false),name+": unexpected missing storage");bool matched{};for(const auto &operand:row.at("boundary").at("operands"))matched|=operand.at("address")==value.address && operand.at("bytes")==value.width;check(matched,name+": source fault provenance differs");};
        try{native_tu15_sound_manager_run(memory,0x21f860u,abi,policy,{},services);}
        catch(const Prefix &){stopped=true;}
        catch(const Missing &value){stopped=true;missing(value);}
        catch(const NativeCampaignSpeechEnvelopePendingErrorTu15 &error){stopped=true;++pending_count;try{std::rethrow_exception(error.cause());}catch(const Prefix &){}catch(const Missing &value){missing(value);}}
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
        check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
        check(stopped==!row.at("completed").get<bool>(),name+": completion differs");
        check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] && abi.floating_point->mxcsr==row.at("after_fp")[2],name+": envelope FP differs");
        check(abi.gp[4]==initial.gp[4]+(stopped?0u:8u),name+": stack cleanup differs");
        check(abi.xmm[0]==row.at("boundary_xmm0").get<std::array<std::uint32_t,4>>(),name+": envelope scalar lanes differ");
        for(unsigned n=0;n<8;++n)if(n!=4)check(abi.gp[n]==initial.gp[n],name+": native kernel changed opaque GP");
        for(unsigned n=1;n<8;++n)check(abi.xmm[n]==initial.xmm[n],name+": native kernel changed preserved SIMD");
        check(abi.x87_depth==initial.x87_depth,name+": live extended depth differs");
        for(unsigned n=0;n<abi.x87_depth;++n)check(!std::memcmp(&abi.x87[n],&initial.x87[n],10),name+": older extended quantity changed");
        check(bool(abi.active_mmx)==bool(initial.active_mmx),name+": MMX ownership differs");
        if(abi.active_mmx){check(abi.active_mmx->physical==initial.active_mmx->physical && abi.active_mmx->known_mask==initial.active_mmx->known_mask,name+": MMX quantities differ");++mmx_cases;}
        if(abi.x87_depth==8)++full_stack_cases;
        check(abi.eflags==initial.eflags && abi.fs_base==initial.fs_base,name+": opaque caller policy changed");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained bytes differ");
        check(memory.applied==row.at("applied_input_timeline"),name+": input timeline differs");
        Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
        if(memory.reads!=expected)std::cerr<<name<<" actual="<<memory.reads.dump()<<" expected="<<expected.dump()<<'\n';
        check(memory.reads==expected,name+": ordered reads differ");reads+=memory.reads.size();
        if(!row.at("boundary").is_null() && (row.at("boundary").at("instruction")==0x21f911u || row.at("boundary").at("instruction")==0x21f91au))++late_returns;++count;
    }
    check(count && faults==3 && pending_count && late_returns==4 && mmx_cases && full_stack_cases,"Speech-envelope matrix incomplete");
    std::cout<<count<<" original speech-envelope cases; "<<reads<<" reads; "<<faults<<" true faults; "<<pending_count<<" pending byte samples; "<<late_returns<<" late returns; "<<mmx_cases<<" MMX and "<<full_stack_cases<<" full x87 profiles passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
