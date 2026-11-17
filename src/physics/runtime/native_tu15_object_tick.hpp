#pragma once
// Boundary adapters for the per-tick object update systems
// (native_tu15_object_tick_systems.hpp): B8F40 update pass, B9090 move pass,
// B9180 post-update pass, BE4A0 one object's update, 10DD10 type updates.
// Their callers and children are still translated TU1.5 code in the live
// support unit, so each adapter owns that ABI and nothing else:
//   - arguments from stack slots, results back into the original's registers
//     (including partial-register writes and the values the original leaves
//     in scratch registers);
//   - the original's register file at every child call: each access role's
//     register effects, the "kept" values the logic hands to a child call
//     (loaded into their callee-saved registers before it, read back after
//     it), and the flags of the instruction that decides them;
//   - the original's stack frame: register saves and restores, the frame
//     slots whose reads and writes are part of the original's access order,
//     return addresses, and the stack pointer as the children actually leave
//     it;
//   - the diagnostic cursor: each access publishes its original instruction
//     (a per-owner table of access role -> instruction), and the RET publishes
//     the exit;
//   - every store goes through the unit's store helper, so guards and the
//     store observer see the original store stream.
// Delete an adapter when its callers and children go native.
//
// Every child stays genuine and is entered through the unit's own original
// route (direct body or dispatch) with the exact register, flag, XMM, x87 and
// stack ABI; its results are read back.
// HALO2_REFERENCE_OBJECT_TICK=1 restores the translated bodies.
// HALO2_VERIFY_OBJECT_TICK=1 runs, for every admitted call, the native owner
// on the live bank (children genuine, run once) while recording its ordered
// accesses and child boundaries, then replays the unmodified translated body:
// its reads are served from the native record and must have the same order,
// address and width, its stores (pc, address, bytes) must be identical, each
// child call must have the same target, position and complete entry ABI (the
// child's recorded results are applied), and the final registers, flags, XMM
// and x87 state must be identical. A summary is printed at exit.
//
// This header also holds the verification machinery (Registers, Trace,
// Replay, admitted, execute) and the boundary framework (boundary::Port,
// boundary::Memory) that the object subtree and cluster distance adapters
// share. `Machine` is the register-file base of the closures that are not
// yet rewritten to the code standard (groups E and F); new code uses
// boundary::Port.
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <xmmintrin.h>

namespace halo2::native_object_tick_detail {
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_TICK");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_TICK");return p && *p;}();
    return value;
}

// Branch outcomes reached by the owners (verification coverage). The
// adapters read them from the order of the original instructions (each
// owner's edge table) and from the scans' ends; the logic carries no
// instrumentation.
enum Branch : unsigned {
    // B8F40
    a_roots_empty, a_root_skip, a_root_update, a_root_attach, a_root_attach_skip,
    a_scan_negative, a_scan_bound, a_scan_end, a_scan_invalid, a_object_skip, a_object_disabled, a_object_update,
    // B9090
    b_roots_empty, b_root_skip, b_root_move, b_scan_negative, b_scan_bound, b_scan_end, b_scan_invalid,
    b_object_skip, b_object_disabled, b_object_move,
    // B9180
    c_scan_negative, c_scan_bound, c_scan_end, c_scan_invalid, c_object_quiet, c_object_skip, c_object_update,
    c_object_moved, c_object_still, c_late_negative, c_late_bound, c_late_end, c_late_invalid, c_late_skip, c_late_update,
    // BE4A0
    d_inactive, d_deleted, d_parent, d_no_parent, d_lights, d_no_lights, d_no_owner, d_owner_same, d_owner_child,
    d_children, d_no_children, d_cluster, d_no_cluster_flag, d_no_cluster, d_countdown, d_changed, d_kept, d_quiet,
    // 10DD10
    e_none, e_callback, e_empty_callback, e_more,
    branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}

inline std::uint32_t sx16(std::uint32_t x){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(x))));}
inline std::uint32_t sx8(std::uint32_t x){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(x))));}
inline std::int32_t si(std::uint32_t x){return std::int32_t(x);}
inline std::uint32_t lo8(std::uint32_t r,std::uint32_t v){return (r&0xffffff00u)|(v&0xffu);}
inline std::uint32_t sar(std::uint32_t x,unsigned n){return std::uint32_t(std::int32_t(x)>>n);}
inline std::uint32_t width_mask(unsigned n){return n>=4?0xffffffffu:(1u<<(n*8u))-1u;}

// Complete register file of a child boundary or exit.
template<class State> struct Registers {
    std::array<std::uint32_t,8> gp{};
    std::remove_cvref_t<decltype(std::declval<State&>().flags)> flags{};
    std::uint32_t other{},fs{},mxcsr{};
    std::array<std::uint32_t,32> xmm{};
    decltype(std::declval<State&>().fp) fp{};
    static Registers capture(const State &s) {
        Registers r;r.gp=s.gp;r.flags=s.flags;r.other=s.other_flags;r.fs=s.fs_base;r.fp=s.fp;r.mxcsr=_mm_getcsr();
        std::memcpy(r.xmm.data(),s.xmm,sizeof r.xmm);return r;
    }
    void apply(State &s)const {
        s.gp=gp;s.flags=flags;s.other_flags=other;s.fs_base=fs;s.fp=fp;std::memcpy(s.xmm,xmm.data(),sizeof xmm);_mm_setcsr(mxcsr);
    }
    bool same(const Registers &o)const {
        bool equal=gp==o.gp && flags.bits()==o.flags.bits() && other==o.other && fs==o.fs && mxcsr==o.mxcsr && xmm==o.xmm &&
            fp.count==o.fp.count && fp.status==o.fp.status;
        if constexpr(requires{fp.top;})equal=equal && fp.top==o.fp.top;
        for(unsigned i=0;i<fp.count && i<8;++i)equal=equal && !std::memcmp(&fp.lanes[i],&o.fp.lanes[i],10);
        return equal;
    }
};
struct Event {
    std::uint32_t kind,pc,address,width,value; // kind 0 read, 1 store
};
template<class State> struct Trace {
    struct Call {std::uint32_t target;std::size_t at;Registers<State> entry,exit;};
    std::vector<Event> events;std::vector<Call> calls;
};

// The register-file closure base of the owners not yet rewritten to the code
// standard (groups E and F). Do not use it for new owners.
template<class State> struct Machine {
    using F=std::remove_cvref_t<decltype(std::declval<State&>().flags)>;
    State &s;std::uint32_t *cursor{};Trace<State> *trace{};
    std::uint64_t hit{};
    std::uint32_t eax{},ecx{},edx{},ebx{},esp{},ebp{},esi{},edi{};
    F f{};
    void mark(unsigned b){hit|=std::uint64_t(1)<<b;}
    std::uint32_t rd(std::uint32_t pc,std::uint32_t a,unsigned n=4) {
        if(cursor)*cursor=pc;
        const auto v=physics_step_detail::r(s.memory,a,n);
        if(trace)trace->events.push_back({0,pc,a,n,v});
        return v;
    }
    void st(std::uint32_t pc,std::uint32_t a,unsigned n,std::uint32_t v) {
        if(cursor)*cursor=pc;
        physics_step_detail::w(s.memory,s.observe,pc,a,n,v);
        if(trace)trace->events.push_back({1,pc,a,n,v&width_mask(n)});
    }
    void push(std::uint32_t pc,std::uint32_t v){esp-=4u;st(pc,esp,4,v);}
    std::uint32_t pop(std::uint32_t pc){const auto v=rd(pc,esp);esp+=4u;return v;}
    void ret(std::uint32_t pc,std::uint32_t bytes){rd(pc,esp);esp+=4u+bytes;}
    void load() {
        eax=s.gp[0];ecx=s.gp[1];edx=s.gp[2];ebx=s.gp[3];esp=s.gp[4];ebp=s.gp[5];esi=s.gp[6];edi=s.gp[7];f=s.flags;
    }
    void out(){s.gp={eax,ecx,edx,ebx,esp,ebp,esi,edi};s.flags=f;}
    void publish(std::uint32_t final_pc){out();if(cursor)*cursor=final_pc;}
    template<class Child> void enter(std::uint32_t target,Child &&child) {
        out();
        if(trace)trace->calls.push_back({target,trace->events.size(),Registers<State>::capture(s),{}});
        child(s);
        if(trace)trace->calls.back().exit=Registers<State>::capture(s);
        load();
    }
    template<class Child> void call(std::uint32_t pc,std::uint32_t next,std::uint32_t target,Child &&child) {
        push(pc,next);enter(target,child);
    }
    void test8(std::uint32_t x){f.logic(x&0xffu,1);}
    void test32(std::uint32_t x){f.logic(x,4);}
    void cmp32(std::uint32_t a,std::uint32_t b){f.minus(a,b,4);}
};

