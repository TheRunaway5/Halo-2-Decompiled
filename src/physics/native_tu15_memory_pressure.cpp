#include "halo2/native_tu15_memory_pressure.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <emmintrin.h>
#include <xmmintrin.h>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver* observe_{};
public:
    explicit Memory(SourcePhysicsContactMemory&m):memory_(m){}
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o)
        :memory_(m),observe_(&o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));
        std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t n,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned k=0;k<width;++k)b[k]=std::byte(n>>(8u*k));
        SourcePhysicsContactRecordStore event{site,p,{},{}};
        if(observe_&&*observe_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
        memory_.write(p,std::span(b).first(width));if(observe_&&*observe_)(*observe_)(event);
    }
};
class SsePolicy {
    NativeTu15PhysicsFrameFp& fp_;unsigned saved_;
public:
    explicit SsePolicy(NativeTu15PhysicsFrameFp&fp):fp_(fp),saved_(_mm_getcsr()){
        if((fp.mxcsr&0xffff0000u)||(fp.mxcsr&0x1f80u)!=0x1f80u)
            throw std::runtime_error("Native memory pressure requires its qualified masked SSE policy");
        _mm_setcsr(fp.mxcsr);
    }
    ~SsePolicy(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(saved_);}
};
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
__m128 integer(std::uint32_t bits){auto out=_mm_setzero_ps();const auto value=std::bit_cast<std::int32_t>(bits);asm volatile("cvtsi2ss %1,%0":"+x"(out):"r"(value):"memory");return out;}
__m128 multiply(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char result;asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;}
bool positive(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>0;}
__m128 pool_free(Memory&m,std::uint32_t pool,NativeTu15MemoryPressureStats*stats){
    if(stats)++stats->pool_queries;std::uint32_t sum{},cursor=pool+0xf0384u;
    for(unsigned n=0;n<16;++n){const auto total=m.word(cursor);const auto occupied=m.word(cursor+0x44u);const auto stride=m.word(cursor-0xf02dcu);sum+=(total-occupied)*stride;cursor-=4u;}
    const auto converted=integer(sum);
    const auto scale=scalar(m.word(0x46a134u));
    return multiply(converted,scale);
}
void missing(NativeTu15MemoryPressureStats*stats,const char*message){if(stats)++stats->unbound_services;throw std::runtime_error(message);}
void begin(const NativeTu15MemoryPressureServices&s,NativeTu15PhysicsFrameFp&fp,NativeTu15MemoryPressureStats*stats){if(!s.begin_cleanup)missing(stats,"Native memory pressure requires actual1CB330 cleanup owner");s.begin_cleanup(fp);}
void finish(const NativeTu15MemoryPressureServices&s,NativeTu15PhysicsFrameFp&fp,NativeTu15MemoryPressureStats*stats){if(!s.finish_cleanup)missing(stats,"Native memory pressure requires actual1CB220 cleanup owner");s.finish_cleanup(fp);}
}
std::uint32_t native_tu15_pool_pressure_metric_bits(SourcePhysicsContactMemory&memory,std::uint32_t pool,
    NativeTu15PhysicsFrameFp&fp,NativeTu15MemoryPressureStats*stats){
    SsePolicy policy(fp);Memory m(memory);return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(pool_free(m,pool,stats))));
}
std::uint32_t native_tu15_memory_pressure_level(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsFrameFp&fp,NativeTu15MemoryPressureStats*stats){
    if(stats)++stats->level_queries;Memory m(memory);
    std::uint32_t first{},second{},level{};
    const auto first_pool=m.word(0x486074u);if(first_pool&&positive(m.word(first_pool+0x20u)))first=3;
    const auto second_pool=m.word(0x486070u);if(second_pool&&positive(m.word(second_pool+0x20u)))second=3;
    const auto pool=m.word(0x48606cu);
    if(pool){
        SsePolicy policy(fp);
        const auto occupied_integer=integer(m.word(pool+0xf0340u));
        const auto scale=scalar(m.word(0x46a144u));
        const auto occupied=multiply(occupied_integer,scale);
        const auto threshold=scalar(m.word(0x456910u));level=3;
        if(greater(threshold,occupied)){
            const auto free=pool_free(m,pool,stats);
            if(!greater(scalar(m.word(0x46a140u)),free))level=3;
            else if(!greater(scalar(m.word(0x46a13cu)),free))level=2;
            else if(!greater(scalar(m.word(0x46a138u)),free))level=1;
            else level=0;
        }
    }
    return std::max(level,std::max(first,second));
}
std::uint32_t native_tu15_restore_emergency_allocator(SourcePhysicsContactMemory&memory,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MemoryPressureStats*stats){
    if(stats)++stats->restorations;Memory m(memory,observe);auto buffer=m.word(0x51df44u),bytes=0x452e8u;
    m.word(0x14968eu,0x51df48u,1,1);
    if(buffer&15u){const auto padding=(0u-buffer)&15u;buffer+=padding;bytes-=padding;}
    const auto allocator=m.word(0x48605cu)+0x105410u;
    if(!allocator){m.word(0x1496e6u,0x486070u,0);return 0;}
    m.word(0x1496b3u,allocator+4u,1);m.word(0x1496bau,allocator+8u,0);
    m.word(0x1496bdu,allocator+12u,0);m.word(0x1496c0u,allocator+16u,0);
    m.word(0x1496c3u,allocator+20u,0xffffffffu);bytes&=0x0ffffffcu;
    m.word(0x1496d0u,allocator+32u,0);m.word(0x1496d3u,allocator+24u,buffer);
    m.word(0x1496d6u,allocator+28u,bytes);m.word(0x1496d9u,allocator,0x46393cu);
    m.word(0x1496dfu,0x486070u,allocator);return allocator;
}
void native_tu15_collect_memory_pressure_batch(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15MemoryPressureServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MemoryPressureStats*stats){
    Memory m(memory,observe);const bool had=m.word(0x486070u)!=0;begin(services,fp,stats);
    m.word(0x1498b1u,0x48b872u,1,1);finish(services,fp,stats);
    m.word(0x1498bfu,0x48b872u,0,1);
    if(had&&!m.word(0x486070u))native_tu15_restore_emergency_allocator(memory,observe,stats);
    if(stats)++stats->cleanup_batches;
}
void native_tu15_collect_memory_pressure(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15MemoryPressureServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MemoryPressureStats*stats){
    if(stats)++stats->calls;Memory m(memory,observe);
    if(m.word(0x48b873u,1)){if(stats)++stats->reentrant_skips;return;}
    auto level=native_tu15_memory_pressure_level(memory,fp,stats);
    auto last_cleanup=m.word(0x486058u);m.word(0x149711u,0x48b873u,1,1);
    std::uint32_t successful{};
    for(std::uint32_t mode=0;mode<3&&std::bit_cast<std::int32_t>(level)>=2;++mode){
        if(std::bit_cast<std::int32_t>(level)>=3){
            if(!services.trim_objects)missing(stats,"Native memory pressure requires actual1CD130 trim owner");
            if(stats)++stats->trims;services.trim_objects(true,fp);last_cleanup=m.word(0x486058u);
        }
        level=native_tu15_memory_pressure_level(memory,fp,stats);
        for(unsigned pass=0;pass<3&&std::bit_cast<std::int32_t>(level)>=2;++pass){
            unsigned removed{};
            for(;;){
                bool recent=false;
                if(last_cleanup!=0xffffffffu){const auto clock=m.word(0x51df54u);recent=std::bit_cast<std::int32_t>(m.word(clock+8u)-last_cleanup)<10;}
                if(!services.remove_candidate)missing(stats,"Native memory pressure requires actual1CCBE0 candidate owner");
                if(stats)++stats->candidate_calls;
                if(services.remove_candidate({mode,0xffffffffu,pass==2,pass>1,recent},fp)){
                    if(stats)++stats->candidate_removals;
                    level=native_tu15_memory_pressure_level(memory,fp,stats);
                    if(std::bit_cast<std::int32_t>(level)<2){last_cleanup=m.word(0x486058u);break;}
                    ++removed;++successful;
                    if(!(successful&7u)){
                        const bool had=m.word(0x486070u)!=0;begin(services,fp,stats);
                        m.word(0x1497edu,0x48b872u,1,1);finish(services,fp,stats);
                        m.word(0x1497fbu,0x48b872u,0,1);
                        if(had&&!m.word(0x486070u))native_tu15_restore_emergency_allocator(memory,observe,stats);
                        if(stats)++stats->cleanup_batches;
                        level=native_tu15_memory_pressure_level(memory,fp,stats);
                        const auto clock=m.word(0x51df54u);last_cleanup=m.word(clock+8u);
                        m.word(0x149822u,0x486058u,last_cleanup);
                    }else last_cleanup=m.word(0x486058u);
                    if(removed<=50)continue;
                }else if(mode==0&&!recent){
                    native_tu15_collect_memory_pressure_batch(memory,fp,services,observe,stats);
                    level=native_tu15_memory_pressure_level(memory,fp,stats);
                    const auto clock=m.word(0x51df54u);last_cleanup=m.word(clock+8u);
                    m.word(0x149857u,0x486058u,last_cleanup);
                }else last_cleanup=m.word(0x486058u);
                break;
            }
        }
    }
    m.word(0x14988cu,0x48b873u,0,1);
}
}
