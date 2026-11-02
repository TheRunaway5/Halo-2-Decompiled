#include "halo2/native_tu15_contact_publication.hpp"
#include "halo2/native_tu15_geometry.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <type_traits>
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace joint_constraint_numeric;
Vector subtract(Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
Vector scalar(std::uint32_t bits){Vector value{};std::memcpy(&value,&bits,4);return value;}
std::uint16_t compare_top_pop(){std::uint16_t sw;asm volatile("fcomp %%st(1);fnstsw %0":"=a"(sw)::"memory","st");return sw;}
std::uint16_t compare_pop(std::uint32_t bits){std::uint16_t sw;asm volatile("fcomps %1;fnstsw %0":"=a"(sw):"m"(bits):"memory","st");return sw;}
bool odd(std::uint16_t status){const auto bits=(status>>8)&5u;return bits==1u||bits==4u;}
void pop(){asm volatile("fstp %%st(0)":::"memory","st");}
void publish_fp(NativeTu15PhysicsFrameFp&fp){asm volatile("fnstcw %0":"=m"(fp.control_word));asm volatile("fnstsw %0":"=a"(fp.status));fp.mxcsr=_mm_getcsr();}
void resume_fp(const NativeTu15PhysicsFrameFp&fp){
    if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native contact publication requires its FP exception owner");
    std::array<std::byte,28>env{};asm volatile("fnstenv %0":"=m"(env)::"memory");const std::uint16_t empty=0xffff;
    std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
}
template<class F>decltype(auto)dependency(NativeTu15PhysicsFrameFp&fp,F&&call){publish_fp(fp);try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){call();resume_fp(fp);return;}else{auto result=call();resume_fp(fp);return result;}}catch(...){resume_fp(fp);throw;}}
#endif
}
void native_tu15_publish_contact(SourcePhysicsContactMemory&memory,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,NativeTu15ContactContext context,
    NativeTu15ContactRecord contact,std::uint32_t manager,std::uint32_t records,std::uint32_t count_field,
    NativeTu15ContactPublicationScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ContactPublicationServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ContactPublicationStats*stats){
    if(stats)++stats->attempts;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(scratch.identity&15u)throw std::runtime_error("Native contact publication requires aligned owned DATA");
        Environment environment(fp,depth);Memory m(memory,observe);const auto p=scratch.identity,c=contact.identity;
        const auto settings=m.word(context.identity+0xcu);push(m.word(settings+4u));
        const auto count=m.word(count_field);asm volatile("fld %%st(0);fmulp %%st,%%st(1)":::"memory","st","st(1)");
        std::uint32_t selected=0xffffffffu,index{};Vector point{};
        if(std::int32_t(count)>0){
            point=m.vector(c+0x10u);auto current=records;
            do{
                const auto cached=m.vector(current),delta=subtract(cached,point);const auto metric=sum_three(multiply(delta,delta));
                m.word(0x30cd63u,p+0x18u,metric);push(m.word(p+0x18u));
                if(odd(compare_top_pop())){
                    const auto normal=m.vector(c+0x40u),cached_normal=m.vector(current+0x20u),difference=subtract(cached_normal,normal);
                    const auto normal_metric=sum_three(multiply(difference,difference));m.word(0x30cd9cu,p+0x14u,normal_metric);
                    push(m.word(p+0x14u));if(odd(compare_pop(m.word(0x46a560u)))){pop();selected=index;push(m.word(p+0x18u));}
                }
                ++index;current+=48u;
            }while(std::int32_t(index)<std::int32_t(count));
            pop();
            if(std::int32_t(selected)>=0){
                const auto target=records+selected*48u;push(m.word(target+0x1cu));m.vector(0x30cddau,target,point);
                auto vector=m.vector(c+0x30u);m.vector(0x30cde1u,target+0x10u,vector);vector=m.vector(c+0x40u);m.vector(0x30cde9u,target+0x20u,vector);
                const auto distance=m.word(c+0x20u);m.word(0x30cdf0u,target+0x2cu,distance);const auto weight=m.word(c+0x24u);
                m.pop_float(0x30cdf6u,target+0x1cu);m.word(0x30cdf9u,target+0xcu,weight);if(stats){++stats->matched_records;++stats->completed;}return;
            }
        }else pop();
        const auto target=std::int32_t(index)<4?records+index*48u:p+0x80u;
        auto vector=m.vector(c+0x10u);m.vector(0x30ce1au,target,vector);vector=m.vector(c+0x30u);m.vector(0x30ce21u,target+0x10u,vector);vector=m.vector(c+0x40u);m.vector(0x30ce29u,target+0x20u,vector);
        const auto weight=m.word(c+0x24u);m.word(0x30ce30u,target+0xcu,weight);const auto distance=m.word(c+0x20u);
        m.word(0x30ce39u,target+0x2cu,distance);const auto first_motion=m.word(first.identity+8u),second_motion=m.word(second.identity+8u);
        dependency(fp,[&]{NativeTu15GeometryMemory data(memory,p+0x40u,first_motion+0x20u,target,observe);native_tu15_transform_geometry_position(data,fp);});
        dependency(fp,[&]{NativeTu15GeometryMemory data(memory,p+0x30u,second_motion+0x20u,target+0x10u,observe);native_tu15_transform_geometry_position(data,fp);});
        const auto current_weight=m.word(c+0x24u);const auto first_world=m.vector(p+0x40u),one=m.vector(0x41c500u);const auto current_distance=m.word(target+0x2cu);
        m.word(0x30ce79u,p+0x14u,current_weight);auto amount=lane<0u>(scalar(m.word(p+0x14u)));const auto table=m.word(manager);
        const auto first_part=multiply(amount,first_world),second_world=m.vector(p+0x30u),remainder=subtract(one,amount),normal=m.vector(target+0x20u),second_part=multiply(remainder,second_world);
        m.vector(0x30cea1u,p+0x50u,add(second_part,first_part));m.word(0x30cea6u,p+0x5cu,current_distance);m.vector(0x30cebau,p+0x60u,normal);
        const auto owner=m.word(table+4u);if(stats)++stats->point_requests;
        const auto point_id=dependency(fp,[&]{if(!services.create_point)throw std::runtime_error("Native contact publication requires current point-creation owner");return services.create_point(manager,{owner},first,second,context,p+0x50u,fp);});
        if(point_id!=0xffffu){
            m.word(0x30ced4u,target+0x1cu,std::uint32_t(point_id)|0x3f000000u);const auto current_count=m.word(count_field)+1u;m.word(0x30cedfu,count_field,current_count);if(stats)++stats->created_points;
            if(std::int32_t(current_count)>4){
                m.word(0x30ceecu,p+0x1cu,records);m.word(0x30cef5u,p+0x28u,records+0x90u);m.word(0x30cefdu,p+0x20u,records+0x30u);m.word(0x30cf05u,p+0x24u,records+0x60u);m.word(0x30cf0fu,p+0x2cu,target);
                if(stats)++stats->reductions;
                const auto remaining=dependency(fp,[&]{return native_tu15_reduce_contact_records(memory,p+0x1cu,manager,fp,depth,services.removal,observe,stats?&stats->reduction:nullptr);});
                m.word(0x30cf1bu,count_field,remaining);
            }
        }
        if(stats)++stats->completed;
#else
        (void)memory;(void)first;(void)second;(void)context;(void)contact;(void)manager;(void)records;(void)count_field;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native contact publication backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
