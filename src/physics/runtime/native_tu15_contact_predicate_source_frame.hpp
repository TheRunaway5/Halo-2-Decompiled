#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_contact_predicate_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <emmintrin.h>

namespace halo2::native_contact_predicate_detail {
enum class SourceFormat { direct, literal };

// This view uses the retained logical x87 owner, source scalar/vector helpers
// and entry-selected store observer. Physical MOVAPS and raw FP transport are
// separately qualified. No user-visible callback may be elided in this view.
template<SourceFormat Format, class State>
bool source_continuation_allowed(const State& s) {
    if (s.image != SourcePhysicsContactImage::TitleUpdate15 ||
        s.fp.count > 8 || s.fp.top > 7 || s.active_mmx) return false;
    std::uint16_t control, status;
    asm volatile("fnstcw %0; fnstsw %1" : "=m"(control), "=a"(status));
    const auto mxcsr = _mm_getcsr();
    if ((control & 0x3fu) != 0x3fu || ((status | s.fp.status) & 0x8080u) ||
        (mxcsr & 0x1f80u) != 0x1f80u || (mxcsr & 0xffff0000u)) return false;
    const auto& cursor = s.services.instruction_cursor;
    if (!s.services.instruction)
        return !s.instruction_cursor && !s.literal_service_sites && !cursor.position && !cursor.literal_service_sites;
    if constexpr (Format == SourceFormat::direct) return false;
    return s.literal_service_sites && s.live_instruction_cursor() == &cursor &&
        cursor.literal_service_sites && cursor.service_sites.size() == 3 &&
        cursor.service_sites[0] == 0x38846eu && cursor.service_sites[1] == 0x21f590u &&
        cursor.service_sites[2] == 0x33b595u &&
        !(cursor.first_trace && *cursor.first_trace) && !(cursor.second_trace && *cursor.second_trace);
}

struct SourceContinuation { std::uint32_t next; const void* owner; };

template<SourceFormat Format, class State> class SourceFrame {
    State& s_;
    const void* token_;

    void at(std::uint32_t pc) {
        if constexpr (Format == SourceFormat::literal) s_.instruction_literal(pc);
    }
    void retired(std::uint32_t next) {
        if (!source_continuation_allowed<Format>(s_)) throw SourceContinuation{next,token_};
    }
    void register_read(std::uint32_t pc, std::uint32_t next, unsigned reg, std::uint32_t address) {
        at(pc);
        s_.gp[reg] = physics_step_detail::r(s_.memory,address,4);
        retired(next);
    }
    void vector_read(std::uint32_t pc, std::uint32_t next, unsigned reg, std::uint32_t address) {
        at(pc);
        s_.xmm[reg] = physics_step_detail::v(s_.memory,address);
        retired(next);
    }
    void source_push(std::uint32_t pc, std::uint32_t next, unsigned reg) {
        at(pc);
        s_.push(pc,s_.gp[reg]);
        retired(next);
    }
    void source_pop(std::uint32_t pc, std::uint32_t next, unsigned reg) {
        at(pc);
        s_.gp[reg] = s_.pop();
        retired(next);
    }
    void load_float(std::uint32_t pc, std::uint32_t next, std::uint32_t address) {
        at(pc);
        s_.fp.push(physics_step_detail::ext(physics_step_detail::f(physics_step_detail::r(s_.memory,address,4))));
        retired(next);
    }
    std::uint16_t threshold(std::uint32_t pc, std::uint32_t next, std::uint32_t address, unsigned mask) {
        at(pc);
        s_.fp.comparison(physics_step_detail::ext(physics_step_detail::f(physics_step_detail::r(s_.memory,address,4))),1);
        // The FCOMP memory access and logical pop retire before FNSTSW AX.
        retired(next);
        const auto status = s_.fp.word();
        s_.set(0,status,2,0);
        s_.flags.logic((status >> 8u) & mask,1);
        return status;
    }
    void scratch(std::uint32_t write, std::uint32_t read, std::uint32_t compare, unsigned address_reg, unsigned value_reg) {
        at(write);
        physics_step_detail::w(s_.memory,s_.observe,write,s_.gp[address_reg],4,
            physics_step_detail::u(_mm_cvtss_f32(s_.xmm[value_reg])));
        // Reacquire the live SP and actual bytes after the completed store.
        retired(read);
        load_float(read,compare,s_.gp[4]+12u);
    }
public:
    SourceFrame(State& s, const void* token) : s_(s), token_(token) {}

    std::uint16_t distance_limit() {
        source_push(0x303800u,0x303801u,5);
        s_.gp[5]=s_.gp[4];s_.gp[4]=s_.flags.logic(s_.gp[4]&0xfffffff0u,4);
        s_.gp[4]=s_.flags.minus(s_.gp[4],12u,4);
        register_read(0x303809u,0x30380cu,1,s_.gp[5]+16u);
        load_float(0x30380cu,0x30380fu,s_.gp[1]+12u);
        source_push(0x30380fu,0x303810u,6);
        return threshold(0x303810u,0x303816u,0x41d018u,0x05u);
    }
    Pair normals() {
        register_read(0x303821u,0x303824u,2,s_.gp[5]+12u);
        vector_read(0x303824u,0x303828u,1,s_.gp[2]+16u);
        vector_read(0x303828u,0x30382cu,2,s_.gp[1]+16u);
        register_read(0x30382cu,0x30382fu,6,s_.gp[5]+20u);
        return {s_.xmm[1],s_.xmm[2]};
    }
    std::uint16_t normal_threshold(const Dot& agreement) {
        s_.xmm[0]=agreement.product;s_.xmm[2]=agreement.y_sum;s_.xmm[3]=agreement.z_sum;
        s_.gp[0]=s_.gp[4]+12u;
        scratch(0x30384fu,0x303853u,0x303857u,0,3);
        return threshold(0x303857u,0x30385au,s_.gp[6]+16u,0x05u);
    }
    Pair positions() {
        vector_read(0x303861u,0x303864u,0,s_.gp[2]);
        vector_read(0x303864u,0x303867u,2,s_.gp[1]);
        return {s_.xmm[0],s_.xmm[2]};
    }
    __m128 separation_normal() const { return s_.xmm[1]; }
    std::uint16_t separation_threshold(const Dot& separation) {
        s_.xmm[0]=separation.product;s_.xmm[1]=separation.y_sum;s_.xmm[2]=separation.z_sum;
        s_.gp[1]=s_.gp[4]+12u;
        scratch(0x30388au,0x30388eu,0x303892u,1,2);
        return threshold(0x303892u,0x303895u,s_.gp[6]+12u,0x41u);
    }
    void finish(bool rejected) {
        const auto output = rejected ? 0x3038acu : 0x30389fu;
        const auto pop_si = rejected ? 0x3038afu : 0x3038a2u;
        const auto reset_sp = rejected ? 0x3038b0u : 0x3038a3u;
        const auto pop_bp = rejected ? 0x3038b2u : 0x3038a5u;
        const auto ret = rejected ? 0x3038b3u : 0x3038a6u;
        register_read(rejected ? 0x3038a9u : 0x30389cu,output,0,s_.gp[5]+8u);
        at(output);
        physics_step_detail::w(s_.memory,s_.observe,output,s_.gp[0],1,rejected ? 1u : 0u);
        retired(pop_si);
        source_pop(pop_si,reset_sp,6);
        s_.gp[4]=s_.gp[5];
        source_pop(pop_bp,ret,5);
        at(ret);s_.pop();s_.gp[4]+=16u;
    }
};

// Zero refuses at entry; one completes; any other result is the next exact
// retained PC after a completed source I/O instruction.
template<SourceFormat Format, class State> std::uint32_t run_source_private(State& s) {
    if (!source_continuation_allowed<Format>(s) || s.fp.count == 8) return 0;
    const std::byte token{};
    try {
        SourceFrame<Format,State> view{s,&token};
        const bool rejected = reject(view);
        view.finish(rejected);
        return 1;
    } catch (const SourceContinuation& resume) {
        if (resume.owner != &token) throw;
        return resume.next;
    }
}
} // namespace halo2::native_contact_predicate_detail
#endif
