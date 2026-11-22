#include "halo2/native_tu15_memory_pressure.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include "halo2/native_tu15_contact_pressure.hpp"
#include "halo2/native_tu15_object_delete_protection.hpp"
#include "halo2/native_tu15_retained_counter_update.hpp"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <limits>
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
[[noreturn]] void missing(NativeTu15MemoryPressureStats*stats,const char*message){if(stats)++stats->unbound_services;throw std::runtime_error(message);}
void begin(const NativeTu15MemoryPressureServices&s,NativeTu15PhysicsFrameFp&fp,NativeTu15MemoryPressureStats*stats){if(!s.begin_cleanup)missing(stats,"Native memory pressure requires actual1CB330 cleanup owner");s.begin_cleanup(fp);}
void finish(const NativeTu15MemoryPressureServices&s,NativeTu15PhysicsFrameFp&fp,NativeTu15MemoryPressureStats*stats){if(!s.finish_cleanup)missing(stats,"Native memory pressure requires actual1CB220 cleanup owner");s.finish_cleanup(fp);}

struct HeapStatistics {std::uint32_t capacity{},allocated{};};
HeapStatistics heap_statistics(const Memory& m,std::uint32_t heap,std::uint32_t capacity) {
    if(!heap)return {};
    std::uint32_t allocated=0;
    for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(m.word(heap+0x20u));++i) {
        const auto extent=m.word(heap+0x1cu);
        const auto data=m.word(heap+0x18u);
        allocated+=m.word(data+extent-4u-i*8u);
    }
    return {capacity,allocated};
}

void read_island_statistics(const Memory& m,std::uint32_t head) {
    for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(m.word(head+4u));++i)
        (void)m.word(m.word(m.word(head)+4u*i)+0x40u);
}

struct ComponentStatistics {std::uint32_t contact{},body{},phantom{},constraint{};};
ComponentStatistics component_statistics(const Memory& m) {
    ComponentStatistics total;
    std::uint32_t index=0;
    for(;;) {
        const auto pool=m.word(0x52be5cu);
        const auto end=std::bit_cast<std::int32_t>(m.word(pool+0x38u));
        if(std::bit_cast<std::int32_t>(index)>=end)return total;
        const auto bitmap=m.word(pool+0x48u);
        while(!(m.word(bitmap+4u*(index>>5u))&(1u<<(index&31u)))) {
            if(std::bit_cast<std::int32_t>(++index)>=end)return total;
        }
        const auto stride=m.word(pool+0x24u);
        const auto component=m.word(pool+0x44u)+stride*index;
        if(!component)return total;
        total.contact+=m.word(component+0x8cu)*72u;
        total.constraint+=(m.word(component+0x84u)&0x7fffffffu)*12u;
        if(const auto links=m.word(component+0x94u))total.phantom+=(m.word(links+8u)&0x7fffffffu)*8u;
        total.body+=(m.word(component+0x78u)&0x7fffffffu)*96u;
        for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(m.word(component+0x74u));++i) {
            const auto count=m.word(m.word(component+0x70u)+96u*i+0x4cu);
            if(std::bit_cast<std::int32_t>(count)>4)total.body+=(count+3u)&~3u;
        }
        ++index;
    }
}

// The report writes only private CRT buffers, but inspects live x87 status.
// Keep the original source load, multiply and binary64 store under that policy.
void scale_report_integer(std::uint32_t value,std::uint32_t scale) {
    std::uint64_t result;
    asm volatile("fildl %1\n\tfmuls %2\n\tfstpl %0"
        :"=m"(result):"m"(value),"m"(scale):"st");
}

void scale_report_scalar(std::uint32_t value,std::uint32_t scale) {
    std::uint64_t result;
    asm volatile("flds %1\n\tfmuls %2\n\tfstpl %0"
        :"=m"(result):"m"(value),"m"(scale):"st");
}

void retain_report_double(std::uint64_t value) {
    std::uint64_t result;
    asm volatile("fldl %1\n\tfstpl %0":"=m"(result):"m"(value):"st");
}
}
std::uint32_t native_tu15_reclamation_body_candidate(SourcePhysicsContactMemory& memory,
    NativeTu15ContactPressureCandidate request,const NativeTu15ObjectDeleteProtection& protected_object) {
    constexpr std::uint32_t none=0xffffffffu;
    Memory m(memory);
    const auto count=m.word(request.body+0x34u);
    const auto component_property=[&] {
        if(positive(count)) {
            const auto properties=m.word(request.body+0x30u);
            for(std::uint32_t i=0;i<count;++i) {
                const auto property=properties+i*8u;
                if(m.word(property)==0x2001u)return m.word(property+4u);
            }
        }
        return 0u;
    };
    if(component_property()==none)return none;
    const auto component=component_property();
    if(component==request.excluded)return none;
    const auto component_rows=m.word(m.word(0x52be5cu)+0x44u);
    const auto object=m.word(component_rows+(component&0xffffu)*0xa0u+8u);
    if(!protected_object)throw std::runtime_error("Native Havok reclamation requires actual B1870 object protection");
    if(protected_object(object)&&!request.second_option)return none;

    const auto rows=m.word(m.word(0x4ecbd0u)+0x44u);
    const auto header=rows+(object&0xffffu)*12u;
    const auto object_data=m.word(header+8u);
    const auto flags=m.word(object_data+4u);
    if(!request.first_option&&!(m.word(header+2u,1)&1u))return none;
    if(flags&0x4000u)return object;
    const auto type=1u<<(m.word(header+3u,1)&31u);
    const auto mode=std::bit_cast<std::int32_t>(request.mode);
    if(type&3u)return mode>=2&&m.word(m.word(header+8u)+0x13cu)==none?object:none;
    return (type&0x800u)&&mode>=1?object:none;
}

