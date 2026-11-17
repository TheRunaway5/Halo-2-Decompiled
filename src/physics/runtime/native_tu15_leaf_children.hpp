#pragma once
// Child calls for the leaf framework (native_tu15_matrix_leaves.hpp).
//
// Some natives call a routine that must stay itself: a state-based native of
// another framework, or another group's owner. The adapter enters such a
// child as a genuine call (ChildIo::call): the Machine is published to the
// live state, so the child starts from the original's registers, flags, XMM
// and x87 at the CALL (the adapter has pushed the return address); the child
// runs through the unit's own route (its dispatch or local run_ function);
// every register comes back into the Machine.
//
// HALO2_VERIFY_MATRIX_LEAVES=1 cannot compute such a leaf on a pure overlay
// (a child writes the live bank). run_with_children() instead runs the native
// on the live bank, recording its ordered accesses (kind, pc, address, width,
// bytes) and each child boundary (target, position among the accesses, entry
// and exit registers). It then runs the binder's reference copy of the
// translated body on a replay bank: reads are served from the record and must
// match in order, address and width; stores must match pc, address and bytes;
// the reference's child calls are routed to replay_child(), which requires
// the same target, position and entry registers and applies the recorded
// exit; the final registers must be identical. Nested leaves inside the
// reference take their translated bodies. Modeled on group H's 303EE0
// (native_tu15_world_leaves.hpp).
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <atomic>
#include <cstdio>
#include <vector>

