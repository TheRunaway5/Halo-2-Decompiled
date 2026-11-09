#pragma once
#include "halo2/source_async_jobs.hpp"
#include <stdexcept>
namespace halo2::async_queue_value_reference {
void valid(unsigned index){if(index>=150)throw std::invalid_argument("source async node outside150-slot bank");}
void chain(const SourceAsyncQueue &queue,std::uint32_t head){
    std::array<bool,150> seen{};
    while(head!=source_async_none){valid(head);
        if(seen[head])throw std::invalid_argument("source async list cycle");
        seen[head]=true;head=queue.nodes[head].next;
    }
}
void unlinked(const SourceAsyncQueue &queue,unsigned node){
    valid(node);chain(queue,queue.work_head);chain(queue,queue.free_head);
    for(auto at=queue.work_head;at!=source_async_none;at=queue.nodes[at].next)
        if(at==node)throw std::invalid_argument("source async insertion needs an unlinked node");
    for(auto at=queue.free_head;at!=source_async_none;at=queue.nodes[at].next)
        if(at==node)throw std::invalid_argument("source async insertion needs a leased node");
}
bool before(const SourceAsyncNode &incoming,const SourceAsyncNode &other,
    SourceAsyncInsertState &state){
    bool result=false;
    if(incoming.priority>other.priority)result=true;
    else if(incoming.priority==other.priority && incoming.callback==source_async_read_callback){
        if(other.callback!=source_async_read_callback)result=true;
        else if((state.valid_word&255u)!=0){
            const auto incoming_file=incoming.task.words[0]-state.file;
            const auto other_file=other.task.words[0]-state.file;
            result=incoming_file<other_file || (incoming_file==other_file &&
                incoming.task.words[3]-state.offset<other.task.words[3]-state.offset);
        }
    }
    if(other.callback==source_async_read_callback){
        state.file=other.task.words[0];state.offset=other.task.words[3];
        state.valid_word=(state.valid_word&0xffffff00u)|1u;
    }
    return result;
}
void insert(SourceAsyncQueue &queue,unsigned node){
    unlinked(queue,node);
    auto &incoming=queue.nodes[node];
    if(queue.work_head==source_async_none || incoming.priority>queue.nodes[queue.work_head].priority){
        incoming.next=queue.work_head;queue.work_head=node;return;
    }
    SourceAsyncInsertState state;
    auto previous=queue.work_head;
    while(queue.nodes[previous].next!=source_async_none){
        const auto next=queue.nodes[previous].next;
        if(before(incoming,queue.nodes[next],state)){
            incoming.next=next;queue.nodes[previous].next=node;return;
        }
        previous=next;
    }
    queue.nodes[previous].next=node;incoming.next=source_async_none;
}
}
