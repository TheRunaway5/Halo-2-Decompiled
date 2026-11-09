#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeAsyncPromotionRead {
    owner,head,head_next,initial_link,advance,advanced_link,node_next,priority
};
enum class NativeAsyncPromotionWrite {head,previous_next,priority};
struct NativeAsyncPromotion {
    std::uint32_t job{},node{},owner{},head{},previous{},next{},priority{};
    bool promoted{};
};

// Reprioritize only the generation still owned by this job. Unlink its real
// queue position, publish the requested priority and reinsert through the
// actual priority/locality owner. Lock, storage and resumed child state belong
// to Access; no processor register, flags or instruction cursor lives here.
template<class Access>bool native_async_promote(Access&access,NativeAsyncPromotion&d){
    using R=NativeAsyncPromotionRead;using W=NativeAsyncPromotionWrite;
    d.promoted=false;access.begin(d);d.job=access.job(d);d.node=access.node(d);
    d.owner=access.read(R::owner,d);
    if(d.owner==d.job){
        d.head=access.read(R::head,d);
        if(d.head==d.node){
            d.next=access.read(R::head_next,d);access.write(W::head,d,d.next);
        }else{
            d.previous=d.head;d.next=access.read(R::initial_link,d);
            while(d.next!=d.node){
                d.previous=access.read(R::advance,d);d.next=access.read(R::advanced_link,d);
            }
            d.next=access.read(R::node_next,d);access.write(W::previous_next,d,d.next);
        }
        d.priority=access.read(R::priority,d);access.prepare_insert(d);
        access.write(W::priority,d,d.priority);access.insert(d);
        d.promoted=true;access.promoted(d);
    }
    access.finish(d);return d.promoted;
}
} // namespace halo2
