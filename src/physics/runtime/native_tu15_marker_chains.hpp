#pragma once
// Boundary adapter for the marker chains, entered at 116060
// (native_tu15_marker_chains_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns:
//   - eight stack arguments, RET 20; SUB ESP,104 makes the frame (the model
//     at [ESP+10], the posed bits at [ESP+14], the pose at [ESP+34] and the
//     goal at [ESP+A4] once EBP, EBX, ESI and EDI are saved); the arguments
//     and locals are addressed from the live ESP, so a child that leaves the
//     stack unbalanced moves them as it does for the original;
//   - 1D4140 gets ECX the definition; 1E5250 and 1E51D0 ESI the records and
//     EBX the record tag; BAD60 EBX the pose or the goal, CL 0 and (object,
//     marker, 1); 1E1390 EAX the model, EBX the matrices, ESI the node and
//     (pose, goal offset, weight, poses); every register a child returns
//     comes back;
//   - XMM0 the weight where the original loads it; the flags of the last
//     instruction that sets them (ADD ESP,104 at the exit);
//   - the direction flag clear (a set one is refused at admission);
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_MARKER_CHAINS=1 restores the translated body.
// HALO2_VERIFY_MARKER_CHAINS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_marker_chains_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_marker_chains_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_marker_chains;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_MARKER_CHAINS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_MARKER_CHAINS");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_object, no_other, light_weight, no_tag, no_nodes, walked, keyed_walk, plain_walk, walk_end, no_other_marker,
    no_object_marker, no_pose, already_posed, no_goal, posed, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Marker chains verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge chains_edges[]{
    {0x11606eu,0x116271u,no_object},{0x11607bu,0x116271u,no_other},{0x116092u,0x116271u,light_weight},
    {0x1160bau,0x11626fu,no_tag},{0x1160c6u,0x11626fu,no_nodes},{0x1160c6u,0x1160dau,walked},
    {0x11615cu,0x116169u,keyed_walk},{0x11615cu,0x116187u,plain_walk},
    {0x116178u,0x11626eu,walk_end},{0x116196u,0x11626eu,walk_end},
    {0x1161a3u,0x116138u,no_other_marker},{0x1161afu,0x116138u,no_object_marker},
    {0x1161d0u,0x116138u,no_pose},{0x1161f2u,0x116138u,already_posed},{0x116213u,0x116138u,no_goal},
    {0x116267u,0x116138u,posed},
};
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available;
template<class State,class Count,class Keyed,class Plain,class Marker,class Chain> struct Chains : Port<State> {
    using Access=logic::marker_chains::Access;
    using Local=logic::marker_chains::Local;
    using Kept=logic::marker_chains::Kept;
    Count &count_child;Keyed &keyed_child;Plain &plain_child;Marker &marker_child;Chain &chain_child;
    // The bytes below the return address: it sits at ESP+depth.
    std::uint32_t depth{};bool keyed_path{};
    Chains(State &s,std::uint32_t *c,Trace<State> *t,Count &n,Keyed &k,Plain &p,Marker &m,Chain &h)
        :Port<State>(s,c,t,chains_edges),count_child(n),keyed_child(k),plain_child(p),marker_child(m),chain_child(h){}
    std::uint32_t local(Local l)const {
        const auto entry=this->gp[esp]+depth;
        switch(l) {
        case Local::model:return entry-0x104u;
        case Local::posed:return entry-0x100u;
        case Local::pose:return entry-0xe0u;
        case Local::goal:return entry-0x70u;
        default:return entry+4u*(1u+unsigned(l));
        }
    }
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    // AND reg,FFFF; SHL reg,4.
    std::uint32_t instance_offset(std::uint32_t handle) {
        return this->flags.shift(this->flags.logic(handle&0xffffu,4),logic::instance_shift,4);
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_argument:return 0x116060u;
        case records_argument:return 0x11606eu;
        case other_argument:return 0x11607bu;
        case weight_argument:return 0x116089u;
        case least_weight:return 0x116092u;
        case instances:save(0x11609fu,gp[ebx]);return 0x1160a0u;                                     // PUSH EBX
        case definition_argument:save(0x1160a6u,gp[esi]);return 0x1160a7u;                          // PUSH ESI
        case definition_tag:return 0x1160aeu;
        case definition_address:return 0x1160bau;
        case definition_nodes:return 0x1160c6u;
        // AND EAX,FFFF; LEA EDX,[EAX+EAX*2].
        case object_table:gp[eax]=this->flags.logic(gp[eax]&0xffffu,4);gp[edx]=gp[eax]*3u;return 0x1160dau;
        case object_datums:return 0x1160dfu;
        case object_address:return 0x1160e2u;
        case object_tag:return 0x1160e6u;
        case object_definition:return 0x1160f0u;
        case object_model:return 0x1160f4u;
        case model_address:return 0x116100u;
        case model_store:return 0x116107u;
        case posed_clear:return 0x116126u;
        case weight_again:return 0x11612fu;
        case definition_again:return 0x116138u;
        case instances_again:return 0x11613fu;
        case keyed:return 0x116150u;
        case first_key:return 0x116157u;
        case record_tag:return 0x11615cu;
        case record_tag_address:return keyed_path?0x116169u:0x116187u;
        case first_key_store:return keyed_path?0x11616du:0x11618bu;
        case second_key:return keyed_path?0x116170u:0x11618eu;
        case second_key_store:return keyed_path?0x116175u:0x116193u;
        case other_marker:return 0x1161a3u;
        case object_marker:return 0x1161afu;
        case object_again:save(0x1161bfu,1u);save(0x1161c1u,gp[eax]);return 0x1161c2u;               // PUSH 1; PUSH EAX
        case pose_node:return 0x1161deu;
        case posed_word:return 0x1161f2u;
        case other_marker_again:return 0x1161fcu;
        case other_again:save(0x1161ffu,1u);save(0x116201u,gp[eax]);return 0x116202u;                // PUSH 1; PUSH EAX
        case poses_argument:return 0x116221u;
        case chain_node:return 0x116228u;
        case matrices_argument:return 0x11622du;
        case model_local:return 0x116235u;
        case model_handle:return 0x116247u;
        case posed_node:return 0x11624fu;
        case posed_read:case posed_store:return 0x116267u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        // SUB ESP,104; CMP EAX,-1; PUSH EBP.
        case object_argument:
            gp[eax]=v;gp[esp]=this->flags.minus(gp[esp],0x104u,4);depth=0x104u;this->compare32(v,logic::none);
            save(0x11606du,gp[ebp]);break;
        case records_argument:gp[ebp]=v;break;
        case other_argument:this->compare32(v,logic::none);break;
        case weight_argument:x[0]=lane0_only(v);break;
        case least_weight:this->flags.scalar_compare(x[0],lane0_only(v),false);break;               // COMISS XMM0,m32
        case instances:case instances_again:gp[ebx]=v;break;
        case definition_argument:case definition_again:gp[esi]=v;break;
        case definition_tag:gp[ecx]=instance_offset(v);break;
        case definition_address:gp[ecx]=v;this->test32(v);break;
        case definition_nodes:gp[ecx]=logic::widen16(v);this->test32(gp[ecx]);break;
        case object_table:gp[eax]=v;break;
        case object_datums:gp[ecx]=v;break;
        case object_address:gp[edx]=v;break;
        case object_tag:case record_tag:gp[eax]=instance_offset(v);break;
        case object_definition:gp[ecx]=v;break;
        case object_model:gp[edx]=instance_offset(v);break;
        case model_address:gp[eax]=v;save(0x116104u,gp[edi]);gp[ecx]=gp[esi];break;                 // PUSH EDI; MOV ECX,ESI
        case weight_again:gp[edi]=v;break;
        case keyed:set_low_byte(gp[eax],v);keyed_path=(v&0xffu)!=0;break;
        case first_key:gp[ecx]=v;this->test8(gp[eax]);break;                                         // TEST AL,AL
        case record_tag_address:gp[ebx]=v;break;
        case second_key:gp[edx]=v;gp[esi]=gp[ebp];break;                                             // MOV ESI,EBP
        // CMP EAX,-1; (TEST EAX,EAX).
        case other_marker:case object_marker:
            gp[eax]=v;this->compare32(v,logic::none);if(v!=logic::none)this->test32(v);break;
        case object_again:case other_again:gp[eax]=v;save(a==object_again?0x1161c9u:0x116209u,gp[eax]);break;
        // MOVSX EAX; MOV ECX,EAX; AND ECX,1F; MOV EDX,1; SHL EDX,CL; SAR EAX,5.
        case pose_node:
            gp[eax]=logic::widen16(v);gp[ecx]=this->flags.logic(gp[eax]&0x1fu,4);
            gp[edx]=this->flags.shift(1u,gp[ecx]&0xffu,4);gp[eax]=this->flags.right(gp[eax],5u,4,true);break;
        case posed_word:this->test32(v&gp[edx]);break;
        case other_marker_again:case model_handle:gp[eax]=v;break;
        case poses_argument:gp[ecx]=v;break;
        case chain_node:gp[esi]=logic::widen16(v);break;
        case matrices_argument:gp[ebx]=v;save(0x116234u,gp[ecx]);break;                              // PUSH ECX
        // PUSH EDI; LEA EDX,[ESP+E4]; PUSH EDX; LEA EAX,[ESP+44]; PUSH EAX.
        case model_local:
            gp[ecx]=v;save(0x116239u,gp[edi]);gp[edx]=local(Local::goal)+logic::goal_offset;save(0x116241u,gp[edx]);
            gp[eax]=local(Local::pose)+logic::pose_matrix;save(0x116246u,gp[eax]);break;
        // MOVSX ECX; MOV EDX,ECX; SAR EDX,5; LEA EAX,[ESP+EDX*4+14]; AND ECX,1F; MOV EDX,1; SHL EDX,CL.
        case posed_node:
            gp[ecx]=logic::widen16(v);gp[edx]=this->flags.right(gp[ecx],5u,4,true);gp[eax]=local(Local::posed)+gp[edx]*4u;
            gp[ecx]=this->flags.logic(gp[ecx]&0x1fu,4);gp[edx]=this->flags.shift(1u,gp[ecx]&0xffu,4);break;
        case posed_read:this->flags.logic(v|gp[edx],4);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::posed_clear){this->gp[edi]+=4u;--this->gp[ecx];}
    }
    void returned(Kept &kept)const{kept={this->gp[ebx],this->gp[esi],this->gp[edi],this->gp[ebp]};}
    std::uint32_t node_count(Kept &kept) {
        this->call(0x11610bu,0x116110u,0x1d4140u,count_child);returned(kept);
        return this->gp[eax];
    }
    // MOV ECX,EAX; ADD ECX,1F; SAR ECX,5; SHL ECX,2; MOV EDX,ECX; SHR ECX,2; XOR EAX,EAX; LEA EDI,[ESP+14].
    void clearing() {
        auto &gp=this->gp;
        gp[ecx]=this->flags.shift(this->flags.right(this->flags.plus(gp[eax],0x1fu,4),5u,4,true),2u,4);gp[edx]=gp[ecx];
        gp[ecx]=this->flags.right(gp[ecx],2u,4,false);gp[eax]=this->flags.logic(0u,4);gp[edi]=local(Local::posed);
    }
    void cleared(){this->gp[ecx]=this->flags.logic(this->gp[edx]&3u,4);}                         // MOV ECX,EDX; AND ECX,3
    std::uint32_t next_record(bool keyed_walk,Kept &kept) {
        if(keyed_walk)this->call(0x116178u,0x11617du,0x1e5250u,keyed_child);
        else this->call(0x116196u,0x11619bu,0x1e51d0u,plain_child);
        returned(kept);this->test8(this->gp[eax]);
        return this->gp[eax];
    }
    // XOR CL,CL; LEA EBX,[ESP+40] or [ESP+B0]; CALL BAD60; TEST AX,AX.
    std::uint32_t marker(bool other,Kept &kept) {
        auto &gp=this->gp;
        set_low_byte(gp[ecx],this->flags.logic(0u,1));gp[ebx]=local(other?Local::goal:Local::pose);
        if(other)this->call(0x116213u,0x116218u,0xbad60u,marker_child);
        else this->call(0x1161d0u,0x1161d5u,0xbad60u,marker_child);
        depth-=12u;returned(kept);this->flags.logic(gp[eax]&0xffffu,2);
        return gp[eax];
    }
    void pose_chain(Kept &kept){this->call(0x11624au,0x11624fu,0x1e1390u,chain_child);depth-=0x10u;returned(kept);}
    void run() {
        auto &gp=this->gp;
        using logic::marker_chains::Exit;
        Memory<Chains> memory{*this};
        const Kept entry{gp[ebx],gp[esi],gp[edi],gp[ebp]};
        const auto exit=logic::marker_chains::pose(memory,*this,entry);
        if(exit==Exit::walked)gp[edi]=this->pop(0x11626eu);
        if(exit!=Exit::not_posed){gp[esi]=this->pop(0x11626fu);gp[ebx]=this->pop(0x116270u);}
        gp[ebp]=this->pop(0x116271u);gp[esp]=this->flags.plus(gp[esp],0x104u,4);
        this->ret(0x116278u,0x20u);this->publish(0x116278u);
    }
};
template<class State,class Reference,class Count,class Keyed,class Plain,class Marker,class Chain>
bool bridge(State &s,Reference reference,Count count,Keyed keyed,Plain plain,Marker marker,Chain chain) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.other_flags&0x400u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x116060u,0x116278u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Chains<State,Count,Keyed,Plain,Marker,Chain>(v,c,t,count,keyed,plain,marker,chain);});
    return true;
}
} // namespace halo2::native_marker_chains_detail
