#pragma once
// Boundary adapter for the resource acquire, 1262F0
// (native_tu15_resource_acquire_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the resource, the
// argument on the stack (RET 4), the answer in EAX; EBX, ESI and EDI saved;
// 126190 gets EBX the resource; 16E8A0 EAX the request and EDX the pool;
// 125FB0 the resource and 1 pushed (RET 8); 16E770 EDX the pool; 1260C0 the
// argument and 0 pushed (RET 8); 1262B0 EDI the record; every register each
// returns coming back. Which children are direct and which go through the
// unit's dispatch differs by unit, so the bridge takes the unit's steps.
// Each access publishes its original instruction.
// HALO2_REFERENCE_RESOURCE_ACQUIRE=1 restores the translated body.
// HALO2_VERIFY_RESOURCE_ACQUIRE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/animation/native_tu15_resource_acquire_systems.hpp"

namespace halo2::native_resource_acquire_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_resource_acquire;
using Access=logic::resource_acquire::Access;
using Kept=logic::resource_acquire::Kept;
using Outcome=logic::resource_acquire::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RESOURCE_ACQUIRE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RESOURCE_ACQUIRE");return v;}
// The outcomes, then: a stale request released.
enum Branch : unsigned {loaded, touched, released, branch_count};
static_assert(unsigned(Outcome::touched)==Branch::touched);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Resource acquire"};return v;}
inline constexpr std::uint32_t sites[]{0x126301u,0x12630cu,0x12631eu,0x12632bu,0x12632eu,0x12633eu,0x126345u,0x126349u,0x12634du,
    0x126357u,0x12635du,0x126370u,0x126373u,0x126379u,0x12637cu,0x12637fu,0x12638au,0x12638eu,0x126391u,0x126394u,0x126397u,
    0x1263a3u,0x1263a9u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::base)+1u);
template<class State,class Steps> struct Adapter : Port<State> {
    Steps &steps;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Steps &st):Port<State>(s,c,t,{}),steps(st){}
    std::uint32_t argument()const{return this->gp[esp]+0x10u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case stamp_store:gp[eax]=3u*fl.logic(gp[eax]&0xffffu,4);break;                                // AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        case offset:gp[esi]=fl.logic(gp[esi]&0xffffu,4);gp[eax]=3u*gp[esi];break;                     // AND ESI,FFFF; LEA EAX,[ESI+ESI*2]
        default:break;
        }
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case requests:case records:gp[edx]=v;break;
        case block:gp[edi]=v;fl.minus(v,logic::none,4);break;                                        // CMP EDI,-1
        case record_data:gp[ebx]=v;break;
        case argument:                                                                               // AND EAX,FFFF; SHL EAX,4; ADD EAX,EBX
            gp[ecx]=v;gp[eax]=fl.plus(fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4),gp[ebx],4);break;
        case records_again:gp[edx]=v;break;
        case records_data:                                                                           // AND EDI,FFFF; SHL EDI,4; ADD EDI,EAX
            gp[eax]=v;gp[edi]=fl.plus(fl.shift(fl.logic(gp[edi]&0xffffu,4),4u,4),v,4);break;
        case block_again:gp[eax]=v;break;
        case cache:gp[ecx]=v;break;
        case blocks:case blocks_data:case blocks_again:case blocks_data_again:gp[edx]=v;break;
        case stamp:gp[edi]=v;break;
        case block_reload:gp[esi]=v;break;
        case shift:gp[ecx]=v;break;
        case offset:gp[eax]=v;break;                                                                  // (SHL EAX,CL: answer())
        case base:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void step(std::uint32_t pc,std::uint32_t target) {
        auto child=[&](State &v){steps(v,target,pc);};
        this->call(pc,pc+5u,target,child);
    }
    void refresh(Kept &kept){kept={this->gp[esi],this->gp[edi]};}
    // MOV EBX,ESI; CALL 126190; CMP EAX,-1.
    std::uint32_t find(Kept &kept) {
        auto &gp=this->gp;
        gp[ebx]=gp[esi];step(0x1262f7u,0x126190u);refresh(kept);
        this->flags.minus(gp[eax],logic::none,4);
        return gp[eax];
    }
    void release(std::uint32_t,Kept &kept){this->reached(Branch::released);step(0x126307u,0x16e8a0u);refresh(kept);}
    // PUSH 1; PUSH ESI; XOR EBX,EBX; CALL 125FB0.
    std::uint32_t choose(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x126314u,1u);this->push(0x126316u,gp[esi]);gp[ebx]=this->flags.logic(0u,4);
        step(0x126319u,0x125fb0u);refresh(kept);
        return gp[eax];
    }
    // MOV EDI,EAX; CALL 16E770.
    logic::resource_acquire::Created create(std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        gp[edi]=gp[eax];step(0x126326u,0x16e770u);refresh(kept);
        return {gp[eax],gp[edx]};
    }
    void pushing(std::uint32_t value){this->push(value?0x12634cu:0x12633cu,value);}
    void load_resource(Kept &kept){step(0x126350u,0x1260c0u);refresh(kept);}
    void touch(std::uint32_t,Kept &kept){step(0x12636bu,0x1262b0u);refresh(kept);}
    // SHL EAX,CL; POP EDI; POP ESI; ADD EAX,ECX.
    std::uint32_t answer(std::uint32_t shifted,std::uint32_t base) {
        auto &gp=this->gp;auto &fl=this->flags;
        fl.shift(gp[eax],gp[ecx]&0xffu,4);
        gp[edi]=this->pop(0x1263afu);gp[esi]=this->pop(0x1263b0u);
        gp[eax]=fl.plus(shifted,base,4);
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        this->push(0x1262f0u,gp[ebx]);this->push(0x1262f1u,gp[esi]);gp[esi]=gp[eax];this->push(0x1262f4u,gp[edi]);
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        logic::resource_acquire::address(memory,*this,gp[esi],outcome);
        this->reached(unsigned(outcome));
        gp[ebx]=this->pop(0x1263b3u);
        this->ret(0x1263b4u,4u);this->publish(0x1263b4u);
    }
};
template<class State,class Reference,class Steps> bool bridge(State &s,Reference reference,Steps steps) {
    return run_owner(s,0x1262f0u,0x1263b4u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Steps>(v,c,t,steps);});
}
} // namespace halo2::native_resource_acquire_detail
