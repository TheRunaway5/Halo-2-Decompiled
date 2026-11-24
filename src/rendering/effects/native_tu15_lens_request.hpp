#pragma once
// Boundary adapter for the lens request, entered at 2DB80
// (native_tu15_lens_request_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ECX the lens, EDI the forward,
// nine arguments on the stack (RET 24); EBX, EBP and ESI saved; 2E210 gets
// EAX the position, ESI the projected lens and color, brightness, opacity
// and scale pushed (RET 10), 20990 EAX the request and the projected lens
// and its size pushed (RET 8), 20B20 ECX the first queued lens and EAX the
// second, every register each returns coming back; EAX the answer over
// what the original leaves, ECX and EDX likewise; XMM0..3 as its arithmetic
// leaves them; the flags of the closing ADD ESP,2C. Each access publishes
// its original instruction.
// HALO2_REFERENCE_LENS_REQUEST=1 restores the translated body.
// HALO2_VERIFY_LENS_REQUEST=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/effects/native_tu15_lens_request_systems.hpp"

namespace halo2::native_lens_request_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_lens_request;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_LENS_REQUEST");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_LENS_REQUEST");return p && *p;}();
    return value;
}
enum Branch : unsigned {unbound, dark, needs_opacity, excluded, out_of_range, refused, queued, reflected, branch_count};
static_assert(unsigned(logic::lens_request::Outcome::reflected)==Branch::reflected);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Lens request verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t request_clears[]{0x2dc67u,0x2dc6bu,0x2dc6fu,0x2dc73u,0x2dc77u};
template<class State,class Project,class Queue,class Link> struct LensRequest : Port<State> {
    using Access=logic::lens_request::Access;
    using Kept=logic::lens_request::Kept;
    Project &project_child;
    Queue &queue_child;
    Link &link_child;
    std::uint32_t depth{},clears{};   // the bytes below the entry the adapter has pushed
    bool saved{};
    LensRequest(State &s,std::uint32_t *c,Trace<State> *t,Project &p,Queue &q,Link &l)
        :Port<State>(s,c,t,{}),project_child(p),queue_child(q),link_child(l){}
    void put(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u*k;}
    // The request lies 0xC above the frame's saves.
    std::uint32_t request(std::uint32_t offset)const{return this->gp[esp]+(depth-0x38u)+0xcu+offset;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[esi],this->gp[esi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case view:return 0x2db83u;
        case position_argument:put(0x2db8cu,gp[ebp]);return 0x2db8du;                             // PUSH EBP
        case result_clear:return 0x2db91u;
        case brightness:return 0x2dba5u;
        case opacity:return 0x2dbb7u;
        // PUSH EBX; MOV EAX,ECX; AND EAX,FFFF; PUSH ESI.
        case tags:put(0x2dbbdu,gp[ebx]);gp[eax]=gp[ecx]&0xffffu;put(0x2dbc5u,gp[esi]);saved=true;return 0x2dbc6u;
        case definition:return 0x2dbd2u;
        case definition_flags:return 0x2dbd8u;
        case kind:return 0x2dbe2u;
        case kind_flags:return 0x2dbe8u;
        case range_begin:return 0x2dbf9u;
        case range_end:return 0x2dbfeu;
        case position_x:return 0x2dc02u;
        case position_y:return 0x2dc07u;
        case position_z:return 0x2dc0cu;
        case camera_x:return 0x2dc11u;
        case camera_y:return 0x2dc19u;
        case camera_z:return 0x2dc21u;
        case forward_y:return 0x2dc2fu;
        case forward_z:return 0x2dc37u;
        case forward_x:return 0x2dc43u;
        case range_end_again:return 0x2dc4fu;
        case request_clear:if(!clears)gp[eax]=fl.logic(0u,4);return request_clears[clears++];       // XOR EAX,EAX
        case kind_again:return 0x2dc7bu;
        case request_view:return 0x2dc7fu;
        case category:return 0x2dc83u;
        case request_kind:return 0x2dc87u;
        case owner:return 0x2dc8bu;
        case request_category:return 0x2dc8fu;
        case ordinal:return 0x2dc93u;
        case request_owner:return 0x2dc97u;
        case scale:return 0x2dc9bu;
        case brightness_again:put(0x2dc9fu,gp[eax]);return 0x2dca0u;                              // PUSH EAX (the scale)
        case request_ordinal:return 0x2dca4u;
        case opacity_again:return 0x2dca8u;
        case color:put(0x2dcacu,gp[edx]);return 0x2dcadu;                                         // PUSH EDX (the opacity)
        case reflections:gp[eax]=fl.logic(0xffffffffu,4);return 0x2dcd6u;                          // OR EAX,-1
        // PUSH 18; LEA ECX,[ESP+24]; PUSH ECX; LEA EAX,[ESP+14].
        case Access::reflected_category:
            put(0x2dcddu,0x18u);gp[ecx]=gp[esp]+0x24u;put(0x2dce3u,gp[ecx]);gp[eax]=gp[esp]+0x14u;
            return 0x2dce8u;
        case result_set:return 0x2dcfcu;
        case result:
            if(saved){gp[esi]=this->pop(0x2dd01u);gp[ebx]=this->pop(0x2dd02u);depth-=8u;}
            return 0x2dd03u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case view:gp[edx]=v;break;
        case position_argument:gp[ebp]=v;break;
        case brightness:x[1]=lane0_only(v);x[0]=_mm_setzero_ps();this->flags.scalar_compare(x[1],x[0],false);break;
        case opacity:x[1]=lane0_only(v);this->flags.scalar_compare(x[1],x[0],false);break;
        case tags:gp[esi]=v;gp[eax]<<=4;break;                                                     // SHL EAX,4
        case definition:gp[ebx]=v;break;
        case kind:case kind_flags:case kind_again:set_low_byte(gp[eax],v);break;
        case range_begin:x[3]=lane0_only(v);break;
        case range_end:this->flags.scalar_compare(x[3],lane0_only(v),true);break;                  // UCOMISS XMM3,[EBX+1C]
        case position_x:x[1]=lane0_only(v);break;
        case position_y:x[2]=lane0_only(v);break;
        case position_z:x[0]=lane0_only(v);break;
        case forward_y:x[3]=lane0_only(v);break;
        case range_end_again:x[1]=lane0_only(v);break;
        case category:case ordinal:case opacity_again:case color:gp[edx]=v;break;
        case owner:case scale:case brightness_again:gp[eax]=v;break;
        case reflections:this->flags.minus(v,0u,2);break;                                           // CMP word [EBX+16],0
        case result:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // SUBSS ×3; LAHF; TEST AH,44.
    void offset(std::uint32_t dx,std::uint32_t dy,std::uint32_t dz) {
        auto &x=this->s.xmm;
        x[1]=lane0_only(dx);x[2]=lane0_only(dy);x[0]=lane0_only(dz);
        const auto ah=(this->flags.bits()|2u)&0xffu;
        this->gp[eax]=(this->gp[eax]&0xffff00ffu)|(ah<<8);this->test8(ah&0x44u);
    }
    // MULSS, MULSS, MULSS, ADDSS, ADDSS; COMISS XMM1,XMM0.
    void measured(std::uint32_t,std::uint32_t y_term,std::uint32_t x_term,std::uint32_t along) {
        auto &x=this->s.xmm;
        x[3]=lane0_only(y_term);x[2]=lane0_only(x_term);x[0]=lane0_only(along);
        this->flags.scalar_compare(x[1],x[0],false);
    }
    // PUSH EAX (brightness); PUSH EDX (color); LEA ESI,[ESP+30]; MOV EAX,EBP; CALL 2E210.
    void project(Kept &kept,std::uint32_t position,std::uint32_t color,std::uint32_t brightness,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        put(0x2dcb1u,brightness);put(0x2dcb2u,color);
        gp[esi]=gp[esp]+0x30u;gp[eax]=position;
        this->call(0x2dcb9u,0x2dcbeu,0x2e210u,project_child);depth-=0x10u;
        refresh(kept);
    }
    // PUSH 18; MOV EAX,ESI; PUSH EAX; LEA EAX,[ESP+14]; CALL 20990; MOV ESI,EAX; CMP ESI,-1
    // (the reflection's pushes precede its category's store).
    std::uint32_t queue(Kept &kept,logic::lens_request::Queue which) {
        auto &gp=this->gp;
        if(which==logic::lens_request::Queue::first) {
            put(0x2dcbeu,0x18u);gp[eax]=gp[esi];put(0x2dcc2u,gp[eax]);gp[eax]=gp[esp]+0x14u;
            this->call(0x2dcc7u,0x2dcccu,0x20990u,queue_child);depth-=8u;
            gp[esi]=gp[eax];this->compare32(gp[esi],logic::none);
        } else {
            this->call(0x2dcf0u,0x2dcf5u,0x20990u,queue_child);depth-=8u;
        }
        refresh(kept);
        return gp[eax];
    }
    // MOV ECX,ESI; CALL 20B20.
    void link(Kept &kept,std::uint32_t first,std::uint32_t second) {
        auto &gp=this->gp;
        gp[ecx]=first;gp[eax]=second;
        this->call(0x2dcf7u,0x2dcfcu,0x20b20u,link_child);
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x2cu,4);depth=0x2cu;                                           // SUB ESP,2C
        Memory<LensRequest> memory{*this};
        logic::lens_request::Outcome outcome{};
        logic::lens_request::submit(memory,*this,gp[ecx],outcome);
        this->reached(unsigned(outcome));
        gp[ebp]=this->pop(0x2dd07u);
        gp[esp]=fl.plus(gp[esp],0x2cu,4);                                                         // ADD ESP,2C
        this->ret(0x2dd0bu,0x24u);this->publish(0x2dd0bu);
    }
};
template<class State,class Reference,class Project,class Queue,class Link>
bool bridge(State &s,Reference reference,Project project,Queue queue,Link link) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x2db80u,0x2dd0bu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return LensRequest<State,Project,Queue,Link>(v,c,t,project,queue,link);});
    return true;
}
} // namespace halo2::native_lens_request_detail
