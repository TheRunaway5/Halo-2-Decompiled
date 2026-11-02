#pragma once
#include "halo2/native_tu15_normalize_vector.hpp"
#include "physics/runtime/physics_instruction_remap_tu15.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
enum class NativeTu15NormalizeProfile : std::uint8_t {Tu15Literal,ImageSelected,Mp11Literal};
inline NativeTu15NormalizeConstants native_tu15_normalize_vector_constants(
    NativeTu15NormalizeProfile profile,SourcePhysicsContactImage image) {
    return profile==NativeTu15NormalizeProfile::Mp11Literal ||
        (profile==NativeTu15NormalizeProfile::ImageSelected && image==SourcePhysicsContactImage::MapPack11)?
        native_tu15_normalize_mp11:native_tu15_normalize_tu15;
}
// State-local transport: the enclosing translated entry already owns the
// source FP environment. Live logical x87 values never enter the host stack.
// This semantic boundary elides transient call-stack events and instruction PCs;
// instrumented/fault-prefix owners must retain their translated reference.
template<class State> bool native_tu15_normalize_vector_apply(State &s,const NativeTu15NormalizeOutcome &result) {
    s.gp[1]=result.magnitude_bits;s.gp[4]+=4u;
    using Compare=NativeTu15NormalizeCompare;
    s.flags.cf=result.compare==Compare::below||result.compare==Compare::unordered;
    s.flags.zf=result.compare==Compare::equal||result.compare==Compare::unordered;
    s.flags.pf=result.compare==Compare::unordered;s.flags.af=s.flags.sf=s.flags.of=false;
    s.fp.status&=~0x200u;
    const auto lane=[](std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));};
    s.xmm[0]=lane(result.xmm0_bits());
    if(result.normalized()) {
        s.xmm[1]=lane(result.published[2]);s.xmm[2]=lane(result.published[1]);
    }
    return result.normalized();
}
template<class State> bool native_tu15_normalize_vector_state(State &s,
    NativeTu15NormalizeResident kernel,NativeTu15NormalizeProfile profile) {
    if constexpr(requires{s.active_mmx;})
        if(s.active_mmx)throw std::runtime_error("Required original mixed MMX/x87 operation owner unavailable");
    if(s.fp.count>3)throw std::runtime_error("Required original solver x87 stack-fault owner unavailable");
    const auto result=kernel(s.memory,s.gp[0],s.observe,native_tu15_normalize_vector_constants(profile,s.image));
    // Preserve final local bytes. The owning service holds any registry lease
    // through this write, or explicitly owns an ordinary adapter without one.
    std::array<std::byte,4> residue{};
    for(unsigned n=0;n<4;++n)residue[n]=std::byte(result.magnitude_bits>>(n*8u));
    s.memory.write(s.gp[4]-4u,residue);
    return native_tu15_normalize_vector_apply(s,result);
}
inline constexpr auto native_tu15_normalize_vector_sites=std::to_array<std::uint32_t>({
    0x30bd0,0x30bd1,0x30bd4,0x30bd7,0x30bd9,0x30bdb,0x30bdd,0x30bdf,
    0x30be1,0x30be3,0x30be5,0x30be7,0x30be9,0x30beb,0x30bed,0x30bef,
    0x30bf1,0x30bf4,0x30bf6,0x30bfc,0x30bfe,0x30c00,0x30c02,0x30c0a,
    0x30c0f,0x30c13,0x30c17,0x30c1b,0x30c1f,0x30c22,0x30c27,0x30c2c,
    0x30c31,0x30c36,0x30c37,0x30c38,0x30c3b,0x30c3c});

