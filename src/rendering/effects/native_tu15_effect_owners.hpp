#pragma once
// Boundary adapters for the effect systems
// (native_tu15_effect_owners_systems.hpp): 20680 lens query reset, C35D0
// light fades (with C3300 and C2A80 inline), 176A40 particle inputs (with
// 252100 inline). Their callers and children are still translated TU1.5 code,
// so each adapter owns that ABI and nothing else:
//   - arguments from stack slots, results back into the original's registers
//     (including the values the original leaves in scratch GP and SSE
//     registers and the x87 C1 of the fade's spill);
//   - the original's register file at every child call (16EA60, 4EFA0): each
//     access role's register effects, the "kept" values handed back after the
//     call, the SSE scratch values the logic passes with it, and the flags of
//     the instruction that decides them;
//   - the original's stack frames, including the inline callees': register
//     saves and restores, argument pushes, return addresses and RET pops, and
//     the frame slots whose reads and writes are part of the original's access
//     order;
//   - the diagnostic cursor: each access publishes its original instruction
//     (a per-owner table of access role -> instruction), and the RET publishes
//     the exit;
//   - every store goes through the unit's store helper, so guards and the
//     store observer see the original store stream;
//   - 252100's jump table, read from the bank at 252430 (a target outside its
//     14 entries throws the unit's "Unbound TU1.5 contact dispatch target").
// Delete an adapter when its callers and children go native.
//
// The genuine children stay translated and are entered through the unit's
// own route with the exact register, flag, XMM, x87 and stack ABI: 16EA60
// through its body, 4EFA0 through its body (combat action, particle frame) or
// the unit's dispatch (death cycle).
// tools/native_effect_owner_replacements.py installs the bridges into the
// build-local specialized copies of the lens flare, effect light view,
// particle frame, combat action and death cycle units.
// HALO2_REFERENCE_EFFECT_OWNERS=1 restores every translated body;
// HALO2_REFERENCE_EFFECT_OWNERS=20680,c35d0,176a40 restores the listed ones.
// HALO2_VERIFY_EFFECT_OWNERS=1 records each admitted native call and replays
// the unmodified translated body against it (native_tu15_object_tick.hpp):
// same ordered accesses, stores, child entry ABI and final registers, flags,
// XMM and x87 state. A summary is printed at exit.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/effects/native_tu15_effect_owners_systems.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <emmintrin.h>
#include <xmmintrin.h>