namespace halo2::native_tu15_leaves {
// The complete register file at entry, exit and child boundaries.
template<class State> struct LeafRegisters {
    std::array<std::uint32_t,8> gp{};
    unsigned flags{};std::uint32_t other{},fs{},mxcsr{};
    std::array<std::uint32_t,32> xmm{};
    decltype(std::declval<State&>().fp) fp{};
    static LeafRegisters capture(const State &s) {
        LeafRegisters r;r.gp=s.gp;r.flags=s.flags.bits();r.other=s.other_flags;r.fs=s.fs_base;r.fp=s.fp;r.mxcsr=_mm_getcsr();
        std::memcpy(r.xmm.data(),s.xmm,sizeof r.xmm);return r;
    }
    void apply(State &s)const {
        s.gp=gp;s.flags.load(flags);s.other_flags=other;s.fs_base=fs;s.fp=fp;std::memcpy(s.xmm,xmm.data(),sizeof xmm);_mm_setcsr(mxcsr);
    }
    bool same(const LeafRegisters &o)const {
        bool equal=gp==o.gp && flags==o.flags && other==o.other && fs==o.fs && mxcsr==o.mxcsr && xmm==o.xmm &&
            fp.count==o.fp.count && fp.status==o.fp.status;
        if constexpr(requires{fp.top;})equal=equal && fp.top==o.fp.top;
        for(unsigned i=0;i<fp.count && i<8;++i)equal=equal && !std::memcmp(&fp.lanes[i],&o.fp.lanes[i],10);
        return equal;
    }
};
// One ordered access of a native (kind 0 read, 1 store) and the child calls.
struct LeafEvent {std::uint32_t kind,pc,address,width;std::array<std::byte,16> bytes;};
template<class State> struct LeafTrace {
    struct Call {std::uint32_t target;std::size_t at;LeafRegisters<State> entry,exit;};
    std::vector<LeafEvent> events;std::vector<Call> calls;
};

// RealIo with genuine child calls and, in verify mode, a record.
template<class State> struct ChildIo : RealIo<State> {
    LeafTrace<State> *trace{};
    bool in_child{};  // a fault inside a child leaves the child's own state
    void record(std::uint32_t kind,std::uint32_t pc,std::uint32_t address,unsigned width,const void *bytes) {
        if(!trace)return;
        LeafEvent e{kind,pc,address,width,{}};std::memcpy(e.bytes.data(),bytes,width);trace->events.push_back(e);
    }
    std::uint32_t rd(std::uint32_t p,std::uint32_t a,unsigned n) {
        const auto value=RealIo<State>::rd(p,a,n);record(0,p,a,n,&value);return value;
    }
    void st(std::uint32_t p,std::uint32_t a,unsigned n,std::uint64_t x) {
        RealIo<State>::st(p,a,n,x);record(1,p,a,n,&x);
    }
    template<class Flags,class Route> void call(Machine<Flags> &m,std::uint32_t target,Route &&route) {
        auto &s=this->s;
        publish_machine(s,m);
        if(trace)trace->calls.push_back({target,trace->events.size(),LeafRegisters<State>::capture(s),{}});
        in_child=true;route(s);in_child=false;
        if(trace)trace->calls.back().exit=LeafRegisters<State>::capture(s);
        m.gp=s.gp;m.flags=s.flags;
        if constexpr(requires{s.other_flags;})m.other_flags=s.other_flags;
        for(unsigned i=0;i<8;++i)m.x[i]=s.xmm[i];
        if constexpr(requires{s.fp.lanes;s.fp.count;s.fp.top;s.fp.status;})m.x87={s.fp.lanes,s.fp.count,s.fp.top,s.fp.status};
    }
};

// The reference's bank: reads are served from the native record (same order,
// address and width); stores must match it (address, width, bytes).
template<class State> struct LeafReplay : SourcePhysicsContactMemory {
    const LeafTrace<State> &trace;mutable std::size_t at{};std::size_t call{};
    mutable std::uint32_t expected_pc{};mutable bool store_pending{};
    explicit LeafReplay(const LeafTrace<State> &t):trace(t){}
    [[noreturn]] void fail(const char *what,std::uint32_t address,std::size_t width)const {
        char text[200];const auto *e=at<trace.events.size()?&trace.events[at]:nullptr;
        std::snprintf(text,sizeof text,"Native TU1.5 leaf differs from its original body at access %zu of %zu: %s %x/%zu, native %s pc=%x %x/%u",
            at,trace.events.size(),what,address,width,e?(e->kind?"store":"read"):"end",e?e->pc:0u,e?e->address:0u,e?e->width:0u);
        throw std::runtime_error(text);
    }
    void serve(std::uint32_t address,std::span<std::byte> output)const {
        if(at<trace.events.size()) {
            const auto &e=trace.events[at];
            if(e.address==address && e.width==output.size()) {
                // A store's preimage read (the observer is present) precedes its write.
                if(e.kind)std::memset(output.data(),0,output.size());
                else {std::memcpy(output.data(),e.bytes.data(),output.size());++at;}
                return;
            }
        }
        fail("read",address,output.size());
    }
    void read(std::uint32_t address,std::span<std::byte> output)const override {serve(address,output);}
    void read_vector(std::uint32_t address,std::span<std::byte,16> output)const override {serve(address,output);}
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        if(at>=trace.events.size())fail("store",address,input.size());
        const auto &e=trace.events[at];
        if(!e.kind || e.address!=address || e.width!=input.size() || std::memcmp(e.bytes.data(),input.data(),input.size()))
            fail("store",address,input.size());
        expected_pc=e.pc;store_pending=true;++at;
    }
};
template<class State> inline thread_local LeafReplay<State> *active_child_replay=nullptr;
// A child call of the reference copy: the native call's target, position and
// entry registers are required, and its exit registers applied.
template<class State> void replay_child(State &s,std::uint32_t target) {
    auto *replay=active_child_replay<State>;
    if(!replay)throw std::runtime_error("TU1.5 leaf reference child outside verification");
    if(replay->call>=replay->trace.calls.size())throw std::runtime_error("TU1.5 leaf reference made an extra child call");
    const auto &c=replay->trace.calls[replay->call];
    if(c.target!=target || c.at!=replay->at || !c.entry.same(LeafRegisters<State>::capture(s))) {
        char text[160];
        std::snprintf(text,sizeof text,"Native TU1.5 leaf child %zu differs: native %x at %zu, translated %x at %zu",
            replay->call,c.target,c.at,target,replay->at);
        throw std::runtime_error(text);
    }
    c.exit.apply(s);++replay->call;
}
struct ChildVerifySummary {
    std::atomic<std::uint64_t> calls{},accesses{},children{};
    ~ChildVerifySummary() {
        if(calls)std::fprintf(stderr,"TU1.5 leaves with children verify: %llu calls, %llu ordered accesses and %llu child boundaries identical\n",
            (unsigned long long)calls.load(),(unsigned long long)accesses.load(),(unsigned long long)children.load());
    }
};
inline ChildVerifySummary &child_summary(){static ChildVerifySummary value;return value;}