// ---- Boundary framework -----------------------------------------------------
namespace boundary {
enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
inline void set_low_byte(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffffff00u)|(value&0xffu);}
inline std::uint32_t low_byte(std::uint32_t reg){return reg&0xffu;}

// Coverage: the branch outcome reached when the original instruction `to`
// follows `from` among the owner's accesses (any instruction when from is 0).
struct Edge {std::uint32_t from,to;unsigned branch;};

// The original's memory, general registers, flags and cursor as one owner
// call sees them. Reads and stores use the unit's helpers; each first
// publishes its instruction to the cursor (so a faulting access reports the
// original instruction) and, in verify mode, is recorded.
template<class State> struct Port {
    using Flags=std::remove_cvref_t<decltype(std::declval<State&>().flags)>;
    State &s;std::uint32_t *cursor;Trace<State> *trace;
    std::span<const Edge> edges;
    std::array<std::uint32_t,8> gp{};Flags flags{};
    std::uint64_t hit{};std::uint32_t previous{};
    Port(State &state,std::uint32_t *c,Trace<State> *t,std::span<const Edge> e):s(state),cursor(c),trace(t),edges(e){}
    // execute() loads the entry registers, runs the owner and reads `hit`.
    void load(){gp=s.gp;flags=s.flags;}
    void reached(unsigned branch){hit|=std::uint64_t(1)<<branch;}
    void at(std::uint32_t pc) {
        if(cursor)*cursor=pc;
        for(const auto &e:edges)if(e.to==pc && (!e.from || e.from==previous))reached(e.branch);
        previous=pc;
    }
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width) {
        at(pc);
        const auto value=physics_step_detail::r(s.memory,address,width);
        if(trace)trace->events.push_back({0,pc,address,width,value});
        return value;
    }
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value) {
        at(pc);
        physics_step_detail::w(s.memory,s.observe,pc,address,width,value);
        if(trace)trace->events.push_back({1,pc,address,width,value&width_mask(width)});
    }
    void push(std::uint32_t pc,std::uint32_t value){gp[esp]-=4u;store(pc,gp[esp],4,value);}
    std::uint32_t pop(std::uint32_t pc){const auto value=read(pc,gp[esp],4);gp[esp]+=4u;return value;}
    // RET n: the return address read, then ESP past it and n argument bytes.
    void ret(std::uint32_t pc,std::uint32_t bytes){read(pc,gp[esp],4);gp[esp]+=4u+bytes;}
    // A genuine translated child, entered through the unit's own route (after
    // the CALL's return address is pushed): it runs on the live state with
    // the exact register, flag, XMM and x87 ABI, and every register comes back.
    template<class Child> void enter(std::uint32_t target,Child &&child) {
        s.gp=gp;s.flags=flags;
        if(trace)trace->calls.push_back({target,trace->events.size(),Registers<State>::capture(s),{}});
        child(s);
        if(trace)trace->calls.back().exit=Registers<State>::capture(s);
        gp=s.gp;flags=s.flags;
    }
    template<class Child> void call(std::uint32_t pc,std::uint32_t next,std::uint32_t target,Child &&child) {
        push(pc,next);enter(target,child);
    }
    // A child through a route (dispatch or the required owner) at its site.
    template<class Route> void call_routed(Route &route,std::uint32_t pc,std::uint32_t next,std::uint32_t target) {
        push(pc,next);enter(target,[&](State &v){route(v,target,pc);});
    }
    void publish(std::uint32_t final_pc){s.gp=gp;s.flags=flags;if(cursor)*cursor=final_pc;}
    // The flags of the instructions that decide them where a child or the
    // caller observes them.
    void test8(std::uint32_t x){flags.logic(x&0xffu,1);}
    void test32(std::uint32_t x){flags.logic(x,4);}
    void compare32(std::uint32_t a,std::uint32_t b){flags.minus(a,b,4);}
};

// The logic's Memory: typed accesses through an owner's boundary. The
// boundary names the original instruction of each access role
// (instruction(role, address), which also performs the register saves the
// original places just before that access) and applies the original
// instruction's register effects (loaded / stored).
template<class Boundary> struct Memory {
    Boundary &b;
    template<class A> std::uint32_t load(A a,std::uint32_t address,unsigned width) {
        const auto value=b.read(b.instruction(a,address),address,width);
        b.loaded(a,address,value);
        return value;
    }
    template<class A> void store(A a,std::uint32_t address,unsigned width,std::uint32_t value) {
        b.store(b.instruction(a,address),address,width,value);
        b.stored(a,address,value);
    }
    template<class A> std::uint32_t load32(A a,std::uint32_t address){return load(a,address,4);}
    template<class A> std::uint32_t load16(A a,std::uint32_t address){return load(a,address,2);}
    template<class A> std::uint32_t load8(A a,std::uint32_t address){return load(a,address,1);}
    template<class A> void store32(A a,std::uint32_t address,std::uint32_t value){store(a,address,4,value);}
    template<class A> void store16(A a,std::uint32_t address,std::uint32_t value){store(a,address,2,value&0xffffu);}
    template<class A> void store8(A a,std::uint32_t address,std::uint32_t value){store(a,address,1,value&0xffu);}
};
} // namespace boundary

namespace logic=halo2::native_object_update;
using namespace boundary;

// The registers of the original's inlined data_array search, kept by the
// scan accesses (instruction: before the read; loaded: its result).
struct SearchRegisters {
    bool first_probe{};
    // MOV limit/bits; the probe: AND/SHL the mask, SAR the word, TEST.
    template<class Gp> void probe(Gp &gp,unsigned position,unsigned mask,unsigned word) {
        if(!first_probe)++gp[position];
        first_probe=false;
        gp[mask]=1u<<(gp[position]&31u);
        gp[word]=sar(gp[position],5);
    }
};
// Coverage and flags at a search's end: TEST of a negative start, or CMP of
// the position against the limit (or NONE).
template<class P> void search_end_flags(P &port,const logic::Search &search,unsigned limit) {
    switch(*search.end) {
    case logic::SearchEnd::negative_start:port.test32(search.index);break;
    case logic::SearchEnd::past_limit:
    case logic::SearchEnd::bits_exhausted:port.compare32(search.index,port.gp[limit]);break;
    case logic::SearchEnd::invalid_index:port.compare32(search.index,logic::none);break;
    }
}
inline unsigned search_branch(const logic::Search &search,unsigned negative) {
    return negative+unsigned(*search.end);  // negative, bound, end, invalid are consecutive
}