namespace halo2::native_effect_owner_detail {
namespace logic=halo2::native_effects;
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx16;
using native_object_tick_detail::sar;
using namespace native_object_tick_detail::boundary;

inline bool owner_reference(std::uint32_t entry) {
    static const std::string value=[]{const auto *p=std::getenv("HALO2_REFERENCE_EFFECT_OWNERS");return std::string(p?p:"");}();
    if(value.empty())return false;
    if(value=="1")return true;
    char name[16];std::snprintf(name,sizeof name,"%x",entry);
    for(std::size_t at=0;at<value.size();) {
        auto end=value.find(',',at);if(end==std::string::npos)end=value.size();
        auto item=value.substr(at,end-at);
        if(item.starts_with("0x")||item.starts_with("0X"))item=item.substr(2);
        for(auto &c:item)c=char(std::tolower(static_cast<unsigned char>(c)));
        if(item==name)return true;
        at=end+1;
    }
    return false;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_EFFECT_OWNERS");return p && *p;}();
    return value;
}

// Decisions reached in verification (coverage). The adapters read them from
// the logic's results and accesses; the logic carries no instrumentation.
enum Branch : unsigned {
    // 20680
    lf_negative, lf_mode_low, lf_mode_high, lf_mode_same, lf_mode_changed, lf_newer, lf_unset, lf_stale,
    lf_flag_in_mode, lf_flag_out_low, lf_flag_out_high,
    // C35D0
    lv_empty, lv_first, lv_first_invalid, lv_none, lv_flagged, lv_unflagged, lv_null_handle, lv_scan_negative,
    lv_scan_bound, lv_scan_found, lv_scan_end, lv_next_none, lv_next_handle, lv_next_invalid, lv_more, lv_last,
    // C3300
    lt_hotter, lt_kept, lt_raised, lt_clamped,
    // C2A80
    fd_wide, fd_narrow, fd_below, fd_wide_ratio, fd_narrow_ratio, fd_over, fd_wide_rest, fd_narrow_rest, fd_mid,
    // 176A40
    sa_empty, sa_value, sa_zero_scan,
    // 252100
    sv_out_of_range, sv_null, sv_age, sv_lifetime, sv_word, sv_word_low, sv_system, sv_system_input, sv_location,
    sv_game_time, sv_no_owner, sv_owner_first, sv_owner_second, sv_fraction, sv_projection, sv_cross_projection, sv_byte,
    sv_wrapped, sv_unwrapped,
    branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls[3]{};
    ~VerifySummary() {
        if(calls[0]||calls[1]||calls[2])
            std::fprintf(stderr,"Effect owners verify: 20680 %llu, C35D0 %llu, 176A40 %llu calls; %llu ordered accesses (%llu stores) "
                "and %llu child boundaries identical, coverage %llx\n",(unsigned long long)calls[0].load(),
                (unsigned long long)calls[1].load(),(unsigned long long)calls[2].load(),
                (unsigned long long)accesses.load(),(unsigned long long)stores.load(),
                (unsigned long long)children.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// MOVSS xmm,m32 and the scalar SSE results: the bits in lane 0, upper lanes zero.
inline __m128 scalar(float value){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(std::bit_cast<std::uint32_t>(value))));}
// CVTSI2SS into a register: lane 0 replaced, the upper lanes kept.
inline __m128 replace_low(__m128 reg,float value) {
    auto lanes=std::bit_cast<std::array<std::uint32_t,4>>(reg);lanes[0]=std::bit_cast<std::uint32_t>(value);
    return std::bit_cast<__m128>(lanes);
}
inline constexpr bool backend=logic::sse::available && logic::x87::available;
// Masked x87 and MXCSR exceptions (the fade's x87 and SSE parts need them).
inline bool masked_controls() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t control;asm volatile("fnstcw %0":"=m"(control));
    return (control&0x3fu)==0x3fu && (_mm_getcsr()&0x1f80u)==0x1f80u;
#else
    return false;
#endif
}

// The logic's Memory, with the single-precision accessors the effect systems
// use (bit-exact: a float is moved as its 32 bits).
template<class Boundary> struct Memory : native_object_tick_detail::boundary::Memory<Boundary> {
    template<class A> float load_float(A a,std::uint32_t address){return std::bit_cast<float>(this->load32(a,address));}
    template<class A> void store_float(A a,std::uint32_t address,float value){this->store32(a,address,std::bit_cast<std::uint32_t>(value));}
};

// ---- 20680 lens query reset ---------------------------------------------------
// No arguments. EDX = the stamp; ECX = the view mode and EAX = the last reset
// stamp once read; after a reset EAX = the "queries issued" byte. The exit
// flags are the deciding TEST's. RET.
template<class State> struct LensQueryReset : Port<State> {
    using Access=logic::lens_query_reset::Access;
    LensQueryReset(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case stamp:return 0x20680u;
        case mode:return 0x2068au;
        case last_stamp:return 0x20693u;
        case record_word:case record_word_store:return 0x206b0u;
        case last_stamp_store:return 0x206c5u;
        case reset_set:return 0x206cbu;
        case queries_store:return 0x206deu;
        case current_on_reset:return 0x206e3u;
        case reset_clear:return 0x206eeu;
        case current_otherwise:return 0x206f5u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        if(a==stamp)gp[edx]=v;
        else if(a==mode)gp[ecx]=v;
        else if(a==last_stamp)gp[eax]=v;
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        Memory<LensQueryReset> memory{{*this}};
        const auto result=logic::lens_query_reset::run(memory);
        if(result.stamp<0)this->reached(lf_negative);
        else {
            const auto mode=*result.mode,stamp=result.stamp,last=*result.last;
            const bool same_in_mode=logic::lens_query_reset::queries_off(mode) && stamp==last;
            this->reached(mode<4?lf_mode_low:mode>7?lf_mode_high:stamp==last?lf_mode_same:lf_mode_changed);
            if(!same_in_mode)this->reached(stamp>last?lf_newer:last==0?lf_unset:lf_stale);
        }
        if(!result.reset) {
            // TEST EDX,EDX (a negative stamp) or TEST EAX,EAX (the last stamp).
            this->test32(result.stamp<0?std::uint32_t(result.stamp):std::uint32_t(*result.last));
            this->ret(0x206ffu,0);this->publish(0x206ffu);
            return;
        }
        // MOV EAX,1, cleared outside modes 4..7; TEST AL,AL; SETE AL.
        const auto mode=*result.mode;
        this->reached(mode<4?lf_flag_out_low:mode>7?lf_flag_out_high:lf_flag_in_mode);
        const bool off=logic::lens_query_reset::queries_off(mode);
        this->test8(off?1u:0u);
        this->gp[eax]=off?0u:1u;
        this->ret(0x206edu,0);this->publish(0x206edu);
    }
};

// ---- C35D0 light fades ----------------------------------------------------------
// No arguments. 16EA60 is entered with ECX = the light array, EDX = 0 (XOR
// EDX,EDX decides its entry flags) and EBP saved. The walk keeps the array in
// EBP, the handle in EDI and the index in ESI; C3300 saves ESI and EDI in its
// frame and the walk continues with what it restored. At exit EAX = -1 (or
// 16EA60's index), ECX/EDX hold the last handle computation's, scan's or
// fade's values, EBX/ESI/EDI/EBP are restored and the flags are CMP -1,-1's.
// RET.
// C3300 saves ECX (its local: it keeps the secondary fade there and pops it
// into ECX), ESI and EDI; EDX = the light. C2A80 takes the radius and the
// distance table on the stack, spills the distance over the radius, and RET
// 8. XMM0..XMM7 end as the last C3300 leaves them.
// Only the register effects a store, a child or the exit can observe are
// mirrored: within the walk, EBX (saved before it and restored after) and
// C3300's and C2A80's scratch GP registers are never observed.
template<class State,class Iterate> struct LightFades : Port<State> {
    using Access=logic::light_fades::Access;
    using LightAccess=logic::light_fades::LightAccess;
    using FadeAccess=logic::light_fades::FadeAccess;
    Iterate &iterate;
    bool walking{};      // EBX and ESI are saved once the walk starts
    bool first_probe{};  // the scan's first bit test (no INC EAX before it)
    LightFades(State &s,std::uint32_t *c,Trace<State> *t,Iterate &i):Port<State>(s,c,t,{}),iterate(i){}

    // C3300's call sites into C2A80, per fade: the table and radius pushes and
    // the CALL with its return address.
    struct FadeSite {std::uint32_t table_push,radius_push,call,next;};
    static constexpr FadeSite fade_sites[]{
        {0xc3340u,0xc3348u,0xc334bu,0xc3350u},{0xc336du,0xc3372u,0xc3375u,0xc337au},
        {0xc3394u,0xc3399u,0xc339cu,0xc33a1u},{0xc33bbu,0xc33c0u,0xc33c3u,0xc33c8u},
        {0xc33d8u,0xc33ddu,0xc33f0u,0xc33f5u}};

    // C35D0 accesses.
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case lights:this->push(0xc35d0u,gp[ebp]);return 0xc35d1u;
        case first_size:return 0xc35e9u;
        case first_data:return 0xc35ecu;
        case first_identifier:return 0xc35f2u;
        case datums:
            // PUSH EBX/ESI before the first light; MOV EDI,EAX after each later one.
            if(!walking){this->push(0xc3604u,gp[ebx]);this->push(0xc3605u,gp[esi]);walking=true;}
            else {gp[edi]=gp[eax];this->reached(lv_more);}
            return 0xc3610u;
        case flags:return 0xc3623u;
        // EAX = the search start (0 for a NONE handle).
        case scan_limit:
            if(gp[edi]==logic::none)this->reached(lv_null_handle);
            gp[eax]=gp[edi]==logic::none?0u:gp[esi]+1u;
            return 0xc3644u;
        case scan_bits:return 0xc364bu;
        // INC EAX after a miss; ECX = the bit's word (SAR EAX,5).
        case scan_word:
            if(!first_probe)++gp[eax];
            first_probe=false;gp[ecx]=sar(gp[eax],5);
            return 0xc3661u;
        case next_size:return 0xc3677u;
        case next_data:return 0xc367au;
        case next_identifier:return 0xc3680u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case lights:gp[ebp]=v;break;
        case first_size:gp[ecx]=v;break;
        case first_data:gp[edx]=v;gp[ecx]*=gp[eax];break;
        case first_identifier:
            gp[edi]=(sx16(v)<<16)|gp[eax];
            if(gp[edi]==logic::none)this->reached(lv_first_invalid);
            break;
        case datums:gp[ecx]=v;gp[esi]=gp[edi]&0xffffu;break;
        case flags:this->reached(v&logic::light_fades::light_flags::active?lv_flagged:lv_unflagged);break;
        case scan_bits:gp[edx]=v;first_probe=true;break;
        case next_size:gp[edx]=v;break;
        case next_data:gp[eax]=v;gp[edx]*=gp[esi];break;
        case next_identifier:
            gp[eax]=(sx16(v)<<16)|gp[esi];
            if(gp[eax]==logic::none)this->reached(lv_next_invalid);
            break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}

