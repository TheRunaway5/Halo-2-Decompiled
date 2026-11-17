#pragma once
// Boundary adapter for the campaign environment's cluster sky selection
// (native_tu15_environment_palette_systems.hpp), entered at 133A70 in the
// campaign environment unit. Its callers are still translated TU1.5 code, so
// this adapter owns that ABI and nothing else:
//   - the arguments: EAX the cluster, EBX the output slot; the result: EAX the
//     chosen sky's tag (NONE without one);
//   - the registers the original leaves behind, with its partial writes
//     (MOV DL / MOV DX keep the upper bits) and the values of each path's
//     last instructions in ECX, EDX and ESI;
//   - the flags of the instruction that decides each exit (the sky index's
//     CMP against the list's count, the sky flag's TEST, or the default sky's
//     TEST/CMP);
//   - the register saves and restores, and each path's RET;
//   - the diagnostic cursor: each access publishes its original instruction
//     (the role -> instruction table below), and the RET publishes the exit;
//   - every store goes through the unit's store helper.
// Delete the adapter when the callers go native.
// HALO2_REFERENCE_ENVIRONMENT_PALETTE=1 restores the translated body.
// HALO2_VERIFY_ENVIRONMENT_PALETTE=1 records the native run and replays the
// unmodified translated body against it (native_tu15_object_tick.hpp).
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_environment_palette_systems.hpp"

namespace halo2::native_environment_palette_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx8;using native_object_tick_detail::sx16;using native_object_tick_detail::si;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_campaign_environment;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ENVIRONMENT_PALETTE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ENVIRONMENT_PALETTE");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    primary_negative, primary_beyond, primary_found, secondary_none, secondary_negative, secondary_beyond,
    secondary_unnamed, secondary_flagged, secondary_found, default_disabled, default_none, default_negative,
    default_beyond, default_unnamed, default_found, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Environment palette verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// ---- 133A70 -------------------------------------------------------------------
