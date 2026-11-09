#include "halo2/native_tu15_convex_simplex_solver.hpp"
#include "halo2/native_tu15_convex_simplex_input_observer.hpp"
#include "halo2/native_tu15_simplex_direction_kernel.hpp"
#include "halo2/native_tu15_simplex_candidate_kernel.hpp"
#include "halo2/native_tu15_simplex_finalize_kernel.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <optional>
#include <stdexcept>
namespace halo2 {
namespace {
void narrow_store(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t bits,unsigned n){
    std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(bits>>(i*8u));m.write(p,std::span(b).first(n));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace joint_constraint_numeric;
Vector subtract(Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
// The semantic facade keeps its established provided-row domain. The shared
// candidate algorithm owns the actual score/read/comparison ordering; this
// adapter refuses wider rows at the same post-primary-comparison boundary.
struct CandidateAccess {
    Memory&m;NativeTu15ConvexSimplexSolverScratch scratch;
    NativeTu15ConvexSimplexSolverStats&stats;
    using Data=NativeTu15SimplexCandidate;
    using P=NativeTu15SimplexCandidatePhase;
    bool early(const Data&d)const{return d.side==NativeTu15SimplexCandidateSide::first_early;}
    std::uint32_t word_read(P,const Data&,std::uint32_t address){return m.word(address);}
    Vector vector_read(P,const Data&,std::uint32_t address){return m.vector(address);}
    void score_write(const Data&d,std::uint32_t bits){m.word(early(d)?0x30eeeau:0x30f4cau,scratch.constant+8u,bits);}
    std::uint16_t compare_score(const Data&){
        push(m.word(scratch.constant+8u));asm volatile("fabs":::"memory","st");
        multiply_scalar(m.word(scratch.constant+8u));const auto threshold=m.word(scratch.constant+4u);
        std::uint16_t relation;asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(threshold):"memory","st");return relation;
    }
    void prepare(P,const Data&d){
        if(d.count!=1u){++stats.projection_refusals;throw std::runtime_error("Native simplex solver requires its multi-edge candidate checks");}
    }
    void word_write(P,const Data&d,std::uint32_t address,std::uint32_t value){m.word(early(d)?0x30ef9eu:0x30f57au,address,value);}
    void complete(const Data&){}
};
struct FinalizeAccess {
    Memory&m;SourcePhysicsContactMemory&storage;
    NativeTu15ConvexSimplexSolverInput input;NativeTu15ConvexSimplexSolverScratch scratch;
    std::uint32_t computed_result;std::optional<Environment>phase;
    using Data=NativeTu15SimplexFinalize;using P=NativeTu15SimplexFinalizePhase;using F=NativeTu15SimplexFinalizeFrame;
    FinalizeAccess(Memory&m,SourcePhysicsContactMemory&storage,NativeTu15ConvexSimplexSolverInput input,
        NativeTu15ConvexSimplexSolverScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,std::uint32_t result)
        :m(m),storage(storage),input(input),scratch(scratch),computed_result(result){phase.emplace(fp,depth);}
    std::uint32_t argument(P p,const Data&)const{return p==P::plane_argument?input.plane:input.support_cache;}
    std::uint32_t frame_address(F f,const Data&)const{return f==F::normalization?scratch.normalization:scratch.constant;}
    Vector vector_read(P,const Data&,std::uint32_t address){return m.vector(address);}
    void vector_write(P p,const Data&,std::uint32_t address,Vector value){m.vector(p==P::length?0x30f60du:0x30f65eu,address,value);}
    std::uint32_t estimate(const Data&d,std::uint32_t){
        // This existing DATA API owns an explicit direct-host estimate policy;
        // the separate raw facade retains its actual input-journal callback.
        auto value=d.xy;asm volatile("rsqrtss %1,%0":"+x"(value):"x"(d.length):"memory");return scalar_bits(value);
    }
    std::uint32_t word_read(P,const Data&,std::uint32_t address,unsigned width){return m.word(address,width);}
    void word_write(P p,const Data&,std::uint32_t address,std::uint32_t value,unsigned width){
        if(width!=4u){narrow_store(storage,address,value,width);return;}
        std::uint32_t pc;
        switch(p){
        case P::estimate_store:pc=0x30f616u;break;
        case P::three:pc=0x30f629u;break;
        case P::half:pc=0x30f63bu;break;
        case P::distance:pc=0x30f67fu;break;
        case P::plane_distance:pc=0x30f68au;break;
        default:throw std::runtime_error("Invalid semantic simplex finalization store");
        }
        m.word(pc,address,value);
        // Preserve the established facade boundary: its source numerical FP
        // environment ends before the byte/key copies, even on memory faults.
        if(p==P::plane_distance)phase.reset();
    }
    void prepare(P,const Data&){}
    std::uint32_t result(const Data&)const{return computed_result;}
    void complete(const Data&){}
};
#endif
}
std::uint32_t native_tu15_solve_convex_contact_simplex(NativeTu15ConvexSimplexMemory&memory,
    NativeTu15ConvexSimplexSolverInput input,NativeTu15ConvexSimplexSolverScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConvexSimplexSolverServices&services,
    const std::function<std::uint64_t()>&clock,NativeTu15ConvexSimplexSolverStats*activity){
    NativeTu15ConvexSimplexSolverStats local;auto&stats=activity?*activity:local;++stats.attempts;
    if(const auto&observer=native_tu15_convex_simplex_input_observer())observer(memory,input,fp,depth);
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth)throw std::runtime_error("Native simplex solver requires empty x87 DATA");
        if((input.work|input.relative_transform|input.plane|scratch.normalization)&15u)
            throw std::runtime_error("Native simplex solver requires aligned owned DATA");
        memory.withhold(input.work,12u);memory.withhold(input.work+0xdu,0x133u);
        memory.withhold(input.plane,16u);
        const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);
        const auto first_count=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(input.support_cache+8u,1)))));
        m.word(0x30e944u,input.work,first_count);
        const auto second_count=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(input.support_cache+9u,1)))));
        m.word(0x30e954u,input.work+4u,second_count);
        const auto first_now=m.word(input.work);
        const auto first_table=m.word(input.first.identity);
        // At30E95D three arguments are still pushed: +28 is the iteration
        // counter, while30E965's +20 is the actual epsilon at caller+14.
        // Keep the source float bits, not a zero inferred from the first store.
        m.word(0x30e965u,scratch.epsilon,0x2edbe6ffu);
        if((first_count!=1u&&first_count!=2u)||second_count!=1u){++stats.initial_count_refusals;throw std::runtime_error("Native simplex solver requires its wider initial simplex");}
        if(!services.features)throw std::runtime_error("Native simplex solver requires native feature retrieval");
        const auto first_owner=m.word(first_table+0x34u);
        ++stats.feature_queries;services.features(memory,input.first,{first_owner},input.support_cache,first_now,input.work+0x20u,fp);
        // The table is loaded before current counts, but the virtual owner
        // operand is read at CALL after those source-current count loads.
        const auto second_table=m.word(input.second.identity);
        const auto current_second=m.word(input.work+4u),current_first=m.word(input.work);
        const auto second_owner=m.word(second_table+0x34u);
        ++stats.feature_queries;services.features(memory,input.second,{second_owner},input.support_cache+current_first*2u,current_second,input.work+0xe0u,fp);
        {
            Environment phase(fp,depth);
            const auto count=m.word(input.work+4u);
            if(count!=1u){++stats.initial_count_refusals;throw std::runtime_error("Native simplex solver requires native transformed-row continuation");}
            const auto point=m.vector(input.work+0xe0u),x=m.vector(input.relative_transform),y=m.vector(input.relative_transform+0x10u);
            const auto x_term=multiply(x,lane<0u>(point)),y_term=multiply(y,lane<0x55u>(point));
            const auto z=m.vector(input.relative_transform+0x20u),z_term=multiply(z,lane<0xaau>(point));
            const auto translation=m.vector(input.relative_transform+0x30u),zt=add(z_term,translation),xy=add(x_term,y_term);
            m.vector(0x30e9e9u,input.work+0xa0u,add(xy,zt));
        }
        std::uint32_t iteration{},result{};bool reduction{};
        for(;;){
          bool project{};
          {
            Environment phase(fp,depth);
            const auto a=m.word(input.work);push(m.word(scratch.epsilon));
            const auto b=m.word(input.work+4u);asm volatile("fadd %%st(0),%%st(0)":::"memory","st");
            m.pop_float(0x30ea13u,scratch.epsilon);
            ++iteration;
            m.word(0x30ea22u,input.work+0x10u,0u);
            if(a==1u&&b==1u){
                const auto opposite=m.vector(input.work+0xa0u),first=m.vector(input.work+0x20u);
                m.vector(0x30f261u,input.work+0x120u,subtract(opposite,first));
                m.word(0x30f26au,input.work+8u,m.word(input.work+4u));
            }else if(a==2u&&b==1u){
                // Exact edge/vertex30F175..30F23C, with source sign-bit tests
                // and shuffle/multiply/subtract order for the triple cross.
                const auto endpoint=m.vector(input.work+0x30u),first=m.vector(input.work+0x20u),other=m.vector(input.work+0xa0u);
                auto projection=native_simplex_direction_numeric::project(endpoint,first,other);
                if(projection.endpoint_sign==projection.first_sign){
                    ++stats.projection_refusals;throw std::runtime_error("Native simplex solver requires its edge endpoint reduction");
                }
                native_simplex_direction_numeric::interior(projection,true);
                m.vector(0x30f22eu,input.work+0x120u,projection.direction);
                m.word(0x30f237u,input.work+8u,m.word(input.work+4u));
            }else{
                ++stats.initial_count_refusals;throw std::runtime_error("Native simplex solver requires its simplex switch continuation");
            }
            const auto axis=m.vector(input.work+0x120u),squared=multiply(axis,axis);
            const auto squared_length=sum_three(squared);
            const auto first_now=m.vector(input.work+0x20u);
            const auto axis_now=m.vector(input.work+0x120u);
            m.word(0x30ee08u,scratch.squared_length,squared_length);
            const auto second_now=m.vector(input.work+0xa0u),product=multiply(subtract(second_now,first_now),axis_now);
            m.word(0x30ee36u,scratch.projection,sum_three(product));
            push(m.word(scratch.projection));multiply_scalar(m.word(scratch.projection));multiply_scalar(m.word(scratch.projection));
            push(m.word(scratch.epsilon));multiply_scalar(m.word(0x41d658u));multiply_scalar(m.word(scratch.squared_length));
            std::uint16_t relation;asm volatile("fcompp;fnstsw %0":"=a"(relation)::"memory","st","st(1)");
            project=(relation&0x100u)!=0;
            if(project){push(m.word(scratch.squared_length));multiply_scalar(m.word(scratch.epsilon));m.pop_float(0x30ee7au,scratch.constant+4u);}
          }
          if(!project){reduction=true;break;}
          const auto first_trial=[&](bool early){
              // The parity of the source iteration selects which shape is
              // tried first. Its cache limit is signed, including -1.
              const auto limit=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(input.support_cache+10u,1)))));
              const auto count=m.word(input.work);
              if(count==limit||m.word(input.work+0x10u)==1u)return false;
              const auto table=m.word(input.first.identity),owner=m.word(table+0x30u);
              if(!services.single.support)throw std::runtime_error("Native simplex solver requires its actual first support owner");
              services.single.support(memory,input.first,{owner},input.work+0x120u,input.work+(count+2u)*16u,fp);
              Environment phase(fp,depth);
              CandidateAccess access{m,scratch,stats};NativeTu15SimplexCandidate candidate;
              candidate.side=early?NativeTu15SimplexCandidateSide::first_early:NativeTu15SimplexCandidateSide::first_late;
              candidate.work=input.work;candidate.count_address=input.work;candidate.direction_address=input.work+0x120u;
              return native_tu15_test_simplex_candidate(access,candidate);
          };
          if((iteration&1u)&&first_trial(true))continue;
          const auto second_limit=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(input.support_cache+11u,1)))));
          const auto second_current=m.word(input.work+4u);
          if(second_current!=second_limit){++stats.projection_refusals;throw std::runtime_error("Native simplex solver requires its second-shape projection support");}
          if((iteration&1u)||!first_trial(false))break;
        }
        if(reduction&&m.word(input.work+0xcu,1u)) {
            // Actual30F5A5..30F5CF: overlap queries keep the current simplex,
            // cap its two signed counts only when their sum exceeds four,
            // and return3. They do not call the30DE60 distance reduction.
            const auto second_count=m.word(input.work+4u),first_count=m.word(input.work);
            if(std::bit_cast<std::int32_t>(first_count+second_count)>4) {
                if(std::bit_cast<std::int32_t>(first_count)>
                    std::bit_cast<std::int32_t>(second_count)) {
                    m.word(0x30f5b5u,input.work,3u);
                    m.word(0x30f5bbu,input.work+4u,1u);
                } else {
                    m.word(0x30f5c3u,input.work,1u);
                    m.word(0x30f5c9u,input.work+4u,3u);
                }
            }
            result=3u;
        } else if(reduction) {
            result=native_tu15_solve_single_vertex_convex_simplex(memory,
                {input.work,input.first,input.second,input.relative_transform},scratch.single,
                fp,depth,services.single,clock,&stats.single);
        }
        FinalizeAccess finish{m,memory,input,scratch,fp,depth,result};NativeTu15SimplexFinalize final;
        final.work=input.work;final.second_count_address=input.work+4u;final.axis_address=input.work+0x120u;
        result=native_tu15_finalize_simplex_contact(finish,final);
        ++stats.completed;return result;
#else
        (void)memory;(void)input;(void)scratch;(void)fp;(void)depth;(void)services;(void)clock;
        throw std::runtime_error("Exact native simplex solver backend unavailable");
#endif
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
