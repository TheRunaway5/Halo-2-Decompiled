#pragma once
// Boundary adapter for the shadow caster collection, entered at 333F0
// (native_tu15_shadow_casters_systems.hpp). Its callers and children keep
// the original ABI, which the adapter owns:
//   - EBX the candidates; [ESP+4] the records, [ESP+8] the mode; RET 8; SUB
//     ESP,20 and EBP saved first, ESI/EDI once there are candidates; locals
//     and arguments are addressed from the live ESP, so a child that leaves
//     the stack unbalanced moves them as it does for the original;
//   - 3D7E0 gets the handle and the fade/shown outputs pushed, with the
//     flags of the mode test; 3E9B0 gets ESI the handle, XMM0 the floored
//     depth, XMM1 the size, XMM2 the radius and the flags of the depth
//     floor comparison; 4A7B0 gets the records and count pushed and EAX the
//     level's table; 4AC20 the table, the records, 4 and 0.0025; every
//     register a child returns comes back;
//   - the exit: EBP restored, ADD ESP,20 decides the flags;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_SHADOW_CASTERS=1 restores the translated body.
// HALO2_VERIFY_SHADOW_CASTERS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/shadows/native_tu15_shadow_casters_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_shadow_casters_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_shadow_casters;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SHADOW_CASTERS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SHADOW_CASTERS");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    quality_negative, quality_high, quality_within, no_candidates, candidates, every, mode_one_taken, mode_one_skipped,
    mode_two_taken, mode_two_skipped, other_mode, hidden, shown, full, room, near_floor, beyond_floor, next_candidate,
    last_candidate, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Shadow casters verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available;

