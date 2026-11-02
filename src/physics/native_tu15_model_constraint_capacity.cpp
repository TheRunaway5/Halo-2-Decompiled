#include "halo2/native_tu15_model_constraint_capacity.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class Capacity {
    SourcePhysicsContactMemory&memory_;NativeTu15PhysicsFrameFp&fp_;
    const NativeTu15ModelConstraintCapacityServices&services_;
    const SourcePhysicsContactRecordObserver&observer_;NativeTu15ModelConstraintCapacityStats*stats_;
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t out{};
        for(unsigned n=0;n<width;++n)out|=std::to_integer<unsigned>(b[n])<<(n*8u);return out;
    }
    void store(std::uint32_t site,std::uint32_t p,std::uint32_t value){
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(4);memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(event);
    }
    std::uint32_t count(std::uint32_t list){const auto pool=word(0x52d690u),entries=word(pool+0x44u);return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(word(entries+(list&0xffffu)*64u+2u,2)))));}
    bool greater(std::uint32_t first,std::uint32_t second){
        if((fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 constraint capacity requires its unsupported SIMD trap owner");
        const auto prior=_mm_getcsr();_mm_setcsr(fp_.mxcsr);const auto a=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(first))),b=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(second)));unsigned char result;
        asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");fp_.mxcsr=_mm_getcsr();_mm_setcsr(prior);return result;
    }
    void remove(std::uint32_t datum){if(!services_.remove_constraint){if(stats_)++stats_->unbound_removals;throw std::runtime_error("Native TU1.5 constraint capacity requires actual22D0A0 lifetime ownership");}services_.remove_constraint(datum,fp_);}
    std::uint32_t stack_candidate(){ //22DD90
        if(stats_)++stats_->stack_queries;auto index=word(0x50ec10u);if(index==0xffffffffu)return index;
        const auto candidate=word(0x50ec14u+index*4u);if(std::bit_cast<std::int32_t>(index)<0)return candidate;
        while(word(0x50ec14u+index*4u)==0xffffffffu){--index;store(0x22ddbau,0x50ec10u,index);if(std::bit_cast<std::int32_t>(index)<0)break;}
        return candidate;
    }
    bool refuse(){if(stats_)++stats_->refusals;return false;}
public:
    Capacity(SourcePhysicsContactMemory&m,NativeTu15PhysicsFrameFp&fp,const NativeTu15ModelConstraintCapacityServices&s,const SourcePhysicsContactRecordObserver&o,NativeTu15ModelConstraintCapacityStats*stats):memory_(m),fp_(fp),services_(s),observer_(o),stats_(stats){}
    bool run(NativeTu15PhysicsComponent component,std::uint32_t slot,std::uint32_t extent){
        if(stats_)++stats_->calls;
        if(slot!=0xffffffffu){const auto entries=word(component.identity+0x70u);if(!(word(entries+slot*0x60u+0x45u,1)&1u)){if(stats_)++stats_->slot_refusals;return false;}}
        const auto list=word(component.identity+0x20u);std::uint32_t selected=0xffffffffu;
        if(list!=0xffffffffu){
            const auto pool=word(0x52d690u),entries=word(pool+0x44u),size=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(word(entries+(list&0xffffu)*64u+2u,2)))));
            if(size==15u){if(stats_)++stats_->local_full;std::uint32_t index{};
                while(less(index,size)){
                    const auto datum=word(entries+(list&0xffffu)*64u+index*4u+4u),instances=word(0x52d68cu),bank=word(instances+0x44u),candidate_extent=word(bank+(datum&0xffffu)*160u+4u);
                    if(stats_)++stats_->local_candidates;if(greater(candidate_extent,extent))selected=index;++index;
                }
            }
        }
        if(list!=0xffffffffu&&!less(count(list),15u)&&selected==0xffffffffu)return refuse();
        if(selected!=0xffffffffu){
            const auto pool=word(0x52d690u),entries=word(pool+0x44u),datum=word(entries+(list&0xffffu)*64u+selected*4u+4u);
            if(stats_)++stats_->local_removals;remove(datum);
        }else{
            const auto pool=word(0x52d68cu);
            if(word(pool+0x3cu)==32u){
                const auto datum=stack_candidate();
                if(datum!=0xffffffffu){const auto entries=word(pool+0x44u),candidate_extent=word(entries+(datum&0xffffu)*160u+4u);
                    if(greater(candidate_extent,extent)){
                        if(stats_)++stats_->global_removals;remove(datum);const auto index=word(0x50ec10u);
                        if(less(0u,index))store(0x1dd8eau,0x50ec10u,index-1u);else store(0x1dd8f1u,0x50ec10u,0xffffffffu);
                    }
                }
            }
        }
        const auto pool=word(0x52d68cu);if(!less(word(pool+0x3cu),32u))return refuse();
        const auto current_list=word(component.identity+0x20u);if(current_list!=0xffffffffu&&!less(count(current_list),15u))return refuse();return true;
    }
};
}
bool native_tu15_admit_model_constraint(SourcePhysicsContactMemory&memory,NativeTu15PhysicsComponent component,
 std::uint32_t slot,std::uint32_t extent,NativeTu15PhysicsFrameFp&fp,const NativeTu15ModelConstraintCapacityServices&services,
 const SourcePhysicsContactRecordObserver&observer,NativeTu15ModelConstraintCapacityStats*stats){return Capacity(memory,fp,services,observer,stats).run(component,slot,extent);}
} // namespace halo2
