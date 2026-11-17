#pragma once
// Boundary adapters for the object subtree systems
// (native_tu15_object_subtree_systems.hpp): BE850 object move, BE610 cluster
// move, C0F60 attachment refresh, 10DD80 type moves, C0200 node notify,
// 1D5790 timers, 1E15D0 countdown, C0910 node lights. Their callers and
// children are still translated TU1.5 code in the live support unit, so each
// adapter owns that ABI and nothing else, with the boundary framework of
// native_tu15_object_tick.hpp (boundary::Port, boundary::Memory):
//   - arguments from registers and stack slots, results back into the
//     original's registers (including partial-register writes and the values
//     the original leaves in scratch registers);
//   - the original's register file at every child call: each access role's
//     register effects, the "kept" values the logic hands to a child call
//     (loaded into their callee-saved registers before it, read back after
//     it), and the flags of the instruction that decides them;
//   - the original's stack frame: register saves and restores, the frame
//     slots (locals, BE850's node bit vector cleared by REP STOSD/STOSB in the
//     live direction flag, C0F60's rewritten handle argument, C0910's walk
//     slots and weights), return addresses, and the stack pointer as the
//     children leave it;
//   - C0910's XMM0/1/2/5, all lanes, as the original leaves them at every
//     child call and at exit (taken from the stored colour bits; the logic's
//     MULSS and COMISS run on the live MXCSR);
//   - the diagnostic cursor (each access publishes its original instruction)
//     and the store stream (every store goes through the unit's helper).
// Delete an adapter when its callers and children go native.
//
// Every child stays genuine and is entered through the unit's own original
// route (direct body, dispatch or the required owner at its original site)
// with the exact register, flag, XMM, x87 and stack ABI. BF0C0/BFF20 (BE850)
// go through the required owner. C0F60's C4D30 and C0910's BCB70 children are
// passed in by the binder; it routes them through the required owner at their
// original sites (the light callers' binding).
// HALO2_REFERENCE_OBJECT_SUBTREE=1 restores the translated bodies.
// HALO2_VERIFY_OBJECT_SUBTREE=1 records each admitted native call (children
// genuine, run once) and replays the unmodified translated body against it,
// as in native_tu15_object_tick.hpp. A summary is printed at exit.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_subtree_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_object_subtree_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx16;
using native_object_tick_detail::sx8;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_subtree;

inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_SUBTREE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_SUBTREE");return p && *p;}();
    return value;
}

// Branch outcomes reached by the owners (verification coverage), two 64-bit
// groups. The adapters read them from the order of the original
// instructions (each owner's edge table) and a few ABI facts.
enum Move : unsigned {
    // BE850
    j_unclustered, j_clustered, j_nodes, j_no_cluster, j_cluster_off, j_cluster_static, j_cluster_clear,
    j_clear_up, j_clear_down, j_parent_set, j_parent_clear, j_not_attached, j_attach_changed, j_attached,
    j_children, j_no_children,
    // BE610
    k_parent_none, k_parent, k_clustered, k_phantom_changed, k_phantom_flag, k_phantom_quiet, k_cluster_empty,
    k_static, k_refused, k_moving, k_flagged, k_extent, k_phantom_skip, k_phantom_update, k_single, k_second,
    k_mode_four, k_release_off, k_release, k_set, k_clear,
    // 10DD80
    f_none, f_callback, f_empty_callback, f_more,
    // C0200
    g_no_model, g_no_first, g_no_second, g_notify,
    move_count
};
enum Attach : unsigned {
    // C0F60
    l_skip_deleted, l_skip_bit, l_ready, l_birth, l_born, l_no_nodes, l_nodes_empty, l_nodes, l_node_set,
    l_node_none, l_node_blocked, l_node_first, l_node_second, l_no_placement, l_placement, l_child, l_no_child,
    l_no_siblings, l_sibling_end, l_sibling,
    // 1E15D0
    h_idle, h_held, h_up_continue, h_up_done, h_down_continue, h_down_done,
    // 1D5790
    i_reset, i_kept,
    // C0910
    m_unlit, m_no_rows, m_rows, m_no_contributions, m_contribution_none, m_contribution_weighted,
    m_contribution_zero, m_no_scales, m_scale_none, m_scale_weighted, m_scale_zero, m_low_x, m_high_x, m_keep_x,
    m_low_y, m_high_y, m_keep_y, m_low_z, m_high_z, m_keep_z, m_more_rows,
    attach_count
};
static_assert(move_count<=64u && attach_count<=64u);
inline std::atomic<std::uint64_t> &move_coverage(){static std::atomic<std::uint64_t> value{0};return value;}
inline std::atomic<std::uint64_t> &attach_coverage(){static std::atomic<std::uint64_t> value{0};return value;}

// MOVSS xmm,m32: the bits in lane 0, the upper lanes zero.
inline __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
// x with lane 0 replaced (MOVAPS then a scalar result).
inline __m128 with_lane0(__m128 x,std::uint32_t bits){return _mm_move_ss(x,scalar(bits));}

