#pragma once
// Boundary adapter for the effect follow, 17C370
// (native_tu15_effect_follow_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: the effect's handle on the
// stack, AL the answer (RET 4); ECX pushed for the tag's definition, then
// EBX, EBP, ESI and EDI, read from the ESP each child left. Every child goes
// through the unit's dispatch, which binds it in either unit: BCDF0 (one
// pushed, RET 4), 17A380, BCFB0, 17C330 (one pushed, RET 4), BCB70 (three
// pushed, RET C), BCD80, 17A0F0 and 17A1A0 (one pushed, RET 4), 17A4A0 and
// 3DDC0 (one pushed, RET 4); every register each returns comes back. Each
// exit leaves through the original's own epilogue. Each access publishes
// its original instruction.
// HALO2_REFERENCE_EFFECT_FOLLOW=1 restores the translated body.
// HALO2_VERIFY_EFFECT_FOLLOW=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_effect_follow_systems.hpp"

namespace halo2::native_effect_follow_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_effect_follow;
using Access=logic::effect_follow::Access;
using Exit=logic::effect_follow::Exit;
using Kept=logic::effect_follow::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_EFFECT_FOLLOW");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_EFFECT_FOLLOW");return v;}
enum Branch : unsigned {removed, located, unlocated, markers, unmarked, node, default_offset, restart, stop, offered, retired, lit,
    light, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Effect follow"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline std::uint32_t low_word(std::uint32_t reg,std::uint32_t v){return (reg&0xffff0000u)|(v&0xffffu);}
inline constexpr std::uint32_t sites[]{0x17c371u,0x17c377u,0x17c37bu,0x17c383u,0x17c395u,0x17c3a4u,0x17c3a8u,0x17c3acu,0x17c3d8u,
    0x17c3e8u,0x17c3eeu,0x17c3fcu,0x17c400u,0x17c411u,0x17c416u,0x17c418u,0x17c41bu,0x17c41eu,0x17c421u,0x17c424u,0x17c427u,
    0x17c42au,0x17c42du,0x17c432u,0x17c439u,0x17c43cu,0x17c440u,0x17c446u,0x17c45fu,0x17c46cu,0x17c478u,0x17c48bu,0x17c48eu,
    0x17c49cu,0x17c4a8u,0x17c4b3u,0x17c4d8u,0x17c4e0u,0x17c4e5u,0x17c4ecu,0x17c4f2u,0x17c4f7u,0x17c4f9u,0x17c4fcu,0x17c4ffu,
    0x17c502u,0x17c4bbu,0x17c4bfu,0x17c4c4u,0x17c507u,0x17c53eu,0x17c542u,0x17c555u,0x17c577u,0x17c57du,0x17c588u,0x17c592u,
    0x17c595u,0x17c59bu,0x17c59eu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::light_second_store)+1u);