// Admission (as run()) and dispatch of a leaf with children. kernel(io, m)
// receives a ChildIo; reference is the binder's reference copy, whose child
// calls are routed to replay_child().
template<class State,class Kernel,class Reference>
bool run_with_children(State &s,Kernel kernel,Reference reference,std::uint32_t final_pc) {
    std::uint32_t *pc=nullptr;
    if(!admitted(s,pc))return false;
    using Flags=std::remove_cvref_t<decltype(s.flags)>;
    const auto execute=[&](LeafTrace<State> *trace) {
        Machine<Flags> m{s.gp,{},s.flags};
        if constexpr(requires{s.other_flags;})m.other_flags=s.other_flags;
        for(unsigned i=0;i<8;++i)m.x[i]=s.xmm[i];
        if constexpr(requires{s.fp.lanes;s.fp.count;s.fp.top;s.fp.status;})m.x87={s.fp.lanes,s.fp.count,s.fp.top,s.fp.status};
        ChildIo<State> io{{s,pc},trace};
        try {kernel(io,m);}
        catch(...) {
            if(!io.in_child){m.materialize_fault_flags();publish_machine(s,m);}
            throw;
        }
        publish_machine(s,m);
        if(pc && final_pc)*pc=final_pc;
    };
    if(!verify_mode()){execute(nullptr);return true;}
    const auto entry=LeafRegisters<State>::capture(s);
    LeafTrace<State> trace;
    execute(&trace);
    const auto final=LeafRegisters<State>::capture(s);
    LeafReplay<State> bank{trace};
    bool pc_mismatch=false;std::uint32_t mismatched_pc=0;
    const SourcePhysicsContactRecordObserver observer=[&](const SourcePhysicsContactRecordStore &e) {
        if(!bank.store_pending || e.instruction!=bank.expected_pc){pc_mismatch=true;mismatched_pc=e.instruction;}
        bank.store_pending=false;
    };
    auto services=s.services;services.instruction=nullptr;
    State original{bank,observer,services,s.image};entry.apply(original);
    {
        struct Scope {
            LeafReplay<State> *previous;
            explicit Scope(LeafReplay<State> *b):previous(active_child_replay<State>){bypass()=true;active_child_replay<State> =b;}
            ~Scope(){bypass()=false;active_child_replay<State> =previous;}
        } held{&bank};
        reference(original);
    }
    if(pc_mismatch) {
        char text[96];std::snprintf(text,sizeof text,"Native TU1.5 leaf store pc differs (translated %x)",mismatched_pc);
        throw std::runtime_error(text);
    }
    if(bank.at!=trace.events.size())throw std::runtime_error("Native TU1.5 leaf made accesses its original body did not");
    if(bank.call!=trace.calls.size())throw std::runtime_error("Native TU1.5 leaf made child calls its original body did not");
    const bool same=final.same(LeafRegisters<State>::capture(original));
    _mm_setcsr(final.mxcsr);  // the live run's sticky state stays the result
    if(!same)throw std::runtime_error("Native TU1.5 leaf registers/flags/XMM differ from its original body");
    child_summary().calls.fetch_add(1);child_summary().accesses.fetch_add(trace.events.size());
    child_summary().children.fetch_add(trace.calls.size());
    return true;
}
} // namespace halo2::native_tu15_leaves
