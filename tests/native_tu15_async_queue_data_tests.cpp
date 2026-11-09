#include "halo2/native_tu15_async_queue_value.hpp"
#include "reference/native_tu15_async_queue_value_reference.hpp"
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool value,const char*why){if(!value)throw std::runtime_error(why);}
}
int main()try{
    std::mt19937_64 random{0x1232c01ab330ull};unsigned comparisons{},insertions{};
    constexpr std::array priorities{0u,1u,0xffffffffu,0x80000000u,0x7fffffffu,100u};
    for(unsigned n=0;n<32768;++n){
        SourceAsyncNode incoming,other;incoming.priority=std::bit_cast<std::int32_t>(priorities[n%priorities.size()]);
        other.priority=n%3?incoming.priority:std::bit_cast<std::int32_t>(std::uint32_t(random()));
        incoming.callback=n&1?source_async_read_callback:0x12345678u;other.callback=n&2?source_async_read_callback:0x87654321u;
        incoming.task.words[0]=std::uint32_t(random());other.task.words[0]=n&4?incoming.task.words[0]:std::uint32_t(random());
        incoming.task.words[3]=std::uint32_t(random());other.task.words[3]=n&8?incoming.task.words[3]:std::uint32_t(random());
        SourceAsyncInsertState a{std::uint32_t(random()),std::uint32_t(random()),std::uint32_t(random())};if(!(n&16))a.valid_word&=0xffffff00u;auto b=a,c=a;
        const auto expected=async_queue_value_reference::before(incoming,other,a),actual=native_async_queue_value_detail::before(incoming,other,b),public_result=source_async_before(incoming,other,c);
        check(expected==actual&&actual==public_result&&a.file==b.file&&a.offset==b.offset&&a.valid_word==b.valid_word&&
            !std::memcmp(&a,&c,sizeof(a)),"Typed locality comparison diverged from preserved/public owner");++comparisons;
    }
    for(unsigned seed=0;seed<128;++seed){
        SourceAsyncQueue a,b,c;const auto count=std::array{0u,1u,2u,3u,7u,17u,149u,150u}[seed%8];
        for(unsigned n=0;n<count;++n){
            SourceAsyncNode node;node.priority=std::bit_cast<std::int32_t>(priorities[(n+seed)%priorities.size()]);
            if(seed&1)node.priority=100;
            node.callback=(n+seed)&1?source_async_read_callback:0x12345678u;
            node.task.words[0]=std::uint32_t(random());node.task.words[3]=std::uint32_t(random());node.next=source_async_none;
            a.nodes[n]=node;b.nodes[n]=node;c.nodes[n]=node;async_queue_value_reference::insert(a,n);native_async_queue_value_detail::insert(b,n);source_async_insert(c,n);
            check(!std::memcmp(&a,&b,sizeof(a))&&!std::memcmp(&a,&c,sizeof(a)),"Typed priority/locality insertion diverged from preserved/public owner");++insertions;
        }
    }
    std::cout<<"PASS private typed async values comparisons="<<comparisons<<" insertions="<<insertions<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
