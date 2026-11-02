#pragma once
#include "halo2/native_tu15_wrapped_contact_process.hpp"
#include "halo2/native_tu15_reversed_contact_process.hpp"
#include <limits>
#include <stdexcept>
#include <utility>
namespace halo2 {
struct NativeTu15ContactProcessBindingScratch {
    std::uint32_t first{},bytes{};
};
struct NativeTu15ContactProcessBindingStats {
    std::uint64_t attempts{},completed{},refusals{},remaining_child_calls{};
    std::uint64_t reversed_attempts{},reversed_completed{},reversed_refusals{};
    NativeTu15ReversedContactProcessStats reversed_wrapped,reversed_compound;
    NativeTu15WrappedContactProcessStats wrapped;
    NativeTu15CompoundProcessStats compound;
};
// An optional diagnostic observes the genuine typed input before its first
// access. It cannot choose an implementation, admission or processing result.
using NativeTu15ContactProcessInputObserver=std::function<void(
    const NativeTu15CompoundChildProcessCall&,const NativeTu15PhysicsFrameFp&)>;
class NativeTu15ContactProcessBinding {
    SourcePhysicsContactMemory&memory_;
    NativeTu15ContactProcessBindingScratch scratch_;
    NativeTu15CompoundProcessServices remaining_;
    SourcePhysicsContactRecordObserver observe_;
    NativeTu15ContactProcessInputObserver input_observe_;
    NativeTu15ContactProcessBindingStats*stats_;
    unsigned depth_{};
public:
    NativeTu15ContactProcessBinding(SourcePhysicsContactMemory&memory,
        NativeTu15ContactProcessBindingScratch scratch,
        NativeTu15CompoundProcessServices remaining,
        const SourcePhysicsContactRecordObserver&observe = {},
        NativeTu15ContactProcessBindingStats*stats = nullptr,
        NativeTu15ContactProcessInputObserver input_observe = {})
        :memory_(memory),scratch_(scratch),remaining_(std::move(remaining)),
         observe_(observe),input_observe_(std::move(input_observe)),stats_(stats){}
    static bool owns(NativeTu15PhysicsVirtualOwner owner){
        return owner.identity==0x2f6ca0u||owner.identity==0x2f7a50u||
            owner.identity==0x2f7480u||owner.identity==0x2f7fb0u;
    }
    void process(const NativeTu15CompoundChildProcessCall&call,
        NativeTu15PhysicsFrameFp&fp){
        if(input_observe_)input_observe_(call,fp);
        if(!owns(call.owner)){
            if(!remaining_.process_child)
                throw std::runtime_error("Native contact processing requires its actual remaining child owner");
            if(stats_)++stats_->remaining_child_calls;
            remaining_.process_child(call,fp);return;
        }
        const bool reversed=call.owner.identity==0x2f7480u||call.owner.identity==0x2f7fb0u;
        if(stats_){++stats_->attempts;if(reversed)++stats_->reversed_attempts;}
        try{
            // Each native caller owns a separate DATA slot until its child
            // returns. Genuine residual control frames must live below this
            // complete arena; the binding never constructs a CPU frame.
            const auto offset=std::uint64_t(depth_)*128u;
            const auto bytes=call.owner.identity==0x2f6ca0u?112u:16u;
            if((scratch_.first&15u)||offset+bytes>scratch_.bytes||
                std::uint64_t(scratch_.first)+offset+bytes>
                    std::numeric_limits<std::uint32_t>::max())
                throw std::runtime_error("Native contact processing exhausted its caller-owned DATA arena");
            const auto slot=scratch_.first+std::uint32_t(offset);
            struct Depth {unsigned&value;explicit Depth(unsigned&v):value(v){++value;}~Depth(){--value;}} depth(depth_);
            NativeTu15CompoundProcessServices children;
            children.process_child=[&](const auto&child,auto&context){process(child,context);};
            if(call.owner.identity==0x2f7480u)
                native_tu15_process_reversed_wrapped_contact_agent(memory_,call.receiver,
                    {call.first.identity},call.second,call.configuration,call.output,
                    fp,children,observe_,stats_?&stats_->reversed_wrapped:nullptr);
            else if(call.owner.identity==0x2f7fb0u)
                native_tu15_process_reversed_compound_contact_agent(memory_,call.receiver,
                    {call.first.identity},call.second,call.configuration,call.output,
                    fp,children,observe_,stats_?&stats_->reversed_compound:nullptr);
            else if(call.owner.identity==0x2f6ca0u)
                native_tu15_process_wrapped_contact_agent(memory_,call.receiver,
                    {call.first.identity},call.second,call.configuration,call.output,
                    {{slot},slot+16u},fp,children,observe_,stats_?&stats_->wrapped:nullptr);
            else
                native_tu15_process_compound_agent(memory_,call.receiver,
                    {call.first.identity},call.second,call.configuration,call.output,
                    {slot},fp,children,observe_,stats_?&stats_->compound:nullptr);
            if(stats_){++stats_->completed;if(reversed)++stats_->reversed_completed;}
        }catch(...){if(stats_){++stats_->refusals;if(reversed)++stats_->reversed_refusals;}throw;}
    }
};
}