// ---- BE850 object move ---------------------------------------------------------
// [ESP+4] = the handle. SUB ESP,20 makes the node bit vector's frame; EBP,
// ESI, EDI saved. Exit: ADD ESP,20 decides the flags; RET 4.
inline constexpr Edge object_move_edges[]{
    {0xbe874u,0xbe883u,j_unclustered},{0xbe874u,0xbe87du,j_clustered},{0xbe883u,0xbe891u,j_nodes},
    {0xbe899u,0xbe916u,j_no_cluster},{0xbe8a4u,0xbe916u,j_cluster_off},
    {0xbe8c7u,0xbe911u,j_cluster_static},{0xbe8c7u,0xbe8d4u,j_cluster_clear},
    {0xbe91bu,0xbe929u,j_parent_set},{0xbe91bu,0xbe932u,j_parent_clear},{0xbe946u,0xbe950u,j_attached},
    {0xbe95eu,0xbe966u,j_children},{0xbe95eu,0xbe98cu,j_no_children},
};
template<class State,class Required,class Cluster,class Clear,class Types,class Attachment,class Self,class Nodes>
struct ObjectMoveBoundary : Port<State> {
    using Access=logic::object_move::Access;
    using Kept=logic::object_move::Move;
    Required &required;Cluster &cluster_child;Clear &clear;Types &types;Attachment &attachment;Self &self;Nodes &nodes;
    std::uint32_t attachments_value{};
    ObjectMoveBoundary(State &s,std::uint32_t *c,Trace<State> *t,Required &q,Cluster &k,Clear &l,Types &y,Attachment &a,Self &x,Nodes &n)
        :Port<State>(s,c,t,object_move_edges),required(q),cluster_child(k),clear(l),types(y),attachment(a),self(x),nodes(n){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:return 0xbe850u;
        case headers:return 0xbe856u;
        case object:return 0xbe86du;
        case cluster:return 0xbe874u;
        case header_type:return 0xbe883u;
        case cluster_again:return 0xbe899u;
        case cluster_flags:return 0xbe8a4u;
        case cluster_table:return 0xbe8b2u;
        case cluster_data:return 0xbe8b8u;
        // AND EAX,FFFF; PUSH EBX; the record offset (index * A0) in EBX.
        case record_flags:gp[eax]&=0xffffu;this->push(0xbe8c0u,gp[ebx]);gp[ebx]=(gp[eax]*5u)<<5;return 0xbe8c7u;
        case node_matrices:return 0xbe8d4u;
        case parent_flags:return 0xbe91bu;
        case previous_set:return 0xbe929u;
        case previous_clear:return 0xbe932u;
        case parent_clear:return 0xbe939u;
        case attachments_flag:return 0xbe946u;
        // Not refreshed: no attachments, or the bounds changed (AL).
        case first_child:
            if(this->previous==0xbe946u)this->reached(attachments_value&logic::header_attachments?j_attach_changed:j_not_attached);
            return 0xbe95eu;
        case child_table:gp[edx]=(gp[esi]&0xffffu)*3u;return 0xbe976u;
        case child_headers:return 0xbe97bu;
        case child_object:return 0xbe97eu;
        case next_sibling:return 0xbe982u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:gp[ecx]=v;break;
        case headers:case cluster_table:case child_object:gp[edx]=v;break;
        case object:gp[esi]=v;gp[ebp]=gp[edx]+gp[eax]*4u;break;
        case cluster:this->compare32(v,logic::none);break;
        // MOV CL; MOV EAX,1; SHL EAX,CL; TEST AL,AL.
        case header_type:set_low_byte(gp[ecx],v);gp[eax]=1u<<(gp[ecx]&31u);this->test8(gp[eax]);break;
        case cluster_again:gp[eax]=v;this->compare32(v,logic::none);break;
        case cluster_flags:set_low_byte(gp[ecx],(v&0xffu)>>6);this->test8(gp[ecx]&1u);break;
        case cluster_data:gp[edi]=v;break;
        case record_flags:gp[eax]=v>>15;gp[ebx]+=gp[edi];this->test8(gp[eax]&1u);break;
        case node_matrices:gp[ecx]=sx16(v);break;
        case parent_flags:set_low_byte(gp[ecx],(v&0xffu)>>2);break;
        case attachments_flag:attachments_value=v;this->test8(v&logic::header_attachments);break;
        case first_child:case next_sibling:gp[esi]=v;this->compare32(v,logic::none);break;
        case child_table:gp[eax]=v;break;
        case child_headers:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::parent_clear)this->test8(v);}  // AND's flags
    // SUB ESP,20; PUSH EBP/ESI/EDI; the handle at [ESP+30] (EDI); EAX = index * 3.
    std::uint32_t handle() {
        auto &gp=this->gp;
        gp[esp]-=0x20u;this->push(0xbe85cu,gp[ebp]);this->push(0xbe85du,gp[esi]);this->push(0xbe85eu,gp[edi]);
        gp[edi]=this->read(0xbe85fu,gp[esp]+0x30u,4);gp[eax]=(gp[edi]&0xffffu)*3u;
        return gp[edi];
    }
    // After PUSH EBX the argument is [ESP+34]; POP EBX follows.
    std::uint32_t handle_again() {
        auto &gp=this->gp;
        gp[edi]=this->read(0xbe911u,gp[esp]+0x34u,4);gp[ebx]=this->pop(0xbe915u);
        return gp[edi];
    }
    // The move lives in EDI (handle), ESI (object, or the child being moved)
    // and EBP (header).
    void keep(const Kept &m,std::uint32_t esi_value){auto &gp=this->gp;gp[edi]=m.handle;gp[esi]=esi_value;gp[ebp]=m.header;}
    void take(Kept &m,std::uint32_t &esi_value)const{const auto &gp=this->gp;m.handle=gp[edi];esi_value=gp[esi];m.header=gp[ebp];}
    // CMP [ESI+B4],-1 decided the flags; PUSH EDI; BE610.
    void move_cluster(Kept &m) {
        keep(m,m.object);this->push(0xbe87du,this->gp[edi]);this->call(0xbe87eu,0xbe883u,0xbe610u,cluster_child);take(m,m.object);
    }
    // TEST AL,AL decided the flags; PUSH EDI; BF0C0 (required owner).
    void rebuild_node_matrices(Kept &m) {
        keep(m,m.object);this->push(0xbe891u,this->gp[edi]);this->call_routed(required,0xbe892u,0xbe897u,0xbf0c0u);take(m,m.object);
    }
    // MOV EDX/ECX the byte and dword counts; XOR EAX; LEA EDI,[ESP+10];
    // REP STOSD; AND ECX,3; REP STOSB; each step in the live direction flag.
    void clear_node_bits(std::uint32_t bytes) {
        auto &gp=this->gp;
        gp[edx]=bytes;gp[ecx]=bytes>>2;gp[eax]=0;gp[edi]=gp[esp]+0x10u;
        const bool down=(this->s.other_flags&0x400u)!=0;
        this->reached(down?j_clear_down:j_clear_up);
        for(;gp[ecx];--gp[ecx]){this->store(0xbe8fbu,gp[edi],4,gp[eax]);gp[edi]+=down?0xfffffffcu:4u;}
        gp[ecx]=gp[edx]&3u;this->test32(gp[ecx]);
        for(;gp[ecx];--gp[ecx]){this->store(0xbe902u,gp[edi],1,gp[eax]);gp[edi]+=down?0xffffffffu:1u;}
    }
    // PUSH 0; PUSH the bit vector (LEA EAX,[ESP+14]); PUSH EBX (the record); 1DBBD0.
    void mark_node_bits(Kept &m,std::uint32_t record) {
        auto &gp=this->gp;gp[esi]=m.object;gp[ebp]=m.header;gp[ebx]=record;
        this->push(0xbe904u,0);gp[eax]=gp[esp]+0x14u;this->push(0xbe90au,gp[eax]);this->push(0xbe90bu,gp[ebx]);
        this->call(0xbe90cu,0xbe911u,0x1dbbd0u,clear);
        m.object=gp[esi];m.header=gp[ebp];
    }
    // 10DD80 takes the handle in EDI.
    void move_types(Kept &m){keep(m,m.object);this->call(0xbe916u,0xbe91bu,0x10dd80u,types);take(m,m.object);}
    // PUSH EDI; BFF20 (required owner); AL = the bounds changed.
    std::uint8_t update_bounds(Kept &m) {
        keep(m,m.object);this->push(0xbe940u,this->gp[edi]);this->call_routed(required,0xbe941u,0xbe946u,0xbff20u);take(m,m.object);
        return std::uint8_t(this->gp[eax]);
    }
    // TEST AL,AL; C0F60(handle, 1, 1, 0, 0).
    void refresh_attachments(Kept &m) {
        auto &gp=this->gp;keep(m,m.object);this->test8(gp[eax]);
        this->push(0xbe950u,0);this->push(0xbe952u,0);this->push(0xbe954u,1);this->push(0xbe956u,1);this->push(0xbe958u,gp[edi]);
        this->call(0xbe959u,0xbe95eu,0xc0f60u,attachment);take(m,m.object);
    }
    // CMP ESI,-1 (the child) decides the flags; PUSH ESI; BE850 (recursive).
    void move_child(Kept &m) {
        keep(m,m.child);this->compare32(this->gp[esi],logic::none);this->push(0xbe966u,this->gp[esi]);
        this->call(0xbe967u,0xbe96cu,0xbe850u,self);take(m,m.child);
    }
    // MOV EAX,EDI; C0200 (the flags are the chain's last CMP ESI,-1).
    void notify_nodes(Kept &m){auto &gp=this->gp;gp[edi]=m.handle;gp[eax]=gp[edi];this->call(0xbe98cu,0xbe991u,0xc0200u,nodes);}
    void run() {
        auto &gp=this->gp;
        Memory<ObjectMoveBoundary> memory{*this};
        logic::object_move::run(memory,*this);
        gp[edi]=this->pop(0xbe991u);gp[esi]=this->pop(0xbe992u);gp[ebp]=this->pop(0xbe993u);
        this->flags.plus(gp[esp],0x20u,4);gp[esp]+=0x20u;
        this->ret(0xbe997u,4);this->publish(0xbe997u);
    }
};

