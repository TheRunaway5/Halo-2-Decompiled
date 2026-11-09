#include "halo2/native_tu15_async_promotion_value.hpp"
#include "reference/native_tu15_async_promotion_value_reference.hpp"
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
using namespace halo2;
int main()try{
    std::mt19937_64 random{0x1236a01232c0ull};unsigned cases{},promoted{},stale{};
    constexpr std::array priorities{0u,1u,0xffffffffu,0x80000000u,0x7fffffffu,100u};
    for(unsigned seed=0;seed<128;++seed){
        SourceAsyncQueue initial;const auto count=std::array{0u,1u,2u,3u,7u,17u,149u,150u}[seed%8];
        for(unsigned n=0;n<count;++n){
            auto&node=initial.nodes[n];node.job=(std::uint32_t(random())<<8u)|n;
            node.priority=std::bit_cast<std::int32_t>(priorities[(n+seed)%priorities.size()]);
            node.callback=(n+seed)&1?source_async_read_callback:0x12345678u;
            node.task.words[0]=std::uint32_t(random());node.task.words[3]=std::uint32_t(random());source_async_insert(initial,n);
        }
        auto a=initial,b=initial,c=initial;
        for(unsigned step=0;count&&step<64;++step){
            const auto slot=unsigned(random()%count);const auto job=a.nodes[slot].job+((step%4==0)?0x100u:0u);
            const auto priority=std::bit_cast<std::int32_t>(priorities[(step+seed)%priorities.size()]);
            const auto old=async_promotion_value_reference::reprioritize(a,job,priority);
            const auto data=native_async_promotion_value_detail::reprioritize(b,job,priority,[]{});
            const auto public_result=source_async_reprioritize(c,job,priority);
            if(old!=data||data!=public_result||std::memcmp(&a,&b,sizeof(a))||std::memcmp(&a,&c,sizeof(a)))
                throw std::runtime_error("Typed promotion diverged from preserved and public queue owners");
            ++cases;if(old)++promoted;else ++stale;
        }
    }
    std::cout<<"PASS typed promotion values cases="<<cases<<" promoted="<<promoted<<" stale="<<stale<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
