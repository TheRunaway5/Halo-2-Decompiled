#include "halo2/source_vehicle_audio_listener_tu15.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
using namespace halo2;
int main(int argc,char** argv)try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    using Json=nlohmann::json;
    const auto need=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
    need(argc==2,"Supply original vehicle listener fixture");
    const auto fixture=Json::parse(std::ifstream(argv[1]));
    need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("entry")=="0012a05c" && fixture.at("rooted_owner")=="00129f10" &&
        fixture.at("every_executed_site_has_rooted_function_provenance").get<bool>() &&
        fixture.at("original_instruction_patches").empty() && fixture.at("substituted_calls").empty() &&
        fixture.at("original_identity_pointer")==0x44ac04 &&
        fixture.at("original_identity_words")==Json::array({0,0,0,0x3f800000}),"Original seat listener authority differs");
    unsigned count{},opposite{},identity{},normalizer{};
    for(const auto& row:fixture.at("cases")){
        using Vector=std::array<std::uint32_t,3>;
        const SourceVehicleAudioListenerInputTu15 input{row.at("position").get<Vector>(),row.at("forward").get<Vector>(),
            row.at("up").get<Vector>(),row.at("unit_point").get<Vector>(),row.at("unit_aim").get<Vector>(),row.at("factor")};
        const auto rounding=row.at("rounding").get<unsigned>();
        const SoundDriverFloatingPointContext fp{std::uint16_t(0x027f|(rounding<<10)),0x1f80u|(rounding<<13)};
        std::uint16_t cw_before,cw_after;asm volatile("fnstcw %0":"=m"(cw_before));const auto csr=_mm_getcsr();
        const auto result=source_vehicle_audio_listener_tu15(input,fp);asm volatile("fnstcw %0":"=m"(cw_after));
        need(cw_before==cw_after && csr==_mm_getcsr(),"Seat listener leaked caller FP environment");
        const auto expected=row.at("result").get<std::array<std::uint32_t,9>>();
        std::array<std::uint32_t,9> actual{};
        std::copy(result.forward.begin(),result.forward.end(),actual.begin());
        std::copy(result.up.begin(),result.up.end(),actual.begin()+3);
        std::copy(result.position.begin(),result.position.end(),actual.begin()+6);
        if(actual!=expected || result.rotation!=row.at("rotation").get<std::array<std::uint32_t,4>>())
            throw std::runtime_error("Original seat blend differs at "+std::to_string(count)+" expected="+Json(expected).dump()+
                " actual="+Json(actual).dump()+" rotation="+Json(result.rotation).dump()+" expected_rotation="+row.at("rotation").dump());
        const auto& callees=row.at("executed_callees");
        opposite+=std::find(callees.begin(),callees.end(),"0011f9c0")!=callees.end();
        normalizer+=std::find(callees.begin(),callees.end(),"0011ffd0")!=callees.end();
        identity+=result.rotation==std::array<std::uint32_t,4>{0,0,0,0x3f800000u};++count;
    }
    need(count==1024 && opposite && identity && normalizer==count,"Original seat blend coverage differs");
    // A semantic audio call must preserve a live host x87 stack and sticky
    // flags too, even when its own requested rounding differs from the host.
    alignas(16) std::array<std::byte,108> saved_host{},before{},after{};
    const auto saved_csr=_mm_getcsr();const std::uint16_t hostile_control=0x0f7f;
    asm volatile("fnsave %0; fldcw %2; fldz; fldz; fdivp; fstp %%st(0); fldpi; fld1; fnsave %1; frstor %1"
        :"=m"(saved_host),"=m"(before):"m"(hostile_control):"memory","st","st(1)");
    _mm_setcsr(0x7f81u);
    const SourceVehicleAudioListenerInputTu15 host_probe{{0,0,0},{0x3f800000u,0,0},{0,0,0x3f800000u},
        {0x3f800000u,0,0},{0,0x3f800000u,0},0x3f000000u};
    (void)source_vehicle_audio_listener_tu15(host_probe,{0x027f,0x1f80});
    const auto restored_csr=_mm_getcsr();asm volatile("fnsave %0; frstor %1":"=m"(after):"m"(saved_host):"memory","st","st(1)");
    _mm_setcsr(saved_csr);
    need(restored_csr==0x7f81u && std::memcmp(before.data(),after.data(),2)==0 &&
        std::memcmp(before.data()+4,after.data()+4,2)==0 && std::memcmp(before.data()+8,after.data()+8,2)==0 &&
        std::memcmp(before.data()+28,after.data()+28,80)==0,"Seat listener changed live host x87 values/tags/sticky flags");
    std::cout<<"PASS "<<count<<" original vehicle seat listener blends; "<<opposite<<" opposite-axis branches, "
        <<normalizer<<" quaternion normalizations; all four rounding modes and caller FP restoration\n";return 0;
#else
    (void)argc;(void)argv;return 77;
#endif
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