template<class State> bool native_tu15_normalize_vector_try(State &s,NativeTu15NormalizeProfile profile) {
    const auto ordinal=s.services.normalize_vector_30bd0_stats?++s.services.normalize_vector_30bd0_stats->attempts:0u;
    const SourcePhysicsContactRuntimeServices::InstructionCursor *cursor=nullptr;
    const bool has_kernel=s.services.normalize_vector_30bd0||s.services.normalize_vector_30bd0_guarded;
    if((has_kernel||s.services.normalize_vector_30bd0_stats)&&s.services.instruction) {
        if constexpr(requires{s.literal_service_sites;s.instruction_cursor;})
            if(s.literal_service_sites)cursor=s.instruction_cursor;
        if(!cursor)if(const auto *remap=s.services.instruction.template target<PhysicsInstructionRemapTu15>())
            if(remap->covers(native_tu15_normalize_vector_sites,0))cursor=remap->live_cursor();
        if(cursor&&!cursor->position)cursor=nullptr;
    }
    const auto callsite=cursor?*cursor->position:0u;
    auto renderer_site=native_tu15_normalize_renderer_sites.size();
    if(s.services.normalize_vector_30bd0_stats)
        for(std::size_t i=0;i<native_tu15_normalize_renderer_sites.size();++i)
            if(native_tu15_normalize_renderer_sites[i]==callsite){renderer_site=i;break;}
    const auto refuse=[&](NativeTu15NormalizeRefusal why) {
        if(auto *stats=s.services.normalize_vector_30bd0_stats) {
            ++stats->refused[std::size_t(why)];
            if(renderer_site<stats->renderer_sites.size()) {
                auto &site=stats->renderer_sites[renderer_site];++site.refused[std::size_t(why)];
                site.last_refused_vector=s.gp[0];site.last_refused_stack=s.gp[4];site.last_refusal=why;
            }
        }
        return false;
    };
    using Refusal=NativeTu15NormalizeRefusal;
    if(!s.services.normalize_vector_30bd0&&!s.services.normalize_vector_30bd0_guarded)return refuse(Refusal::NoKernel);
    if(profile==NativeTu15NormalizeProfile::Tu15Literal && s.image!=SourcePhysicsContactImage::TitleUpdate15)
        return refuse(Refusal::Profile);
    if(s.fp.count>3)return refuse(Refusal::Depth); // literal owns the original refusal prefix
    if(s.services.instruction) {
        if(!cursor)return refuse(Refusal::Instruction);
        if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))
            return refuse(Refusal::Trace);
    }
    if(s.observe)return refuse(Refusal::Observer);
    const auto v=s.gp[0],e=s.gp[4];
    if(std::uint64_t(v)+12u>0x100000000ull || e<4u || std::uint64_t(e)+4u>0x100000000ull)
        return refuse(Refusal::Pages);
    const auto constants=native_tu15_normalize_vector_constants(profile,s.image);
    const auto overlaps=[](std::uint64_t a,unsigned na,std::uint64_t b,unsigned nb) {
        return a<b+nb && b<a+na;
    };
    if(overlaps(e-4u,8,v,12)||overlaps(e-4u,8,constants.epsilon,4)||overlaps(e-4u,8,constants.one,4))
        return refuse(Refusal::Pages);
    {
        const auto write_extent=[&](std::uint32_t address,std::size_t bytes)->std::optional<std::span<const std::byte>> {
            if(const auto *page=s.memory.direct_page(address,bytes)) {
                if(!page->writable)return std::nullopt;
                return std::span<const std::byte>(page->bytes+(address&4095u),bytes);
            }
            if(s.services.normalize_vector_30bd0_guarded)return s.memory.try_write_extent(address,bytes);
            return std::nullopt; // Raw adapters retain full-page-only admission.
        };
        const auto vector=write_extent(v,12),stack=write_extent(e-4u,8);
        if(!vector||!stack||vector->size()!=12||stack->size()!=8)return refuse(Refusal::Pages);
        // Exact sub-page owners carry the same ordinary-storage proof as a
        // full writable page. Distinct source extents can still share backing.
        const auto backing=[](auto view){return std::uint64_t(reinterpret_cast<std::uintptr_t>(view.data()));};
        const auto scratch=backing(*stack);
        if(overlaps(scratch,8,backing(*vector),12))return refuse(Refusal::Pages);
        // Authored constants can be separate four-byte owners. Prove only
        // those exact extents; never publish a page over their unowned gaps.
        const auto constant_view=[&](std::uint32_t address)->std::optional<std::span<const std::byte>> {
            if(const auto *page=s.memory.direct_page(address,4))return std::span<const std::byte>(page->bytes+(address&4095u),4);
            return s.memory.try_read_view(address,4);
        };
        const auto epsilon=constant_view(constants.epsilon),one=constant_view(constants.one);
        if(!epsilon||!one||epsilon->size()!=4||one->size()!=4)return refuse(Refusal::Constants);
        if(overlaps(scratch,8,backing(*epsilon),4)||overlaps(scratch,8,backing(*one),4))return refuse(Refusal::Pages);
        // All extent borrows end here, before any callback or source write.
        // Registry policies remain the guarded service's responsibility.
    }
    bool normalized;
    if(s.services.normalize_vector_30bd0_guarded) {
        if constexpr(requires{s.active_mmx;})
            if(s.active_mmx)throw std::runtime_error("Required original mixed MMX/x87 operation owner unavailable");
        const auto result=s.services.normalize_vector_30bd0_guarded(s.memory,v,e,s.observe,constants);
        if(!result)return refuse(Refusal::Policies);
        normalized=native_tu15_normalize_vector_apply(s,*result);
    }else normalized=native_tu15_normalize_vector_state(s,s.services.normalize_vector_30bd0,profile);
    if(cursor)*cursor->position=normalized?0x30c37u:0x30c3cu;
    if(auto *stats=s.services.normalize_vector_30bd0_stats) {
        ++stats->admitted;stats->last_admitted_site=callsite;
        if(renderer_site<stats->renderer_sites.size()) {
            ++stats->renderer_admitted;
            auto &site=stats->renderer_sites[renderer_site];
            if(!site.admitted)site.first_admitted_ordinal=ordinal;
            site.last_admitted_ordinal=ordinal;
            ++stats->renderer_sites[renderer_site].admitted;
        }
    }
    return true;
}
}
