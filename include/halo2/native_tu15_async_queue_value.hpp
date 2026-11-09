#pragma once
#include "halo2/native_tu15_async_queue_kernel.hpp"
#include "halo2/source_async_jobs.hpp"

namespace halo2::native_async_queue_value_detail {
struct OrderAccess {
    const SourceAsyncNode&incoming;const SourceAsyncNode&other;SourceAsyncInsertState&history;
    std::uint32_t read(NativeAsyncOrderRead kind,const NativeAsyncOrder&)const{
        using R=NativeAsyncOrderRead;
        switch(kind){
        case R::other_callback:return other.callback;
        case R::history:return 0;
        case R::incoming_callback:return incoming.callback;
        case R::valid:return history.valid_word;
        case R::history_file:return history.file;
        case R::incoming_file:return incoming.task.words[0];
        case R::other_file:case R::record_file:return other.task.words[0];
        case R::history_offset:return history.offset;
        case R::incoming_offset:return incoming.task.words[3];
        case R::other_offset:case R::record_offset:return other.task.words[3];
        }
        return 0;
    }
    void write(NativeAsyncOrderWrite kind,const NativeAsyncOrder&,std::uint32_t value){
        switch(kind){
        case NativeAsyncOrderWrite::file:history.file=value;break;
        case NativeAsyncOrderWrite::offset:history.offset=value;break;
        case NativeAsyncOrderWrite::valid:history.valid_word=(history.valid_word&0xffffff00u)|(value&255u);break;
        }
    }
    void decision(const NativeAsyncOrder&)const{}
    void finish(const NativeAsyncOrder&)const{}
};
inline bool before(const SourceAsyncNode&incoming,const SourceAsyncNode&other,SourceAsyncInsertState&history){
    OrderAccess access{incoming,other,history};NativeAsyncOrder data;
    data.incoming_priority=incoming.priority;data.other_priority=other.priority;data.read_callback=source_async_read_callback;
    return native_async_before(access,data);
}
struct InsertAccess {
    SourceAsyncQueue&queue;std::uint32_t node;SourceAsyncInsertState history_state{};
    static bool is_null(std::uint32_t value){return value==source_async_none;}
    static std::uint32_t null(){return source_async_none;}
    std::uint32_t read(NativeAsyncInsertRead kind,const NativeAsyncInsertion&d)const{
        using R=NativeAsyncInsertRead;
        switch(kind){
        case R::head:return queue.work_head;
        case R::incoming:case R::saved_incoming:return node;
        case R::incoming_priority:return std::bit_cast<std::uint32_t>(queue.nodes[d.incoming].priority);
        case R::head_priority:return std::bit_cast<std::uint32_t>(queue.nodes[d.head].priority);
        case R::head_next:return queue.nodes[d.head].next;
        case R::incoming_callback:return queue.nodes[d.incoming].callback;
        case R::successor:case R::insertion_next:case R::after_compare_next:return queue.nodes[d.previous].next;
        case R::successor_callback:return queue.nodes[d.successor].callback;
        case R::saved_callback:return d.incoming_callback;
        case R::successor_priority:return std::bit_cast<std::uint32_t>(queue.nodes[d.successor].priority);
        case R::saved_priority:return d.incoming_priority;
        }
        return source_async_none;
    }
    void write(NativeAsyncInsertWrite kind,const NativeAsyncInsertion&d,std::uint32_t value){
        switch(kind){
        case NativeAsyncInsertWrite::head:queue.work_head=value;break;
        case NativeAsyncInsertWrite::new_next:queue.nodes[d.incoming].next=value;break;
        case NativeAsyncInsertWrite::previous_next:queue.nodes[d.previous].next=value;break;
        }
    }
    NativeAsyncCompareResume compare(const NativeAsyncInsertion&d){
        return{before(queue.nodes[d.incoming],queue.nodes[d.successor],history_state),d.previous,d.successor};
    }
    void begin(const NativeAsyncInsertion&)const{}
    void history(const NativeAsyncInsertion&){}
    void priority(const NativeAsyncInsertion&)const{}
    void candidates(const NativeAsyncInsertion&)const{}
    void callback(const NativeAsyncInsertion&)const{}
    void restore_candidates(const NativeAsyncInsertion&)const{}
    void restore_previous(const NativeAsyncInsertion&)const{}
    void finish(const NativeAsyncInsertion&)const{}
};
// Caller retains the existing queue validation and exclusive ownership checks.
inline void insert(SourceAsyncQueue&queue,std::uint32_t node){
    InsertAccess access{queue,node};NativeAsyncInsertion data;native_async_insert(access,data);
}
} // namespace halo2::native_async_queue_value_detail