    // C3300 accesses.
    std::uint32_t instruction(LightAccess a,std::uint32_t address) {
        auto &gp=this->gp;
        using enum LightAccess;
        switch(a) {
        // CALL C3300; PUSH ECX.
        case lights:this->push(0xc362cu,0xc3631u);this->push(0xc3300u,gp[ecx]);return 0xc3301u;
        case datums:return 0xc3306u;
        case tag_table:return 0xc3309u;
        case overall_range:return 0xc330eu;
        // EDX = the light (computed after the range load).
        case definition_tag:gp[edx]=address-std::uint32_t(offsetof(logic::light_fades::LightDatum,definition));return 0xc3324u;
        case definition:this->push(0xc3327u,gp[esi]);this->push(0xc332eu,gp[edi]);return 0xc3332u;
        case overall_radius:return 0xc3336u;
        case overall_class:return 0xc3339u;
        case diffuse_radius:return 0xc3350u;
        case diffuse_range:return 0xc3353u;
        case overall_store:return 0xc335eu;
        case diffuse_class:return 0xc3366u;
        case specular_radius:return 0xc337au;
        case specular_range:return 0xc337du;
        case diffuse_store:return 0xc3385u;
        case specular_class:return 0xc338du;
        case secondary_radius:return 0xc33a1u;
        case secondary_range:return 0xc33a4u;
        case specular_store:return 0xc33acu;
        case secondary_class:return 0xc33b4u;
        case volume_radius:return 0xc33c8u;
        case volume_range:return 0xc33cbu;
        case volume_range_scale:return 0xc33d0u;
        case secondary_store:return 0xc33e8u;
        case volume_store:gp[edi]=this->pop(0xc33f8u);return 0xc33f9u;
        case diffuse_limit:return 0xc3404u;
        case diffuse_reload:return 0xc3411u;
        case diffuse_copy:return 0xc341au;
        case secondary_limit:return 0xc3420u;
        }
        return 0;
    }
    void loaded(LightAccess,std::uint32_t,std::uint32_t){}
    // POP ESI follows the volume fade's store.
    void stored(LightAccess a,std::uint32_t,std::uint32_t) {
        if(a==LightAccess::volume_store)this->gp[esi]=this->pop(0xc3401u);
    }