// ---- B8F40 update pass --------------------------------------------------------
// No arguments. PUSH ECX makes the slot that keeps the root count. At exit
// the object globals are in EAX, ECX or EDX by the scan's end (with that
// end's TEST/CMP flags), EDI/ESI/EBP/EBX are restored and ECX is popped from
// the slot. RET.
inline constexpr Edge update_pass_edges[]{
    {0xb8f64u,0xb8fb8u,a_roots_empty},{0xb8f85u,0xb8f98u,a_root_update},{0xb8f85u,0xb8fafu,a_root_skip},
    {0xb8f9eu,0xb8fa9u,a_root_attach},{0xb8f9eu,0xb8fafu,a_root_attach_skip},
    {0xb901eu,0xb8fccu,a_object_skip},{0xb901eu,0xb906bu,a_object_skip},
    {0xb904eu,0xb8fccu,a_object_disabled},{0xb904eu,0xb906bu,a_object_disabled},{0xb904eu,0xb9060u,a_object_update},
};
template<class State,class Update,class Attach> struct UpdatePass : Port<State> {
    using Access=logic::update_pass::Access;
    Update &update;Attach &attach;
    SearchRegisters search;std::optional<logic::SearchEnd> end;
    UpdatePass(State &s,std::uint32_t *c,Trace<State> *t,Update &u,Attach &a):Port<State>(s,c,t,update_pass_edges),update(u),attach(a){}
    // The exit's instructions differ by how the scan ended.
    std::uint32_t exit_pc(std::uint32_t negative,std::uint32_t past,std::uint32_t other)const {
        return *end==logic::SearchEnd::negative_start?negative:*end==logic::SearchEnd::past_limit?past:other;
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case roots:return 0xb8f41u;
        case state:return 0xb8f46u;
        case root_count:this->push(0xb8f4cu,gp[ebx]);this->push(0xb8f4du,gp[ebp]);gp[ebx]=gp[eax]+0x1d00u;return 0xb8f54u;
        case updating_set:return 0xb8f64u;
        case root_handle:return 0xb8f70u;
        case root_table:return 0xb8f7du;
        case root_headers:return 0xb8f82u;
        case root_flags:return 0xb8f85u;
        case root_flags_after:return 0xb8f9eu;
        case scan_table:return 0xb8fb8u;
        case scan_limit:gp[eax]=gp[edi]+1u;return 0xb8fccu;
        case scan_bits:return 0xb8fd7u;
        case scan_word:search.probe(gp,eax,ebx,ecx);return 0xb8ff1u;
        case datum_size:return 0xb9010u;
        case datum_data:return 0xb9013u;
        case datum_identifier:return 0xb901bu;
        case datum_flags:return 0xb901eu;
        case object_table:return 0xb9037u;
        case object_headers:return 0xb903du;
        case object:return 0xb904au;
        case object_flags:return 0xb904eu;
        case state_again:return exit_pc(0xb906bu,0xb907au,0xb8ffbu);
        case updating_clear: {
            const auto pops=exit_pc(0xb9070u,0xb9080u,0xb9001u);
            gp[edi]=this->pop(pops);gp[esi]=this->pop(pops+1u);gp[ebp]=this->pop(pops+2u);
            return exit_pc(0xb9073u,0xb9083u,0xb9004u);
        }
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case roots:case root_count:case root_table:case object:gp[eax]=v;break;
        case state:case root_headers:case datum_size:case object_table:gp[ecx]=v;break;
        case root_handle:gp[esi]=v;gp[edx]=(v&0xffffu)*3u;break;
        case root_flags:set_low_byte(gp[eax],v);gp[edi]=gp[ecx]+gp[edx]*4u;break;
        case root_flags_after:set_low_byte(gp[eax],v);break;
        case scan_table:gp[esi]=v;gp[edi]=logic::none;break;
        case scan_limit:gp[edi]=v;break;
        case scan_bits:gp[edx]=v;search.first_probe=true;break;
        case datum_data:gp[edx]=v;gp[ecx]=gp[ecx]*gp[eax]+v;break;
        case datum_identifier:gp[edx]=sx16(v);break;
        case datum_flags:set_low_byte(gp[ecx],v);gp[edx]=(gp[edx]<<16)|gp[eax];gp[edi]=gp[eax];break;
        case object_headers:gp[ecx]=v;gp[eax]=(gp[edx]&0xffffu)*3u;break;
        case object_flags:set_low_byte(gp[ecx],(v&0xffu)>>3);break;
        case state_again:gp[*end==logic::SearchEnd::negative_start?eax:*end==logic::SearchEnd::past_limit?ecx:edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // Frame. The root count lives in the PUSH ECX slot ([ESP+10] after the
    // four register saves).
    void keep_root_count(std::uint32_t count) {
        auto &gp=this->gp;
        this->push(0xb8f5au,gp[esi]);gp[ebp]=0;this->push(0xb8f5fu,gp[edi]);
        this->store(0xb8f60u,gp[esp]+0x10u,4,count);
    }
    // MOV EAX,[ESP+10]; INC EBP; CMP EBP,EAX.
    std::uint32_t root_count() {
        auto &gp=this->gp;
        gp[eax]=this->read(0xb8fafu,gp[esp]+0x10u,4);++gp[ebp];this->compare32(gp[ebp],gp[eax]);
        return gp[eax];
    }
    // The walk lives in EBX (list), EBP (position), ESI (handle), EDI (header).
    void keep(const logic::update_pass::RootWalk &w){auto &gp=this->gp;gp[ebx]=w.list;gp[ebp]=w.position;gp[esi]=w.handle;gp[edi]=w.header;}
    void take(logic::update_pass::RootWalk &w)const{const auto &gp=this->gp;w.list=gp[ebx];w.position=gp[ebp];w.handle=gp[esi];w.header=gp[edi];}
    // TEST AL,8 decides the flags; PUSH ESI is the handle argument.
    void update_root(logic::update_pass::RootWalk &w) {
        keep(w);this->test8(this->gp[eax]&logic::header_flags::late_update);this->push(0xb8f98u,this->gp[esi]);
        this->call(0xb8f99u,0xb8f9eu,0xbe4a0u,update);take(w);
    }
    void attach_root(logic::update_pass::RootWalk &w) {
        keep(w);this->test8(this->gp[eax]&logic::header_flags::late_update);this->push(0xb8fa9u,this->gp[esi]);
        this->call(0xb8faau,0xb8fafu,0x10e630u,attach);take(w);
    }
    // The scan lives in ESI (array) and EDI (index); TEST CL,1 (the object's
    // disabled bit, shifted down) decides the flags; PUSH EDX is the handle.
    void update_object(std::uint32_t handle,logic::ObjectScan &scan) {
        auto &gp=this->gp;
        gp[esi]=scan.array;gp[edi]=scan.index;gp[edx]=handle;
        this->test8(gp[ecx]&1u);this->push(0xb9060u,gp[edx]);
        this->call(0xb9061u,0xb9066u,0xbe4a0u,update);
        scan.array=gp[esi];scan.index=gp[edi];
    }
    void search_ended(const logic::Search &s) {
        end=s.end;this->gp[eax]=s.index;search_end_flags(*this,s,edi);this->reached(search_branch(s,a_scan_negative));
    }
    void run() {
        auto &gp=this->gp;
        this->push(0xb8f40u,gp[ecx]);
        Memory<UpdatePass> memory{*this};
        logic::update_pass::run(memory,*this);
        const auto pops=exit_pc(0xb9077u,0xb9087u,0xb9008u);
        gp[ebx]=this->pop(pops);gp[ecx]=this->pop(pops+1u);
        this->ret(pops+2u,0);this->publish(pops+2u);
    }
};

// ---- B9090 move pass ----------------------------------------------------------
// No arguments. The exit flags are 102910's; EDX holds the object globals.
// EDI/ESI/EBP/EBX restored. RET.
inline constexpr Edge move_pass_edges[]{
    {0xb90aeu,0xb90ecu,b_roots_empty},{0xb90cdu,0xb90b4u,b_root_skip},{0xb90cdu,0xb90ecu,b_root_skip},
    {0xb90cdu,0xb90d8u,b_root_move},
    {0xb9150u,0xb90fcu,b_object_skip},{0xb9150u,0xb9121u,b_object_skip},
    {0xb915eu,0xb90fcu,b_object_disabled},{0xb915eu,0xb9121u,b_object_disabled},{0xb915eu,0xb916cu,b_object_move},
};
template<class State,class Move,class Transform,class Finish> struct MovePass : Port<State> {
    using Access=logic::move_pass::Access;
    Move &move;Transform &transform;Finish &finished;
    SearchRegisters search;
    MovePass(State &s,std::uint32_t *c,Trace<State> *t,Move &m,Transform &x,Finish &f)
        :Port<State>(s,c,t,move_pass_edges),move(m),transform(x),finished(f){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case roots:return 0xb9090u;
        case root_count:this->push(0xb9095u,gp[ebx]);this->push(0xb9096u,gp[ebp]);return 0xb9097u;
        case state:this->push(0xb909du,gp[esi]);this->push(0xb909eu,gp[edi]);gp[ebx]=gp[eax]+0x1d00u;return 0xb90a5u;
        case updating_set:return 0xb90aeu;
        case root_handle:return 0xb90b4u;
        case root_table:return 0xb90b7u;
        case root_headers:return 0xb90c7u;
        case root_flags:return 0xb90cdu;
        case scan_table:return 0xb90ecu;
        case scan_limit:gp[eax]=gp[edi]+1u;return 0xb90fcu;
        case scan_bits:return 0xb9103u;
        case scan_word:search.probe(gp,eax,ebx,ecx);return 0xb9117u;
        case datum_size:return 0xb913au;
        case datum_data:return 0xb913du;
        case datum_identifier:return 0xb9143u;
        case datum_flags:return 0xb9150u;
        case object:return 0xb915bu;
        case object_flags:return 0xb915eu;
        case state_again:return 0xb9126u;
        case updating_clear:gp[edi]=this->pop(0xb912cu);gp[esi]=this->pop(0xb912du);gp[ebp]=this->pop(0xb912eu);return 0xb912fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case roots:case object:gp[eax]=v;break;
        case root_count:gp[ebp]=v;break;
        case state:gp[eax]=v;gp[edi]=0;break;
        case root_handle:gp[esi]=v;break;
        case root_table:gp[edx]=v;gp[ecx]=(gp[esi]&0xffffu)*3u;break;
        case root_headers:gp[eax]=v+gp[ecx]*4u;break;
        case root_flags:case datum_flags:set_low_byte(gp[eax],v);break;
        case scan_table:gp[esi]=v;gp[edi]=logic::none;break;
        case scan_limit:gp[edi]=v;break;
        case scan_bits:gp[edx]=v;search.first_probe=true;break;
        case datum_size:gp[ecx]=v;break;
        case datum_data:gp[ebp]=v;gp[ecx]*=gp[eax];break;
        case datum_identifier:gp[ecx]+=gp[ebp];gp[edx]=(sx16(v)<<16)|gp[eax];gp[edi]=gp[eax];break;
        case object_flags:set_low_byte(gp[ecx],(v&0xffu)>>3);break;
        case state_again:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The walk lives in EBX (list), EBP (count), EDI (position), ESI (handle).
    void keep(const logic::move_pass::RootWalk &w){auto &gp=this->gp;gp[ebx]=w.list;gp[ebp]=w.count;gp[edi]=w.position;gp[esi]=w.handle;}
    void take(logic::move_pass::RootWalk &w)const{const auto &gp=this->gp;w.list=gp[ebx];w.count=gp[ebp];w.position=gp[edi];w.handle=gp[esi];}
    void move_root(logic::move_pass::RootWalk &w) {
        keep(w);this->test8(this->gp[eax]&logic::header_flags::late_update);this->push(0xb90d8u,this->gp[esi]);
        this->call(0xb90d9u,0xb90deu,0xbe850u,move);take(w);
    }
    // 10E710 takes the position in ECX and the handle in EAX; the flags are
    // BE850's.
    void transform_root(logic::move_pass::RootWalk &w) {
        keep(w);this->gp[ecx]=this->gp[edi];this->gp[eax]=this->gp[esi];
        this->call(0xb90e2u,0xb90e7u,0x10e710u,transform);take(w);
    }
    void move_object(std::uint32_t handle,logic::ObjectScan &scan) {
        auto &gp=this->gp;
        gp[esi]=scan.array;gp[edi]=scan.index;gp[edx]=handle;
        this->test8(gp[ecx]&1u);this->push(0xb916cu,gp[edx]);
        this->call(0xb916du,0xb9172u,0xbe850u,move);
        scan.array=gp[esi];scan.index=gp[edi];
    }
    void search_ended(const logic::Search &s) {
        this->gp[eax]=s.index;search_end_flags(*this,s,edi);this->reached(search_branch(s,b_scan_negative));
    }
    void finish(){this->call(0xb9121u,0xb9126u,0x102910u,finished);}
    void run() {
        Memory<MovePass> memory{*this};
        logic::move_pass::run(memory,*this);
        this->gp[ebx]=this->pop(0xb9133u);
        this->ret(0xb9134u,0);this->publish(0xb9134u);
    }
};

// ---- B9180 post-update pass ---------------------------------------------------
// No arguments. Ends in JMP C13B0 at B921A (the tail returns to this pass's
// caller): EDX holds the object globals, the flags are the delete scan's
// end, EDI/ESI/EBP/EBX restored.
inline constexpr Edge late_pass_edges[]{
    {0xb9244u,0xb9267u,c_object_update},{0xb9270u,0xb927au,c_object_moved},
    {0xb9270u,0xb919du,c_object_still},{0xb9270u,0xb91cbu,c_object_still},
    {0xb92a0u,0xb91e7u,c_late_skip},{0xb92a0u,0xb920cu,c_late_skip},{0xb92a0u,0xb92aau,c_late_update},
};
template<class State,class Update,class Move,class Finish,class Detach,class Release,class Tail>
struct LatePass : Port<State> {
    using Access=logic::late_pass::Access;
    Update &update;Move &move;Finish &finished;Detach &detach;Release &release;Tail &tail;
    SearchRegisters search;bool deleting{};
    std::uint32_t late_flags{},after_flags{},doomed_flags{};
    LatePass(State &s,std::uint32_t *c,Trace<State> *t,Update &u,Move &m,Finish &f,Detach &d,Release &r,Tail &x)
        :Port<State>(s,c,t,late_pass_edges),update(u),move(m),finished(f),detach(d),release(r),tail(x){}
    // An object whose held flag was cleared and that did not update was
    // either not marked for a late update (quiet) or not eligible (skip).
    void after_held_clear() {
        if(this->previous==0xb9244u)this->reached(late_flags&logic::header_flags::late_update?c_object_skip:c_object_quiet);
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case state:return 0xb9180u;
        case scan_table:this->push(0xb9185u,gp[ebx]);this->push(0xb9186u,gp[ebp]);return 0xb9187u;
        case updating_set:this->push(0xb918du,gp[esi]);this->push(0xb918eu,gp[edi]);return 0xb918fu;
        case scan_limit:after_held_clear();gp[eax]=gp[ebx]+1u;return 0xb919du;
        case scan_bits:return 0xb91a4u;
        case scan_word:search.probe(gp,eax,edi,ecx);return 0xb91c1u;
        case datum_size:return 0xb9224u;
        case datum_data:return 0xb9227u;
        case datum_flags:return 0xb922du;
        case datum_identifier:return 0xb9233u;
        case held_clear:return 0xb9244u;
        // AND AL,F7; TEST AL,AL; PUSH EDI (the handle argument) precede the store.
        case late_clear:
            set_low_byte(gp[eax],gp[eax]&~logic::header_flags::late_update);this->test8(gp[eax]);
            this->push(0xb9267u,gp[edi]);return 0xb9268u;
        case flags_after:return 0xb9270u;
        case delete_table:return 0xb91d0u;
        case delete_limit:gp[eax]=gp[edi]+1u;return 0xb91e7u;
        case delete_bits:return 0xb91eeu;
        case delete_word:search.probe(gp,eax,edi,ecx);return 0xb9202u;
        case delete_size:return 0xb928au;
        case delete_data:return 0xb928du;
        case delete_identifier:return 0xb9293u;
        case delete_flags:return 0xb92a0u;
        case state_again:return 0xb920cu;
        case updating_clear:gp[edi]=this->pop(0xb9212u);gp[esi]=this->pop(0xb9213u);gp[ebp]=this->pop(0xb9214u);return 0xb9215u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case state:gp[eax]=v;break;
        case scan_table:gp[ebp]=v;break;
        case scan_limit:case delete_limit:gp[esi]=v;break;
        case scan_bits:case delete_bits:gp[edx]=v;search.first_probe=true;break;
        case datum_size:gp[esi]=v;break;
        case datum_data:gp[edi]=v;gp[esi]*=gp[eax];break;
        case datum_flags:set_low_byte(gp[edx],v);gp[esi]+=gp[edi];break;
        // MOVSX EDI; AND DL,DF; SHL/OR the handle; MOV EBX,EAX; MOV AL,DL.
        case datum_identifier:
            gp[edi]=(sx16(v)<<16)|gp[eax];set_low_byte(gp[edx],gp[edx]&~logic::header_flags::held);
            gp[ebx]=gp[eax];set_low_byte(gp[eax],gp[edx]);break;
        case flags_after:after_flags=v;break;
        case delete_table:gp[ebp]=v;gp[edi]=logic::none;break;
        case delete_size:gp[ecx]=v;break;
        case delete_data:gp[edx]=v;gp[ecx]*=gp[eax];break;
        case delete_identifier:gp[ecx]+=gp[edx];gp[esi]=(sx16(v)<<16)|gp[eax];gp[edi]=gp[eax];break;
        case delete_flags:doomed_flags=v;break;
        case state_again:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        if(a==Access::updating_set)this->gp[ebx]=logic::none;  // MOV EBX,-1: the scan's position
        else if(a==Access::held_clear)late_flags=v;
    }
    // The late scan lives in EBP (array), EBX (index), ESI (header), EDI (handle).
    void keep(const logic::late_pass::LateScan &l){auto &gp=this->gp;gp[ebp]=l.array;gp[ebx]=l.index;gp[esi]=l.header;gp[edi]=l.handle;}
    void take(logic::late_pass::LateScan &l)const{const auto &gp=this->gp;l.array=gp[ebp];l.index=gp[ebx];l.header=gp[esi];l.handle=gp[edi];}
    // The delete scan lives in EBP (array), EDI (index), ESI (handle).
    void keep(const logic::late_pass::DeleteScan &d){auto &gp=this->gp;gp[ebp]=d.array;gp[edi]=d.index;gp[esi]=d.handle;}
    void take(logic::late_pass::DeleteScan &d)const{const auto &gp=this->gp;d.array=gp[ebp];d.index=gp[edi];d.handle=gp[esi];}
    // The handle argument was pushed before the flag store (late_clear).
    void update_object(logic::late_pass::LateScan &l){keep(l);this->call(0xb926bu,0xb9270u,0xbe4a0u,update);take(l);}
    void move_object(logic::late_pass::LateScan &l) {
        keep(l);this->test8(after_flags&logic::header_flags::moved);this->push(0xb927au,this->gp[edi]);
        this->call(0xb927bu,0xb9280u,0xbe850u,move);take(l);
    }
    void finish_updates() {
        after_held_clear();deleting=true;
        this->call(0xb91cbu,0xb91d0u,0x102910u,finished);
    }
    void detach_object(logic::late_pass::DeleteScan &d) {
        keep(d);this->test8(doomed_flags&logic::header_flags::pending_delete);
        this->push(0xb92aau,1);this->push(0xb92acu,this->gp[esi]);this->call(0xb92adu,0xb92b2u,0xba3e0u,detach);take(d);
    }
    void release_object(logic::late_pass::DeleteScan &d) {
        keep(d);this->push(0xb92b2u,1);this->push(0xb92b4u,this->gp[esi]);this->call(0xb92b5u,0xb92bau,0xba490u,release);take(d);
    }
    void search_ended(const logic::Search &s) {
        this->gp[eax]=s.index;search_end_flags(*this,s,esi);
        this->reached(search_branch(s,deleting?c_late_negative:c_scan_negative));
    }
    // POP EBX, then JMP C13B0: the tail child returns to this pass's caller.
    void finish() {
        this->gp[ebx]=this->pop(0xb9219u);
        if(this->cursor)*this->cursor=0xb921au;
        this->enter(0xc13b0u,tail);
    }
    void run() {
        Memory<LatePass> memory{*this};
        logic::late_pass::run(memory,*this);
    }
};

// ---- BE4A0 one object ---------------------------------------------------------
// [ESP+4] = the handle (the slot is rewritten with the header). AL = the
// result (asleep 0, held 1, counting down 1, else `changed`) over the
// original's EAX; the flags of the deciding TEST or DEC. EBX/ESI/EDI/EBP
// restored. RET 4.
inline constexpr Edge object_update_edges[]{
    {0xbe4c6u,0xbe602u,d_inactive},{0xbe4c6u,0xbe4d4u,d_deleted},
    {0xbe4e5u,0xbe4f5u,d_parent},{0xbe4e5u,0xbe4fau,d_no_parent},
    {0xbe50du,0xbe518u,d_lights},{0xbe50du,0xbe51du,d_no_lights},{0xbe4ffu,0xbe51du,d_no_lights},
    {0xbe52du,0xbe54eu,d_no_owner},{0xbe53du,0xbe54eu,d_owner_same},{0xbe53du,0xbe547u,d_owner_child},
    {0xbe564u,0xbe570u,d_children},{0xbe564u,0xbe596u,d_no_children},
    {0xbe5a1u,0xbe5afu,d_cluster},{0xbe5a1u,0xbe5cau,d_no_cluster_flag},{0xbe596u,0xbe5cau,d_no_cluster},
    {0xbe5cau,0xbe5d6u,d_countdown},{0xbe5cau,0xbe5f9u,d_changed},{0xbe5edu,0xbe5f9u,d_kept},{0xbe5edu,0xbe5f6u,d_quiet},
};
template<class State,class Dispatch,class Timers,class Types,class Lights,class Self,class Cluster>
struct ObjectUpdate : Port<State> {
    using Access=logic::object_update::Access;
    Dispatch &dispatch;Timers &timers_child;Types &types;Lights &lights;Self &self;Cluster &cluster_child;
    std::uint32_t mode_flag{};
    ObjectUpdate(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d,Timers &o,Types &y,Lights &l,Self &x,Cluster &k)
        :Port<State>(s,c,t,object_update_edges),dispatch(d),timers_child(o),types(y),lights(l),self(x),cluster_child(k){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:return 0xbe4b1u;
        case headers:return 0xbe4b6u;
        case header_flags:gp[esi]<<=2;return 0xbe4bcu;
        // AND AL,FB; PUSH EBP precede the object read.
        case object:set_low_byte(gp[eax],gp[eax]&~logic::header_flags::moved);this->push(0xbe4deu,gp[ebp]);return 0xbe4dfu;
        case moved_clear:return 0xbe4e2u;
        case parent_flags:return 0xbe4e5u;
        case game:return 0xbe4fau;
        case mode:return 0xbe4ffu;
        case mode_flags:return 0xbe50du;
        case timer_table:return 0xbe51du;
        case timer_headers:return 0xbe523u;
        case timer_object:return 0xbe526u;
        case timers_offset:return 0xbe52du;
        case block_offset:return 0xbe536u;
        case timer_owner:return 0xbe53du;
        case first_child:return 0xbe564u;
        case child_table:gp[edx]=(gp[esi]&0xffffu)*3u;return 0xbe582u;
        case child_headers:return 0xbe587u;
        case child_object:return 0xbe58au;
        case next_sibling:return 0xbe58eu;
        case cluster:return 0xbe596u;
        case cluster_flags:return 0xbe5a1u;
        case cluster_table:return 0xbe5afu;
        case cluster_data:return 0xbe5bdu;
        case countdown:return 0xbe5cau;
        // TEST AL,AL; DEC AL precede the store.
        case countdown_store:this->test8(gp[eax]);set_low_byte(gp[eax],this->flags.increment(gp[eax]&0xffu,1,true));return 0xbe5d6u;
        case sleep_flags:return 0xbe5edu;
        // AND AL,FD; TEST AL,AL precede the store.
        case sleep_store:set_low_byte(gp[eax],gp[eax]&~logic::header_flags::awake);this->test8(gp[eax]);return 0xbe5f6u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:case game:case child_table:case cluster:gp[eax]=v;break;
        case headers:case timer_table:case child_headers:gp[ecx]=v;break;
        // MOV AL; ADD ECX,ESI (the header); XOR BL,BL.
        case header_flags:set_low_byte(gp[eax],v);gp[ecx]+=gp[esi];set_low_byte(gp[ebx],0);break;
        case object:gp[ebp]=v;break;
        case parent_flags:set_low_byte(gp[edx],(v&0xffu)>>2);break;
        case mode:gp[eax]=sx8(v);break;
        case mode_flags:mode_flag=v;break;
        case timer_headers:case child_object:gp[edx]=v;break;
        case timer_object:gp[ecx]=v;gp[eax]=logic::none;break;
        case timers_offset:this->flags.minus(v,0xffffu,2);break;
        case block_offset:gp[esi]=sx16(v);break;
        case timer_owner:gp[ecx]=v;gp[esi]+=gp[ebp];this->compare32(v,gp[eax]);break;
        case first_child:case next_sibling:gp[esi]=v;break;
        case cluster_flags:set_low_byte(gp[ecx],(v&0xffu)>>6);break;
        // MOV EDX; AND EAX,FFFF; the record offset (index * A0) in EDI.
        case cluster_table:gp[edx]=v;gp[eax]&=0xffffu;gp[edi]=(gp[eax]*5u)<<5;break;
        case cluster_data:gp[eax]=v;gp[edi]=this->flags.plus(gp[edi],v,4);break;
        case countdown:set_low_byte(gp[eax],v);break;
        case sleep_flags:set_low_byte(gp[eax],v);this->test8(v&logic::header_flags::moved);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // Frame. PUSH EBX/ESI/EDI, then the handle argument at [ESP+10] (EDI);
    // ESI = index * 3 (the row, times four, before the flags read).
    std::uint32_t handle() {
        auto &gp=this->gp;
        this->push(0xbe4a0u,gp[ebx]);this->push(0xbe4a1u,gp[esi]);this->push(0xbe4a2u,gp[edi]);
        gp[edi]=this->read(0xbe4a3u,gp[esp]+0x10u,4);gp[esi]=(gp[edi]&0xffffu)*3u;
        return gp[edi];
    }
    void keep_header(std::uint32_t header){this->store(0xbe4c6u,this->gp[esp]+0x10u,4,header);}
    // After PUSH EBP the argument slot is [ESP+14].
    std::uint32_t header(){auto &gp=this->gp;gp[ecx]=this->read(0xbe5e9u,gp[esp]+0x14u,4);return gp[ecx];}
    // The update lives in EDI (handle), ESI (row, or the child being
    // updated), EBP (object) and BL (changed).
    void keep(const logic::object_update::Update &u,std::uint32_t esi_value) {
        auto &gp=this->gp;gp[edi]=u.handle;gp[esi]=esi_value;gp[ebp]=u.object;set_low_byte(gp[ebx],u.changed);
    }
    void take(logic::object_update::Update &u,std::uint32_t *esi_value) {
        const auto &gp=this->gp;u.handle=gp[edi];if(esi_value)*esi_value=gp[esi];u.object=gp[ebp];u.changed=std::uint8_t(gp[ebx]);
    }
    // Before 1D5790 ESI is the timer block and the rest of the update is in
    // registers; the calls after it see whatever ESI holds then.
    void keep_rest(const logic::object_update::Update &u) {
        auto &gp=this->gp;gp[edi]=u.handle;gp[ebp]=u.object;set_low_byte(gp[ebx],u.changed);
    }
    // TEST DL,1 (the parent bit, shifted down); MOV EAX,EDI; BDA50 (dispatch).
    void parent_update(logic::object_update::Update &u) {
        keep(u,u.row);this->test8(this->gp[edx]&1u);this->gp[eax]=this->gp[edi];
        this->call_routed(dispatch,0xbe4f5u,0xbe4fau,0xbda50u);take(u,&u.row);
    }
    // TEST byte [EBP+D8],1; MOV EAX,EDI; B37A0 (dispatch).
    void mode_update(logic::object_update::Update &u) {
        keep(u,u.row);this->test8(mode_flag&1u);this->gp[eax]=this->gp[edi];
        this->call_routed(dispatch,0xbe518u,0xbe51du,0xb37a0u);take(u,&u.row);
    }
    // ESI = the timer block; CMP ECX,EAX (the owner against NONE) decides
    // the flags. The result replaces BL (MOV BL,AL).
    std::uint8_t timers(logic::object_update::Update &u,std::uint32_t) {
        keep_rest(u);this->call(0xbe547u,0xbe54cu,0x1d5790u,timers_child);take(u,nullptr);
        return std::uint8_t(this->gp[eax]);
    }
    std::uint8_t type_updates(logic::object_update::Update &u) {
        keep_rest(u);this->push(0xbe54eu,this->gp[edi]);this->call(0xbe54fu,0xbe554u,0x10dd10u,types);take(u,nullptr);
        return std::uint8_t(this->gp[eax]);
    }
    // PUSH EDI; OR BL,AL (the flags); D8330 (dispatch).
    std::uint8_t damage(logic::object_update::Update &u) {
        this->push(0xbe554u,this->gp[edi]);keep_rest(u);this->test8(u.changed);
        this->call_routed(dispatch,0xbe557u,0xbe55cu,0xd8330u);take(u,nullptr);
        return std::uint8_t(this->gp[eax]);
    }
    std::uint8_t node_lights(logic::object_update::Update &u) {
        this->push(0xbe55cu,this->gp[edi]);keep_rest(u);this->test8(u.changed);
        this->call(0xbe55fu,0xbe564u,0xc0910u,lights);take(u,nullptr);
        return std::uint8_t(this->gp[eax]);
    }
    // CMP ESI,-1 (the child) decides the flags; PUSH ESI; BE4A0 (recursive).
    std::uint8_t update_child(logic::object_update::Update &u) {
        keep(u,u.child);this->compare32(u.child,logic::none);this->push(0xbe570u,this->gp[esi]);
        this->call(0xbe571u,0xbe576u,0xbe4a0u,self);take(u,&u.child);
        return std::uint8_t(this->gp[eax]);
    }
    // EDI = the cluster record (ADD EDI,EAX decides the flags).
    void cluster_update(logic::object_update::Update &u,std::uint32_t) {
        auto &gp=this->gp;gp[ebp]=u.object;set_low_byte(gp[ebx],u.changed);
        this->call(0xbe5c5u,0xbe5cau,0x1d6fb0u,cluster_child);
        u.object=gp[ebp];u.changed=std::uint8_t(gp[ebx]);
    }
    void run() {
        auto &gp=this->gp;
        Memory<ObjectUpdate> memory{*this};
        const auto result=logic::object_update::run(memory,*this);
        switch(result.exit) {
        case logic::object_update::Exit::asleep:
            this->test8(gp[eax]&logic::header_flags::awake);
            gp[edi]=this->pop(0xbe602u);gp[esi]=this->pop(0xbe603u);set_low_byte(gp[eax],gp[ebx]);gp[ebx]=this->pop(0xbe606u);
            this->ret(0xbe607u,4);this->publish(0xbe607u);return;
        case logic::object_update::Exit::held:
            this->test8(gp[eax]&logic::header_flags::held);
            gp[edi]=this->pop(0xbe4d4u);gp[esi]=this->pop(0xbe4d5u);set_low_byte(gp[eax],1);gp[ebx]=this->pop(0xbe4d8u);
            this->ret(0xbe4d9u,4);this->publish(0xbe4d9u);return;
        case logic::object_update::Exit::counting_down:
            gp[ebp]=this->pop(0xbe5dcu);gp[edi]=this->pop(0xbe5ddu);gp[esi]=this->pop(0xbe5deu);
            set_low_byte(gp[eax],1);gp[ebx]=this->pop(0xbe5e1u);
            this->ret(0xbe5e2u,4);this->publish(0xbe5e2u);return;
        case logic::object_update::Exit::updated:
            // TEST BL,BL when changed; otherwise the sleep path's TEST.
            if(result.changed)this->test8(result.changed);
            gp[ebp]=this->pop(0xbe5f9u);gp[edi]=this->pop(0xbe5fau);gp[esi]=this->pop(0xbe5fbu);
            set_low_byte(gp[eax],result.changed);gp[ebx]=this->pop(0xbe5feu);
            this->ret(0xbe5ffu,4);this->publish(0xbe5ffu);return;
        }
    }
};

// ---- 10DD10 type updates ------------------------------------------------------
// [ESP+4] = the handle. AL = the OR of the callbacks' AL over the original's
// EAX; TEST ECX,ECX (the chain's end) decides the flags. EBX/EBP/ESI/EDI
// restored. RET 4.
inline constexpr Edge type_updates_edges[]{
    {0x10dd3du,0x10dd75u,e_none},{0x10dd53u,0x10dd5au,e_callback},{0x10dd53u,0x10dd63u,e_empty_callback},
    {0x10dd63u,0x10dd51u,e_more},
};
template<class State,class Dispatch> struct TypeUpdates : Port<State> {
    using Access=logic::type_updates::Access;
    Dispatch &dispatch;
    TypeUpdates(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,type_updates_edges),dispatch(d){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:return 0x10dd10u;
        case headers:return 0x10dd16u;
        case object:return 0x10dd29u;
        case type:return 0x10dd2du;
        case definition:this->push(0x10dd34u,gp[esi]);this->push(0x10dd35u,gp[edi]);return 0x10dd36u;
        case chain_first:return 0x10dd3du;
        case chain_entry:return 0x10dd51u;
        case update:return 0x10dd53u;
        // INC ESI; MOVSX EDX,SI.
        case chain_next:++gp[esi];gp[edx]=sx16(gp[esi]);return 0x10dd63u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case table:gp[ecx]=v;break;
        case headers:gp[edx]=v;break;
        case object:case chain_entry:case update:gp[eax]=v;break;
        case type:gp[ecx]=sx8(v);break;
        case definition:gp[edi]=v;break;
        case chain_first:gp[ecx]=v;gp[eax]=gp[edi]+0x84u;set_low_byte(gp[ebx],0);gp[esi]=0;break;
        case chain_next:gp[ecx]=v;gp[eax]=gp[edi]+gp[edx]*4u+0x84u;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EBX/EBP, then the handle argument at [ESP+C] (EBP).
    std::uint32_t handle() {
        auto &gp=this->gp;
        this->push(0x10dd19u,gp[ebx]);this->push(0x10dd1au,gp[ebp]);
        gp[ebp]=this->read(0x10dd1bu,gp[esp]+0xcu,4);gp[eax]=(gp[ebp]&0xffffu)*3u;
        return gp[ebp];
    }
    // The walk lives in EBP (handle), EDI (definition), ESI (position) and
    // BL (changed). TEST EAX,EAX (the callback) decides the flags.
    std::uint8_t type_update(std::uint32_t callback,logic::type_updates::ChainWalk &w) {
        auto &gp=this->gp;
        gp[ebp]=w.handle;gp[edi]=w.definition;gp[esi]=w.position;set_low_byte(gp[ebx],w.changed);
        this->test32(callback);this->push(0x10dd5au,gp[ebp]);
        this->call(0x10dd5bu,0x10dd5du,callback,[&](State &v){dispatch(v,callback,0x10dd5bu);});
        w.handle=gp[ebp];w.definition=gp[edi];w.position=gp[esi];w.changed=std::uint8_t(gp[ebx]);
        return std::uint8_t(gp[eax]);
    }
    void run() {
        auto &gp=this->gp;
        Memory<TypeUpdates> memory{*this};
        const auto changed=logic::type_updates::run(memory,*this);
        this->test32(gp[ecx]);
        gp[edi]=this->pop(0x10dd75u);gp[esi]=this->pop(0x10dd76u);gp[ebp]=this->pop(0x10dd77u);
        set_low_byte(gp[eax],changed);gp[ebx]=this->pop(0x10dd7au);
        this->ret(0x10dd7bu,4);this->publish(0x10dd7bu);
    }
};

// ---- Verification ---------------------------------------------------------
// Serves the translated reference's reads from the native record (same
// order, address and width) and checks its stores and child boundaries.
template<class State> struct Replay : SourcePhysicsContactMemory {
    const Trace<State> &trace;mutable std::size_t at{};std::size_t call{};
    mutable std::uint32_t expected_pc{};mutable bool store_pending{};
    explicit Replay(const Trace<State> &t):trace(t){}
    [[noreturn]] void fail(const char *what,std::uint32_t address,std::size_t width)const {
        char text[200];
        const auto *e=at<trace.events.size()?&trace.events[at]:nullptr;
        std::snprintf(text,sizeof text,"Native object tick differs from the translated body at access %zu of %zu: %s %x/%zu, native %s pc=%x %x/%u",
            at,trace.events.size(),what,address,width,e?(e->kind?"store":"read"):"end",e?e->pc:0u,e?e->address:0u,e?e->width:0u);
        throw std::runtime_error(text);
    }
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        if(at<trace.events.size()) {
            const auto &e=trace.events[at];
            if(e.address==address && e.width==output.size()) {
                // A store preimage read (observer present) precedes its write.
                const auto value=e.kind?0u:e.value;
                for(std::size_t n=0;n<output.size();++n)output[n]=std::byte(value>>(8*n));
                if(!e.kind)++at;
                return;
            }
        }
        fail("read",address,output.size());
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        std::uint32_t value=0;for(std::size_t n=0;n<input.size() && n<4;++n)value|=std::to_integer<std::uint32_t>(input[n])<<(8*n);
        if(at>=trace.events.size())fail("store",address,input.size());
        const auto &e=trace.events[at];
        if(!e.kind || e.address!=address || e.width!=input.size() || e.value!=value)fail("store",address,input.size());
        expected_pc=e.pc;store_pending=true;++at;
    }
};
template<class State> inline thread_local Replay<State> *active_replay=nullptr;
// The reference body's child calls: same target, position and entry ABI as
// the native closure's genuine call; its recorded results are applied.
template<class State> void replay(State &s,std::uint32_t target) {
    auto *r=active_replay<State>;
    if(!r)throw std::runtime_error("Object tick reference child outside verification");
    if(r->call>=r->trace.calls.size())throw std::runtime_error("Object tick reference made an extra child call");
    const auto &c=r->trace.calls[r->call];
    if(c.target!=target || c.at!=r->at || !c.entry.same(Registers<State>::capture(s))) {
        char text[160];
        std::snprintf(text,sizeof text,"Native object tick child %zu differs: native %x at %zu, translated %x at %zu",
            r->call,c.target,c.at,target,r->at);
        throw std::runtime_error(text);
    }
    c.exit.apply(s);++r->call;
}
struct Tally {std::atomic<std::uint64_t> accesses{},stores{},children{};};
struct VerifySummary : Tally {
    std::atomic<std::uint64_t> calls[5]{};
    ~VerifySummary() {
        if(calls[0]||calls[1]||calls[2]||calls[3]||calls[4])
            std::fprintf(stderr,"Object tick verify: B8F40 %llu, B9090 %llu, B9180 %llu, BE4A0 %llu, 10DD10 %llu calls; "
                "%llu ordered accesses (%llu stores) and %llu child boundaries identical, coverage %llx\n",
                (unsigned long long)calls[0].load(),(unsigned long long)calls[1].load(),(unsigned long long)calls[2].load(),
                (unsigned long long)calls[3].load(),(unsigned long long)calls[4].load(),(unsigned long long)accesses.load(),
                (unsigned long long)stores.load(),(unsigned long long)children.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// Admission keeps every case outside the authenticated domain translated:
// another image, active traces, arbitrary store observers, full callbacks
// without a literal cursor (except the census diagnostic) and service sites.
template<class State> bool admitted(const State &s,std::uint32_t first,std::uint32_t last,std::uint32_t *&cursor,
                                    bool reference=reference_mode()) {
    if(reference || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    cursor=nullptr;
    if(!s.services.instruction)return true;
    if(native_census_detail::uncursored(s,first,last))return true;
    if(!s.literal_service_sites || !s.instruction_cursor || !s.instruction_cursor->position)return false;
    const auto *policy=s.instruction_cursor;
    if((policy->first_trace && *policy->first_trace) || (policy->second_trace && *policy->second_trace) ||
        (s.observe && !policy->observation_only_stores))return false;
    for(const auto site:policy->service_sites)if(site>=first && site<=last)return false;
    cursor=policy->position;
    return true;
}
// Run a closure; in verify mode record it and replay the translated body.
template<class State,class Reference,class Make> void execute(State &s,std::uint32_t *cursor,bool verifying,
    std::atomic<std::uint64_t> &hits,std::atomic<std::uint64_t> &calls,Tally &total,Reference reference,Make make) {
    if(!verifying) {
        auto closure=make(s,cursor,nullptr);closure.load();closure.run();
        hits.fetch_or(closure.hit,std::memory_order_relaxed);return;
    }
    const auto entry=Registers<State>::capture(s);
    Trace<State> trace;
    {auto closure=make(s,cursor,&trace);closure.load();closure.run();hits.fetch_or(closure.hit);}
    const auto final=Registers<State>::capture(s);
    Replay<State> bank{trace};
    std::uint32_t mismatched_pc=0;bool pc_mismatch=false;
    const SourcePhysicsContactRecordObserver observer=[&](const SourcePhysicsContactRecordStore &e) {
        if(!bank.store_pending || e.instruction!=bank.expected_pc){pc_mismatch=true;mismatched_pc=e.instruction;}
        bank.store_pending=false;
    };
    auto services=s.services;services.instruction=nullptr;
    State original{bank,observer,services,s.image};entry.apply(original);
    auto *previous=active_replay<State>;active_replay<State> = &bank;
    try {reference(original);}catch(...) {active_replay<State> = previous;throw;}
    active_replay<State> = previous;
    if(pc_mismatch) {
        char text[96];std::snprintf(text,sizeof text,"Native object tick store pc differs (translated %x)",mismatched_pc);
        throw std::runtime_error(text);
    }
    if(bank.at!=trace.events.size() || bank.call!=trace.calls.size())
        throw std::runtime_error("Native object tick made accesses or child calls the translated body did not");
    const bool same=final.same(Registers<State>::capture(original));
    _mm_setcsr(final.mxcsr); // the live run's sticky state stays the result
    if(!same)throw std::runtime_error("Native object tick registers/flags/FP differ from the translated body");
    calls.fetch_add(1);total.accesses.fetch_add(trace.events.size());
    std::uint64_t stores=0;for(const auto &e:trace.events)stores+=e.kind;
    total.stores.fetch_add(stores);total.children.fetch_add(trace.calls.size());
}

template<class State,class Reference,class Update,class Attach>
bool bridge_b8f40(State &s,Reference reference,Update update,Attach attach) {
    std::uint32_t *cursor;if(!admitted(s,0xb8f40u,0xb9089u,cursor))return false;
    execute(s,cursor,verify_mode(),coverage(),summary().calls[0],summary(),reference,[&](State &v,std::uint32_t *c,Trace<State> *t){
        return UpdatePass<State,Update,Attach>(v,c,t,update,attach);});
    return true;
}
template<class State,class Reference,class Move,class Transform,class Finish>
bool bridge_b9090(State &s,Reference reference,Move move,Transform transform,Finish finish) {
    std::uint32_t *cursor;if(!admitted(s,0xb9090u,0xb9172u,cursor))return false;
    execute(s,cursor,verify_mode(),coverage(),summary().calls[1],summary(),reference,[&](State &v,std::uint32_t *c,Trace<State> *t){
        return MovePass<State,Move,Transform,Finish>(v,c,t,move,transform,finish);});
    return true;
}
template<class State,class Reference,class Update,class Move,class Finish,class Detach,class Release,class Tail>
bool bridge_b9180(State &s,Reference reference,Update update,Move move,Finish finish,Detach detach,Release release,Tail tail) {
    std::uint32_t *cursor;if(!admitted(s,0xb9180u,0xb92bau,cursor))return false;
    execute(s,cursor,verify_mode(),coverage(),summary().calls[2],summary(),reference,[&](State &v,std::uint32_t *c,Trace<State> *t){
        return LatePass<State,Update,Move,Finish,Detach,Release,Tail>(v,c,t,update,move,finish,detach,release,tail);});
    return true;
}
template<class State,class Reference,class Dispatch,class Owner,class Types,class Lights,class Self,class Cluster>
bool bridge_be4a0(State &s,Reference reference,Dispatch dispatch,Owner owner,Types types,Lights lights,Self self,Cluster cluster) {
    std::uint32_t *cursor;if(!admitted(s,0xbe4a0u,0xbe607u,cursor))return false;
    execute(s,cursor,verify_mode(),coverage(),summary().calls[3],summary(),reference,[&](State &v,std::uint32_t *c,Trace<State> *t){
        return ObjectUpdate<State,Dispatch,Owner,Types,Lights,Self,Cluster>(v,c,t,dispatch,owner,types,lights,self,cluster);});
    return true;
}
template<class State,class Reference,class Dispatch>
bool bridge_10dd10(State &s,Reference reference,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x10dd10u,0x10dd7bu,cursor))return false;
    execute(s,cursor,verify_mode(),coverage(),summary().calls[4],summary(),reference,[&](State &v,std::uint32_t *c,Trace<State> *t){
        return TypeUpdates<State,Dispatch>(v,c,t,dispatch);});
    return true;
}
} // namespace halo2::native_object_tick_detail
