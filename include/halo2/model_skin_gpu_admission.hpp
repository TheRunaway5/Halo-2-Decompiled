#pragma once
#include "halo2/model.hpp"
#include <algorithm>
#include <bit>
#if defined(__x86_64__) || defined(_M_X64)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace model_skin_admission_detail {
inline std::uint32_t absolute_bits(float value)noexcept {
    return std::bit_cast<std::uint32_t>(value)&0x7fffffffu;
}
inline bool bounded(float value,std::uint32_t low,std::uint32_t high)noexcept {
    const auto bits=absolute_bits(value);
    return bits==0 || (bits>=low && bits<=high);
}
inline bool finite_bounded(float value,std::uint32_t high)noexcept {
    return absolute_bits(value)<=high;
}
inline bool normal_bounded(std::uint32_t bits,std::uint32_t high)noexcept {
    return bits==0 || (bits>=0x00800000u && bits<=high);
}
// Only called for a nonzero normal absolute encoding. Integer classification
// must not raise floating-point exceptions on rejected NaNs or denormals.
inline int exponent(std::uint32_t bits)noexcept {return int(bits>>23)-127;}
}
inline bool model_skin_gpu_host_effects_admitted()noexcept {
#if defined(__x86_64__) || defined(_M_X64)
    const auto csr=_mm_getcsr();
    // The SSE kernel can raise only precision for the bounded domain below.
    // Require that sticky flag already set, all exceptions masked and RTE.
    return (csr&0x7f80u)==0x1f80u && (csr&0x20u);
#else
    return false;
#endif
}
inline bool model_skin_gpu_arithmetic_admitted(std::span<const Vertex> vertices,
    std::span<const SkinBinding> bindings,std::span<const std::array<float,12>> palette)noexcept {
    if(vertices.size()!=bindings.size())return false;
    // Every row retains the original finite/upper gate, including unused
    // rows. Small palettes also summarize each row once; their normal-domain
    // check still applies only when the descending kernel references that row.
    constexpr int absent=128;
    struct RowSummary {int coefficient_exponent,translation_exponent;bool normal_valid;};
    std::array<RowSummary,256> summaries;
    const bool summarized=palette.size()<=summaries.size();
    for(std::size_t row_index=0;row_index<palette.size();++row_index) {
        if(summarized) {
            auto &summary=summaries[row_index];summary={absent,absent,true};
            for(unsigned k=0;k<12;++k) {
                const auto bits=model_skin_admission_detail::absolute_bits(palette[row_index][k]);
                if(bits>0x49800000u)return false;
                if(!model_skin_admission_detail::normal_bounded(bits,0x49800000u))summary.normal_valid=false;
                if(bits>=0x00800000u) {
                    const auto exponent=model_skin_admission_detail::exponent(bits);
                    summary.coefficient_exponent=std::min(summary.coefficient_exponent,exponent);
                    if(k%4==3)summary.translation_exponent=std::min(summary.translation_exponent,exponent);
                }
            }
        } else for(auto value:palette[row_index])
            if(!model_skin_admission_detail::finite_bounded(value,0x49800000u))return false;
    }
    for(std::size_t i=0;i<vertices.size();++i) {
        const auto &v=vertices[i];
        int attribute_exponent=absent;
        for(const auto *attribute:{&v.position,&v.normal,&v.tangent,&v.binormal})
            for(auto value:{attribute->x,attribute->y,attribute->z}) {
                const auto bits=model_skin_admission_detail::absolute_bits(value);
                if(!model_skin_admission_detail::normal_bounded(bits,0x49800000u))return false;
                if(bits)attribute_exponent=std::min(attribute_exponent,model_skin_admission_detail::exponent(bits));
            }
        int last=-1,nonzero=0,weight_exponent=absent;
        for(unsigned j=0;j<4;++j) {
            const auto bits=model_skin_admission_detail::absolute_bits(bindings[i].weights[j]);
            if(!model_skin_admission_detail::normal_bounded(bits,0x3f800000u))return false;
            if(bits) {
                last=int(j);++nonzero;
                weight_exponent=std::min(weight_exponent,model_skin_admission_detail::exponent(bits));
            }
        }
        if(last<0)continue;
        int palette_exponent=absent,translation_exponent=absent;
        for(int j=last;j>=0;--j) {
            const auto bone=bindings[i].bones[unsigned(j)];if(bone>=palette.size())return false;
            const bool contributes=model_skin_admission_detail::absolute_bits(bindings[i].weights[unsigned(j)])!=0;
            if(summarized) {
                const auto &summary=summaries[bone];
                if(!summary.normal_valid)return false;
                if(contributes) {
                    palette_exponent=std::min(palette_exponent,summary.coefficient_exponent);
                    translation_exponent=std::min(translation_exponent,summary.translation_exponent);
                }
            } else for(unsigned k=0;k<12;++k) {
                const auto bits=model_skin_admission_detail::absolute_bits(palette[bone][k]);
                if(!model_skin_admission_detail::normal_bounded(bits,0x49800000u))return false;
                if(contributes && bits) {
                    palette_exponent=std::min(palette_exponent,model_skin_admission_detail::exponent(bits));
                    if(k%4==3)translation_exponent=std::min(translation_exponent,model_skin_admission_detail::exponent(bits));
                }
            }
        }
        if(palette_exponent==absent)continue; // Every evaluated row product is signed zero.
        // Products are RTE binary32. With one nonzero influence no nonzero
        // row cancellation occurs. Otherwise their p+w-23 lattice also bounds
        // every descending row sum. DP3/DPH cancellation uses b+a-23.
        const auto blend_exponent=palette_exponent+weight_exponent-(nonzero>1?23:0);
        if(blend_exponent<-126 ||
           (attribute_exponent!=absent && blend_exponent+attribute_exponent-23<-126))return false;
        // With one nonzero influence b is only a row magnitude bound. Its
        // translation can have finer spacing than the DP3 result and cancel
        // that result into a subnormal; retain the translation's own lattice.
        if(nonzero==1 && translation_exponent!=absent &&
           (model_skin_admission_detail::absolute_bits(v.position.x) ||
            model_skin_admission_detail::absolute_bits(v.position.y) ||
            model_skin_admission_detail::absolute_bits(v.position.z)) &&
           translation_exponent+weight_exponent-23<-126)return false;
    }
    return true;
}
}