// Each exit's pops (EDI, ESI, EBP, EBX, ECX) and its RET.
struct Epilogue {std::uint32_t edi,esi,ebp,ebx,ret;};
inline constexpr Epilogue epilogues[]{{0x17c3c7u,0x17c3ccu,0x17c3cfu,0x17c3d3u,0x17c3d5u},{0x17c527u,0x17c52cu,0x17c52fu,0x17c533u,0x17c535u},
    {0x17c5a4u,0x17c5a7u,0x17c5aau,0x17c5aeu,0x17c5b0u}};
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};   // bytes pushed beyond ECX, EBX, EBP, ESI and EDI (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void routed(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,pc+5u,target,child);depth-=popped;
        auto &gp=this->gp;
        kept.placed=gp[ebx];kept.effect_address=gp[ebp];kept.object=gp[esi];kept.handle=gp[edi];
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case effects_load:case definition_load:case owner_load:case first_marker_load:case first_marker_owner:case second_marker_owner:
        case node_owner:case default_x:case default_y:case definition_reload:case lights_definition:case lights_definition_flags:
            gp[eax]=v;break;
        case effects_data:case center_x:case center_y:gp[ebx]=v;break;
        case instances_load:case headers_load:case datums_load:case object_flags:case center_z:case object_position:
        case second_marker_load:case default_offset_load:case default_z:case lights_load:case light_second_load:gp[edx]=v;break;
        case argument_load:case handle_reload:gp[edi]=v;break;
        case tag_load:case owner_reload:case object_load:case object_location:case marker_owner:case light_first_load:gp[ecx]=v;break;
        case flags_load:gp[ecx]=v;break;                                                                    // XOR ECX,ECX; MOV CX,word
        case no_cluster_load:gp[edx]=low_word(gp[edx],v);break;                                            // MOV DX,word
        case node_load:case flags_word:gp[eax]=low_word(gp[eax],v);break;                                  // MOV AX,word
        case flags_byte:case light_active:set_low_byte(gp[ecx],v);break;                                   // MOV CL
        case light_owner:case lights_data:gp[esi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The effect and its tag.
    void reserving(){this->push(0x17c370u,this->gp[ecx]);depth=0u-16u;}                                   // PUSH ECX
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x17c376u,0x17c37au,0x17c381u,0x17c382u};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t effect_offset(std::uint32_t handle) {                                                  // MOV EBP,EDI; AND EBP,FFFF; IMUL EBP,EBP,190
        auto &fl=this->flags;
        return this->gp[ebp]=fl.product(fl.logic(handle&logic::handle_mask,4),logic::effect_size);
    }
    std::uint32_t effect_address(std::uint32_t offset,std::uint32_t data){return this->gp[ebp]=this->flags.plus(offset,data,4);}   // ADD EBP,EBX
    std::uint32_t tag_offset(std::uint32_t tag) {                                                       // AND ECX,FFFF; SHL ECX,4
        auto &fl=this->flags;
        return this->gp[ecx]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);
    }
    std::uint32_t no_owner(){return this->gp[esi]=this->flags.logic(logic::none,4);}                         // OR ESI,-1
    bool different(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a!=b;}                       // CMP
    bool verified(Kept &kept,std::uint32_t,bool light) {                                                 // PUSH ESI / PUSH -1; MOV EAX,ESI; CALL BCDF0; TEST EAX,EAX
        auto &gp=this->gp;
        if(light){put(0x17c558u,logic::any_type);gp[eax]=kept.object;routed(0x17c55cu,0xbcdf0u,4u,kept);}
        else{put(0x17c3b6u,kept.object);routed(0x17c3b7u,0xbcdf0u,4u,kept);}
        this->test32(gp[eax]);
        return gp[eax]!=0u;
    }
    void remove(Kept &kept){this->gp[eax]=kept.handle;routed(0x17c3c2u,0x17a380u,0u,kept);this->reached(Branch::removed);}   // MOV EAX,EDI; CALL 17A380
    std::uint32_t root(Kept &kept,bool light) {                                                          // (MOV ECX,ESI); CALL BCFB0
        if(light){this->gp[ecx]=kept.object;routed(0x17c567u,0xbcfb0u,0u,kept);}
        else routed(0x17c3e3u,0xbcfb0u,0u,kept);
        return this->gp[eax];
    }
    std::uint32_t root_index(std::uint32_t root_handle) {                                               // MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]
        return this->gp[ecx]=3u*this->flags.logic(root_handle&logic::handle_mask,4);
    }
    bool located(std::uint32_t flags_value) {                                                            // SHR EDX,8; TEST DL,1
        auto &gp=this->gp;
        gp[edx]=this->flags.right(flags_value,logic::object::located_bit,4,false);this->flags.logic(gp[edx]&1u,1);
        this->reached((gp[edx]&1u)?Branch::located:Branch::unlocated);
        return gp[edx]&1u;
    }
    void center_source(std::uint32_t object_address){this->gp[edx]=object_address+logic::object::center;}    // LEA EDX,[ECX+88]
    std::uint32_t center_target(std::uint32_t effect_address){return this->gp[esi]=effect_address+logic::effect::center;}   // LEA ESI,[EBP+30]
    // The markers and the node.
    std::uint32_t flags_word(std::uint32_t v){this->flags.logic(0u,4);return v;}                             // XOR ECX,ECX (MOV CX loaded)
    bool following(std::uint32_t flags_value) {                                                          // MOV DL,CL; SHR DL,1; TEST DL,1; SHR ECX,9; TEST CL,1
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[edx],fl.right(flags_value&0xffu,logic::effect::kept_bit,1,false));fl.logic(gp[edx]&1u,1);
        if(gp[edx]&1u)return true;
        gp[ecx]=fl.right(gp[ecx],logic::effect::persistent_bit,4,false);fl.logic(gp[ecx]&1u,1);
        return gp[ecx]&1u;
    }
    bool markers(Kept &kept) {                                                                           // PUSH ECX; CALL 17C330; TEST AL,AL
        auto &gp=this->gp;
        put(0x17c462u,gp[ecx]);routed(0x17c463u,0x17c330u,4u,kept);
        this->test8(gp[eax]);
        this->reached(low_byte(gp[eax])?Branch::markers:Branch::unmarked);
        return low_byte(gp[eax])!=0u;
    }
    bool named(std::uint32_t marker){this->test32(marker);return marker!=0u;}                                 // TEST EAX,EAX
    void first_output(std::uint32_t effect_address) {                                                   // LEA EDX,[EBP+6C]; PUSH EDX; PUSH EAX
        auto &gp=this->gp;
        gp[edx]=effect_address+logic::effect::first_out;put(0x17c476u,gp[edx]);put(0x17c477u,gp[eax]);
    }
    bool marked(Kept &kept,bool) {                                                                       // PUSH EAX; CALL BCB70; TEST AL,AL
        auto &gp=this->gp;
        put(0x17c47bu,gp[eax]);routed(0x17c47cu,0xbcb70u,12u,kept);
        this->test8(gp[eax]);
        return low_byte(gp[eax])!=0u;
    }
    std::uint32_t placing(bool placed) {                                                                 // MOV BL,1 / XOR BL,BL
        auto &gp=this->gp;
        set_low_byte(gp[ebx],placed?1u:this->flags.logic(0u,1));
        return gp[ebx];
    }
    void second_output(Kept &kept) {                                                                     // LEA ECX,[EBP+70]; PUSH ECX; PUSH EDX; PUSH EAX; CALL BCB70
        auto &gp=this->gp;
        gp[ecx]=kept.effect_address+logic::effect::second_out;put(0x17c494u,gp[ecx]);put(0x17c495u,gp[edx]);put(0x17c496u,gp[eax]);
        routed(0x17c497u,0xbcb70u,12u,kept);
    }
    bool noded(std::uint32_t node){this->flags.logic(node&0xffffu,2);return (node&0xffffu)!=0u;}               // TEST AX,AX
    void node_index(std::uint32_t){this->gp[edi]=sx16(this->gp[eax]);}                                       // MOVSX EDI,AX
    void place_node(Kept &kept) {                                                                        // LEA ESI,[EBP+24]; CALL BCD80
        this->gp[esi]=kept.effect_address+logic::effect::offset;
        routed(0x17c4aeu,0xbcd80u,0u,kept);
        this->reached(Branch::node);
    }
    bool placed_tested(std::uint32_t placed){this->test8(placed);return (placed&0xffu)!=0u;}                  // TEST BL,BL
    std::uint32_t cleared(std::uint32_t node) {                                                          // CMP word,0; XORPS XMM0,XMM0
        this->flags.minus(node,0u,2);this->s.xmm[0]=_mm_setzero_ps();
        if(node)this->reached(Branch::default_offset);
        return 0u;
    }
    void offset_target(std::uint32_t effect_address){this->gp[ecx]=effect_address+logic::effect::offset;}   // LEA ECX,[EBP+24]
    // Restart, stop or retire.
    bool tag_retire(std::uint32_t v){return this->flags.logic(v&(1u<<logic::definition::retire_bit),1)!=0u;}   // TEST byte,1
    bool restarted(std::uint32_t) {                                                                      // SHR CL,2; TEST CL,1
        auto &gp=this->gp;
        set_low_byte(gp[ecx],this->flags.right(gp[ecx]&0xffu,logic::effect::restarted_bit,1,false));this->flags.logic(gp[ecx]&1u,1);
        return gp[ecx]&1u;
    }
    void restart(Kept &kept){put(0x17c4cfu,0u);routed(0x17c4d1u,0x17a0f0u,4u,kept);this->reached(Branch::restart);}   // PUSH 0; CALL 17A0F0
    bool placed_restarted(std::uint32_t) {                                                               // MOV DL,AL; SHR DL,2; TEST DL,1
        auto &gp=this->gp;
        set_low_byte(gp[edx],this->flags.right(gp[eax]&0xffu,logic::effect::restarted_bit,1,false));this->flags.logic(gp[edx]&1u,1);
        return gp[edx]&1u;
    }
    bool placed_retire(std::uint32_t) {                                                                  // SHR AL,4; TEST AL,1
        auto &gp=this->gp;
        set_low_byte(gp[eax],this->flags.right(gp[eax]&0xffu,logic::effect::retire_bit,1,false));this->flags.logic(gp[eax]&1u,1);
        return gp[eax]&1u;
    }
    void stop(Kept &kept){put(0x17c538u,kept.handle);routed(0x17c539u,0x17a1a0u,4u,kept);this->reached(Branch::stop);}   // PUSH EDI; CALL 17A1A0
    bool retire(Kept &kept) {                                                                            // MOV EAX,EDI; CALL 17A4A0; TEST AL,AL
        auto &gp=this->gp;
        this->reached(Branch::offered);
        gp[eax]=kept.handle;routed(0x17c51eu,0x17a4a0u,0u,kept);
        this->test8(gp[eax]);
        if(low_byte(gp[eax]))this->reached(Branch::retired);
        return low_byte(gp[eax])!=0u;
    }
    // The light.
    bool lit(std::uint32_t flags_value) {                                                                // MOV ECX,EAX; SHR ECX,3; TEST CL,1; SHR EAX,4; TEST AL,1
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.right(flags_value,logic::definition::light_bit,4,false);fl.logic(gp[ecx]&1u,1);
        bool on=gp[ecx]&1u;
        if(!on){gp[eax]=fl.right(gp[eax],logic::definition::second_light_bit,4,false);fl.logic(gp[eax]&1u,1);on=gp[eax]&1u;}
        if(on)this->reached(Branch::lit);
        return on;
    }
    std::uint32_t light_index(Kept &kept,std::uint32_t) {                                               // PUSH EAX; CALL 3DDC0
        put(0x17c56cu,this->gp[eax]);routed(0x17c56du,0x3ddc0u,4u,kept);
        return this->gp[eax];
    }
    std::uint32_t light_offset(std::uint32_t index) {                                                   // AND EAX,FFFF; SHL EAX,8
        auto &fl=this->flags;
        return this->gp[eax]=fl.shift(fl.logic(index&logic::handle_mask,4),logic::light_shift,4);
    }
    std::uint32_t light_address(std::uint32_t offset,std::uint32_t data){return this->gp[eax]=this->flags.plus(offset,data,4);}   // ADD EAX,ESI
    bool active(std::uint32_t v){this->test8(v);if(v&0xffu)this->reached(Branch::light);return (v&0xffu)!=0u;}   // TEST CL,CL
    void finish(Exit exit) {
        auto &gp=this->gp;
        const auto &e=epilogues[unsigned(exit)];
        gp[edi]=this->pop(e.edi);
        if(exit!=Exit::updated)gp[ebp]=this->flags.logic(0u,4);                                           // XOR EBP,EBP
        gp[eax]=this->flags.logic(0u,4);gp[esi]=this->pop(e.esi);                                          // XOR EAX,EAX; POP ESI
        this->test32(gp[ebp]);gp[ebp]=this->pop(e.ebp);set_low_byte(gp[eax],this->flags.zf?1u:0u);         // TEST EBP,EBP; POP EBP; SETZ AL
        gp[ebx]=this->pop(e.ebx);gp[ecx]=this->pop(e.ebx+1u);
        this->ret(e.ret,4u);this->publish(e.ret);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::effect_follow::update(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]}));
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x17c370u,0x17c5b0u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_effect_follow_detail