// ---- BE610 cluster move --------------------------------------------------------
// [ESP+4] = the handle. SUB ESP,58 makes the frame: [ESP+C] the object, the
// phantom result byte, the bounds/extent locals. EBX and EDI saved (EBP and
// ESI too while the cluster moves). Exit: ADD ESP,58 decides the flags; RET 4.
inline constexpr Edge cluster_move_edges[]{
    {0xbe793u,0xbe7b8u,k_parent_none},{0xbe793u,0xbe7a3u,k_parent},{0xbe63au,0xbe644u,k_clustered},
    {0xbe66bu,0xbe680u,k_phantom_changed},{0xbe671u,0xbe687u,k_phantom_flag},{0xbe671u,0xbe757u,k_phantom_quiet},
    {0xbe687u,0xbe757u,k_cluster_empty},{0xbe692u,0xbe744u,k_static},{0xbe6a3u,0xbe744u,k_refused},{0xbe6a3u,0xbe6b4u,k_moving},
    {0xbe6cau,0xbe6ebu,k_flagged},{0xbe6cau,0xbe6d6u,k_extent},{0xbe706u,0xbe744u,k_phantom_skip},{0xbe706u,0xbe718u,k_phantom_update},
    {0xbe71du,0xbe74cu,k_single},{0xbe71du,0xbe72fu,k_second},
    {0xbe76cu,0xbe7b8u,k_mode_four},{0xbe76cu,0xbe7c7u,k_mode_four},
    {0xbe771u,0xbe7b8u,k_release_off},{0xbe771u,0xbe7c7u,k_release_off},{0xbe771u,0xbe77fu,k_release},
    {0u,0xbe7b8u,k_set},{0u,0xbe7c7u,k_clear},
};
template<class State,class Dispatch,class Moving,class Bounds,class Extent,class Collide,class Release>
struct ClusterMoveBoundary : Port<State> {
    using Access=logic::cluster_move::Access;
    using Kept=logic::cluster_move::ClusterMove;
    Dispatch &dispatch;Moving &moving;Bounds &bounds;Extent &extent;Collide &collide;Release &release;
    ClusterMoveBoundary(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d,Moving &m,Bounds &b,Extent &e,Collide &k,Release &r)
        :Port<State>(s,c,t,cluster_move_edges),dispatch(d),moving(m),bounds(b),extent(e),collide(k),release(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:return 0xbe613u;
        case headers:return 0xbe618u;
        case object:this->push(0xbe62au,gp[edi]);return 0xbe62bu;
        case object_flags:return 0xbe62fu;
        case parent:return 0xbe793u;
        case parent_object:gp[edx]=(gp[eax]&0xffffu)*3u;return 0xbe7a3u;
        case parent_flags:return 0xbe7a7u;
        case cluster:return 0xbe644u;
        case cluster_table:return 0xbe64au;
        case cluster_data:return 0xbe650u;
        case pending_clear:return 0xbe680u;
        case in_cluster_test:return 0xbe671u;
        case phantom_count:return 0xbe687u;
        case fixed_flags:return 0xbe692u;
        case extent_flags:return 0xbe6bau;
        case second_flags:return 0xbe71du;
        case phantom_default:return 0xbe744u;
        case game:return 0xbe75bu;
        case mode:return 0xbe761u;
        case release_word:return 0xbe771u;
        case release_reference:return 0xbe77fu;
        case release_clear:return 0xbe78au;
        case in_cluster_set:return 0xbe7b8u;
        case in_cluster_clear:return 0xbe7c7u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:case parent:case parent_object:case cluster:case phantom_count:case release_reference:gp[eax]=v;break;
        case headers:case cluster_table:gp[ecx]=v;break;
        case object:gp[edi]=v;break;
        case object_flags:set_low_byte(gp[eax],(v&0xffu)>>6);break;
        case parent_flags:set_low_byte(gp[ebx],v&1u);break;
        case cluster_data:case game:gp[edx]=v;break;
        case fixed_flags:gp[edx]=v>>8;this->test8(gp[edx]&1u);break;
        // SHR EDX,1; LEA ECX,[ESP+50]; LEA EBX,[ESP+38]; MOV [ESP+18],ECX.
        case extent_flags:
            gp[edx]=v>>1;gp[ecx]=gp[esp]+0x50u;gp[ebx]=gp[esp]+0x38u;this->store(0xbe6cau,gp[esp]+0x18u,4,gp[ecx]);break;
        case second_flags:gp[ecx]=v>>1;this->test8(gp[ecx]&1u);break;
        case phantom_default:gp[ecx]=v;gp[edi]=v;break;
        // MOV AL; SETE BL (the phantom result was 0); POP ESI; POP EBP.
        case mode:
            set_low_byte(gp[eax],v);set_low_byte(gp[ebx],(gp[ecx]&0xffu)==0?1u:0u);
            gp[esi]=this->pop(0xbe76bu);gp[ebp]=this->pop(0xbe76cu);break;
        case release_word:gp[eax]=v>>11;this->test8(gp[eax]&1u);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EBX; the handle at [ESP+60] (EBX); EDX = index * 3.
    std::uint32_t handle() {
        auto &gp=this->gp;
        this->push(0xbe61bu,gp[ebx]);gp[ebx]=this->read(0xbe61cu,gp[esp]+0x60u,4);gp[eax]=gp[ebx]&0xffffu;gp[edx]=gp[eax]*3u;
        return gp[ebx];
    }
    void keep_object(std::uint32_t object){this->store(0xbe63au,this->gp[esp]+0xcu,4,object);}
    // PUSH EBP; AND EAX,FFFF; the record (its offset, index * A0, plus the
    // data in EDX; ADD decides the flags) in EBP and ESI (after PUSH ESI);
    // 1D9EC0 (dispatch). The cluster move lives in EBX (handle), EDI (object)
    // and EBP (record).
    std::uint8_t update_phantom(Kept &c) {
        auto &gp=this->gp;
        gp[ebx]=c.handle;gp[edi]=c.object;
        this->push(0xbe653u,gp[ebp]);gp[eax]&=0xffffu;gp[ebp]=this->flags.plus(c.record-gp[edx],gp[edx],4);
        this->push(0xbe661u,gp[esi]);gp[esi]=gp[ebp];
        this->call_routed(dispatch,0xbe664u,0xbe669u,0x1d9ec0u);
        c.handle=gp[ebx];c.object=gp[edi];c.record=gp[ebp];
        return std::uint8_t(gp[eax]);
    }
    void keep_phantom_result(std::uint8_t result){this->store(0xbe66bu,this->gp[esp]+0x13u,1,result);}
    // MOV AL,[ESP+13]; TEST AL,AL.
    std::uint8_t phantom_result() {
        auto &gp=this->gp;set_low_byte(gp[eax],this->read(0xbe706u,gp[esp]+0x13u,1));this->test8(gp[eax]);
        return std::uint8_t(gp[eax]);
    }
    std::uint8_t final_phantom_result() {
        auto &gp=this->gp;set_low_byte(gp[ecx],this->read(0xbe757u,gp[esp]+0x13u,1));return std::uint8_t(gp[ecx]);
    }
    // MOV EDI,EBP; TEST DL,1 (fixed, clear); 1D9F50; TEST AL,AL.
    bool start_moving(Kept &c) {
        auto &gp=this->gp;
        gp[ebx]=c.handle;gp[ebp]=c.record;gp[edi]=gp[ebp];this->test8(gp[edx]&1u);
        this->call(0xbe6a3u,0xbe6a8u,0x1d9f50u,moving);
        c.handle=gp[ebx];c.record=gp[ebp];this->test8(gp[eax]);
        return (gp[eax]&0xffu)!=0;
    }
    // LEA EAX,[ESP+34]; PUSH EAX; 1DC610 (RET 4).
    void compute_bounds(Kept &c) {
        auto &gp=this->gp;
        gp[ebx]=c.handle;gp[ebp]=c.record;gp[eax]=gp[esp]+0x34u;this->push(0xbe6b4u,gp[eax]);
        this->call(0xbe6b5u,0xbe6bau,0x1dc610u,bounds);c.record=gp[ebp];
    }
    // TEST DL,1; EAX = the handle argument ([ESP+6C]); EDI/ESI = the extent's
    // locals; BB200; EAX = [ESP+18] (the local the extent filled).
    void compute_extent(Kept &c) {
        auto &gp=this->gp;
        gp[ebp]=c.record;this->test8(gp[edx]&1u);
        gp[eax]=this->read(0xbe6d6u,gp[esp]+0x6cu,4);gp[edi]=gp[esp]+0x50u;gp[esi]=gp[esp]+0x38u;
        this->call(0xbe6e2u,0xbe6e7u,0xbb200u,extent);
        c.record=gp[ebp];gp[eax]=this->read(0xbe6e7u,gp[esp]+0x18u,4);
    }
    // Without the extent EBX and EAX are 0 (XOR decides the flags). ESI = the
    // handle argument; B9460(handle, local, EBX, EAX, 0, 0, 0, 0, 1) (RET 24).
    void query_collisions(Kept &c,bool without_extent) {
        auto &gp=this->gp;
        gp[ebp]=c.record;
        if(without_extent){gp[ebx]=0;gp[eax]=0;this->test32(0);}
        gp[esi]=this->read(0xbe6ebu,gp[esp]+0x6cu,4);
        this->push(0xbe6efu,1);this->push(0xbe6f1u,0);this->push(0xbe6f3u,0);this->push(0xbe6f5u,0);this->push(0xbe6f7u,0);
        this->push(0xbe6f9u,gp[eax]);this->push(0xbe6fau,gp[ebx]);
        gp[eax]=gp[esp]+0x78u;this->push(0xbe6ffu,gp[eax]);this->push(0xbe700u,gp[esi]);
        this->call(0xbe701u,0xbe706u,0xb9460u,collide);
        c.handle=gp[esi];c.record=gp[ebp];
    }
    // LEA ECX,[ESP+28]; XOR EAX; MOV EDX,EBP; XOR EDI (the flags); 1D8C80 (dispatch).
    void move_phantom(Kept &c) {
        auto &gp=this->gp;
        gp[esi]=c.handle;gp[ebp]=c.record;gp[ecx]=gp[esp]+0x28u;gp[eax]=0;gp[edx]=gp[ebp];gp[edi]=0;this->test32(0);
        this->call_routed(dispatch,0xbe718u,0xbe71du,0x1d8c80u);
        c.handle=gp[esi];c.record=gp[ebp];
    }
    // LEA ECX,[ESP+1C]; XOR EAX (the flags); MOV EDX,EBP; 1D8D80 (dispatch); MOV EDI,ECX.
    void move_second_phantom(Kept &c) {
        auto &gp=this->gp;
        gp[esi]=c.handle;gp[ebp]=c.record;gp[ecx]=gp[esp]+0x1cu;gp[eax]=0;gp[edx]=gp[ebp];this->test32(0);
        this->call_routed(dispatch,0xbe72fu,0xbe734u,0x1d8d80u);
        gp[edi]=gp[ecx];c.handle=gp[esi];c.record=gp[ebp];
    }
    // B9770: EAX = the handle (from EBX or ESI, wherever the walk keeps it);
    // ECX = [ESP+28] when the phantom moved (ECX/EDI otherwise hold the
    // default). PUSH 0; B9770 (dispatch, RET 4); the object is read back from
    // its slot ([ESP+14]) into EDI.
    void finish_phantoms(Kept &c,bool moved,std::uint32_t fallback) {
        auto &gp=this->gp;
        if(moved)gp[ecx]=gp[esp]+0x28u;
        else gp[ecx]=gp[edi]=fallback;
        gp[eax]=c.handle;
        this->push(0xbe74cu,0);
        this->call_routed(dispatch,0xbe74eu,0xbe753u,0xb9770u);
        gp[edi]=this->read(0xbe753u,gp[esp]+0x14u,4);c.object=gp[edi];
    }
    // TEST AL,1 (the release bit) decided the flags; EAX = the cluster
    // reference; 1E1980. The object stays in EDI, in_cluster in BL.
    void release_cluster(Kept &c,std::uint32_t) {
        auto &gp=this->gp;gp[edi]=c.object;set_low_byte(gp[ebx],c.in_cluster);
        this->call(0xbe785u,0xbe78au,0x1e1980u,release);
        c.object=gp[edi];c.in_cluster=std::uint8_t(gp[ebx]);
    }
    void run() {
        auto &gp=this->gp;
        gp[esp]-=0x58u;
        Memory<ClusterMoveBoundary> memory{*this};
        const bool in_cluster=logic::cluster_move::run(memory,*this);
        const auto pops=in_cluster?0xbe7bfu:0xbe7ceu,last=in_cluster?0xbe7c4u:0xbe7d3u;
        gp[edi]=this->pop(pops);gp[ebx]=this->pop(pops+1u);
        this->flags.plus(gp[esp],0x58u,4);gp[esp]+=0x58u;
        this->ret(last,4);this->publish(last);
    }
};

// ---- C0F60 attachment refresh --------------------------------------------------
// [ESP+4] handle, [+8] detach, [+C] reconnect, [+10] siblings, [+14] ready
// (bytes). EBX, EBP, ESI, EDI saved. The flags of the deciding TEST/CMP; RET 14.
inline constexpr Edge attachment_refresh_edges[]{
    {0xc0f96u,0xc1072u,l_skip_deleted},{0xc0fa0u,0xc1072u,l_skip_bit},{0xc0fa0u,0xc0fbfu,l_ready},
    {0xc0f8du,0xc0fb0u,l_birth},{0xc0fb0u,0xc1072u,l_born},
    {0xc0fbfu,0xc101eu,l_no_nodes},{0xc0fd0u,0xc101eu,l_nodes_empty},{0xc0fd0u,0xc0fe5u,l_nodes},
    {0xc0fe5u,0xc0fe5u,l_node_set},{0xc0fe5u,0xc101au,l_node_set},
    {0xc0ff7u,0xc0fffu,l_node_first},{0xc1004u,0xc100cu,l_node_second},
    {0xc101eu,0xc1039u,l_no_placement},{0xc101eu,0xc1029u,l_placement},
    {0xc1039u,0xc1059u,l_no_child},{0xc1039u,0xc1041u,l_child},
    {0xc1059u,0xc1072u,l_no_siblings},{0xc1061u,0xc1072u,l_sibling_end},{0xc1061u,0xc1069u,l_sibling},
};
template<class State,class Birth,class First,class Second,class Placement,class Self>
struct AttachmentRefreshBoundary : Port<State> {
    using Access=logic::attachment_refresh::Access;
    using Kept=logic::attachment_refresh::Refresh;
    Birth &birth_child;First &first;Second &second;Placement &placement;Self &self;
    bool first_node{};std::uint32_t light{};
    AttachmentRefreshBoundary(State &s,std::uint32_t *c,Trace<State> *t,Birth &b,First &x,Second &y,Placement &p,Self &z)
        :Port<State>(s,c,t,attachment_refresh_edges),birth_child(b),first(x),second(y),placement(p),self(z){}
    // A visited attachment without a light, or one the birth blocked.
    void node_skipped(){if(this->previous==0xc0fecu)this->reached(light==logic::none?l_node_none:l_node_blocked);}
    // The node walk's ESI (the entry's light) and EBP (the count): set at the
    // first entry, stepped (ADD ESI,8; DEC EBP) after each.
    void step_node() {
        auto &gp=this->gp;
        node_skipped();
        if(first_node){gp[esi]=gp[ecx]+4u;gp[ebp]=gp[eax];first_node=false;}
        else {gp[esi]+=8u;gp[ebp]=this->flags.increment(gp[ebp],4,true);}
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case table:return 0xc0f74u;
        case headers:return 0xc0f7au;
        case object:return 0xc0f8du;
        case header_flags:return 0xc0f96u;
        case skip_flags:return 0xc0fa0u;
        case node_flags:return 0xc0fbfu;
        case nodes_size:return 0xc0fc9u;
        case nodes_offset:return 0xc0fd0u;
        case node_attached:step_node();return 0xc0fe5u;
        case node_light:return 0xc0fecu;
        case node_light_again:return 0xc100cu;
        case placement_flags:return 0xc101eu;
        case first_child:return 0xc1039u;
        case next_sibling:return 0xc1061u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:case node_light_again:gp[ecx]=v;break;
        case headers:gp[edx]=v;break;
        case object:gp[edi]=v;gp[eax]=gp[edx]+gp[eax]*4u;break;
        case header_flags:this->test8(v&logic::header_flags::pending_delete);break;
        case skip_flags:this->test8(v&logic::attachment_flags::skip);break;
        case node_flags:gp[eax]=v>>4;this->test8(gp[eax]&1u);break;
        case nodes_size:gp[eax]=sx16(v);break;
        // MOVSX ECX; SHR EAX,3; ADD ECX,EDI; TEST EAX,EAX.
        case nodes_offset:gp[ecx]=sx16(v);gp[eax]>>=3;gp[ecx]+=gp[edi];this->test32(gp[eax]);first_node=true;break;
        case node_attached:set_low_byte(gp[eax],v);break;
        case node_light:gp[eax]=v;light=v;break;
        case placement_flags:gp[edx]=v>>6;this->test8(gp[edx]&1u);break;
        case first_child:case next_sibling:gp[eax]=v;this->compare32(v,logic::none);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The handle argument at [ESP+14] (ESI); the sibling loop rewrites it.
    std::uint32_t handle(){auto &gp=this->gp;gp[esi]=this->read(0xc0f70u,gp[esp]+0x14u,4);return gp[esi];}
    // The node walk ends (its last DEC EBP); the handle again into ESI.
    std::uint32_t handle_again() {
        auto &gp=this->gp;node_skipped();gp[ebp]=this->flags.increment(gp[ebp],4,true);
        gp[esi]=this->read(0xc101au,gp[esp]+0x14u,4);return gp[esi];
    }
    // MOV CL,[ESP+24] (ready); EAX = index * 3.
    bool ready() {
        auto &gp=this->gp;set_low_byte(gp[ecx],this->read(0xc0f7du,gp[esp]+0x24u,1));gp[eax]=(gp[esi]&0xffffu)*3u;
        return (gp[ecx]&0xffu)!=0;
    }
    bool detach_requested() {
        auto &gp=this->gp;set_low_byte(gp[ecx],this->read(0xc0ff7u,gp[esp]+0x18u,1));this->test8(gp[ecx]);
        return (gp[ecx]&0xffu)!=0;
    }
    bool reconnect_requested() {
        auto &gp=this->gp;set_low_byte(gp[eax],this->read(0xc1004u,gp[esp]+0x1cu,1));this->test8(gp[eax]);
        return (gp[eax]&0xffu)!=0;
    }
    bool siblings_requested() {
        auto &gp=this->gp;set_low_byte(gp[eax],this->read(0xc1059u,gp[esp]+0x20u,1));this->test8(gp[eax]);
        return (gp[eax]&0xffu)!=0;
    }
    void continue_with(std::uint32_t sibling){this->store(0xc1069u,this->gp[esp]+0x14u,4,sibling);}
    // The node walk lives in ESI (the entry's light field), EBP (the count),
    // EDI (object) and BL (blocked).
    void keep_node(const Kept &r){auto &gp=this->gp;gp[esi]=r.entry+4u;gp[ebp]=r.remaining;gp[edi]=r.object;set_low_byte(gp[ebx],r.blocked);}
    void take_node(Kept &r)const{const auto &gp=this->gp;r.entry=gp[esi]-4u;r.remaining=gp[ebp];r.object=gp[edi];r.blocked=std::uint8_t(gp[ebx]);}
    // TEST CL,CL (ready, clear); MOV ECX,ESI; BBD10; MOV BL,AL; TEST BL,BL.
    std::uint8_t birth(Kept &r) {
        auto &gp=this->gp;
        this->test8(gp[ecx]);gp[esi]=r.handle;gp[edi]=r.object;gp[ecx]=gp[esi];
        this->call(0xc0fb0u,0xc0fb5u,0xbbd10u,birth_child);
        r.handle=gp[esi];r.object=gp[edi];set_low_byte(gp[ebx],gp[eax]);this->test8(gp[ebx]);
        return std::uint8_t(gp[eax]);
    }
    // C5250 (the flags of TEST CL,CL).
    void detach_light(Kept &r){keep_node(r);this->call(0xc0fffu,0xc1004u,0xc5250u,first);take_node(r);}
    // PUSH ECX (the light, read again); C4D30 (the flags of TEST AL,AL).
    void reconnect_light(Kept &r,std::uint32_t light) {
        keep_node(r);this->gp[ecx]=light;this->push(0xc100eu,this->gp[ecx]);this->call(0xc100fu,0xc1014u,0xc4d30u,second);take_node(r);
    }
    // C3750(handle, detach, reconnect): the arguments are read from the frame.
    void place(Kept &r) {
        auto &gp=this->gp;
        gp[eax]=this->read(0xc1029u,gp[esp]+0x1cu,4);gp[ecx]=this->read(0xc102du,gp[esp]+0x18u,4);
        this->push(0xc1031u,gp[eax]);this->push(0xc1032u,gp[ecx]);
        gp[esi]=r.handle;gp[edi]=r.object;set_low_byte(gp[ebx],r.blocked);this->push(0xc1033u,gp[esi]);
        this->call(0xc1034u,0xc1039u,0xc3750u,placement);
        r.handle=gp[esi];r.object=gp[edi];r.blocked=std::uint8_t(gp[ebx]);
    }
    // C0F60(child, detach, reconnect, 1, BL == 0): TEST BL,BL; SETE DL.
    void refresh_child(Kept &r,std::uint32_t child) {
        auto &gp=this->gp;
        gp[ecx]=this->read(0xc1041u,gp[esp]+0x1cu,4);
        gp[esi]=r.handle;gp[edi]=r.object;set_low_byte(gp[ebx],r.blocked);gp[eax]=child;
        this->test8(gp[ebx]);set_low_byte(gp[edx],(gp[ebx]&0xffu)==0?1u:0u);this->push(0xc104au,gp[edx]);
        gp[edx]=this->read(0xc104bu,gp[esp]+0x1cu,4);
        this->push(0xc104fu,1);this->push(0xc1051u,gp[ecx]);this->push(0xc1052u,gp[edx]);this->push(0xc1053u,gp[eax]);
        this->call(0xc1054u,0xc1059u,0xc0f60u,self);
        r.handle=gp[esi];r.object=gp[edi];r.blocked=std::uint8_t(gp[ebx]);
    }
    void run() {
        auto &gp=this->gp;
        this->push(0xc0f60u,gp[ebx]);this->push(0xc0f61u,gp[ebp]);this->push(0xc0f62u,gp[esi]);this->push(0xc0f63u,gp[edi]);
        Memory<AttachmentRefreshBoundary> memory{*this};
        logic::attachment_refresh::run(memory,*this);
        gp[edi]=this->pop(0xc1072u);gp[esi]=this->pop(0xc1073u);gp[ebp]=this->pop(0xc1074u);gp[ebx]=this->pop(0xc1075u);
        this->ret(0xc1076u,0x14u);this->publish(0xc1076u);
    }
};

// ---- 10DD80 type moves ---------------------------------------------------------
// EDI = the handle. ESI saved when the chain is not empty. TEST ECX,ECX (the
// chain's end) decides the flags; RET.
inline constexpr Edge type_moves_edges[]{
    {0x10dda5u,0x10ddd0u,f_none},{0x10ddb9u,0x10ddc0u,f_callback},{0x10ddb9u,0x10ddc3u,f_empty_callback},
    {0x10ddc3u,0x10ddb7u,f_more},
};
template<class State,class Dispatch> struct TypeMovesBoundary : Port<State> {
    using Access=logic::type_moves::Access;
    Dispatch &dispatch;bool saved{};
    TypeMovesBoundary(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,type_moves_edges),dispatch(d){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:return 0x10dd80u;
        case headers:return 0x10dd86u;
        case object:return 0x10dd93u;
        case type:return 0x10dd97u;
        case definition:return 0x10dd9eu;
        case chain_first:return 0x10dda5u;
        // PUSH ESI; MOV ESI,EAX (the chain slot) before the first entry.
        case chain_entry:if(!saved){this->push(0x10ddb4u,gp[esi]);gp[esi]=gp[eax];saved=true;}return 0x10ddb7u;
        case move:return 0x10ddb9u;
        case chain_next:return 0x10ddc3u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:gp[ecx]=v;break;
        case headers:gp[edx]=v;gp[eax]=(gp[edi]&0xffffu)*3u;break;
        case object:case definition:case chain_entry:case move:gp[eax]=v;break;
        case type:gp[ecx]=sx8(v);break;
        case chain_first:gp[ecx]=v;gp[eax]+=0x84u;break;
        case chain_next:gp[ecx]=v;gp[esi]+=4u;gp[eax]=gp[esi];break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t handle()const{return this->gp[edi];}
    // The walk lives in EDI (handle) and ESI (chain slot). TEST EAX,EAX (the
    // callback); PUSH EDI; the callback (dispatch).
    void type_move(std::uint32_t callback,logic::type_moves::ChainWalk &w) {
        auto &gp=this->gp;
        gp[edi]=w.handle;gp[esi]=w.slot;this->test32(callback);this->push(0x10ddc0u,gp[edi]);
        this->call_routed(dispatch,0x10ddc1u,0x10ddc3u,callback);
        w.handle=gp[edi];w.slot=gp[esi];
    }
    void run() {
        auto &gp=this->gp;
        Memory<TypeMovesBoundary> memory{*this};
        logic::type_moves::run(memory,*this);
        this->test32(gp[ecx]);
        if(saved)gp[esi]=this->pop(0x10ddcfu);
        this->ret(0x10ddd0u,0);this->publish(0x10ddd0u);
    }
};

// ---- C0200 node notify ---------------------------------------------------------
// EAX = the handle. PUSH ECX makes the slot BAC30 fills; EBX saved. The flags
// of the deciding CMP, or 10E1F0's; RET.
inline constexpr Edge node_notify_edges[]{
    {0xc022eu,0xc0265u,g_no_model},{0xc0244u,0xc0265u,g_no_first},{0xc0249u,0xc0265u,g_no_second},{0xc0249u,0xc0252u,g_notify},
};
template<class State,class Matrices,class Notify> struct NodeNotifyBoundary : Port<State> {
    using Access=logic::node_notify::Access;
    Matrices &matrices;Notify &notify;
    NodeNotifyBoundary(State &s,std::uint32_t *c,Trace<State> *t,Matrices &m,Notify &n)
        :Port<State>(s,c,t,node_notify_edges),matrices(m),notify(n){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:return 0xc0201u;
        case headers:return 0xc0207u;
        case tags:return 0xc020au;
        // PUSH EBX; MOV EBX,EAX (the handle); EAX = index * 3.
        case object:this->push(0xc0210u,gp[ebx]);gp[ebx]=gp[eax];gp[eax]=(gp[eax]&0xffffu)*3u;return 0xc021bu;
        case definition_index:return 0xc021fu;
        case definition:return 0xc022au;
        case model:return 0xc022eu;
        case model_definition:gp[eax]=(gp[eax]&0xffffu)<<4;return 0xc0240u;
        case first_node_tag:return 0xc0244u;
        case second_node_tag:return 0xc0249u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:case tags:gp[ecx]=v;break;
        case headers:gp[edx]=v;break;
        case object:case definition:case model_definition:gp[eax]=v;break;
        case definition_index:gp[edx]=(v&0xffffu)<<4;break;
        case model:gp[eax]=v;gp[edx]=logic::none;this->compare32(v,gp[edx]);break;
        case first_node_tag:case second_node_tag:this->compare32(v,gp[edx]);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t handle()const{return this->gp[eax];}
    // LEA EAX,[ESP+4] (the slot); PUSH EAX; MOV EAX,EBX; BAC30 (RET 4).
    void node_matrices(std::uint32_t) {
        auto &gp=this->gp;
        gp[eax]=gp[esp]+4u;this->push(0xc0252u,gp[eax]);gp[eax]=gp[ebx];
        this->call(0xc0255u,0xc025au,0xbac30u,matrices);
    }
    // MOV ECX,[ESP+4]; PUSH EAX (BAC30's result); PUSH ECX; 10E1F0 (RET 8).
    void notify_nodes() {
        auto &gp=this->gp;
        gp[ecx]=this->read(0xc025au,gp[esp]+4u,4);this->push(0xc025eu,gp[eax]);this->push(0xc025fu,gp[ecx]);
        this->call(0xc0260u,0xc0265u,0x10e1f0u,notify);
    }
    void run() {
        auto &gp=this->gp;
        this->push(0xc0200u,gp[ecx]);
        Memory<NodeNotifyBoundary> memory{*this};
        logic::node_notify::run(memory,*this);
        gp[ebx]=this->pop(0xc0265u);gp[ecx]=this->pop(0xc0266u);
        this->ret(0xc0267u,0);this->publish(0xc0267u);
    }
};

// ---- 1E15D0 countdown ----------------------------------------------------------
// ECX = the timer. AL = 1 when the timer ran, else 0. EBX saved. The flags
// of the deciding TEST/CMP; RET.
inline constexpr Edge countdown_edges[]{
    {0x1e15d1u,0x1e1614u,h_idle},{0x1e15dau,0x1e1614u,h_held},{0x1e15edu,0x1e1614u,h_up_continue},
    {0x1e15edu,0x1e15f4u,h_up_done},{0x1e1605u,0x1e1614u,h_down_continue},{0x1e1605u,0x1e160cu,h_down_done},
};
template<class State> struct CountdownBoundary : Port<State> {
    using Access=logic::countdown::Access;
    CountdownBoundary(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,countdown_edges){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case limit:return 0x1e15d1u;
        case flags:return 0x1e15dau;
        case count_up:return 0x1e15e7u;
        case count_up_store:return 0x1e15edu;
        // OR DL,2 (held) precedes the stores.
        case limit_store:set_low_byte(gp[edx],gp[edx]|logic::countdown_flags::held);this->test8(gp[edx]);return 0x1e15f4u;
        case held_up:return 0x1e15f6u;
        case count_down:return 0x1e15fdu;
        case count_down_store:return 0x1e1605u;
        case zero_store:set_low_byte(gp[edx],gp[edx]|logic::countdown_flags::held);this->test8(gp[edx]);return 0x1e160cu;
        case held_down:return 0x1e160fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOV BL (the limit); XOR AL,AL; TEST BL,BL.
        case limit:set_low_byte(gp[ebx],v);set_low_byte(gp[eax],0);this->test8(gp[ebx]);break;
        case flags:set_low_byte(gp[edx],v);this->test8(gp[edx]&logic::countdown_flags::held);break;
        // INC AL; CMP AL,BL.
        case count_up:set_low_byte(gp[eax],v+1u);this->flags.minus(gp[eax]&0xffu,gp[ebx]&0xffu,1);break;
        // DEC BL; MOV AL,BL; TEST AL,AL.
        case count_down:set_low_byte(gp[ebx],v-1u);set_low_byte(gp[eax],gp[ebx]);this->test8(gp[eax]);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        this->push(0x1e15d0u,gp[ebx]);
        Memory<CountdownBoundary> memory{*this};
        const auto exit=logic::countdown::run(memory,gp[ecx]);
        using enum logic::countdown::Exit;
        if(exit==running || exit==reached_limit)set_low_byte(gp[eax],1);
        const auto pop=exit==reached_limit?0x1e15fbu:0x1e1614u;
        gp[ebx]=this->pop(pop);this->ret(pop+1u,0);this->publish(pop+1u);
    }
};

// ---- 1D5790 timers -------------------------------------------------------------
// ESI = the timer block. AL = the OR of both timers (OR AL,BL decides the
// flags). EBX saved. RET.
inline constexpr Edge timers_edges[]{{0x1d579bu,0x1d57a3u,i_reset},{0x1d579bu,0x1d57aeu,i_kept}};
template<class State,class Timer> struct TimersBoundary : Port<State> {
    using Access=logic::timers::Access;
    Timer &timer;
    TimersBoundary(State &s,std::uint32_t *c,Trace<State> *t,Timer &x):Port<State>(s,c,t,timers_edges),timer(x){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case first_held:return 0x1d579bu;
        case reset_limit:set_low_byte(this->gp[eax],0);return 0x1d57a3u;  // XOR AL,AL
        case reset_counter:return 0x1d57a6u;
        case reset_flags:return 0x1d57a8u;
        }
        return 0;
    }
    void loaded(Access,std::uint32_t,std::uint32_t){}
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t block()const{return this->gp[esi];}
    // ECX = the timer; the block stays in ESI, the first result in BL.
    // 1E15D0 leaves the timer's address in ECX.
    std::uint8_t run_first_timer(logic::timers::Timers &t) {
        auto &gp=this->gp;
        gp[esi]=t.block;gp[ecx]=t.timer;
        this->call(0x1d5794u,0x1d5799u,0x1e15d0u,timer);
        t.block=gp[esi];t.timer=gp[ecx];set_low_byte(gp[ebx],gp[eax]);
        return std::uint8_t(gp[eax]);
    }
    // The flags of TEST [ESI+67],2 or XOR AL,AL (both zero).
    std::uint8_t run_second_timer(logic::timers::Timers &t) {
        auto &gp=this->gp;
        this->test8(0);gp[esi]=t.block;gp[ecx]=t.timer;set_low_byte(gp[ebx],t.first);
        this->call(0x1d57aeu,0x1d57b3u,0x1e15d0u,timer);
        t.block=gp[esi];t.first=std::uint8_t(gp[ebx]);
        return std::uint8_t(gp[eax]);
    }
    void run() {
        auto &gp=this->gp;
        this->push(0x1d5790u,gp[ebx]);
        Memory<TimersBoundary> memory{*this};
        const auto result=logic::timers::run(memory,*this);
        set_low_byte(gp[eax],result);this->test8(gp[eax]);
        gp[ebx]=this->pop(0x1d57b5u);this->ret(0x1d57b6u,0);this->publish(0x1d57b6u);
    }
};

// ---- C0910 node lights ---------------------------------------------------------
// [ESP+4] = the handle. SUB ESP,24 makes the frame: the definition, the
// result byte, the row walk (light offset, source row, rows left), the
// contribution walk (index, entry offset) and the two weights. ESI, EBX,
// EBP, EDI saved as the walk needs them. AL = the result; ADD ESP,24 decides
// the flags; RET 4.
inline constexpr Edge node_lights_edges[]{
    {0xc094au,0xc0b4du,m_unlit},{0xc095cu,0xc0b41u,m_no_rows},{0xc095cu,0xc0980u,m_rows},
    {0xc09c6u,0xc0a2fu,m_no_contributions},{0xc09d7u,0xc0a14u,m_contribution_none},
    {0xc09ecu,0xc09feu,m_contribution_weighted},{0xc09ecu,0xc09f8u,m_contribution_zero},
    {0xc0a2fu,0xc0aa6u,m_no_scales},{0xc0a45u,0xc0a98u,m_scale_none},
    {0xc0a57u,0xc0a6bu,m_scale_weighted},{0xc0a57u,0xc0a63u,m_scale_zero},
    {0xc0aa9u,0xc0ac8u,m_low_x},{0xc0ab8u,0xc0ac8u,m_high_x},{0xc0ab8u,0xc0ac4u,m_keep_x},
    {0xc0accu,0xc0ae9u,m_low_y},{0xc0ad7u,0xc0ae9u,m_high_y},{0xc0ad7u,0xc0ae4u,m_keep_y},
    {0xc0aedu,0xc0b06u,m_low_z},{0xc0af4u,0xc0b06u,m_high_z},{0xc0af4u,0xc0b01u,m_keep_z},
    {0xc0b29u,0xc099bu,m_more_rows},
};
template<class State,class Weight,class Accumulate> struct NodeLightsBoundary : Port<State> {
    using Access=logic::node_lights::Access;
    using Kept=logic::node_lights::LitRow;
    Weight &weight;Accumulate &accumulate_child;
    bool first_scale{};
    NodeLightsBoundary(State &s,std::uint32_t *c,Trace<State> *t,Weight &w,Accumulate &a)
        :Port<State>(s,c,t,node_lights_edges),weight(w),accumulate_child(a){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case table:return 0xc0917u;
        case headers:return 0xc091du;
        case object:gp[eax]=(gp[eax]&0xffffu)*3u;return 0xc0928u;
        case definition_index:return 0xc092cu;
        case tags:return 0xc092eu;
        // SHL ECX,4 (the tag row); PUSH ESI.
        case definition:gp[ecx]=(gp[ecx]&0xffffu)<<4;this->push(0xc093du,gp[esi]);return 0xc093eu;
        case lit_flag:return 0xc0942u;
        case lights_size:return 0xc0954u;
        case lights_offset:this->push(0xc095bu,gp[ebx]);return 0xc095cu;
        case light_block:return 0xc09a1u;
        case copy_red:return 0xc09afu;
        case store_red:return 0xc09b5u;
        case copy_green:return 0xc09b7u;
        case store_green:return 0xc09bau;
        case copy_blue:return 0xc09bdu;
        case store_blue:return 0xc09c0u;
        case contribution_count:return 0xc09c3u;
        case contribution_entries:return 0xc09d0u;
        case contribution:return 0xc09d7u;
        case accumulate_value:return 0xc09feu;
        case contribution_count_again:return 0xc0a1cu;
        case scale_count:return 0xc0a2fu;
        // XOR EDI,EDI starts the scale walk.
        case scale_entries:if(first_scale){gp[edi]=0;first_scale=false;}return 0xc0a40u;
        case scale:return 0xc0a45u;
        case scale_red:return 0xc0a74u;
        case scaled_red:return 0xc0a78u;
        case scale_green:return 0xc0a7cu;
        case scaled_green:return 0xc0a85u;
        case scale_blue:return 0xc0a8au;
        case scaled_blue:return 0xc0a93u;
        case scale_count_again:return 0xc0a98u;
        // XORPS XMM0 (zero) precedes the first COMISS.
        case red_test:x[0]=_mm_setzero_ps();return 0xc0aa6u;
        case limit:return 0xc0aa9u;
        case red_value:return 0xc0ab8u;
        case red_keep:return 0xc0ac4u;
        case green_test:return 0xc0ac8u;
        case red_store:return 0xc0accu;
        case green_value:return 0xc0ad7u;
        case green_keep:return 0xc0ae4u;
        case blue_test:return 0xc0ae9u;
        case green_store:return 0xc0aedu;
        case blue_value:return 0xc0af4u;
        case blue_keep:return 0xc0b01u;
        case blue_store:return 0xc0b12u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case table:case definition_index:gp[ecx]=v;break;
        case headers:case tags:gp[edx]=v;break;
        case object:case copy_blue:case accumulate_value:case scale:gp[eax]=v;break;
        case definition:gp[esi]=v;break;
        // MOV DL (the flag); XOR CL,CL.
        case lit_flag:set_low_byte(gp[edx],v);set_low_byte(gp[ecx],0);break;
        case lights_size:gp[edx]=sx16(v);break;
        case lights_offset:gp[ebx]=sx16(v)+gp[eax];break;
        case light_block:gp[ebp]=v;break;
        // MOV ESI (red); ADD EBP,ECX (the light row); MOV ECX,EBX (the lit row).
        case copy_red:gp[esi]=v;gp[ebp]+=gp[ecx];gp[ecx]=gp[ebx];break;
        case copy_green:gp[esi]=v;break;
        case contribution_entries:gp[edi]=v;break;
        case contribution:gp[eax]=v;gp[edi]+=gp[edx];break;
        case contribution_count_again:gp[ecx]=v;break;
        case scale_count:gp[eax]=v;gp[esi]=0;first_scale=true;break;
        case scale_entries:gp[eax]=v+gp[edi];break;
        // INC ESI; ADD EDI,28 step the scale walk.
        case scale_count_again:gp[eax]=v;++gp[esi];gp[edi]+=0x28u;break;
        case limit:x[2]=scalar(v);break;
        default:break;
        }
    }
    // The colour stores leave XMM1 (or XMM0) as the original: red's product
    // keeps the weight register's upper lanes (MOVAPS then MULSS); the others
    // are MOVSS loads, so their upper lanes are zero.
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case scaled_red:x[1]=with_lane0(x[0],v);break;
        case scaled_green:case scaled_blue:case red_store:case green_store:x[1]=scalar(v);break;
        case blue_store:x[0]=scalar(v);break;
        default:break;
        }
    }
    // SUB ESP,24 (in run); the handle at [ESP+28].
    std::uint32_t handle(){auto &gp=this->gp;gp[eax]=this->read(0xc0913u,gp[esp]+0x28u,4);return gp[eax];}
    void keep_definition(std::uint32_t definition){this->store(0xc094au,this->gp[esp]+0x24u,4,definition);}
    std::uint32_t definition(){auto &gp=this->gp;gp[esi]=this->read(0xc099bu,gp[esp]+0x30u,4);gp[edx]=0;return gp[esi];}
    // PUSH EBP; the source row ([ESP+18]); PUSH EDI; the result byte (1); the
    // light offset (0); the lit rows (EBX += rows * 12); the rows left.
    void begin_rows(std::uint32_t,std::uint32_t rows) {
        auto &gp=this->gp;
        gp[eax]=rows;gp[edx]=0;
        this->push(0xc0980u,gp[ebp]);gp[ecx]=gp[eax]*3u;this->store(0xc0984u,gp[esp]+0x18u,4,gp[ebx]);
        this->push(0xc0988u,gp[edi]);this->store(0xc0989u,gp[esp]+0x13u,1,1);this->store(0xc098eu,gp[esp]+0x18u,4,gp[edx]);
        gp[ebx]+=gp[ecx]*4u;this->store(0xc0995u,gp[esp]+0x2cu,4,gp[eax]);
    }
    std::uint32_t current_light_offset(){auto &gp=this->gp;gp[ecx]=this->read(0xc09a7u,gp[esp]+0x18u,4);return gp[ecx];}
    std::uint32_t current_source_row(){auto &gp=this->gp;gp[eax]=this->read(0xc09abu,gp[esp]+0x1cu,4);return gp[eax];}
    std::uint32_t light_offset(){auto &gp=this->gp;gp[esi]=this->read(0xc0b06u,gp[esp]+0x18u,4);return gp[esi];}
    std::uint32_t source_row(){auto &gp=this->gp;gp[ecx]=this->read(0xc0b0au,gp[esp]+0x1cu,4);return gp[ecx];}
    std::uint32_t remaining(){auto &gp=this->gp;gp[eax]=this->read(0xc0b0eu,gp[esp]+0x2cu,4);return gp[eax];}
    // ADD ESI,10; ADD ECX,C; ADD EBX,C; DEC EAX; the three slots.
    void next_row(std::uint32_t light,std::uint32_t source,std::uint32_t left) {
        auto &gp=this->gp;
        gp[esi]=light;gp[ecx]=source;gp[ebx]+=0xcu;gp[eax]=this->flags.increment(gp[eax],4,true);
        this->store(0xc0b21u,gp[esp]+0x18u,4,light);this->store(0xc0b25u,gp[esp]+0x1cu,4,source);this->store(0xc0b29u,gp[esp]+0x2cu,4,left);
    }
    void reset_contribution_index(){this->store(0xc09c6u,this->gp[esp]+0x24u,4,this->gp[edx]);}
    void reset_contribution_offset(){this->store(0xc09ccu,this->gp[esp]+0x14u,4,this->gp[edx]);}
    std::uint32_t contribution_entry_offset(){auto &gp=this->gp;gp[edx]=this->read(0xc09d3u,gp[esp]+0x14u,4);return gp[edx];}
    std::uint32_t contribution_index(){auto &gp=this->gp;gp[eax]=this->read(0xc0a14u,gp[esp]+0x24u,4);return gp[eax];}
    std::uint32_t contribution_offset(){auto &gp=this->gp;gp[edx]=this->read(0xc0a18u,gp[esp]+0x14u,4);return gp[edx];}
    void next_contribution(std::uint32_t index,std::uint32_t offset) {
        auto &gp=this->gp;gp[eax]=index;gp[edx]=offset;
        this->store(0xc0a25u,gp[esp]+0x24u,4,index);this->store(0xc0a29u,gp[esp]+0x14u,4,offset);
    }
    // BCB70(handle argument, function, weight slot): TEST EAX,EAX (the
    // function); the walk lives in EBP (light row) and EBX (lit row), with
    // the entry in EDI, or the scale index and offset in ESI and EDI. TEST
    // AL,AL after.
    bool weigh(std::uint32_t function,std::uint32_t handle_pc,std::uint32_t slot,std::uint32_t pc) {
        auto &gp=this->gp;
        this->test32(function);gp[eax]=function;gp[edx]=this->read(handle_pc,gp[esp]+0x38u,4);gp[ecx]=gp[esp]+slot;
        this->push(handle_pc+8u,gp[ecx]);this->push(handle_pc+9u,gp[eax]);this->push(handle_pc+10u,gp[edx]);
        this->call(pc,pc+5u,0xbcb70u,weight);
        this->test8(gp[eax]);
        return (gp[eax]&0xffu)!=0;
    }
    bool weigh_contribution(Kept &r,std::uint32_t function) {
        auto &gp=this->gp;gp[edi]=r.entry;gp[ebp]=r.light_row;gp[ebx]=r.lit_row;
        const bool weighed=weigh(function,0xc09e1u,0x20u,0xc09ecu);
        r.entry=gp[edi];r.light_row=gp[ebp];r.lit_row=gp[ebx];
        return weighed;
    }
    void clear_contribution_weight(){this->s.xmm[0]=_mm_setzero_ps();this->store(0xc09f8u,this->gp[esp]+0x20u,4,0);}
    std::uint32_t contribution_weight() {
        const auto v=this->read(0xc0a01u,this->gp[esp]+0x20u,4);this->s.xmm[5]=scalar(v);return v;
    }
    // 134730: EAX = the entry's value, ESI/ECX = its two accumulate fields,
    // EDI = the lit row, XMM5 = the weight.
    void accumulate(Kept &r,std::uint32_t value,std::uint32_t weight) {
        auto &gp=this->gp;
        gp[edi]=r.entry;gp[ebp]=r.light_row;gp[ebx]=r.lit_row;gp[eax]=value;this->s.xmm[5]=scalar(weight);
        gp[esi]=gp[edi]+0x14u;gp[ecx]=gp[edi]+8u;gp[edi]=gp[ebx];
        this->call(0xc0a0fu,0xc0a14u,0x134730u,accumulate_child);
        r.light_row=gp[ebp];r.lit_row=gp[ebx];
    }
    bool weigh_scale(Kept &r,std::uint32_t function) {
        auto &gp=this->gp;gp[esi]=r.scale_index;gp[edi]=r.scale_offset;gp[ebp]=r.light_row;gp[ebx]=r.lit_row;
        const bool weighed=weigh(function,0xc0a4cu,0x28u,0xc0a57u);
        r.scale_index=gp[esi];r.scale_offset=gp[edi];r.light_row=gp[ebp];r.lit_row=gp[ebx];
        return weighed;
    }
    void clear_scale_weight(){this->s.xmm[0]=_mm_setzero_ps();this->store(0xc0a63u,this->gp[esp]+0x28u,4,0);}
    std::uint32_t scale_weight(){const auto v=this->read(0xc0a6bu,this->gp[esp]+0x28u,4);this->s.xmm[0]=scalar(v);return v;}
    std::uint32_t result(){auto &gp=this->gp;set_low_byte(gp[eax],this->read(0xc0b33u,gp[esp]+0x13u,1));return gp[eax]&0xffu;}
    void run() {
        auto &gp=this->gp;
        gp[esp]-=0x24u;
        Memory<NodeLightsBoundary> memory{*this};
        const auto result=logic::node_lights::run(memory,*this);
        std::uint32_t last;
        switch(result.exit) {
        case logic::node_lights::Exit::unlit:
            this->test8(gp[edx]&1u);set_low_byte(gp[eax],gp[ecx]);gp[esi]=this->pop(0xc0b4du);last=0xc0b51u;break;
        case logic::node_lights::Exit::no_rows:
            gp[eax]=result.rows;gp[edx]=0;this->compare32(gp[eax],gp[edx]);
            gp[ebx]=this->pop(0xc0b41u);set_low_byte(gp[eax],gp[ecx]);gp[esi]=this->pop(0xc0b44u);last=0xc0b48u;break;
        default:
            gp[edi]=this->pop(0xc0b37u);gp[ebp]=this->pop(0xc0b38u);gp[ebx]=this->pop(0xc0b39u);gp[esi]=this->pop(0xc0b3au);
            last=0xc0b3eu;break;
        }
        this->flags.plus(gp[esp],0x24u,4);gp[esp]+=0x24u;
        this->ret(last,4);this->publish(last);
    }
};