    // C2A80 accesses.
    std::uint32_t instruction(FadeAccess a,std::uint32_t) {
        using enum FadeAccess;
        switch(a) {
        case position_x:return 0xc2a80u;
        case minimum:return 0xc2a82u;
        case camera_x:return 0xc2a8du;
        case position_y:return 0xc2a93u;
        case camera_y:return 0xc2a99u;
        case position_z:return 0xc2a9fu;
        case camera_z:return 0xc2aa6u;
        case class_distance:return 0xc2ac8u;
        case one:return 0xc2ae7u;
        }
        return 0;
    }
    void loaded(FadeAccess,std::uint32_t,std::uint32_t){}
    void stored(FadeAccess,std::uint32_t,std::uint32_t){}

    // Frame: C35D0.
    // PUSH EDI; XOR EDX,EDX; MOV ECX,EBP; CALL 16EA60. (Its OR EDI,-1 is
    // replaced by the first handle or popped before anything sees it.)
    std::uint32_t first_in_use(logic::light_fades::LightWalk &walk) {
        auto &gp=this->gp;
        this->push(0xc35d7u,gp[edi]);
        gp[edx]=this->flags.logic(0,4);gp[ecx]=gp[ebp];
        this->call(0xc35dcu,0xc35e1u,0x16ea60u,[&](State &v){iterate(v);});
        walk.lights=gp[ebp];
        this->reached(gp[eax]==logic::none?lv_empty:lv_first);
        return gp[eax];
    }
    // The search's end: OR EAX,-1; ESI = the found index.
    void searched(const logic::Search &found) {
        auto &gp=this->gp;
        if(!found.end){this->reached(lv_scan_found);this->reached(lv_next_handle);gp[esi]=found.index;}
        else {
            switch(*found.end) {
            case native_object_update::SearchEnd::negative_start:this->reached(lv_scan_negative);break;
            case native_object_update::SearchEnd::past_limit:this->reached(lv_scan_bound);break;
            case native_object_update::SearchEnd::bits_exhausted:this->reached(lv_scan_end);break;
            case native_object_update::SearchEnd::invalid_index:this->reached(lv_scan_found);break;
            }
            this->reached(lv_next_none);
        }
        gp[eax]=logic::none;
    }
    // Frame: C3300. POP ECX (the local); RET. The walk continues with the EDI
    // and ESI that C3300 restored, and XMM0..XMM7 hold its scratch values.
    void light_faded(logic::light_fades::LightWalk &walk,const logic::light_fades::LightResidue &residue) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        gp[ecx]=this->pop(0xc3426u);
        this->ret(0xc3427u,0);
        const auto &v=residue.volume;
        x[0]=scalar(residue.secondary);x[1]=scalar(v.clamped);x[2]=scalar(v.range);x[3]=scalar(v.divisor);
        x[4]=scalar(v.minimum);x[5]=scalar(v.ratio);x[6]=scalar(residue.overall);x[7]=scalar(residue.diffuse);
        this->reached(residue.diffuse_limited?lt_hotter:lt_kept);
        this->reached(residue.secondary_limited?lt_raised:lt_clamped);
        walk.handle=gp[edi];walk.index=gp[esi];
    }
    // PUSH table; PUSH radius.
    void pass_fade_arguments(const logic::light_fades::FadeCall &call) {
        const auto &site=fade_sites[unsigned(call.fade)];
        this->push(site.table_push,call.distances);
        this->push(site.radius_push,std::bit_cast<std::uint32_t>(call.radius));
    }
    // The secondary fade in the local (the PUSH ECX slot, [ESP+10] here), and
    // read back after the volume fade (POP EDI/ESI have run: [ESP]).
    void keep_secondary(float value){this->store(0xc33e2u,this->gp[esp]+0x10u,4,std::bit_cast<std::uint32_t>(value));}
    float reload_secondary(){return std::bit_cast<float>(this->read(0xc340cu,this->gp[esp],4));}

    // Frame: C2A80.
    void enter_fade(const logic::light_fades::FadeCall &call) {
        const auto &site=fade_sites[unsigned(call.fade)];
        this->push(site.call,site.next);
    }
    std::uint32_t distance_table(){return this->read(0xc2aa2u,this->gp[esp]+8u,4);}
    float radius(){return std::bit_cast<float>(this->read(0xc2ac4u,this->gp[esp]+4u,4));}
    // FSTP m32 over the radius argument; C1 is the store's.
    void spill_distance(logic::x87::Single distance) {
        this->store(0xc2acbu,this->gp[esp]+4u,4,std::bit_cast<std::uint32_t>(distance.value));
        auto &status=this->s.fp.status;
        status=std::uint16_t((status&~0x200u)|(distance.rounded_up?0x200u:0u));
    }
    float reload_distance(){return std::bit_cast<float>(this->read(0xc2ad4u,this->gp[esp]+4u,4));}
    void return_from_fade(const logic::light_fades::FadeResult &result) {
        using logic::light_fades::FadeEnd;
        this->reached(result.range_divides?fd_wide:fd_narrow);
        if(result.end!=FadeEnd::inside)this->reached(result.range_divides?fd_wide_ratio:fd_narrow_ratio);
        switch(result.end) {
        case FadeEnd::inside:this->reached(fd_below);this->ret(0xc2af8u,8);break;
        case FadeEnd::beyond:this->reached(fd_over);this->ret(0xc2b19u,8);break;
        case FadeEnd::within:
            this->reached(fd_mid);this->reached(result.range_divides?fd_wide_rest:fd_narrow_rest);
            this->ret(0xc2b2cu,8);break;
        }
    }

    void run() {
        auto &gp=this->gp;
        Memory<LightFades> memory{{*this}};
        logic::light_fades::run(memory,*this);
        // CMP EDI,-1 or CMP EAX,-1, both NONE here.
        this->compare32(logic::none,logic::none);
        if(walking) {
            this->reached(lv_last);
            gp[esi]=this->pop(0xc3690u);gp[ebx]=this->pop(0xc3691u);
        } else this->reached(lv_none);
        gp[edi]=this->pop(0xc3692u);gp[ebp]=this->pop(0xc3693u);
        this->ret(0xc3694u,0);this->publish(0xc3694u);
    }
};

