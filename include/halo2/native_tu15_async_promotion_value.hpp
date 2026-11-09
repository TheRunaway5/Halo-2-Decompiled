#pragma once
#include "halo2/native_tu15_async_promotion_kernel.hpp"
#include "halo2/source_async_jobs.hpp"
#include <bit>
#include <stdexcept>

namespace halo2::native_async_promotion_value_detail {
template<class Validate>struct View {
    SourceAsyncQueue&queue;std::uint32_t requested_job;std::int32_t requested_priority;Validate&validate;
    void begin(const NativeAsyncPromotion&)const{}
    std::uint32_t job(const NativeAsyncPromotion&)const{return requested_job;}
    std::uint32_t node(const NativeAsyncPromotion&d)const{return d.job&255u;}
    std::uint32_t read(NativeAsyncPromotionRead kind,const NativeAsyncPromotion&d)const{
        using R=NativeAsyncPromotionRead;
        switch(kind){
        case R::owner:return queue.nodes[d.node].job;
        case R::head:validate();return queue.work_head;
        case R::head_next:case R::node_next:return queue.nodes[d.node].next;
        case R::initial_link:case R::advance:case R::advanced_link:
            if(d.previous==source_async_none)throw std::invalid_argument("source async unlink needs a queued node");
            return queue.nodes[d.previous].next;
        case R::priority:return std::bit_cast<std::uint32_t>(requested_priority);
        }
        return 0;
    }
    void write(NativeAsyncPromotionWrite kind,const NativeAsyncPromotion&d,std::uint32_t value){
        switch(kind){
        case NativeAsyncPromotionWrite::head:queue.work_head=value;break;
        case NativeAsyncPromotionWrite::previous_next:queue.nodes[d.previous].next=value;break;
        case NativeAsyncPromotionWrite::priority:queue.nodes[d.node].priority=std::bit_cast<std::int32_t>(value);break;
        }
    }
    void prepare_insert(const NativeAsyncPromotion&)const{}
    void insert(const NativeAsyncPromotion&d){source_async_insert(queue,d.node);}
    void promoted(const NativeAsyncPromotion&)const{}
    void finish(const NativeAsyncPromotion&)const{}
};
template<class Validate>bool reprioritize(SourceAsyncQueue&queue,std::uint32_t job,std::int32_t priority,Validate&&validate){
    View<Validate>view{queue,job,priority,validate};NativeAsyncPromotion data;return native_async_promote(view,data);
}
} // namespace halo2::native_async_promotion_value_detail