inline constexpr Edge caster_edges[]{
    {0x333fdu,0x33403u,quality_negative},{0x3340cu,0x3341au,quality_high},{0x3340cu,0x33416u,quality_within},
    {0x33420u,0x33571u,no_candidates},{0x33420u,0x3342au,candidates},
    {0x33445u,0x3346fu,every},{0x33445u,0x33562u,other_mode},
    {0x33476u,0x33562u,hidden},{0x33476u,0x33483u,shown},
    {0x33483u,0x33562u,full},{0x33483u,0x3348eu,room},
    {0x33562u,0x33433u,next_candidate},{0x33562u,0x3356fu,last_candidate},
};
template<class State,class Fade,class Classify,class Arrange,class Finish> struct Casters : Port<State> {
    using Access=logic::shadow_casters::Access;
    using Local=logic::shadow_casters::Local;
    using Kept=logic::shadow_casters::Kept;
    Fade &fade_child;Classify &classify_child;Arrange &arrange_child;Finish &finish_child;
    std::uint32_t frame{};bool saved{};
    Casters(State &s,std::uint32_t *c,Trace<State> *t,Fade &f,Classify &k,Arrange &a,Finish &n)
        :Port<State>(s,c,t,caster_edges),fade_child(f),classify_child(k),arrange_child(a),finish_child(n){}
    std::uint32_t local(Local l)const {
        switch(l) {
        case Local::records:return frame+4u;
        case Local::mode:return frame+8u;
        case Local::shown:return frame-0x1du;
        case Local::count:return frame-0x1cu;
        case Local::level:return frame-0x18u;
        case Local::fade:return frame-0x14u;
        case Local::candidates:return frame-0x10u;
        case Local::center:return frame-0xcu;
        }
        return 0;
    }
    std::uint32_t entry()const{return this->gp[ebx];}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case quality:return 0x333f3u;
        case count_clear:return 0x333fdu;
        case level_zero:return 0x33403u;
        case level_three:this->compare32(gp[eax],logic::level_count);return 0x3340cu;               // CMP EAX,3
        case level_setting:return 0x33416u;
        case candidate_count_read:return 0x3341au;
        case candidate_count_store:return 0x33420u;
        case records_argument:saved=true;this->push(0x3342au,gp[esi]);this->push(0x3342bu,gp[edi]);return 0x3342cu;
        case handles_table:return 0x33433u;
        case flags_table:return 0x33436u;
        case handle_entry:gp[eax]=logic::widen16(gp[ebp]&0xffffu);return 0x3343cu;                 // MOVSX EAX,BP
        case mode_argument:return 0x3343fu;
        case flag_entry:return 0x33445u;
        case count_check:return 0x33483u;
        case fade_read:return 0x3348eu;
        // MOV EAX,ESI; AND EAX,FFFF; LEA EDX,[EAX+EAX*2].
        case headers:gp[eax]=this->flags.logic(gp[esi]&0xffffu,4);gp[edx]=gp[eax]*3u;return 0x3349eu;
        case rows:return 0x334a3u;
        case object_data:return 0x334a6u;
        case radius:return 0x334aau;
        case center_x:gp[edx]=gp[eax]+logic::object_center;return 0x334b2u;                       // LEA EDX,[EAX+30]
        case center_x_store:return 0x334b4u;
        case center_y:return 0x334b8u;
        case center_z:return 0x334bbu;
        case record_fade:return 0x334beu;
        case record_handle:return 0x334c3u;
        case record_handle_again:return 0x334c6u;
        case view_z:return 0x334c8u;
        case view_x:return 0x334d0u;
        case center_x_reload:return 0x334d8u;
        case center_z_store:return 0x334deu;
        case center_z_reload:return 0x334e2u;
        case view_y:return 0x334ecu;
        case center_y_store:return 0x334f4u;
        case center_y_reload:return 0x334f8u;
        case view_offset:return 0x33502u;
        case depth_minimum:return 0x33519u;
        case projection:return 0x33529u;
        case size_scale:return 0x33539u;
        case record_size:return 0x33541u;
        case count_reload:return 0x3354bu;
        case record_class:return 0x33550u;
        case record_pad2:return 0x33553u;
        case record_pad1:return 0x33557u;
        case count_store:return 0x3355bu;
        case candidates_reload:return 0x33562u;
        case level_read:
            if(saved){gp[edi]=this->pop(0x3356fu);gp[esi]=this->pop(0x33570u);}
            return 0x33571u;
        case count_read:return 0x33575u;
        case records_reload:return 0x3357fu;
        case records_again:return 0x33590u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOV EAX,[4C66F4]; PUSH EBP; XOR EBP,EBP; CMP EAX,EBP.
        case quality:
            gp[eax]=v;this->push(0x333f8u,gp[ebp]);gp[ebp]=this->flags.logic(0,4);this->compare32(v,0);break;
        case candidate_count_read:gp[eax]=logic::widen16(v);this->compare32(gp[eax],gp[ebp]);break;
        case records_argument:gp[edi]=this->flags.plus(v,8u,4);break;                              // ADD EDI,8
        case handles_table:case mode_argument:case rows:case center_x:case center_y:case count_read:
        case records_again:
            gp[ecx]=v;
            if(a==mode_argument)this->test32(v);
            if(a==count_read)gp[eax]=this->flags.product(gp[eax],logic::level_table_size);         // IMUL EAX,484
            break;
        case flags_table:case center_z:case records_reload:gp[edx]=v;break;
        case handle_entry:gp[esi]=v;break;
        // MOV AX,[EDX+EAX*2]; the mode's tests (TEST ECX,ECX was before).
        case flag_entry: {
            gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);
            const auto ah=(v>>8)&0xffu;
            if(gp[ecx]) {
                this->compare32(gp[ecx],1u);
                if(gp[ecx]==1u){this->test8(ah&0x40u);this->reached((ah&0x40u)?mode_one_taken:mode_one_skipped);}
                else {
                    this->compare32(gp[ecx],2u);
                    if(gp[ecx]==2u){this->test8(ah);this->reached((ah&0x80u)?mode_two_taken:mode_two_skipped);}
                }
            }
            break;
        }
        case count_check:this->compare32(v,logic::record_limit);break;
        case fade_read:this->s.xmm[0]=lane0_only(v);break;
        case headers:case object_data:case level_read:gp[eax]=v;break;
        case radius:this->s.xmm[2]=lane0_only(v);break;
        case count_reload:gp[ecx]=this->flags.increment(v,4,false);break;                         // INC ECX
        // MOV EAX,[candidates]; INC EBP; CMP EBP,EAX.
        case candidates_reload:gp[eax]=v;gp[ebp]=this->flags.increment(gp[ebp],4,false);this->compare32(gp[ebp],v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::count_store)this->gp[edi]=this->flags.plus(this->gp[edi],logic::record_stride,4);  // ADD EDI,14
    }
    void returned(Kept &kept,std::uint32_t below) {
        frame=this->gp[esp]+below;
        kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};
    }
    // LEA EAX (shown); PUSH EAX; LEA ECX (fade); PUSH ECX; PUSH ESI; CALL
    // 3D7E0; TEST AL,AL.
    std::uint32_t shadow_fade(Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=local(Local::shown);this->push(0x3346fu,gp[eax]);
        gp[ecx]=local(Local::fade);this->push(0x33474u,gp[ecx]);this->push(0x33475u,gp[esi]);
        this->call(0x33476u,0x3347bu,0x3d7e0u,fade_child);returned(kept,0x2cu);
        this->test8(gp[eax]);
        return gp[eax];
    }
    // XMM0 the floored depth, XMM1 the size, XMM2 the radius; the flags of
    // COMISS depth,minimum.
    void projected(const logic::shadow_casters::Projection &p) {
        auto &x=this->s.xmm;
        this->flags.scalar_compare(lane0_only(p.depth),lane0_only(p.minimum),false);
        this->reached(logic::sse::not_above(p.depth,p.minimum)?near_floor:beyond_floor);
        x[0]=lane0_only(p.floored_depth);x[1]=lane0_only(p.size);x[2]=lane0_only(p.radius);
    }
    std::uint32_t classify(Kept &kept) {
        this->call(0x33546u,0x3354bu,0x3e9b0u,classify_child);returned(kept,0x2cu);
        return this->gp[eax]&0xffu;
    }
    // PUSH ECX (count); LEA EAX,[EAX+5332C8]; PUSH EDX (records); CALL 4A7B0.
    void arrange(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x33583u,gp[ecx]);gp[eax]+=logic::globals::level_tables;this->push(0x3358au,gp[edx]);
        this->call(0x3358bu,0x33590u,0x4a7b0u,arrange_child);returned(kept,0x24u);
    }
    // PUSH 0.0025, 4, ECX (records), EAX; CALL 4AC20.
    void finish(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x33594u,logic::final_constant);this->push(0x33599u,logic::final_count);
        this->push(0x3359bu,gp[ecx]);this->push(0x3359cu,gp[eax]);
        this->call(0x3359du,0x335a2u,0x4ac20u,finish_child);returned(kept,0x24u);
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        gp[esp]=this->flags.minus(gp[esp],0x20u,4);                                                // SUB ESP,20
        Memory<Casters> memory{*this};
        logic::shadow_casters::collect(memory,*this);
        gp[ebp]=this->pop(0x335a2u);
        gp[esp]=this->flags.plus(gp[esp],0x20u,4);
        this->ret(0x335a6u,8u);this->publish(0x335a6u);
    }
};

template<class State,class Reference,class Fade,class Classify,class Arrange,class Finish>
bool bridge(State &s,Reference reference,Fade fade,Classify classify,Arrange arrange,Finish finish) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x333f0u,0x335a6u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Casters<State,Fade,Classify,Arrange,Finish>(v,c,t,fade,classify,arrange,finish);});
    return true;
}
} // namespace halo2::native_shadow_casters_detail