// ---- Verification summary and bridges -------------------------------------
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls[8]{};
    ~VerifySummary() {
        std::uint64_t any=0;for(const auto &c:calls)any|=c.load();
        if(any)std::fprintf(stderr,"Object subtree verify: BE850 %llu, BE610 %llu, C0F60 %llu, 10DD80 %llu, C0200 %llu, "
            "1D5790 %llu, 1E15D0 %llu, C0910 %llu calls; %llu ordered accesses (%llu stores) and %llu child boundaries identical, "
            "coverage %llx/%llx\n",
            (unsigned long long)calls[0].load(),(unsigned long long)calls[1].load(),(unsigned long long)calls[2].load(),
            (unsigned long long)calls[3].load(),(unsigned long long)calls[4].load(),(unsigned long long)calls[5].load(),
            (unsigned long long)calls[6].load(),(unsigned long long)calls[7].load(),(unsigned long long)accesses.load(),(unsigned long long)stores.load(),
            (unsigned long long)children.load(),(unsigned long long)move_coverage().load(),
            (unsigned long long)attach_coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// C0910's SSE runs as host MULSS/COMISS on the live MXCSR (asm-ordered).
inline constexpr bool native_cluster_backend() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return true;
#else
    return false;
#endif
}
template<class State> bool admit(const State &s,std::uint32_t first,std::uint32_t last,std::uint32_t *&cursor) {
    return native_object_tick_detail::admitted(s,first,last,cursor,reference_mode());
}
template<class State,class Reference,class Required,class Cluster,class Clear,class Types,class Attachment,class Self,class Nodes>
bool bridge_be850(State &s,Reference reference,Required required,Cluster cluster,Clear clear,Types types,
                  Attachment attachment,Self self,Nodes nodes) {
    std::uint32_t *cursor;if(!admit(s,0xbe850u,0xbe997u,cursor))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),move_coverage(),summary().calls[0],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return ObjectMoveBoundary<State,Required,Cluster,Clear,Types,Attachment,Self,Nodes>(v,c,t,required,cluster,clear,types,
                attachment,self,nodes);});
    return true;
}
template<class State,class Reference,class Dispatch,class Moving,class Bounds,class Extent,class Collide,class Release>
bool bridge_be610(State &s,Reference reference,Dispatch dispatch,Moving moving,Bounds bounds,Extent extent,Collide collide,
                  Release release) {
    std::uint32_t *cursor;if(!admit(s,0xbe610u,0xbe7d3u,cursor))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),move_coverage(),summary().calls[1],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return ClusterMoveBoundary<State,Dispatch,Moving,Bounds,Extent,Collide,Release>(v,c,t,dispatch,moving,bounds,extent,
                collide,release);});
    return true;
}
template<class State,class Reference,class Birth,class First,class Second,class Placement,class Self>
bool bridge_c0f60(State &s,Reference reference,Birth birth,First first,Second second,Placement placement,Self self) {
    std::uint32_t *cursor;if(!admit(s,0xc0f60u,0xc1076u,cursor))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),attach_coverage(),summary().calls[2],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return AttachmentRefreshBoundary<State,Birth,First,Second,Placement,Self>(v,c,t,birth,first,second,placement,self);});
    return true;
}
template<class State,class Reference,class Dispatch> bool bridge_10dd80(State &s,Reference reference,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admit(s,0x10dd80u,0x10ddd0u,cursor))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),move_coverage(),summary().calls[3],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return TypeMovesBoundary<State,Dispatch>(v,c,t,dispatch);});
    return true;
}
template<class State,class Reference,class Matrices,class Notify>
bool bridge_c0200(State &s,Reference reference,Matrices matrices,Notify notify) {
    std::uint32_t *cursor;if(!admit(s,0xc0200u,0xc0267u,cursor))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),move_coverage(),summary().calls[4],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return NodeNotifyBoundary<State,Matrices,Notify>(v,c,t,matrices,notify);});
    return true;
}
template<class State,class Reference,class Timer> bool bridge_1d5790(State &s,Reference reference,Timer timer) {
    std::uint32_t *cursor;if(!admit(s,0x1d5790u,0x1d57b6u,cursor))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),attach_coverage(),summary().calls[5],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return TimersBoundary<State,Timer>(v,c,t,timer);});
    return true;
}
template<class State,class Reference> bool bridge_1e15d0(State &s,Reference reference) {
    std::uint32_t *cursor;if(!admit(s,0x1e15d0u,0x1e1615u,cursor))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),attach_coverage(),summary().calls[6],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return CountdownBoundary<State>(v,c,t);});
    return true;
}
template<class State,class Reference,class Weight,class Accumulate>
bool bridge_c0910(State &s,Reference reference,Weight weight,Accumulate accumulate) {
    if(!native_cluster_backend())return false;
    std::uint32_t *cursor;if(!admit(s,0xc0910u,0xc0b51u,cursor))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),attach_coverage(),summary().calls[7],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return NodeLightsBoundary<State,Weight,Accumulate>(v,c,t,weight,accumulate);});
    return true;
}
} // namespace halo2::native_object_subtree_detail