std::uint32_t native_tu15_select_reclamation_candidate(SourcePhysicsContactMemory& memory,
    NativeTu15MemoryPressureCandidate request,const NativeTu15ObjectDeleteProtection& protected_object) {
    constexpr std::uint32_t none=0xffffffffu;
    Memory m(memory);
    const auto world=m.word(0x52be48u);
    const auto islands=world+(request.final_pass?8u:0x14u);
    std::uint32_t selected=none;
    auto priority=std::numeric_limits<std::int32_t>::min();
    const auto visit=[&](std::uint32_t island) {
        for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(m.word(island+0x40u));++i) {
            const auto body=m.word(m.word(island+0x3cu)+i*4u);
            const auto current=std::bit_cast<std::int32_t>(m.word(body+0x8cu));
            if(current<=priority)continue;
            const auto candidate=native_tu15_reclamation_body_candidate(memory,
                {body,request.mode,request.excluded,request.aggressive,request.recent_cleanup},protected_object);
            if(candidate==none)continue;
            priority=current;
            selected=candidate;
        }
    };
    if(!request.final_pass)visit(m.word(world+0x2cu));
    for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(m.word(islands+4u));++i)
        visit(m.word(m.word(islands)+i*4u));
    return selected;
}

bool native_tu15_reclaim_memory_if_no_candidate(SourcePhysicsContactMemory& memory,
    NativeTu15MemoryPressureCandidate request,NativeTu15MemoryPressureStats* stats) {
    const auto selected=native_tu15_select_reclamation_candidate(memory,request,
        [&](std::uint32_t object){return native_tu15_object_delete_protected(memory,object);});
    if(selected==0xffffffffu)return false;
    char message[112]{};
    std::snprintf(message,sizeof(message),
        "Native memory pressure requires actual1CCCDC selected-object reclamation for datum %08X",
        selected);
    missing(stats,message);
}

std::uint32_t native_tu15_pool_pressure_metric_bits(SourcePhysicsContactMemory&memory,std::uint32_t pool,
    NativeTu15PhysicsFrameFp&fp,NativeTu15MemoryPressureStats*stats){
    SsePolicy policy(fp);Memory m(memory);return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(pool_free(m,pool,stats))));
}
void native_tu15_record_havok_memory_statistics(SourcePhysicsContactMemory& memory,
        NativeTu15PhysicsFrameFp& fp,const SourcePhysicsContactRecordObserver& observe) {
    Memory m(memory);
    const auto world=m.word(0x52be48u);
    const auto allocator=m.word(0x48c938u);
    const auto table=allocator?m.word(allocator):0;
    if(!world || !table || m.word(table+0x1cu)!=0x2e4d60u)
        throw std::runtime_error("Native Havok statistics requires the actual 2E4D60 allocator statistics owner");
    if(m.word(0x46ec80u)!=0x331e10u || m.word(0x46e8e0u)!=0x41fa48u ||
       m.word(0x46e8e8u)!=1u || m.word(0x46e8ecu,1)!=0x2eu)
        throw std::runtime_error("Native Havok statistics requires the initialized fixed-decimal CRT provider");
    if(!m.word(0x48606cu) || !m.word(0x486068u) || !m.word(0x52be5cu))
        throw std::runtime_error("Native Havok statistics requires its live pool bindings");
    if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
       (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
        throw std::runtime_error("Native Havok statistics requires masked x87 and SSE exceptions");

    (void)m.word(world+0xcu);
    (void)m.word(world+0x18u);
    (void)m.word(m.word(world+0x2cu)+0x40u);
    native_tu15_adjust_retained_counter(memory,0x1cd178u,0x52d888u,2,1u,observe);
    read_island_statistics(m,m.word(0x52be48u)+8u);
    read_island_statistics(m,m.word(0x52be48u)+0x14u);
    native_tu15_adjust_retained_counter(memory,0x1cd1f2u,0x52d888u,2,0xffffffffu,observe);

    const auto startup=heap_statistics(m,m.word(0x486074u),0x30u);
    const auto overflow=heap_statistics(m,m.word(0x486070u),0x30u);
    const auto pool=m.word(0x48606cu);
    (void)m.word(pool+0xf0340u);
    const auto remaining=m.word(pool+0x1cu)-m.word(pool+0x20u)+1u;
    const auto usage=native_tu15_pool_pressure_metric_bits(memory,pool,fp);
    const auto diagnostics=heap_statistics(m,m.word(0x486068u),0x15040u);
    const auto kibibytes=m.word(0x46a524u);
    const auto header=m.word(0x46a568u)|(std::uint64_t(m.word(0x46a56cu))<<32u);
    const auto percent=m.word(0x4500b8u);
    {
        const halo2::detail::NativeTu15ArithmeticEnvironment arithmetic(fp);
        scale_report_scalar(usage,percent);
        scale_report_integer(0u,kibibytes);
        retain_report_double(header);
        for(const auto heap:{diagnostics,overflow,startup}) {
            scale_report_integer(heap.capacity,kibibytes);
            scale_report_integer(heap.allocated,kibibytes);
        }
        scale_report_integer(0x2000u-remaining,kibibytes);
        scale_report_integer(0x2000u,kibibytes);
        scale_report_integer(0xf03d0u,kibibytes);
    }
    const auto meta=component_statistics(m);
    const halo2::detail::NativeTu15ArithmeticEnvironment arithmetic(fp);
    scale_report_integer(meta.body,kibibytes);
    scale_report_integer(meta.contact,kibibytes);
    scale_report_integer(meta.constraint,kibibytes);
    scale_report_integer(meta.phantom,kibibytes);
    scale_report_integer(meta.body+meta.contact+meta.constraint+meta.phantom,kibibytes);
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
