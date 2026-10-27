#include "halo2/native_tu15_constraint_step.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;}
    NativeTu15PhysicsVector vector(std::uint32_t p)const{std::array<std::byte,16>b{};m_.read_vector(p,b);NativeTu15PhysicsVector v{};for(unsigned n=0;n<4;++n)for(unsigned i=0;i<4;++i)v[n]|=std::to_integer<unsigned>(b[n*4u+i])<<(8u*i);return v;}
    void write(std::uint32_t site,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore store{site,p,{},{}};if(observe_){store.before.resize(b.size());m_.read(p,store.before);store.written.assign(b.begin(),b.end());}m_.write(p,b);if(observe_)observe_(store);}
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8u*i));write(site,p,b);}
    void byte(std::uint32_t site,std::uint32_t p,std::uint8_t v){const std::array b{std::byte(v)};write(site,p,b);}
    void vector(std::uint32_t site,std::uint32_t p,const NativeTu15PhysicsVector&v){std::array<std::byte,16>b{};for(unsigned n=0;n<4;++n)for(unsigned i=0;i<4;++i)b[n*4u+i]=std::byte(v[n]>>(8u*i));write(site,p,b);}
    NativeTu15PhysicsVirtualOwner owner(std::uint32_t object,std::uint32_t offset)const{return{word(word(object)+offset)};}
};
bool signed_less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
std::uint32_t align16(std::uint32_t bytes){return(bytes+15u)&0xfffffff0u;}
template<class Function>void require(const Function&function,const char*why){if(!function)throw std::runtime_error(why);}
enum class AllocationPhase {whole,jacobians,schemas};
std::uint32_t allocate(Memory&m,std::uint32_t bytes,AllocationPhase phase,NativeTu15PhysicsFrameFp&fp,const NativeTu15ConstraintStepServices&s,NativeTu15ConstraintStepStats*stats){
    const NativeTu15ConstraintStackAllocator allocator{m.word(0x48c938u)};
    const auto remaining=m.word(allocator.identity+0xcu);const auto rounded=align16(bytes);
    if(signed_less(remaining,rounded)){
        const auto owner=m.owner(allocator.identity,0x24u);require(s.allocate_stack_block,"Native TU1.5 constraint step requires its actual stack allocation owner");
        if(stats)++stats->allocated_blocks;return s.allocate_stack_block(allocator,owner,rounded,fp);
    }
    const auto result=m.word(allocator.identity+8u);
    const auto cursor_site=phase==AllocationPhase::whole?0x326390u:phase==AllocationPhase::jacobians?0x3262d7u:0x326339u;
    const auto remaining_site=phase==AllocationPhase::whole?0x326393u:phase==AllocationPhase::jacobians?0x3262deu:0x32633fu;
    m.word(cursor_site,allocator.identity+8u,result+rounded);m.word(remaining_site,allocator.identity+0xcu,remaining-rounded);return result;
}
NativeTu15ConstraintAccumulator setup_native(Memory&m,NativeTu15PhysicsMotion motion,NativeTu15PhysicsVirtualOwner owner,NativeTu15ConstraintAccumulator output){
    const auto p=output.identity;const NativeTu15PhysicsVector zero{};
    if(owner.identity==0x323560u){
        m.byte(0x323566u,p,2);m.vector(0x32356cu,p+0x30u,zero);
        m.vector(0x323574u,p+0x40u,m.vector(motion.identity+0x70u));
        m.vector(0x32357cu,p+0x20u,m.vector(motion.identity+0x50u));
        m.vector(0x323584u,p+0x10u,m.vector(motion.identity+0x40u));
        m.vector(0x323588u,p+0x50u,zero);m.vector(0x32358cu,p+0x60u,zero);m.vector(0x323590u,p+0x70u,zero);
        m.word(0x323599u,p+0x50u,0x3f800000u);m.word(0x32359cu,p+0x64u,0x3f800000u);m.word(0x32359fu,p+0x78u,0x3f800000u);m.byte(0x3235a5u,p+0xcu,1);
    }else{
        m.byte(0x3248d9u,p,2);m.vector(0x3248dcu,p+0x30u,zero);m.vector(0x3248e0u,p+0x10u,zero);m.vector(0x3248e4u,p+0x20u,zero);m.vector(0x3248e8u,p+0x40u,zero);m.vector(0x3248ecu,p+0x50u,zero);m.vector(0x3248f0u,p+0x60u,zero);m.vector(0x3248f7u,p+0x70u,zero);
    }
    return{p+0x80u};
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 constraint profiling requires its unsupported FP trap owner");
        mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr_);}
};
std::uint32_t count_as_float(std::uint32_t count){std::uint32_t bits;const auto value=std::bit_cast<std::int32_t>(count);asm volatile("fildl %1\n\tfstps %0":"=m"(bits):"m"(value):"memory","st");return bits;}
#endif
}
void native_tu15_step_island_constraints(SourcePhysicsContactMemory&memory,const NativeTu15PhysicsConstraintInput&input,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConstraintStepServices&services,const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintStepStats*stats){
    Memory m(memory,observe);if(stats)++stats->islands;const auto island=input.island.identity;
    const auto result_bytes=m.word(island+0x18u)*4u+8u;
    auto workspace=m.word(0x491350u);m.byte(0x32621fu,workspace+0x10u,1);
    const auto initial=m.word(workspace+8u);workspace=m.word(0x491350u);auto arena_end=initial+m.word(workspace+0xcu);
    auto accumulator_base=initial;auto body_limit=accumulator_base+m.word(island+0x14u)+0x90u;
    auto jacobian_limit=body_limit+m.word(island+0xcu);auto result_slots=jacobian_limit;
    std::uint32_t allocated_block{},secondary_jacobians{},schema_start{};
    while(true){
        const auto schema_bytes=m.word(island+0x10u);schema_start=result_slots+result_bytes;const auto required_end=schema_start+schema_bytes+4u;
        if(required_end<=arena_end)break;
        if(body_limit>=arena_end){
            const auto bytes=required_end-initial;const auto block=allocate(m,bytes,AllocationPhase::whole,fp,services,stats);
            allocated_block=block;arena_end=block+bytes;accumulator_base=block;
            body_limit=block+m.word(island+0x14u)+0x90u;jacobian_limit=body_limit+m.word(island+0xcu);result_slots=jacobian_limit;
        }else if(jacobian_limit>=arena_end){
            const auto secondary_bytes=2u*m.word(island+8u)-arena_end+m.word(island+0xcu)+body_limit;
            const auto bytes=secondary_bytes+schema_bytes+result_bytes+4u;
            const auto block=allocate(m,bytes,AllocationPhase::jacobians,fp,services,stats);
            jacobian_limit=arena_end-m.word(island+8u);arena_end=block+bytes;allocated_block=block;secondary_jacobians=block;result_slots=block+secondary_bytes;
        }else{
            const auto bytes=schema_bytes+result_bytes+4u;const auto block=allocate(m,bytes,AllocationPhase::schemas,fp,services,stats);
            allocated_block=block;arena_end=block+bytes;result_slots=block;
        }
    }
    const auto body_end=input.bodies.identity+input.body_count*4u;
    m.byte(0x3263b0u,accumulator_base,2);const NativeTu15PhysicsVector zero{};
    m.vector(0x3263b3u,accumulator_base+0x30u,zero);m.vector(0x3263b7u,accumulator_base+0x10u,zero);m.vector(0x3263bbu,accumulator_base+0x20u,zero);m.vector(0x3263bfu,accumulator_base+0x40u,zero);m.vector(0x3263c3u,accumulator_base+0x50u,zero);m.vector(0x3263cdu,accumulator_base+0x60u,zero);m.vector(0x3263d9u,accumulator_base+0x70u,zero);
    NativeTu15ConstraintAccumulator cursor{accumulator_base+0x80u};const auto first_accumulator=cursor;
    for(auto p=input.bodies.identity;p<body_end;p+=4u){
        const NativeTu15PhysicsMotion motion{m.word(m.word(p)+0x3cu)};const auto offset=cursor.identity-accumulator_base;
        if(m.word(motion.identity+8u)!=offset)m.word(0x3263f5u,motion.identity+8u,offset);
        const auto owner=m.owner(motion.identity,0xcu);
        if(owner.identity==0x323560u||owner.identity==0x3248d0u){if(stats)++stats->native_accumulator_setups;cursor=setup_native(m,motion,owner,cursor);}
        else{require(services.setup_motion,"Native TU1.5 constraint step requires its actual motion accumulator setup owner");if(stats)++stats->other_accumulator_setups;cursor=services.setup_motion(motion,owner,input.step,input.gravity,cursor,fp);}
    }
    m.byte(0x32641cu,cursor.identity,3);
    NativeTu15ConstraintBuildContext context{body_limit,schema_start,jacobian_limit,secondary_jacobians};
    const auto constraint_end=input.constraints+input.constraint_count*4u;
    for(auto p=input.constraints;p<constraint_end;p+=4u){
        const auto constraint=m.word(p);const auto second=m.word(m.word(constraint+0x10u)+0x3cu);
        const auto first=m.word(m.word(constraint+0xcu)+0x3cu);const auto first_offset=m.word(first+8u);
        m.word(0x32645du,input.jacobian_context,accumulator_base+first_offset);
        const auto second_offset=m.word(second+8u);m.word(0x32646cu,input.jacobian_context+4u,accumulator_base+second_offset);
        if(context.jacobians>=context.jacobian_limit){context.jacobians=context.secondary_jacobians;context.jacobian_limit=0xffffffffu;}
        const auto virtual_table=m.word(m.word(p));const NativeTu15Constraint receiver{m.word(p)};
        const NativeTu15PhysicsVirtualOwner owner{m.word(virtual_table+0x24u)};
        require(services.build_constraint,"Native TU1.5 constraint step requires its actual constraint schema builder owner");if(stats)++stats->constraint_builds;services.build_constraint(receiver,owner,input.jacobian_context,context,fp);
    }
    m.word(0x3264a7u,context.schemas,0x400u);
    auto result_count=m.word(island+0x18u);
    if(signed_less(0,result_count)){
        std::uint32_t index{};do{m.word(0x3264c4u,result_slots+index*4u,0);result_count=m.word(island+0x18u);++index;}while(signed_less(index,result_count));
    }
    require(services.solve_rows,"Native TU1.5 constraint step requires its actual solver326A40");if(stats)++stats->solver_calls;
    const auto result=services.solve_rows(input.solver_configuration,schema_start,{accumulator_base},result_slots,fp);
    native_tu15_finish_island_constraints(memory,{input,first_accumulator,body_end,allocated_block,arena_end},result,fp,depth,services,observe,stats);
}
void native_tu15_finish_island_constraints(SourcePhysicsContactMemory&memory,const NativeTu15ConstraintStepContinuation&context,std::uint32_t result,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConstraintStepServices&services,const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintStepStats*stats){
    Memory m(memory,observe);const auto&input=context.input;const auto island=input.island.identity;
    const auto body_end=context.body_end,allocated_block=context.allocated_block,arena_end=context.arena_end;
    const auto profile=m.word(0x48c974u);const auto profile_limit=m.word(0x48c978u);
    if(profile<profile_limit){
        m.word(0x3264fau,profile,0x41f4a4u);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        Environment environment(fp,depth);const auto count=m.word(island+0x18u);m.word(0x326506u,profile+4u,count_as_float(count));m.word(0x326509u,0x48c974u,profile+8u);
#else
        throw std::runtime_error("Exact native TU1.5 constraint profiling backend unavailable");
#endif
    }
    if(result==1u){
        const auto parameter=m.word(input.solver_configuration+0x20u);auto cursor=context.first_accumulator;
        for(auto p=input.bodies.identity;p<body_end;p+=4u){
            const NativeTu15PhysicsMotion motion{m.word(m.word(p)+0x3cu)};const auto owner=m.owner(motion.identity,0x10u);
            if(owner.identity==0x324800u){if(stats)++stats->native_fixed_commits;cursor.identity+=0x80u;}
            else{require(services.commit_motion,"Native TU1.5 constraint step requires its actual motion commit owner");if(stats)++stats->other_motion_commits;cursor=services.commit_motion(motion,owner,input.step,parameter,cursor,fp);}
        }
    }
    const auto workspace=m.word(0x491350u);m.byte(0x326557u,workspace+0x10u,0);
    if(!allocated_block)return;
    const NativeTu15ConstraintStackAllocator allocator{m.word(0x48c938u)};const auto bytes=align16(arena_end-allocated_block);
    if(bytes){
        const auto marker=m.word(allocator.identity+0x14u);
        if(marker==allocated_block||marker==0){
            const auto owner=m.owner(allocator.identity,0x28u);require(services.release_stack_block,"Native TU1.5 constraint step requires its actual stack release owner");if(stats)++stats->released_blocks;services.release_stack_block(allocator,owner,allocated_block,bytes,fp);return;
        }
    }
    const auto data=m.word(allocator.identity+8u);const auto remaining=m.word(allocator.identity+0xcu);
    m.word(0x326590u,allocator.identity+8u,data-bytes);m.word(0x326593u,allocator.identity+0xcu,remaining+bytes);
}
} // namespace halo2
