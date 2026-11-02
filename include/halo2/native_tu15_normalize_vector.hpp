#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
struct NativeTu15NormalizeConstants {std::uint32_t epsilon,one;};
inline constexpr NativeTu15NormalizeConstants native_tu15_normalize_tu15{0x46a0ecu,0x46a0d0u};
inline constexpr NativeTu15NormalizeConstants native_tu15_normalize_mp11{0x46504cu,0x465030u};
enum class NativeTu15NormalizeCompare : std::uint8_t {above,below,equal,unordered};
enum class NativeTu15NormalizeRefusal : std::uint8_t {
    NoKernel,Profile,Depth,Instruction,Trace,Observer,Pages,Constants,Policies,Count
};
inline constexpr auto native_tu15_normalize_renderer_sites=std::to_array<std::uint32_t>({
    0x324a1,0x32630,0x3e7aa,0xc26d3,0xc5676,0xd4134,0xd41fa,0xd4bef,
    0xd6a9f,0x166ded,0x166edc,0x166fd2,0x1670c2,0x177a68,0x183d32});
struct NativeTu15NormalizeRendererSiteStats {
    std::uint64_t admitted{};
    std::uint64_t first_admitted_ordinal{},last_admitted_ordinal{};
    std::array<std::uint64_t,std::size_t(NativeTu15NormalizeRefusal::Count)> refused{};
    std::uint32_t last_refused_vector{},last_refused_stack{};
    NativeTu15NormalizeRefusal last_refusal=NativeTu15NormalizeRefusal::Count;
};
struct NativeTu15NormalizeStats {
    std::uint64_t attempts{};
    std::uint64_t admitted{};
    std::uint64_t renderer_admitted{};
    std::uint32_t last_admitted_site{};
    // Gate attempts, not unique calls: a refused required() shortcut can
    // reach another family's guarded body before the literal executes.
    std::array<std::uint64_t,std::size_t(NativeTu15NormalizeRefusal::Count)> refused{};
    std::array<NativeTu15NormalizeRendererSiteStats,native_tu15_normalize_renderer_sites.size()> renderer_sites{};
};
struct NativeTu15NormalizeOutcome {
    std::uint32_t magnitude_bits{};
    NativeTu15NormalizeCompare compare=NativeTu15NormalizeCompare::above;
    std::array<std::uint32_t,3> published{};
    bool normalized() const{return compare!=NativeTu15NormalizeCompare::above;}
    std::uint32_t xmm0_bits() const{return normalized()?magnitude_bits:0u;}
};
// Complete scalar algorithm. The rounded magnitude is retained on refusal,
// while the comparison class comes from the unrounded x87 magnitude.
NativeTu15NormalizeOutcome native_tu15_normalize_vector_30bd0(SourcePhysicsContactMemory&,
    std::uint32_t vector,NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15NormalizeConstants = native_tu15_normalize_tu15);
// For translated callers whose source environment is already installed.
// It accumulates source flags in place and leaves the host x87 stack empty.
NativeTu15NormalizeOutcome native_tu15_normalize_vector_30bd0_resident(SourcePhysicsContactMemory&,
    std::uint32_t vector,const SourcePhysicsContactRecordObserver&,
    NativeTu15NormalizeConstants);
using NativeTu15NormalizeResident = NativeTu15NormalizeOutcome (*)(SourcePhysicsContactMemory&,
    std::uint32_t,const SourcePhysicsContactRecordObserver&,NativeTu15NormalizeConstants);
}
