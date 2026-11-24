#pragma once
// Boundary adapter for the device tree, entered at 10C5D0
// (native_tu15_device_tree_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: the handle on the stack (RET 4);
// ESI saved; B9390 gets EAX the handle, 10C5D0 the child pushed (RET 4),
// every register each returns coming back; EAX, ECX and EDX as the original
// leaves them; the flags of the closing CMP ESI,-1. Each access publishes
// its original instruction.
// HALO2_REFERENCE_DEVICE_TREE=1 restores the translated body.
// HALO2_VERIFY_DEVICE_TREE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_device_tree_systems.hpp"

namespace halo2::native_device_tree_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_device_tree;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_DEVICE_TREE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_DEVICE_TREE");return p && *p;}();
    return value;
}
enum Branch : unsigned {other, device, descended, branch_count};
static_assert(unsigned(logic::device_tree::Outcome::device)==Branch::device);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Device tree verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Marked,class Descend> struct DeviceTree : Port<State> {
    using Access=logic::device_tree::Access;
    Marked &marked_child;
    Descend &descend_child;
    DeviceTree(State &s,std::uint32_t *c,Trace<State> *t,Marked &m,Descend &d):Port<State>(s,c,t,{}),marked_child(m),descend_child(d){}
    std::uint32_t argument()const{return this->gp[esp]+4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case handle:return 0x10c5d0u;
        case objects:return 0x10c5d4u;
        case object_data:return 0x10c5dau;
        // MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]; PUSH ESI.
        case object:gp[ecx]=3u*(gp[eax]&0xffffu);this->push(0x10c5e8u,gp[esi]);return 0x10c5e9u;
        case type:return 0x10c5edu;
        case flags:case flags_store:return 0x10c602u;
        case first_child:return 0x10c60eu;
        case objects_again:return 0x10c61cu;
        case object_data_again:return 0x10c622u;
        // MOV EAX,ESI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case child_object:gp[eax]=3u*(gp[esi]&0xffffu);return 0x10c62fu;
        case next:return 0x10c633u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case handle:gp[eax]=v;break;
        case objects:case object_data:case object_data_again:gp[edx]=v;break;
        case object:gp[esi]=v;break;
        // MOV CL,[ESI+AA]; MOV EDX,1; SHL EDX,CL; TEST EDX,380.
        case type:set_low_byte(gp[ecx],v);gp[edx]=1u<<(v&31u);this->test32(gp[edx]&logic::device_types);break;
        case first_child:case next:gp[esi]=v;this->compare32(v,logic::none);break;                  // CMP ESI,-1
        case objects_again:gp[ecx]=v;break;
        case child_object:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::flags_store)this->flags.logic(v,4);}   // OR [ESI+12C],4
    std::uint32_t marked(std::uint32_t,std::uint32_t) {
        this->call(0x10c609u,0x10c60eu,0xb9390u,marked_child);
        return this->gp[esi];
    }
    // PUSH ESI; CALL 10C5D0.
    std::uint32_t descend(std::uint32_t child) {
        this->reached(Branch::descended);
        this->push(0x10c616u,child);
        this->call(0x10c617u,0x10c61cu,0x10c5d0u,descend_child);
        return this->gp[esi];
    }
    void run() {
        auto &gp=this->gp;
        Memory<DeviceTree> memory{*this};
        this->reached(unsigned(logic::device_tree::mark(memory,*this)));
        gp[esi]=this->pop(0x10c63bu);
        this->ret(0x10c63cu,4u);this->publish(0x10c63cu);
    }
};
template<class State,class Reference,class Marked,class Descend> bool bridge(State &s,Reference reference,Marked marked,Descend descend) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x10c5d0u,0x10c63cu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return DeviceTree<State,Marked,Descend>(v,c,t,marked,descend);});
    return true;
}
} // namespace halo2::native_device_tree_detail
