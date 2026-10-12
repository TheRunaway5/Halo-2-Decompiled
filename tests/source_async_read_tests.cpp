#include "halo2/source_async_jobs.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <span>
#include <stdexcept>
#include <vector>

using namespace halo2;
namespace {
std::vector<std::uint32_t> evaluate(std::span<const std::uint32_t> in){
    if(in.empty())throw std::invalid_argument("source async read fixture is empty");
    if(in[0]==0){
        if(in.size()!=11)throw std::invalid_argument("source async read request fixture length differs");
        SourceAsyncQueue queue;queue.requests_added=in[9];queue.counter=in[10];
        SourceAsyncRequest request;
        request.task=source_async_read_task({in[1],in[2],in[3],in[4],in[5]});
        request.done=in[6];request.priority=std::bit_cast<std::int32_t>(in[7]);
        request.category=in[8];request.callback=source_async_read_callback;
        const auto clear=source_async_begin_request(queue,request);
        std::vector<std::uint32_t> out(request.task.words.begin(),request.task.words.end());
        out.insert(out.end(),{queue.requests_added,clear,0,queue.counter});return out;
    }
    if(in[0]==1 || in[0]==2){
        if(in.size()!=15)throw std::invalid_argument("source async read phase fixture length differs");
        SourceAsyncTask task;for(unsigned i=0;i<10;++i)task.words[i]=in[1+i];
        const auto step=source_async_read_begin(task,in[11]);
        if(in[0]==1)return {step.file,step.offset,step.buffer,step.size};
        const auto completed=source_async_read_finish(task,step,{in[12],in[13]});
        std::vector<std::uint32_t> out(task.words.begin(),task.words.end());
        out.insert(out.end(),{std::bit_cast<std::uint32_t>(completed.callback_result),
            completed.cumulative_write.has_value(),completed.cumulative_write.value_or(0),
            completed.cumulative_write.value_or(in[14])});return out;
    }
    if(in[0]==3){
        if(in.size()!=4)throw std::invalid_argument("source async callback fixture length differs");
        const auto effect=source_async_callback_effect(std::bit_cast<std::int32_t>(in[1]),in[2]);
        return {effect.remove,effect.signal_again,effect.set_done.has_value(),
            effect.set_done.value_or(0),effect.set_done?1u:in[3]};
    }
    if(in[0]==4){
        if(in.size()!=17)throw std::invalid_argument("source async generic request fixture length differs");
        SourceAsyncQueue queue;queue.requests_added=in[15];queue.counter=in[16];
        SourceAsyncRequest request;
        for(unsigned i=0;i<10;++i)request.task.words[i]=in[1+i];
        request.priority=std::bit_cast<std::int32_t>(in[11]);request.category=in[12];
        request.callback=in[13];request.done=in[14];
        const auto done=source_async_begin_request(queue,request);
        std::vector<std::uint32_t> out{queue.requests_added,done,0,queue.counter};
        out.insert(out.end(),request.task.words.begin(),request.task.words.end());return out;
    }
    if(in[0]==5){
        if(in.size()!=31)throw std::invalid_argument("source async bound request fixture length differs");
        SourceAsyncQueue queue;auto &node=queue.nodes[0];
        node.priority=std::bit_cast<std::int32_t>(in[1]);node.job=in[2];
        for(unsigned i=0;i<10;++i)node.task.words[i]=in[3+i];
        node.category=in[13];node.callback=in[14];node.done=in[15];node.next=in[16];
        SourceAsyncRequest request;
        for(unsigned i=0;i<10;++i)request.task.words[i]=in[17+i];
        request.priority=std::bit_cast<std::int32_t>(in[27]);request.category=in[28];
        request.callback=in[29];request.done=in[30];
        source_async_bind_request(queue,0,request);
        std::vector<std::uint32_t> out{std::bit_cast<std::uint32_t>(node.priority),node.job};
        out.insert(out.end(),node.task.words.begin(),node.task.words.end());
        out.insert(out.end(),{node.category,node.callback,node.done,node.next});return out;
    }
    throw std::invalid_argument("unknown source async read fixture kind");
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream file;if(!probe){if(argc!=2)throw std::invalid_argument("source async read fixture required");
        file.open(argv[1]);if(!file)throw std::invalid_argument("source async read fixture missing");}
    auto &input=probe?std::cin:file;std::string line;unsigned count=0,checked=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        const auto split=line.find('|');std::istringstream before(line.substr(0,split));before>>std::hex;
        std::uint32_t word;std::vector<std::uint32_t> in,expected;
        while(before>>word)in.push_back(word);
        if(!before.eof())throw std::invalid_argument("source async read input malformed");
        const auto actual=evaluate(in);
        if(probe){for(auto value:actual)std::cout<<std::hex<<value<<' ';std::cout<<'\n';continue;}
        if(split==std::string::npos)throw std::invalid_argument("source async read fixture separator missing");
        std::istringstream after(line.substr(split+1));after>>std::hex;
        while(after>>word)expected.push_back(word);
        if(!after.eof() || actual!=expected)
            throw std::runtime_error("source async read differs at case "+std::to_string(count));
        ++count;checked+=unsigned(actual.size());
    }
    if(!probe){
        if(count!=8706 || checked!=112058)
            throw std::runtime_error("source async read original fixture is incomplete");
        unsigned controls=0;const auto reject=[&](auto &&action){bool failed=false;
            try{action();}catch(const std::invalid_argument&){failed=true;}
            if(!failed)throw std::runtime_error("unsupported async input admitted");
            ++controls;
        };
        SourceAsyncQueue queue;queue.counter=0xffffffffu;queue.requests_added=0xffffffffu;
        source_async_initialize(queue);
        if(queue.counter!=0xffffffffu || queue.requests_added!=0xffffffffu ||
           queue.free_head!=0 || queue.work_head!=source_async_none)
            throw std::runtime_error("source async initialization reset external counters");
        SourceAsyncRequest request;request.done=0x3000;request.callback=source_async_read_callback;
        if(source_async_begin_request(queue,request)!=0x3000 || queue.requests_added!=0)
            throw std::runtime_error("source async request counter does not wrap");
        std::vector<std::uint32_t> jobs;
        for(unsigned i=0;i<150;++i){
            const auto node=source_async_take_free(queue);
            if(node!=i)throw std::runtime_error("source async free-node order differs");
            request.priority=std::int32_t(i%7);request.task=source_async_read_task({72,0x4000,4,i,0});
            source_async_bind_request(queue,*node,request);jobs.push_back(source_async_publish(queue,*node));
        }
        if(source_async_take_free(queue) || source_async_count(queue)!=150)
            throw std::runtime_error("source async150-node exhaustion boundary differs");
        for(auto job:jobs){const auto node=job&255u;
            source_async_unlink(queue,node);source_async_return_free(queue,node);}
        if(source_async_count(queue)!=0 || queue.free_head!=149)
            throw std::runtime_error("source async return-free ordering differs");
        ++controls;
        reject([&]{source_async_insert(queue,149);}); // Still on the free list.
        queue.nodes[149].next=149;
        reject([&]{source_async_take_free(queue);});
        queue=SourceAsyncQueue{};queue.work_head=2;queue.nodes[2].next=2;
        reject([&]{source_async_count(queue);});
        queue=SourceAsyncQueue{};
        reject([&]{source_async_unlink(queue,0);});
        source_async_insert(queue,0);
        reject([&]{source_async_return_free(queue,0);});
        source_async_insert(queue,1);queue.free_head=2;
        reject([&]{source_async_return_free(queue,1);});
        if(queue.work_head!=0 || queue.nodes[0].next!=1 || queue.free_head!=2 ||
           queue.nodes[1].next!=source_async_none)
            throw std::runtime_error("source async invalid return mutated an owned list");
        reject([&]{source_async_reprioritize(queue,0xff,0);});
        request.done=0;reject([&]{source_async_begin_request(queue,request);});
        SourceAsyncTask task=source_async_read_task({72,0x4000,4,0,0});
        auto step=source_async_read_begin(task,4);
        reject([&]{source_async_read_finish(task,step,{source_async_none,1});});
        step.offset=1;reject([&]{source_async_read_finish(task,step,{0,0});});
        std::cout<<count<<" source async read/protocol cases; "<<checked<<" words exact; "
            <<controls<<" lifetime/ownership controls\n";
    }
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
