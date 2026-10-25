#pragma once
#include "halo2/source_shadow_native_prefix_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <cerrno>
#include <cstdlib>
#include <limits>

namespace halo2::source_shadow_native_prefix_tu15_detail {
inline thread_local SourceShadowNativePrefixTu15Stats stats{};
inline const bool reference=[] {
    const auto saved_errno=errno;
    bool enabled=false;
    for(const auto *name:{"HALO2_REFERENCE_SHADOW_NATIVE_PREFIXES","HALO2_REFERENCE_SMALL_MEMORY_TRANSFERS","HALO2_REFERENCE_SCALAR_VECTOR_READS",
            "HALO2_REFERENCE_LITERAL_INSTRUCTIONS","HALO2_REFERENCE_INSTRUCTION_CALLBACKS"}) {
        const auto *value=std::getenv(name);enabled|=value && *value;
    }
    errno=saved_errno;
    return enabled;
}();

// The original/caller-fenv qualification currently covers Linux x86 only.
// Other platform backends retain the complete literal route.
#if defined(__linux__) && (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
inline __m128 load(const std::byte *address) noexcept {
    __m128 value;
    asm volatile("movups (%1),%0":"=x"(value):"r"(address):"memory");
    return value;
}
inline __m128 multiply(__m128 a,__m128 b) noexcept {
    asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;
}
inline __m128 add(__m128 a,__m128 b) noexcept {
    asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;
}
inline __m128 subtract(__m128 a,__m128 b) noexcept {
    asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;
}
inline bool has_nan(__m128 a,__m128 b,__m128 c,__m128 d) noexcept {
    const auto absolute=_mm_set1_epi32(0x7fffffff),infinity=_mm_set1_epi32(0x7f800000);
    const auto nan=[&](__m128 value) {
        return _mm_cmpgt_epi32(_mm_and_si128(_mm_castps_si128(value),absolute),infinity);
    };
    return _mm_movemask_epi8(_mm_or_si128(_mm_or_si128(nan(a),nan(b)),
        _mm_or_si128(nan(c),nan(d))))!=0;
}
#endif


inline const std::array<bool,3> specific_reference=[] {
    const auto saved_errno=errno;std::array<bool,3> values{};unsigned n{};
    for(const auto *name:{"HALO2_REFERENCE_SHADOW_2E66B2_PREFIX","HALO2_REFERENCE_SHADOW_2E6810_PREFIX","HALO2_REFERENCE_SHADOW_2E6A50_PREFIX"}) {
        const auto *p=std::getenv(name);values[n++]=p && *p;
    }
    errno=saved_errno;return values;
}();

inline bool overlaps_cursor(const std::uint32_t *position,std::span<const std::byte> bytes) noexcept {
    if(!position)return false;
    const auto cursor=reinterpret_cast<std::uintptr_t>(position);
    const auto first=reinterpret_cast<std::uintptr_t>(bytes.data());
    // Compare differences instead of forming potentially overflowing ends.
    return cursor>=first?cursor-first<bytes.size():first-cursor<sizeof(*position);
}

// Three authenticated27-PC read-only prefixes share this exact operand graph.
// Leave each original branch/tail literal, and requalify views every iteration.
// Inline operation comments name the middle prefix's corresponding PCs.
template<unsigned PlaneGp,unsigned MaskGp,unsigned StackOffset,std::uint32_t LastPc,unsigned PrefixIndex>
inline bool run_common(physics_contact_runtime_detail::State &s) noexcept {
    ++stats.attempts;
    if(reference || specific_reference[PrefixIndex]) {++stats.declined_reference;return false;}
    const auto *cursor=s.instruction_cursor;
    if(s.services.instruction && (!s.literal_service_sites || !cursor ||
       (cursor->first_trace && *cursor->first_trace) ||
       (cursor->second_trace && *cursor->second_trace))) {
        ++stats.declined_instruction;return false;
    }
    // literal_service_sites already checked the exact callback type, executor,
    // and three PCs38846E/21F590/33B595; none lies within this source prefix.
#if defined(__linux__) && (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
    constexpr auto max=std::numeric_limits<std::uint32_t>::max();
    const auto ecx=s.gp[PlaneGp],edx=s.gp[2],esp=s.gp[4];
    if(ecx<0x210u || ecx>max-0x1fu || edx>max-15u || esp>max-(StackOffset+15u)) {
        ++stats.declined_view;return false;
    }
    const auto planes=s.memory.try_read_view(ecx-0x210u,0x230u);
    const auto point=s.memory.try_read_view(edx,16);
    const auto stack=s.memory.try_read_view(esp+StackOffset,16);
    if(!planes || !point || !stack || planes->size()!=0x230u ||
       point->size()!=16 || stack->size()!=16) {++stats.declined_view;return false;}
    // Literal cursor publication precedes every operand read. A caller-owned
    // cursor may occupy source backing, so retain those writes and reads when
    // its uint32 object overlaps any borrowed bytes, including partial overlap.
    const auto *position=s.services.instruction?cursor->position:nullptr;
    if(overlaps_cursor(position,*planes) || overlaps_cursor(position,*point) ||
       overlaps_cursor(position,*stack)) {++stats.declined_view;return false;}
    unsigned saved_mxcsr;
    asm volatile("stmxcsr %0":"=m"(saved_mxcsr)::"memory");
    if((saved_mxcsr&0x1f80u)!=0x1f80u) {++stats.declined_numeric;return false;}
    // No memory operand has been consumed yet. Every refusal above leaves the
    // literal path's register, FP, observer, cursor and fault prefix intact.
    // Volatile SSE operations preserve original load/arithmetic operand order,
    // four lanes, NaN payload selection, and the live MXCSR without contraction.
    const auto *p=planes->data();
    auto x0=load(p+0x220u);                         //2E6810
    auto x1=load(p+0x210u);                         //2E6814
    auto x7=load(p+0x1f0u);                         //2E6817
    x1=multiply(x1,s.xmm[2]);                       //2E681B
    x1=add(x1,x0);                                 //2E681E
    x0=x1;                                        //2E6821
    x1=load(p+0x200u);                             //2E6824
    x7=multiply(x7,s.xmm[4]);                       //2E6828
    x1=multiply(x1,s.xmm[3]);                       //2E682B
    x7=add(x7,x1);                                 //2E682E
    x1=x7;                                        //2E6831
    x7=load(p+0x10u);                              //2E6834
    x1=add(x1,x0);                                 //2E683B
    x0=load(p+0x20u);                              //2E683E
    x7=multiply(x7,s.xmm[5]);                       //2E6845
    auto x5=load(point->data());                    //2E6848
    x5=multiply(x5,load(stack->data()));            //2E684B
    x7=add(x7,x0);                                 //2E6850
    x0=load(p);                                    //2E6853
    x0=multiply(x0,s.xmm[6]);                       //2E685A
    x5=add(x5,x0);                                 //2E685D
    x0=x5;                                        //2E6860
    x0=add(x0,x7);                                 //2E6863
    x5=x0;                                        //2E6866
    x5=subtract(x5,x1);                            //2E6869
    // A compiler may commute the literal intrinsic's NaN operands. This
    // performance-only seam preserves that existing path's exact payloads:
    // detect NaNs with integer SSE2 before publishing any register or cursor,
    // discard speculative sticky flags, then execute the unchanged literal
    // prefix. All arithmetic participants propagate into these live outputs.
    if(has_nan(x0,x1,x5,x7)) {
        asm volatile("ldmxcsr %0"::"m"(saved_mxcsr):"memory");
        ++stats.declined_numeric;return false;
    }
    s.xmm[0]=x0;s.xmm[1]=x1;s.xmm[5]=x5;s.xmm[7]=x7;
    s.gp[MaskGp]=std::uint32_t(_mm_movemask_ps(x5));      //2E686C
    s.flags.minus(s.gp[MaskGp],15u,4);                   //2E686F
    if(s.services.instruction)*cursor->position=LastPc;
    ++stats.admitted;
    return true;
#else
    ++stats.declined_view;return false;
#endif
}
inline bool run_2e66b2(physics_contact_runtime_detail::State &s) noexcept { return run_common<1,0,64,0x2e6711u,0>(s); }
inline bool run_2e6810(physics_contact_runtime_detail::State &s) noexcept { return run_common<1,3,32,0x2e686fu,1>(s); }
inline bool run_2e6a50(physics_contact_runtime_detail::State &s) noexcept { return run_common<0,2,32,0x2e6aafu,2>(s); }

}
