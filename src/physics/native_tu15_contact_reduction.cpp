#include "halo2/native_tu15_contact_reduction.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace joint_constraint_numeric;
Vector subtract(Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::uint16_t compare_metric(std::uint32_t bits){std::uint16_t sw;asm volatile("flds %1;fcomp %%st(1);fnstsw %0":"=a"(sw):"m"(bits):"memory","st");return sw;}
void pop_metric(){asm volatile("fstp %%st(0)":::"memory","st");}
void publish_fp(NativeTu15PhysicsFrameFp&fp){asm volatile("fnstcw %0":"=m"(fp.control_word));asm volatile("fnstsw %0":"=a"(fp.status));fp.mxcsr=_mm_getcsr();}
void resume_fp(const NativeTu15PhysicsFrameFp&fp){
    if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
        throw std::runtime_error("Native contact reduction requires its FP exception owner");
    std::array<std::byte,28>env{};asm volatile("fnstenv %0":"=m"(env)::"memory");const std::uint16_t empty=0xffff;
    std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);
    asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
}
#endif
}
std::uint32_t native_tu15_reduce_contact_records(SourcePhysicsContactMemory&memory,
    std::uint32_t array,std::uint32_t manager,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ContactDistanceServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ContactReductionStats*stats){
    if(stats)++stats->attempts;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth>6u)throw std::runtime_error("Native contact reduction requires its x87 capacity owner");
        Environment environment(fp,depth);Memory m(memory,observe);
        std::array<bool,5>selected{};std::uint32_t chosen{};
        push(m.word(0x46a0e8u));const auto initial=m.word(array);
        const auto update=[&](std::uint32_t metric,unsigned index){
            if(!(compare_metric(metric)&0x4100u)){pop_metric();chosen=index;push(metric);}
        };
        for(unsigned n=0;n<5;++n){
            if(selected[n]||n==chosen)continue;
            const auto point=m.word(array+n*4u);const auto other=m.vector(point),first=m.vector(initial);
            const auto delta=subtract(first,other);update(sum_three(multiply(delta,delta)),n);
        }
        const auto first_slot=chosen*4u;pop_metric();push(m.word(0x46a0e8u));
        auto anchor=m.word(array+first_slot);selected[chosen]=true;
        for(unsigned n=0;n<5;++n){
            if(selected[n])continue;
            const auto point=m.word(array+n*4u);const auto other=m.vector(point),first=m.vector(anchor);
            const auto delta=subtract(first,other);update(sum_three(multiply(delta,delta)),n);
        }
        const auto second_anchor=m.word(array+chosen*4u);pop_metric();anchor=m.word(array+first_slot);
        push(m.word(0x46a0e8u));const auto second=m.vector(second_anchor),first=m.vector(anchor);
        selected[chosen]=true;const auto axis=subtract(first,second);
        for(unsigned n=0;n<5;++n){
            if(selected[n])continue;
            const auto point=m.word(array+n*4u);const auto other=m.vector(point),origin=m.vector(second_anchor);
            const auto delta=subtract(origin,other);
            const auto left=multiply(lane<0xc9u>(delta),lane<0xd2u>(axis));
            const auto right=multiply(lane<0xd2u>(delta),lane<0xc9u>(axis));
            const auto cross=subtract(left,right);update(sum_three(multiply(cross,cross)),n);
        }
        anchor=m.word(array+chosen*4u);pop_metric();push(m.word(0x46a0e8u));selected[chosen]=true;
        for(unsigned n=0;n<5;++n){
            if(selected[n])continue;
            const auto point=m.word(array+n*4u);const auto other=m.vector(point),origin=m.vector(anchor);
            const auto delta=subtract(origin,other);update(sum_three(multiply(delta,delta)),n);
        }
        pop_metric();selected[chosen]=true;std::uint32_t count=5;
        for(unsigned n=0;n<count;){
            if(selected[n]){++n;continue;}
            const auto record=m.word(array+n*4u),point=m.word(record+0x1cu,2),table=m.word(manager),owner=m.word(table+8u);
            if(stats)++stats->removed_points;
            publish_fp(fp);
            try{if(!services.remove_point)throw std::runtime_error("Native contact reduction requires current point-removal owner");services.remove_point(manager,{owner},std::uint16_t(point),fp);resume_fp(fp);}
            catch(...){resume_fp(fp);throw;}
            const auto last=m.word(array+(count-1u)*4u);auto value=m.vector(last);const auto destination=m.word(array+n*4u);
            m.vector(0x30ccc0u,destination,value);value=m.vector(last+0x10u);m.vector(0x30ccc7u,destination+0x10u,value);
            value=m.vector(last+0x20u);m.vector(0x30ccd3u,destination+0x20u,value);
            selected[n]=selected[count-1u];--count;if(stats)++stats->record_moves;
        }
        if(stats)++stats->completed;return count;
#else
        (void)memory;(void)array;(void)manager;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native contact reduction backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
