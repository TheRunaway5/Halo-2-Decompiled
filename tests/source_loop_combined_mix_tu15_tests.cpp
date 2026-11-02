#include "halo2/source_loop_combined_mix_tu15.hpp"
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

int main(int argc,char**argv)try {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(argc!=2)throw std::runtime_error("Supply the independent original combined-loop fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
       fixture.at("cases").size()!=512 || !fixture.at("original_instruction_patches").empty() ||
       !fixture.at("substituted_calls").empty())
        throw std::runtime_error("Combined-loop reference authority/scope differs");
    const auto operand=[&](const char* key,unsigned offset=0) {
        const auto raw=fixture.at("constants").at(key).get<std::string>();
        std::uint32_t result{};
        for(unsigned i=0;i<4;++i)result|=std::stoul(raw.substr((offset+i)*2,2),nullptr,16)<<(i*8);
        return result;
    };
    halo2::SourceLoopCombinedMixConstantsTu15 constants{};
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<3;++j)
        constants.directions[i][j]=operand("0044eda0",i*12+j*4);
    constants.half=operand("0046a0cc");constants.one=operand("0046a0cc",4);
    constants.normalization_epsilon=operand("0046a0ec");
    constants.minimum_distance=operand("0046a180");
    const auto& production=halo2::source_loop_combined_constants_tu15;
    if(constants.directions!=production.directions || constants.half!=production.half || constants.one!=production.one ||
        constants.normalization_epsilon!=production.normalization_epsilon || constants.minimum_distance!=production.minimum_distance)
        throw std::runtime_error("Production combined constants differ from the original executable");
    halo2::SoundDriverParameterConstants parameters{};
    parameters.normalized_parameter_range_epsilon_bits=operand("0046a0ec");
    parameters.mono_channel_count_bits=constants.one;
    unsigned failures{},index{},reset_cases{},normalize_cases{},power_cases{};
    std::array<unsigned,4> rounding_cases{};
    for(const auto& row:fixture.at("cases")) {
        const auto previous=row.at("previous_frame").get<std::uint8_t>();
        const auto frame=row.at("frame").get<std::uint8_t>();
        const auto before=row.at("prior").get<std::array<std::uint32_t,4>>();
        const auto reset=halo2::source_loop_combined_reset_tu15(before,previous,frame);
        const auto rounding=row.at("rounding").get<unsigned>();
        if(rounding>=4)throw std::runtime_error("Unexpected original rounding mode");
        ++rounding_cases[rounding];reset_cases+=previous!=frame;
        const auto point=row.at("point").get<std::array<std::uint32_t,3>>();
        const auto caller_mxcsr=_mm_getcsr();
        const auto caller_rounding=std::fegetround();
        std::uint16_t caller_control,returned_control;
        asm volatile("fnstcw %0" : "=m"(caller_control));
        const auto out=halo2::source_loop_combined_mix_tu15(reset,point,
            row.at("minimum"),row.at("maximum"),row.at("distance"),constants,parameters,
            {std::uint16_t(0x027fu|(rounding<<10)),0x1f80u|(rounding<<13)});
        asm volatile("fnstcw %0" : "=m"(returned_control));
        if(_mm_getcsr()!=caller_mxcsr || std::fegetround()!=caller_rounding || returned_control!=caller_control)
            throw std::runtime_error("Combined-loop kernel leaked its floating-point context");
        normalize_cases+=point!=out.normalized;
        const auto real=[](std::uint32_t value){return std::bit_cast<float>(value);};
        double power{};for(auto coefficient:out.speakers) {
            const auto weighted=double(real(coefficient))*real(out.gain);power+=weighted*weighted;
        }
        power_cases+=power>1;
        if(reset!=row.at("after_reset").get<std::array<std::uint32_t,4>>() ||
           out.minimum!=row.at("clamped_minimum").get<std::uint32_t>() ||
           out.gain!=row.at("gain").get<std::uint32_t>() ||
           out.normalized!=row.at("normalized").get<std::array<std::uint32_t,3>>() ||
           out.speakers!=row.at("speakers").get<std::array<std::uint32_t,4>>() ||
           out.accumulated!=row.at("result").get<std::array<std::uint32_t,4>>()) {
            if(failures++<8)std::cerr<<"Combined-loop case "<<index<<" expected "<<row.dump()
                <<" actual "<<nlohmann::json{{"minimum",out.minimum},{"gain",out.gain},
                    {"normalized",out.normalized},{"speakers",out.speakers},{"result",out.accumulated}}<<'\n';
        }
        ++index;
    }
    if(failures)throw std::runtime_error(std::to_string(failures)+" combined-loop numeric cases differ from original execution");
    for(auto count:rounding_cases)if(count!=128)throw std::runtime_error("Original rounding coverage incomplete");
    if(reset_cases==0 || normalize_cases==0 || power_cases==0)
        throw std::runtime_error("Original frame/normalization/power controls are missing");
    std::cout<<"PASS "<<index<<" original combined-loop contributions, "<<reset_cases
        <<" frame resets, "<<normalize_cases<<" normalizations, "<<power_cases<<" reciprocal-power cases\n";
    return 0;
#else
    (void)argc;(void)argv;return 77;
#endif
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
