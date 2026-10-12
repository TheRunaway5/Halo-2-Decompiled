#include "halo2/source_async_jobs.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace halo2;
namespace {
using NodeWords=std::array<std::uint32_t,16>;
std::int32_t integer(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::vector<std::uint32_t> evaluate(std::span<const std::uint32_t> words){
    if(words.empty())throw std::invalid_argument("source async fixture kind missing");
    if(words[0]==0){
        if(words.size()!=12)throw std::invalid_argument("source async comparator fixture length differs");
        SourceAsyncNode incoming,other;
        incoming.priority=integer(words[1]);other.priority=integer(words[2]);
        incoming.callback=words[3];other.callback=words[4];
        incoming.task.words[0]=words[5];incoming.task.words[3]=words[6];
        other.task.words[0]=words[7];other.task.words[3]=words[8];
        SourceAsyncInsertState state{words[9],words[10],words[11]};
        const auto before=source_async_before(incoming,other,state);
        return {before,state.file,state.offset,state.valid_word};
    }
    const bool owned=words[0]>=7 && words[0]<=9;
    const unsigned width=owned?8:7;
    if(words.size()<width || words[5]>150 || words[width-1] ||
       words.size()!=width+words[5]*16 || (owned && words[5]!=150))
        throw std::invalid_argument("source async bank fixture length or reserved word differs");
    SourceAsyncQueue queue;queue.work_head=words[1];queue.counter=words[4];
    if(owned){queue.free_head=words[3];queue.requests_added=words[6];}
    const auto count=words[5];
    for(unsigned i=0;i<count;++i){
        NodeWords raw;
        for(unsigned j=0;j<16;++j)raw[j]=words[width+i*16+j];
        queue.nodes[i]=std::bit_cast<SourceAsyncNode>(raw);
    }
    std::uint32_t result=0;
    switch(words[0]){
        case 1:source_async_insert(queue,words[2]);break;
        case 2:result=source_async_publish(queue,words[2]);break;
        case 3:result=source_async_reprioritize(queue,words[2],integer(words[3]));break;
        case 4:result=source_async_count(queue);break;
        case 5:result=source_async_has_category(queue,words[2]);break;
        case 6:source_async_unlink(queue,words[2]);break;
        case 7:source_async_initialize(queue);break;
        case 8:result=source_async_take_free(queue).value_or(source_async_none);break;
        case 9:source_async_return_free(queue,words[2]);break;
        default:throw std::invalid_argument("unknown source async fixture kind");
    }
    std::vector<std::uint32_t> output{result,queue.work_head,queue.counter};
    if(owned)output.insert(output.end(),{queue.free_head,queue.requests_added});
    for(unsigned i=0;i<count;++i){
        const auto raw=std::bit_cast<NodeWords>(queue.nodes[i]);
        output.insert(output.end(),raw.begin(),raw.end());
    }
    return output;
}
unsigned negative_controls(){
    unsigned controls=0;
    const auto rejected=[&](auto &&action){
        bool failed=false;try{action();}catch(const std::invalid_argument&){failed=true;}
        if(!failed)throw std::runtime_error("unproved source async queue bank was admitted");
        ++controls;
    };
    SourceAsyncQueue queue;
    rejected([&]{source_async_insert(queue,150);});
    rejected([&]{source_async_reprioritize(queue,0x120096,2);});
    queue.work_head=0;queue.nodes[0].next=0;
    rejected([&]{source_async_count(queue);});
    rejected([&]{source_async_has_category(queue,0);});
    rejected([&]{source_async_insert(queue,1);});
    rejected([&]{source_async_unlink(queue,0);});
    queue.nodes[0].next=150;
    rejected([&]{source_async_count(queue);});
    queue.nodes[0].next=source_async_none;
    rejected([&]{source_async_insert(queue,0);});
    rejected([&]{source_async_unlink(queue,1);});
    queue.nodes[1].job=0x12340001;
    rejected([&]{source_async_reprioritize(queue,0x12340001,2);});
    // A stale full generation returns before inspecting queue links. This
    // verifies the source ID guard independently of the admitted bank corpus.
    queue.nodes[0].next=0;
    const auto untouched=queue;
    if(source_async_reprioritize(queue,0x12340101,2) ||
       std::bit_cast<NodeWords>(queue.nodes[0])!=std::bit_cast<NodeWords>(untouched.nodes[0]) ||
       std::bit_cast<NodeWords>(queue.nodes[1])!=std::bit_cast<NodeWords>(untouched.nodes[1]) ||
       queue.work_head!=untouched.work_head || queue.counter!=untouched.counter)
        throw std::runtime_error("source async stale generation consumed or changed the queue");
    ++controls;
    queue=SourceAsyncQueue{};queue.free_head=0;
    rejected([&]{source_async_insert(queue,0);});
    rejected([&]{source_async_publish(queue,0);});
    const std::array<std::uint32_t,1> unknown{10};
    rejected([&]{evaluate(unknown);});
    queue=SourceAsyncQueue{};queue.work_head=0;queue.nodes[0].next=1;queue.free_head=2;
    rejected([&]{source_async_return_free(queue,0);});
    rejected([&]{source_async_return_free(queue,1);});
    if(queue.work_head!=0 || queue.nodes[0].next!=1 ||
       queue.nodes[1].next!=source_async_none || queue.free_head!=2)
        throw std::runtime_error("source async invalid return mutated an owned list");
    rejected([&]{source_async_return_free(queue,2);});
    queue=SourceAsyncQueue{};queue.free_head=0;queue.nodes[0].next=0;
    rejected([&]{source_async_take_free(queue);});
    rejected([&]{source_async_return_free(queue,1);});
    queue=SourceAsyncQueue{};queue.nodes[0].job=0;
    rejected([&]{source_async_return_free(queue,0);});
    queue=SourceAsyncQueue{};
    if(source_async_take_free(queue) || queue.free_head!=source_async_none)
        throw std::runtime_error("source async empty take did not retain its availability boundary");
    ++controls;
    return controls;
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream file;
    if(!probe){
        if(argc!=2)throw std::invalid_argument("source async fixture required");
        file.open(argv[1]);if(!file)throw std::invalid_argument("source async fixture missing");
    }
    auto &input=probe?std::cin:file;std::string line;unsigned cases=0;std::size_t checked=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        const auto divider=line.find('|');
        std::istringstream before(line.substr(0,divider));before>>std::hex;
        std::vector<std::uint32_t> words,expected;std::uint32_t word;
        while(before>>word)words.push_back(word);
        if(!before.eof())throw std::invalid_argument("source async input malformed");
        const auto actual=evaluate(words);
        if(probe){for(const auto value:actual)std::cout<<std::hex<<value<<' ';std::cout<<'\n';continue;}
        if(divider==std::string::npos)throw std::invalid_argument("source async fixture separator missing");
        std::istringstream after(line.substr(divider+1));after>>std::hex;
        while(after>>word)expected.push_back(word);
        if(!after.eof() || actual!=expected)
            throw std::runtime_error("original source async queue differs at case "+std::to_string(cases));
        ++cases;checked+=actual.size();
    }
    if(!probe){
        if(cases!=6483 || checked!=3623481)
            throw std::runtime_error("source async queue frozen corpus incomplete: expected 6483 cases and 3623481 words; got "+
                std::to_string(cases)+" cases and "+std::to_string(checked)+" words");
        std::cout<<cases<<" source async queue cases; "<<checked<<" words exact; "
            <<negative_controls()<<" missing-bank/cycle/generation controls\n";
    }
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
