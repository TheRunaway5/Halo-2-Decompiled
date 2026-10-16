#include <nlohmann/json.hpp>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <xmmintrin.h>

using Json=nlohmann::json;
namespace {
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
struct Result {std::array<std::uint32_t,4> lanes;std::uint32_t mxcsr;};
Result original_mulss(const std::array<std::uint32_t,4>& left,
    const std::array<std::uint32_t,4>& right,std::uint32_t control) {
    Result result{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // Exact original29BF64 bytes F3 0F59 E9: MULSS XMM5,XMM1. No callback,
    // formatter, JSON operation, or compiler float arithmetic occurs between
    // loading the known MXCSR and recording the physical instruction's status.
    asm volatile("ldmxcsr %4\n\tmovups %2, %%xmm5\n\tmovups %3, %%xmm1\n\tmulss %%xmm1, %%xmm5\n\tstmxcsr %0\n\tmovups %%xmm5, %1"
        :"=m"(result.mxcsr),"=m"(result.lanes):"m"(left),"m"(right),"m"(control):"xmm1","xmm5","memory");
#else
    throw std::runtime_error("Original scalar SSE status calibration requires the genuine x86 instruction");
#endif
    return result;
}
struct RestoreMxcsr {std::uint32_t saved=_mm_getcsr();~RestoreMxcsr(){_mm_setcsr(saved);}};
}
int main(int argc,char** argv) {
    try {
        check(argc==3,"Expected authenticated original late-startup fixture and report path");
        std::ifstream in(argv[1]);Json fixture;in>>fixture;
        check(fixture.at("passed")==true&&fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
            fixture.at("executed_instruction_bytes").at("0029bf64")=="f30f59e9","Original scalar SSE calibration bytes are unauthenticated");
        RestoreMxcsr restore;
        Json reports=Json::array();
        for(const auto& row:fixture.at("cases")) {
            std::size_t total=0,precision=0;Json first=nullptr;
            for(const auto& sample:row.at("simd_precision_samples")) {
                check(sample.at("instruction")==0x29bf64&&sample.at("instruction_hex")=="f30f59e9","Scalar SSE calibration source instruction differs");
                const auto left=sample.at("left").get<std::array<std::uint32_t,4>>();
                const auto right=sample.at("right").get<std::array<std::uint32_t,4>>();
                const auto before=sample.at("before_mxcsr").get<std::uint32_t>();
                const auto result=original_mulss(left,right,before);
                check(result.lanes==sample.at("result").get<decltype(result.lanes)>(),"Physical original MULSS result differs from original emulator execution");
                check((result.mxcsr&~0x3fu)==(before&~0x3fu),"Original MULSS changed an MXCSR control bit");
                const auto delta=result.mxcsr^sample.at("after_mxcsr").get<std::uint32_t>();
                check(delta==0||delta==0x20,"Original MULSS status differs by more than the isolated precision flag");
                if(delta) {
                    ++precision;
                    if(first.is_null())first={{"sample_index",total},{"left",left},{"right",right},{"result",result.lanes},
                        {"native_mxcsr",result.mxcsr},{"original_emulator_mxcsr",sample.at("after_mxcsr")}};
                }
                ++total;
            }
            check(total>0&&precision>0,"Original scalar SSE calibration did not reproduce the observed precision-status difference");
            reports.push_back({{"fill",row.at("fill")},{"source_mulss_samples_compared",total},{"precision_status_differences",precision},
                {"first_difference",first},{"native_instruction_bytes","f30f59e9"},{"passed",true}});
        }
        Json report={{"schema","H2_LATE_STARTUP_ORIGINAL_SIMD_STATUS_CALIBRATION_V1"},{"passed",true},{"cases",reports},
            {"original_instruction",0x29bf64},{"original_instruction_hex","f30f59e9"},
            {"conclusion","The identical physical MULSS on original captured operands sets MXCSR precision; Unicorn leaves that status clear. All128 result bits and control bits match."},
            {"hardware","native host x86 SSE"},{"observer_or_formatter_between_instruction_and_status",false},
            {"xbox_hardware_capture",false},{"complete_x87_or_simd_exception_parity_verified",false}};
        std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save original scalar SSE calibration report");
        std::cout<<reports.size()<<" genuine original MULSS operand cases calibrated; native precision-status difference isolated\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
