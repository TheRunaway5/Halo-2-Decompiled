#pragma once
#include "halo2/native_tu15_wrapped_contact_query.hpp"
#include "halo2/native_tu15_contact_pair_collectors.hpp"
#include "halo2/native_tu15_convex_overlap_query.hpp"
#include <limits>

namespace halo2 {
struct NativeTu15OverlapQueryBindingScratch {std::uint32_t first{},bytes{};};
struct NativeTu15OverlapQueryBindingStats {
    std::uint64_t calls{},completed{},refusals{},wrapped{},reversed{},convex{},empty{},
        pair_callbacks{},reversed_pair_callbacks{},marked_pairs{};
};
// Typed query composition only. Its caller owns the same guarded memory for
// every parent, solver and result consumer, and supplies the genuine native
//30E930 service. Unsupported owners refuse; no CPU executor is available.
class NativeTu15OverlapQueryBinding {
    SourcePhysicsContactMemory&memory_;
    NativeTu15OverlapQueryBindingScratch scratch_;
    NativeTu15ConvexOverlapQueryServices services_;
    SourcePhysicsContactRecordObserver observe_;
    std::function<void(NativeTu15ContactQueryCollector,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsBody,NativeTu15PhysicsBody)>pair_observe_;
    NativeTu15OverlapQueryBindingStats*stats_;
    unsigned depth_{},collector_depth_{};
    struct Depth {unsigned&value;explicit Depth(unsigned&v):value(v){++value;}~Depth(){--value;}};
public:
    NativeTu15OverlapQueryBinding(SourcePhysicsContactMemory&memory,
        NativeTu15OverlapQueryBindingScratch scratch,
        NativeTu15ConvexOverlapQueryServices services,
        const SourcePhysicsContactRecordObserver&observe={},
        NativeTu15OverlapQueryBindingStats*stats=nullptr,
        std::function<void(NativeTu15ContactQueryCollector,NativeTu15PhysicsVirtualOwner,
            NativeTu15PhysicsBody,NativeTu15PhysicsBody)>pair_observe={})
        :memory_(memory),scratch_(scratch),services_(std::move(services)),
         observe_(observe),pair_observe_(std::move(pair_observe)),stats_(stats){}
    static bool owns(NativeTu15PhysicsVirtualOwner owner){
        return owner.identity==0x2f73c0u||owner.identity==0x2f6f40u||
            owner.identity==0x2f8410u||owner.identity==0x8eb50u;
    }
    void collect(NativeTu15ContactQueryCollector receiver,
        NativeTu15PhysicsVirtualOwner owner,NativeTu15PhysicsBody first,
        NativeTu15PhysicsBody second,NativeTu15PhysicsFrameFp&fp){
        if(pair_observe_)pair_observe_(receiver,owner,first,second);
        if(stats_)++stats_->pair_callbacks;
        if(owner.identity==0x30ba80u){
            native_tu15_mark_contact_pair(memory_,receiver,observe_);
            if(stats_)++stats_->marked_pairs;return;
        }
        if(owner.identity!=0x30b960u)
            throw std::runtime_error("Native overlap query requires its current pair collector owner="+
                std::to_string(owner.identity));
        if(collector_depth_>=scratch_.bytes/12u)
            throw std::runtime_error("Native overlap collector exhausted caller-owned DATA depth");
        Depth depth(collector_depth_);if(stats_)++stats_->reversed_pair_callbacks;
        NativeTu15ContactPairCollectorServices next;
        next.emit_pair=[&](auto target,auto method,auto a,auto b,auto&context){collect(target,method,a,b,context);};
        native_tu15_forward_reversed_contact_pair(memory_,receiver,first,second,fp,next);
    }
    void query(NativeTu15IslandCollisionAgent receiver,NativeTu15PhysicsVirtualOwner owner,
        NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
        NativeTu15PhysicsStepConfiguration configuration,
        NativeTu15ContactQueryCollector collector,NativeTu15PhysicsFrameFp&fp){
        if(stats_)++stats_->calls;
        try{
            if(!owns(owner))throw std::runtime_error("Native overlap query requires its actual child owner="+
                std::to_string(owner.identity));
            if(owner.identity==0x8eb50u){
                native_tu15_ignore_contact_process();
                if(stats_){++stats_->empty;++stats_->completed;}return;
            }
            //512B per active query: descriptor, affine DATA, scoped collector
            // or the leaf's complete plane/relative/simplex work packet.
            const auto offset=std::uint64_t(depth_)*512u;
            if((scratch_.first&15u)||offset+512u>scratch_.bytes||
                std::uint64_t(scratch_.first)+offset+512u>
                    std::numeric_limits<std::uint32_t>::max())
                throw std::runtime_error("Native overlap query exhausted caller-owned DATA");
            const auto slot=scratch_.first+std::uint32_t(offset);Depth depth(depth_);
            if(owner.identity==0x2f8410u){
                if(stats_)++stats_->convex;
                auto services=services_;
                services.collect=[&](auto target,auto method,auto a,auto b,auto&context){collect(target,method,a,b,context);};
                native_tu15_query_convex_overlap(memory_,receiver,first,second,configuration,
                    collector,{slot+16u},fp,0,services,observe_);
            } else {
                NativeTu15ConditionalContactServices children;
                children.query=[&](auto child,auto method,auto a,auto b,auto settings,auto output,auto&context){
                    query(child,method,{a.identity},b,settings,output,context);
                };
                if(owner.identity==0x2f73c0u){
                    if(stats_)++stats_->reversed;
                    native_tu15_query_reversed_wrapped_contact_agent(memory_,receiver,first,
                        second,configuration,collector,{{slot+0x1e8u},{{slot+0x160u},slot+0x170u}},
                        fp,children,observe_);
                }else{
                    if(stats_)++stats_->wrapped;
                    native_tu15_query_wrapped_contact_agent(memory_,receiver,first,second,
                        configuration,collector,{{slot+0x180u},slot+0x190u},fp,children,observe_);
                }
            }
            if(stats_)++stats_->completed;
        }catch(...){if(stats_)++stats_->refusals;throw;}
    }
};
} // namespace halo2
