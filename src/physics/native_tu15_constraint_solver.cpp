#include "native_tu15_constraint_solver_internal.hpp"
#include <cstring>
#include <utility>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
using namespace native_tu15_solver_detail;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};
    std::array<std::byte,28>native_{};unsigned host_mxcsr_{};
    NativeTu15PhysicsFrameFp&fp_;bool active_{};
    static void supported(const NativeTu15PhysicsFrameFp&fp){if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 solver requires its unsupported FP trap owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>2)throw std::runtime_error("Native TU1.5 solver exceeds its preserved x87 depth");supported(fp);
        host_mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
    void publish(){asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){supported(fp_);const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class Function>void dependency(Function&&function){publish();try{function();}catch(...){resume();throw;}resume();}
};
__m128 simd(const Vector&v){return _mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(v[3]),std::bit_cast<std::int32_t>(v[2]),std::bit_cast<std::int32_t>(v[1]),std::bit_cast<std::int32_t>(v[0])));}
Vector words(__m128 value){Vector out;_mm_storeu_si128(reinterpret_cast<__m128i*>(out.data()),_mm_castps_si128(value));return out;}
__m128 add_ps(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub_ps(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul_ps(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add_ss(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::uint32_t projected_xyz(__m128 value){const auto xy=add_ss(_mm_shuffle_ps(value,value,0x55),value);return words(add_ss(_mm_shuffle_ps(value,value,0xaa),xy))[0];}
void push(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void multiply(std::uint32_t value){asm volatile("fmuls %0"::"m"(value):"memory","st");}
std::uint32_t spill(){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");return value;}
std::uint32_t difference(std::uint32_t first,std::uint32_t second){push(first);std::uint32_t value;asm volatile("fsubs %1\n\tfstps %0":"=m"(value):"m"(second):"memory","st");return value;}
bool signed_less(std::uint32_t first,std::uint32_t second){return std::bit_cast<std::int32_t>(first)<std::bit_cast<std::int32_t>(second);}

void load_primary_header(Access&m,PrimaryState&state){state.first_accumulator=m.word(state.schema+0xcu);state.second_accumulator=m.word(state.schema+0x10u);state.jacobian=m.word(state.schema+4u);state.schema+=0x18u;}
void accumulate_velocities(Access&m,std::uint32_t first,bool add_saved){
    auto cursor=first;auto type=m.byte(cursor);
    for(;;){
        if(type==3)return;
        if(type!=2)throw std::runtime_error("Native TU1.5 solver requires its malformed accumulator-tag owner");
        const auto angular=simd(m.vector(cursor+0x20u));
        if(add_saved){const auto working=simd(m.vector(cursor+0x50u));m.vector(0x32823cu,cursor+0x50u,words(add_ps(working,angular)));}
        else m.vector(0x328206u,cursor+0x50u,words(angular));
        const auto linear=simd(m.vector(cursor+0x10u));
        if(add_saved){const auto working=simd(m.vector(cursor+0x40u));m.vector(0x32824bu,cursor+0x40u,words(add_ps(working,linear)));}
        else m.vector(0x32820eu,cursor+0x40u,words(linear));
        type=m.byte(cursor+0x80u);cursor+=0x80u;
    }
}
// A secondary projection subtracts the current body velocity contribution
// from each retained Jacobian bias. Projected SIMD scalars spill to binary32
// before the native x87 subtraction, exactly as in the original.
void project_full(Access&m,std::uint32_t first,std::uint32_t second,std::uint32_t row){
    const auto jacobian_a=simd(m.vector(row+0x10u));const auto angular_a=simd(m.vector(first+0x20u));
    const auto linear_b=simd(m.vector(second+0x10u));const auto angular_b=simd(m.vector(second+0x20u));
    auto projected=mul_ps(angular_a,jacobian_a);auto linear_a=simd(m.vector(first+0x10u));
    linear_a=sub_ps(linear_a,linear_b);const auto jacobian_linear=simd(m.vector(row));linear_a=mul_ps(linear_a,jacobian_linear);
    const auto jacobian_b=simd(m.vector(row+0x20u));const auto projected_b=mul_ps(angular_b,jacobian_b);
    projected=add_ps(projected,projected_b);projected=add_ps(projected,linear_a);const auto contribution=projected_xyz(projected);
    const auto bias=m.word(row+0xcu);m.word(0x32855eu,row+0xcu,difference(bias,contribution));
}
void project_angular(Access&m,std::uint32_t first,std::uint32_t second,std::uint32_t row,std::uint32_t site){
    const auto jacobian_a=simd(m.vector(row));const auto angular_a=simd(m.vector(first+0x20u));const auto angular_b=simd(m.vector(second+0x20u));
    auto projected=mul_ps(angular_a,jacobian_a);const auto jacobian_b=simd(m.vector(row+0x10u));const auto projected_b=mul_ps(angular_b,jacobian_b);
    projected=add_ps(projected,projected_b);const auto contribution=projected_xyz(projected);const auto bias=m.word(row+0x1cu);m.word(site,row+0x1cu,difference(bias,contribution));
}
void project_pair(Access&m,std::uint32_t first,std::uint32_t second,std::uint32_t row,std::uint32_t first_site,std::uint32_t second_site){
    const auto angular_a=simd(m.vector(first+0x20u));const auto row_first_a=simd(m.vector(row+0x10u));
    const auto linear_b=simd(m.vector(second+0x10u));const auto row_first_b=simd(m.vector(row+0x20u));
    auto first_projected=mul_ps(angular_a,row_first_a);const auto row_second_a=simd(m.vector(row+0x40u));auto second_projected=mul_ps(angular_a,row_second_a);
    auto linear_a=simd(m.vector(first+0x10u));linear_a=sub_ps(linear_a,linear_b);const auto angular_b=simd(m.vector(second+0x20u));
    const auto first_b=mul_ps(angular_b,row_first_b);const auto row_second_b=simd(m.vector(row+0x50u));const auto second_b=mul_ps(angular_b,row_second_b);
    const auto row_first_linear=simd(m.vector(row));second_projected=add_ps(second_projected,second_b);first_projected=add_ps(first_projected,first_b);
    const auto row_second_linear=simd(m.vector(row+0x30u));const auto second_linear=mul_ps(linear_a,row_second_linear);second_projected=add_ps(second_projected,second_linear);
    const auto second_contribution=projected_xyz(second_projected);const auto first_linear=mul_ps(linear_a,row_first_linear);first_projected=add_ps(first_projected,first_linear);const auto first_contribution=projected_xyz(first_projected);
    const auto first_bias=m.word(row+0xcu);m.word(first_site,row+0xcu,difference(first_bias,first_contribution));
    const auto second_bias=m.word(row+0x3cu);m.word(second_site,row+0x3cu,difference(second_bias,second_contribution));
}
void secondary_projection(Access&m,std::uint32_t first_schema,NativeTu15ConstraintSolverStats*stats){
    auto schema=first_schema;std::uint32_t first{},second{},row{};std::optional<std::int32_t> pending;
    auto header=[&]{first=m.word(schema+0xcu);second=m.word(schema+0x10u);row=m.word(schema+4u);schema+=0x18u;};header();
    for(;;){
        const auto type=pending?std::exchange(pending,{}).value():m.opcode(schema+2u);
        if(std::uint32_t(type)>6u)continue;
        if(type==0)return;
        if(type==1){header();continue;}
        if(type==2){schema+=m.byte(schema+1u);continue;}
        if(type==3||type==4){
            do{if(type==3){project_full(m,first,second,row);row+=0x30u;}else{project_angular(m,first,second,row,0x3285bbu);row+=0x20u;}if(stats)++stats->secondary_rows;
                schema+=m.byte(schema+1u);
            }while(m.opcode(schema+2u)==type);
            continue;
        }
        if(type==5){
            project_pair(m,first,second,row,0x3282fdu,0x32830eu);row+=0x60u;if(stats)stats->secondary_rows+=2;
            schema+=m.byte(schema+1u);const auto next=m.opcode(schema+2u);
            if(next!=5){pending=next;continue;}
            project_pair(m,first,second,row,0x3283c5u,0x3283d6u);row+=0x60u;if(stats)stats->secondary_rows+=2;
            schema+=m.byte(schema+1u);if(m.byte(schema+2u)!=6u)continue;
        }
        project_pair(m,first,second,row,0x32848du,0x3284a5u);
        project_angular(m,first,second,row+0x60u,0x3284e5u);row+=0x80u;if(stats)stats->secondary_rows+=3;
        schema+=m.byte(schema+1u);if(m.byte(schema)==1u){header();continue;}
    }
}
void export_results(Access&m,const NativeTu15ConstraintSolverInput&input,NativeTu15ConstraintSolverStats*stats){
    auto schema=input.schemas,result=input.result_slots;std::uint32_t stride{},row{},output{};
    auto header=[&]{stride=m.word(schema+0x14u);row=m.word(schema+4u);output=m.word(schema+8u);schema+=0x18u;};header();
    constexpr std::array<unsigned,17>family{0,1,2,3,4,2,2,5,6,4,4,2,4,2,7,7,7};
    auto export_scalar=[&](std::uint32_t target,std::uint32_t result_at,std::uint32_t bias,std::uint32_t copy_site,std::uint32_t float_site){const auto value=m.word(result_at);m.word(copy_site,target,value);push(m.word(bias));multiply(m.word(input.configuration+0x10u));m.word(float_site,target+4u,spill());};
    for(;;){
        const auto opcode=m.opcode(schema);if(std::uint32_t(opcode)>16u)continue;const auto type=family[unsigned(opcode)];
        if(type==0)return;
        if(type==1){header();continue;}
        if(type==7){schema+=m.byte(schema+1u);continue;}
        if(type==2||type==4){
            do{export_scalar(output,result,row+(type==2?0xcu:0x1cu),type==2?0x328677u:0x3286bau,type==2?0x328686u:0x3286c7u);
                output+=stride;row+=type==2?0x30u:0x20u;const auto length=m.byte(schema+1u);const auto next=m.opcode(schema+length);schema+=length;result+=4u;if(stats)++stats->exported_rows;if(next!=opcode)break;
            }while(true);
            continue;
        }
        if(type==3){
            export_scalar(output,result,row+0xcu,0x328630u,0x328640u);output+=stride;
            export_scalar(output,result+4u,row+0x3cu,0x328648u,0x328655u);output+=stride;row+=0x60u;result+=8u;schema+=8u;if(stats)stats->exported_rows+=2;continue;
        }
        auto target=m.word(schema+4u);
        if(type==5){
            export_scalar(target,result,row+0xcu,0x3286f2u,0x328706u);const auto step=m.word(schema+0x10u);target+=step;
            export_scalar(target,result+4u,row+0x3cu,0x328711u,0x32871cu);row+=0x60u;result+=8u;schema+=0x14u;if(stats)stats->exported_rows+=2;continue;
        }
        export_scalar(target,result,row+0xcu,0x328736u,0x32874au);target+=m.word(schema+0x10u);
        export_scalar(target,result+4u,row+0x3cu,0x328755u,0x328762u);output=0;target+=m.word(schema+0x10u);
        export_scalar(target,result+8u,row+0x7cu,0x32876du,0x328778u);row+=0x80u;result+=12u;schema+=0x18u;if(stats)stats->exported_rows+=3;
    }
}
#endif
}
std::uint32_t native_tu15_solve_constraint_rows(SourcePhysicsContactMemory&memory,const NativeTu15ConstraintSolverInput&input,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConstraintSolverServices&services,const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintSolverStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);Access m(memory,observe);if(stats)++stats->calls;
    if(!m.byte(0x48d480u)){
        if(!services.initialize)throw std::runtime_error("Native TU1.5 solver requires actual initializer3268D0");if(stats)++stats->initializations;
        environment.dependency([&]{services.initialize(fp);});if(!m.byte(0x48d480u))return 0;
    }
    auto iteration_limit=m.word(input.configuration+0x1cu);std::uint32_t iteration{};
    if(signed_less(0,iteration_limit))do{
        auto saved_first=m.word(input.configuration+4u);auto saved_second=m.word(input.configuration+8u);
        PrimaryState state{input.configuration,input.schemas,0,0,0,input.result_slots,{}};load_primary_header(m,state);
        for(;;){
            const auto opcode=state.next_opcode?std::exchange(state.next_opcode,{}).value():m.opcode(state.schema);
            if(std::uint32_t(opcode)>16u)continue;
            if(opcode==0)break;
            if(opcode==1){load_primary_header(m,state);continue;}
            if(opcode==15){saved_second=m.word(input.configuration+8u);saved_first=m.word(input.configuration+4u);m.word(0x32817cu,input.configuration+8u,m.word(state.schema+8u));m.word(0x328182u,input.configuration+4u,m.word(state.schema+4u));state.schema+=12u;continue;}
            if(opcode==16){m.word(0x328194u,input.configuration+4u,saved_first);m.word(0x32819bu,input.configuration+8u,saved_second);state.schema+=4u;continue;}
            if(opcode==14){const NativeTu15PhysicsVirtualOwner owner{m.word(state.schema+4u)};if(!services.custom_schema)throw std::runtime_error("Native TU1.5 solver requires its actual custom schema owner");if(stats)++stats->custom_schemas;environment.dependency([&]{services.custom_schema(owner,state.jacobian,state.schema+8u,fp);});state.schema+=m.byte(state.schema+1u);continue;}
            if(stats)++stats->primary_dispatches;solve_primary_rows(m,state,unsigned(opcode));
        }
        accumulate_velocities(m,input.accumulators.identity,iteration!=0);secondary_projection(m,input.schemas,stats);++iteration;if(stats)++stats->iterations;
        iteration_limit=m.word(input.configuration+0x1cu);
    }while(signed_less(iteration,iteration_limit));
    export_results(m,input,stats);return 1;
#else
    (void)memory;(void)input;(void)fp;(void)depth;(void)services;(void)observe;(void)stats;throw std::runtime_error("Exact native TU1.5 constraint solver backend unavailable");
#endif
}
} // namespace halo2