// ---- 176A40 particle inputs -------------------------------------------------------
// [ESP+4] the mask, +8 the emitter, +C the location, +10 the particle, +14
// the outputs. The kept values live in EBX (the pending mask, also in the mask
// slot), EBP (the outputs), EDI (the input, also in the outputs slot) and ESI
// (the particle); only the 4EFA0 call observes them (they are restored at
// exit), so they are loaded there from the logic's Kept and read back after
// it. TEST EBX,EBX decides the exit flags; RET 14.
// 252100: EAX = the input, ECX/EDX/ESI = the emitter, location and particle;
// a 1C-byte frame ([ESP] the truncation spill, +4 the vector, +10 the left
// axis); XMM0 = the value, XMM1 = 0. 4EFA0 is entered with ECX = the vector
// and TEST EDX,EDX's flags.
template<class State,class Normalize> struct ParticleInputs : Port<State> {
    using Access=logic::particle_inputs::Access;
    using Reader=logic::particle_inputs::Reader;
    Normalize &child;  // 4EFA0
    bool looping{};
    Reader current{};
    ParticleInputs(State &s,std::uint32_t *c,Trace<State> *t,Normalize &n):Port<State>(s,c,t,{}),child(n){}

    bool animation()const{return current==Reader::explosion_animation;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        const bool forward=animation();  // the explosion animation's copy of an instruction
        switch(a) {
        case particle_age:return 0x252121u;
        case particle_emit_time:return 0x252132u;
        case random_word:return current==Reader::particle_random_1_2?0x252143u:0x252160u;
        case word_scale:return current==Reader::particle_random_1_2?0x25214cu:0x252169u;
        case emitter_age:return 0x25217du;
        case emitter_random:return 0x25218eu;
        case system_lod:return 0x2521a0u;
        case game_time:return 0x2521a9u;
        case elapsed_ticks:return 0x2521aeu;
        case seconds_per_tick:return 0x2521b3u;
        case effect:return current==Reader::effect_a_scale?0x2521bcu:0x2521e2u;
        case effect_table:return current==Reader::effect_a_scale?0x2521c4u:0x2521eau;
        case effect_data:return current==Reader::effect_a_scale?0x2521cau:0x2521f0u;
        case effect_scale:return current==Reader::effect_a_scale?0x2521d8u:0x2521feu;
        case rotation:return 0x252217u;
        case rotation_spill:return 0x25221cu;
        case rotation_reload:return 0x252221u;
        case rotation_again:return 0x25222au;
        case position_x:return forward?0x25224cu:0x252306u;
        case camera_x:return forward?0x252251u:0x25230bu;
        case offset_x:return forward?0x252259u:0x25232bu;
        case position_y:return forward?0x25225fu:0x252331u;
        case camera_y:return forward?0x252264u:0x252336u;
        case offset_y:return forward?0x25226cu:0x25233eu;
        case position_z:return forward?0x252272u:0x252344u;
        case camera_z:return forward?0x252277u:0x252349u;
        case offset_z:return forward?0x252283u:0x252351u;
        case up_y:return 0x252313u;
        case forward_z:return forward?0x252294u:0x25231bu;
        case forward_y:return forward?0x2522a2u:0x252323u;
        case up_z:return 0x252357u;
        case left_x:return 0x252379u;
        case up_x:return 0x252371u;
        case forward_x:return forward?0x2522aeu:0x25237fu;
        case left_y:return 0x2523a3u;
        case left_z:return 0x2523a9u;
        case normalized_z:return forward?0x25228eu:0x2523bau;
        case normalized_y:return forward?0x25229cu:0x2523c6u;
        case normalized_x:return forward?0x2522b6u:0x2523d6u;
        case left_z_again:return 0x2523b4u;
        case left_y_again:return 0x2523c0u;
        case left_x_again:return 0x2523d0u;
        case projection_rotation:return forward?0x2522c0u:0x2523e0u;
        case projection_spill:return forward?0x2522c5u:0x2523e8u;
        case projection_reload:return forward?0x2522cau:0x2523edu;
        case location_random_byte:return 0x252419u;
        case byte_scale:return 0x252421u;
        case one:return forward?0x2522eau:0x252409u;
        case output:return 0x176a93u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOVZX EAX (random 1, 2) or ECX (random 3, 4).
        case random_word:gp[current==Reader::particle_random_1_2?eax:ecx]=v;break;
        case game_time:case effect:gp[eax]=v;break;
        case effect_table:case location_random_byte:gp[edx]=v;break;
        // MOV ECX; AND EAX,FFFF; IMUL EAX,190.
        case effect_data:
            gp[ecx]=v;gp[eax]=(gp[eax]&0xffffu)*std::uint32_t(sizeof(logic::particle_inputs::EffectDatum));break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}

    // Frame: 176A40's arguments and slots (four saves deep inside the loop).
    std::uint32_t mask() {
        auto &gp=this->gp;
        this->push(0x176a40u,gp[ebx]);gp[ebx]=this->read(0x176a41u,gp[esp]+8u,4);
        if(!gp[ebx])this->reached(sa_empty);
        return gp[ebx];
    }
    std::uint32_t outputs() {
        this->push(0x176a47u,this->gp[ebp]);
        return this->read(0x176a48u,this->gp[esp]+0x1cu,4);
    }
    void clear_input(){this->store(0x176a4cu,this->gp[esp]+0x1cu,4,0);}
    // PUSH ESI/EDI before the first input; BSF ECX,[mask slot].
    std::uint32_t pending() {
        auto &gp=this->gp;
        if(!looping){this->push(0x176a56u,gp[esi]);this->push(0x176a57u,gp[edi]);looping=true;}
        this->reached(sa_value);
        return this->read(0x176a60u,gp[esp]+0x14u,4);
    }
    std::uint32_t unscanned_input(){this->reached(sa_zero_scan);return this->gp[ecx];}
    void keep_input(std::uint32_t input){this->store(0x176a65u,this->gp[esp]+0x24u,4,input);}
    std::uint32_t input(){return this->read(0x176a69u,this->gp[esp]+0x24u,4);}
    std::uint32_t particle(){return this->read(0x176a6du,this->gp[esp]+0x20u,4);}
    std::uint32_t location(){auto &gp=this->gp;gp[edx]=this->read(0x176a71u,gp[esp]+0x1cu,4);return gp[edx];}
    std::uint32_t emitter(){auto &gp=this->gp;gp[ecx]=this->read(0x176a7eu,gp[esp]+0x18u,4);return gp[ecx];}
    // EBX = the pending mask; MOV EAX,EDI (the input); the mask slot.
    void keep_pending(const logic::particle_inputs::Kept &kept) {
        auto &gp=this->gp;
        gp[ebx]=kept.remaining;gp[eax]=kept.input;
        this->store(0x176a88u,gp[esp]+0x14u,4,kept.remaining);
    }

    // Frame: 252100. CALL; XORPS XMM1; SUB ESP,1C; MOVAPS XMM0,XMM1.
    void enter_value() {
        auto &gp=this->gp;auto &x=this->s.xmm;
        this->push(0x176a8cu,0x176a91u);
        x[1]=_mm_setzero_ps();gp[esp]-=0x1cu;x[0]=x[1];
    }
    // The jump table at 252430.
    Reader reader(std::uint32_t input) {
        switch(this->read(0x252112u,0x252430u+input*4u,4)) {
        case 0x252119u:current=Reader::particle_age;break;
        case 0x25212au:current=Reader::particle_emit_time;break;
        case 0x25213bu:current=Reader::particle_random_1_2;break;
        case 0x252158u:current=Reader::particle_random_3_4;break;
        case 0x252175u:current=Reader::emitter_age;break;
        case 0x252186u:current=Reader::emitter_random;break;
        case 0x252198u:current=Reader::system_lod;break;
        case 0x2521a9u:current=Reader::game_time;break;
        case 0x2521bcu:current=Reader::effect_a_scale;break;
        case 0x2521e2u:current=Reader::effect_b_scale;break;
        case 0x25220fu:current=Reader::particle_rotation;break;
        case 0x25223cu:current=Reader::explosion_animation;break;
        case 0x2522f6u:current=Reader::explosion_rotation;break;
        case 0x252415u:current=Reader::location_random;break;
        default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");
        }
        return current;
    }
    std::uint32_t local_scratch(){return this->gp[esp];}
    std::uint32_t local_vector(){return this->gp[esp]+4u;}
    std::uint32_t local_left(){return this->gp[esp]+0x10u;}
    // 4EFA0 with ECX = the vector (LEA ECX,[ESP+4]), the kept values in
    // their registers and the SSE scratch values the original has in
    // XMM0..XMM5 there; TEST EDX,EDX (the location) decides the flags. The
    // kept values come back from it.
    void call_normalize(std::uint32_t vector,logic::particle_inputs::Kept &kept) {
        auto &gp=this->gp;
        gp[ebx]=kept.remaining;gp[ebp]=kept.outputs;gp[edi]=kept.input;gp[esi]=kept.particle;
        gp[ecx]=vector;this->test32(gp[edx]);
        const auto site=animation()?0x252289u:0x2523afu;
        this->push(site,animation()?0x25228eu:0x2523b4u);
        this->enter(0x4efa0u,[&](State &v){child(v,site);});
        kept.remaining=gp[ebx];kept.outputs=gp[ebp];kept.input=gp[edi];kept.particle=gp[esi];
    }
    void normalize(std::uint32_t vector,logic::particle_inputs::Kept &kept,const logic::particle_inputs::AnimationScratch &scratch) {
        this->s.xmm[0]=scalar(scratch.offset_z);
        call_normalize(vector,kept);
    }
    void normalize(std::uint32_t vector,logic::particle_inputs::Kept &kept,const logic::particle_inputs::RotationScratch &scratch) {
        auto &x=this->s.xmm;
        x[0]=scalar(scratch.left_y);x[1]=scalar(scratch.left_z);x[2]=scalar(scratch.up_y_forward_x);
        x[3]=scalar(scratch.forward_z_up_x);x[4]=scalar(scratch.forward_x);x[5]=scalar(scratch.up_x);
        call_normalize(vector,kept);
    }
    // ADD ESP,1C; RET, at the input's own return. EAX = the truncated
    // fraction; XMM0 = the value, XMM1 = 0; the rotation's CVTSI2SS XMM2.
    void return_from_value(const logic::particle_inputs::InputResult &result) {
        using logic::particle_inputs::Exit;
        auto &gp=this->gp;auto &x=this->s.xmm;
        if(result.truncated)gp[eax]=std::uint32_t(*result.truncated);
        x[0]=scalar(result.value);x[1]=_mm_setzero_ps();
        if(result.converted)x[2]=replace_low(x[2],*result.converted);
        gp[esp]+=0x1cu;
        std::uint32_t exit=0x25242cu;
        if(!result.reader)this->reached(sv_out_of_range);
        else if(result.exit==Exit::no_record)this->reached(sv_null);
        else {
            switch(*result.reader) {
            case Reader::particle_age:this->reached(sv_age);exit=0x252129u;break;
            case Reader::particle_emit_time:this->reached(sv_lifetime);exit=0x25213au;break;
            case Reader::particle_random_1_2:this->reached(sv_word);exit=0x252157u;break;
            case Reader::particle_random_3_4:this->reached(sv_word_low);exit=0x252174u;break;
            case Reader::emitter_age:this->reached(sv_system);exit=0x252185u;break;
            case Reader::emitter_random:this->reached(sv_system_input);exit=0x252197u;break;
            case Reader::system_lod:this->reached(sv_location);exit=0x2521a8u;break;
            case Reader::game_time:this->reached(sv_game_time);exit=0x2521bbu;break;
            case Reader::effect_a_scale:case Reader::effect_b_scale: {
                const bool first=*result.reader==Reader::effect_a_scale;
                if(result.exit==Exit::no_effect){this->reached(sv_no_owner);exit=0x25220eu;}
                else {this->reached(first?sv_owner_first:sv_owner_second);exit=first?0x2521e1u:0x252207u;}
                break;
            }
            case Reader::particle_rotation:this->reached(sv_fraction);break;
            case Reader::explosion_animation:this->reached(sv_projection);break;
            case Reader::explosion_rotation:this->reached(sv_cross_projection);break;
            case Reader::location_random:this->reached(sv_byte);break;
            }
            if(result.exit==Exit::wrapped) {
                this->reached(sv_wrapped);exit=*result.reader==Reader::explosion_animation?0x2522f5u:0x252414u;
            } else if(result.exit==Exit::unwrapped)this->reached(sv_unwrapped);
        }
        this->ret(exit,0);
    }

    void run() {
        auto &gp=this->gp;
        Memory<ParticleInputs> memory{{*this}};
        const bool looped=logic::particle_inputs::run(memory,*this);
        this->test32(gp[ebx]);
        if(looped){gp[edi]=this->pop(0x176a9bu);gp[esi]=this->pop(0x176a9cu);}
        gp[ebp]=this->pop(0x176a9du);gp[ebx]=this->pop(0x176a9eu);
        this->ret(0x176a9fu,0x14u);this->publish(0x176a9fu);
    }
};

// ---- Admission and binding --------------------------------------------------
template<class State,class Reference> bool bridge_20680(State &s,Reference reference) {
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x20680u,0x206ffu,cursor,owner_reference(0x20680u)))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls[0],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return LensQueryReset<State>(v,c,t);});
    return true;
}
// C2A80 needs five free x87 slots, x87 (not MMX) ownership and masked
// exceptions; its SSE operations need masked MXCSR exceptions.
template<class State,class Reference,class Iterate> bool bridge_c35d0(State &s,Reference reference,Iterate iterate) {
    if(!backend || !masked_controls() || s.fp.count>3u)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xc2a80u,0xc3694u,cursor,owner_reference(0xc35d0u)))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls[1],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return LightFades<State,Iterate>(v,c,t,iterate);});
    return true;
}
// 252100's SSE needs masked MXCSR exceptions; neither body holds a live
// service PC (0x21F590 lies between them, outside both).
template<class State,class Reference,class Normalize> bool bridge_176a40(State &s,Reference reference,Normalize normalize) {
    if(!backend || (_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x176a40u,0x176a9fu,cursor,owner_reference(0x176a40u)))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls[2],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ParticleInputs<State,Normalize>(v,c,t,normalize);});
    return true;
}
} // namespace halo2::native_effect_owner_detail
