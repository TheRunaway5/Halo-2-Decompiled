#include "halo2/source_async_jobs.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
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
}
bool source_async_before(const SourceAsyncNode &incoming,const SourceAsyncNode &other,
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
void source_async_initialize(SourceAsyncQueue &queue){
    const auto counter=queue.counter,added=queue.requests_added;
    queue=SourceAsyncQueue{};queue.counter=counter;queue.requests_added=added;
    for(unsigned i=0;i<150;++i)queue.nodes[i].next=i+1<150?i+1:source_async_none;
    queue.free_head=0;
}
std::optional<unsigned> source_async_take_free(SourceAsyncQueue &queue){
    chain(queue,queue.free_head);
    if(queue.free_head==source_async_none)return {};
    const auto at=queue.free_head;queue.free_head=queue.nodes[at].next;
    queue.nodes[at].next=source_async_none;return at;
}
void source_async_insert(SourceAsyncQueue &queue,unsigned node){
    unlinked(queue,node);
    auto &incoming=queue.nodes[node];
    if(queue.work_head==source_async_none || incoming.priority>queue.nodes[queue.work_head].priority){
        incoming.next=queue.work_head;queue.work_head=node;return;
    }
    SourceAsyncInsertState state;
    auto previous=queue.work_head;
    while(queue.nodes[previous].next!=source_async_none){
        const auto next=queue.nodes[previous].next;
        if(source_async_before(incoming,queue.nodes[next],state)){
            incoming.next=next;queue.nodes[previous].next=node;return;
        }
        previous=next;
    }
    queue.nodes[previous].next=node;incoming.next=source_async_none;
}
std::uint32_t source_async_publish(SourceAsyncQueue &queue,unsigned node){
    source_async_insert(queue,node);
    queue.nodes[node].job=(queue.counter<<8)|node;++queue.counter;
    return queue.nodes[node].job;
}
void source_async_unlink(SourceAsyncQueue &queue,unsigned node){
    valid(node);chain(queue,queue.work_head);
    if(queue.work_head==node)queue.work_head=queue.nodes[node].next;
    else{
        auto previous=queue.work_head;
        while(previous!=source_async_none && queue.nodes[previous].next!=node)
            previous=queue.nodes[previous].next;
        if(previous==source_async_none)throw std::invalid_argument("source async unlink needs a queued node");
        queue.nodes[previous].next=queue.nodes[node].next;
    }
    queue.nodes[node].job=source_async_none;
}
bool source_async_reprioritize(SourceAsyncQueue &queue,std::uint32_t job,std::int32_t priority){
    const auto node=job&255u;valid(node);
    if(queue.nodes[node].job!=job)return false;
    //120CE0 unlinks without clearing its existing job identity.
    source_async_unlink(queue,node);queue.nodes[node].job=job;
    queue.nodes[node].priority=priority;source_async_insert(queue,node);return true;
}
unsigned source_async_count(const SourceAsyncQueue &queue){
    chain(queue,queue.work_head);unsigned count=0;
    for(auto at=queue.work_head;at!=source_async_none;at=queue.nodes[at].next)++count;
    return count;
}
bool source_async_has_category(const SourceAsyncQueue &queue,std::uint32_t category){
    chain(queue,queue.work_head);
    for(auto at=queue.work_head;at!=source_async_none;at=queue.nodes[at].next)
        if(queue.nodes[at].category==category)return true;
    return false;
}
void source_async_return_free(SourceAsyncQueue &queue,unsigned node){
    //120C94 is reached only after120A30 removed the node. JobNONE alone
    // cannot establish this: complete120900 insertion does not assign an ID.
    unlinked(queue,node);
    if(queue.nodes[node].job!=source_async_none)
        throw std::invalid_argument("source async return needs an unlinked node");
    queue.nodes[node].next=queue.free_head;queue.free_head=node;
}
std::uint32_t source_async_begin_request(SourceAsyncQueue &queue,const SourceAsyncRequest &request){
    if(!request.done)throw std::invalid_argument("source async request requires actual done storage");
    ++queue.requests_added;return request.done;
}
void source_async_bind_request(SourceAsyncQueue &queue,unsigned node,const SourceAsyncRequest &request){
    valid(node);unlinked(queue,node);
    queue.nodes[node].task=request.task;queue.nodes[node].priority=request.priority;
    queue.nodes[node].category=request.category;queue.nodes[node].callback=request.callback;
    queue.nodes[node].done=request.done;
}
SourceAsyncCallbackEffect source_async_callback_effect(std::int32_t result,std::uint32_t done){
    SourceAsyncCallbackEffect effect;
    if(!result)effect.signal_again=true;
    else{effect.remove=true;if(result==1 && done)effect.set_done=done;}
    return effect;
}
SourceAsyncTask source_async_read_task(const SourceAsyncReadInput &input){
    SourceAsyncTask task;
    task.words[0]=input.file;task.words[1]=input.buffer;task.words[2]=input.size;
    task.words[3]=input.offset;task.words[4]=input.bytes_read;return task;
}
SourceAsyncReadStep source_async_read_begin(const SourceAsyncTask &task,std::uint32_t limit){
    const auto remaining=task.words[2]-task.words[5];
    return {task.words[0],task.words[3],task.words[1]+task.words[5],remaining<limit?remaining:limit};
}
SourceAsyncReadCompletion source_async_read_finish(SourceAsyncTask &task,
    const SourceAsyncReadStep &step,const SourceAsyncReadResult &result){
    if(step.file!=task.words[0] || step.offset!=task.words[3] ||
       step.buffer!=task.words[1]+task.words[5])
        throw std::invalid_argument("source async read result does not match prepared task");
    std::uint32_t transferred=0;SourceAsyncReadCompletion completion;
    if(result.seek_low!=source_async_none){
        transferred=result.bytes_read;task.words[5]+=transferred;task.words[3]+=transferred;
        if(task.words[4])completion.cumulative_write=task.words[5];
    }else if(result.bytes_read)
        throw std::invalid_argument("source failed seek cannot supply a ReadFile outcome");
    completion.callback_result=(task.words[2]!=task.words[5] && step.size<=transferred)?0:1;
    return completion;
}
} // namespace halo2
