#pragma once
// Boundary adapter for the tag group fixups, 11EF70
// (native_tu15_tag_fixups_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: no arguments; a 0x14-byte frame
// below the saved EDI (the iterator's state at ESP+4); 125790 gets EDI the
// iterator and answers EAX; 1CAF40 EDX the block, 21A580 the registers as
// they stand, 1F2C70 0 pushed (RET 4), 20D810 EAX the block, 11F060 and
// 165B30 nothing; every register each returns coming back. Each access
// publishes its original instruction; the group's CMP chain sets the flags
// the original's does.
// HALO2_REFERENCE_TAG_FIXUPS=1 restores the translated body.
// HALO2_VERIFY_TAG_FIXUPS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_tag_fixups_systems.hpp"

namespace halo2::native_tag_fixups_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_tag_fixups;
using Access=logic::tag_fixups::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_TAG_FIXUPS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_TAG_FIXUPS");return v;}
// The outcome, then each group's fixup.
enum Branch : unsigned {done, creature, biped, collision, physics, vehicle, other, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Tag fixups"};return v;}
inline constexpr std::uint32_t sites[]{0x11ef78u,0x11ef80u,0x11efa0u,0x11efacu,0x11efd8u,0x11eff1u,0x11f01au};
template<class State,class Steps> struct Adapter : Port<State> {
    Steps &steps;
    bool first{true};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Steps &st):Port<State>(s,c,t,{}),steps(st){}
    std::uint32_t iterator(std::uint32_t offset)const{return this->gp[esp]+4u+offset;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case group:gp[ecx]=fl.shift(logic::widen16(gp[eax]),4u,4);break;                              // MOVSX ECX,AX; SHL ECX,4
        case creature_data:case biped_data:case vehicle_data:gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);break;   // AND EAX,FFFF; SHL EAX,4
        default:break;
        }
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case instances:gp[edx]=v;break;
        case group: {                                                                                // the CMP chain
            namespace g=logic::group;
            gp[ecx]=v;fl.minus(v,g::creature,4);
            if(v>g::creature){fl.minus(v,g::physics,4);if(v!=g::physics)fl.minus(v,g::vehicle,4);}
            else if(v!=g::creature){fl.minus(v,g::biped,4);if(v!=g::biped)fl.minus(v,g::collision,4);}
            if(v!=g::creature && v!=g::biped && v!=g::collision && v!=g::physics && v!=g::vehicle)this->reached(Branch::other);
            break;
        }
        case creature_data:gp[edx]=fl.plus(v,logic::block::creature,4);break;                         // ADD EDX,D4
        case biped_data:gp[edx]=fl.plus(v,logic::block::biped,4);break;                               // ADD EDX,264
        case vehicle_data:gp[eax]=fl.plus(v,logic::block::vehicle,4);break;                           // ADD EAX,2AC
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void step(std::uint32_t pc,std::uint32_t target) {
        auto child=[&](State &v){steps(v,target);};
        this->call(pc,pc+5u,target,child);
    }
    // LEA EDI,[ESP+4]; CALL 125790; CMP EAX,-1.
    std::uint32_t next() {
        auto &gp=this->gp;
        gp[edi]=iterator(0u);
        step(first?0x11ef88u:0x11f035u,0x125790u);first=false;
        this->flags.minus(gp[eax],logic::none,4);
        return gp[eax];
    }
    void creature_block(std::uint32_t){this->reached(Branch::creature);step(0x11effbu,0x1caf40u);}
    void biped_block(std::uint32_t){this->reached(Branch::biped);step(0x11efe2u,0x1caf40u);}
    void collision(std::uint32_t){this->reached(Branch::collision);step(0x11efc9u,0x21a580u);}
    // PUSH 0; CALL 1F2C70 (through the unit's dispatch).
    void physics(std::uint32_t){this->reached(Branch::physics);this->push(0x11f02au,0u);step(0x11f02cu,0x1f2c70u);}
    void vehicle_block(std::uint32_t){this->reached(Branch::vehicle);step(0x11f023u,0x20d810u);}
    void postprocess(){step(0x11f043u,0x11f060u);step(0x11f048u,0x165b30u);}
    void run() {
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],0x14u,4);this->push(0x11ef73u,gp[edi]);gp[edi]=iterator(0u);   // SUB ESP,14; PUSH EDI; LEA EDI,[ESP+4]
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::tag_fixups::run(memory,*this)));
        gp[edi]=this->pop(0x11f04du);gp[esp]=this->flags.plus(gp[esp],0x14u,4);                      // POP EDI; ADD ESP,14
        this->ret(0x11f051u,0u);this->publish(0x11f051u);
    }
};
template<class State,class Reference,class Steps> bool bridge(State &s,Reference reference,Steps steps) {
    return run_owner(s,0x11ef70u,0x11f051u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Steps>(v,c,t,steps);});
}
} // namespace halo2::native_tag_fixups_detail