// EAX = the cluster, EBX = the output slot; EAX = the sky's tag. ESI and EDI
// (and EBP around the tag lookup) are saved; each path has its own exit.
template<class State> struct Palette : Port<State> {
    using Access=logic::cluster_sky::Access;
    Palette(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case scenario_data:return 0x133a70u;
        case output_clear:return 0x133a79u;
        case own_sky:return 0x133a7fu;
        case own_count:return 0x133a8fu;
        case own_again:return 0x133a99u;
        case own_list:return 0x133a9du;
        case own_output:gp[edi]=this->pop(0x133aa3u);return 0x133aa4u;
        case own_tag:return 0x133aa6u;
        case visible_sky:return 0x133aabu;
        case visible_count:return 0x133ab7u;
        case visible_list:return 0x133ac1u;
        case visible_tag:return 0x133ac7u;
        // AND ESI,FFFF; PUSH EBP.
        case tag_instances:gp[esi]=this->flags.logic(gp[esi]&0xffffu,4);this->push(0x133ad5u,gp[ebp]);return 0x133ad6u;
        case visible_definition:return 0x133adfu;
        case visible_flags:return 0x133ae3u;
        case visible_output:return 0x133aeau;
        case visible_tag_again:return 0x133aecu;
        case bsp_index:return 0x133af2u;
        case bsp_list:return 0x133af9u;
        case bsp_flags:return 0x133b04u;
        case default_index:return 0x133b0au;
        case default_count:return 0x133b19u;
        case default_list:return 0x133b23u;
        case default_tag:return 0x133b26u;
        case default_output:return 0x133b32u;
        case default_tag_again:return 0x133b34u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOV ECX; PUSH ESI; MOV ESI,EAX (the cluster).
        case scenario_data:gp[ecx]=v;this->push(0x133a76u,gp[esi]);gp[esi]=gp[eax];break;
        // MOV DL; OR EAX,-1; CMP DL,FF; PUSH EDI; then TEST DL,DL (the sign).
        case own_sky:
            set_low_byte(gp[edx],v);gp[eax]=logic::none;this->flags.minus(v,logic::no_sky,1);
            this->push(0x133a88u,gp[edi]);
            if(v!=logic::no_sky){this->test8(v);if(logic::negative_byte(v))this->reached(primary_negative);}
            break;
        // MOV EDI; MOVSX EDX,DL; CMP EDX,EDI.
        case own_count:gp[edi]=v;gp[edx]=sx8(gp[edx]);this->compare32(gp[edx],v);
            this->reached(si(gp[edx])>=si(v)?primary_beyond:primary_found);break;
        case own_again:gp[eax]=sx8(v);break;
        case own_list:gp[ecx]=v+gp[eax]*8u;break;                     // LEA ECX,[ECX+EAX*8]
        // MOV DL; CMP DL,FF; then TEST DL,DL (the sign).
        case visible_sky:
            set_low_byte(gp[edx],v);this->flags.minus(v,logic::no_sky,1);
            if(v==logic::no_sky)this->reached(secondary_none);
            else {this->test8(v);if(logic::negative_byte(v))this->reached(secondary_negative);}
            break;
        // MOV ESI; MOVSX EDX,DL; CMP EDX,ESI.
        case visible_count:gp[esi]=v;gp[edx]=sx8(gp[edx]);this->compare32(gp[edx],v);
            if(si(gp[edx])>=si(v))this->reached(secondary_beyond);
            break;
        case visible_list:gp[esi]=v;gp[edi]=v+gp[edx]*8u;break;       // LEA EDI,[ESI+EDX*8]
        case visible_tag:gp[esi]=v;this->compare32(v,logic::none);if(v==logic::none)this->reached(secondary_unnamed);break;
        case tag_instances:gp[ebp]=v;gp[esi]=this->flags.shift(gp[esi],4,4);break;   // SHL ESI,4
        case visible_definition:gp[esi]=v;break;
        // TEST byte [ESI+10],10; POP EBP.
        case visible_flags:
            this->test8(v&logic::sky_flags::not_visible_from_other_clusters);gp[ebp]=this->pop(0x133ae7u);
            this->reached(v&logic::sky_flags::not_visible_from_other_clusters?secondary_flagged:secondary_found);
            break;
        case bsp_index:gp[edx]=sx16(v);break;
        // MOV EDI; IMUL EDX,EDX,44; ADD EDX,EDI.
        case bsp_list:gp[edi]=v;gp[edx]=this->flags.plus(this->flags.product(gp[edx],0x44u),v,4);break;
        case bsp_flags:this->test8(v&logic::structure_bsp_flags::default_sky);
            if(!(v&logic::structure_bsp_flags::default_sky))this->reached(default_disabled);
            break;
        // MOV DX; CMP DX,-1; then TEST DX,DX (the sign).
        case default_index:
            gp[edx]=(gp[edx]&0xffff0000u)|v;this->flags.minus(gp[edx],0xffffu,2);
            if(v==0xffffu)this->reached(default_none);
            else {this->flags.logic(gp[edx],2);if(v&0x8000u)this->reached(default_negative);}
            break;
        // MOV ESI; MOVSX EDX,DX; CMP EDX,ESI.
        case default_count:gp[esi]=v;gp[edx]=sx16(gp[edx]);this->compare32(gp[edx],v);
            if(si(gp[edx])>=si(v))this->reached(default_beyond);
            break;
        case default_list:gp[ecx]=v;break;
        // MOV ESI; CMP ESI,-1; LEA ECX,[ECX+EDX*8].
        case default_tag:gp[esi]=v;this->compare32(v,logic::none);gp[ecx]+=gp[edx]*8u;
            this->reached(v==logic::none?default_unnamed:default_found);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // POP EDI; POP ESI; RET.
    void leave(std::uint32_t pops,std::uint32_t last) {
        auto &gp=this->gp;
        gp[edi]=this->pop(pops);gp[esi]=this->pop(pops+1u);
        this->ret(last,0);this->publish(last);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Palette> memory{*this};
        const auto result=logic::cluster_sky::select(memory,gp[eax],gp[ebx]);
        gp[eax]=result.sky_tag;
        switch(result.choice) {
        case logic::cluster_sky::Choice::own_sky:
            gp[esi]=this->pop(0x133aa9u);
            this->ret(0x133aaau,0);this->publish(0x133aaau);return;
        case logic::cluster_sky::Choice::visible_sky:leave(0x133aefu,0x133af1u);return;
        case logic::cluster_sky::Choice::default_sky:
        case logic::cluster_sky::Choice::no_sky:leave(0x133b37u,0x133b39u);return;
        }
    }
};

template<class State,class Reference> bool bridge(State &s,Reference reference) {
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x133a70u,0x133b39u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Palette<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_environment_palette_detail
