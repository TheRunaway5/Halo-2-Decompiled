#include "halo2/native_tu15_constraint_step_residuals.hpp"
#include "halo2/native_tu15_dynamic_accumulator.hpp"
#include "halo2/native_tu15_motion_commit.hpp"
#include <stdexcept>
#include <sstream>
namespace halo2 {
namespace {
void put(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8u*i));m.write(p,b);}
std::uint32_t word(SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(b[i])<<(8u*i);return value;}
}
void native_tu15_step_island_constraints_with_residuals(SourcePhysicsContactMemory&m,
  const NativeTu15PhysicsConstraintInput&input,NativeTu15PhysicsFrameFp&fp,unsigned depth,
  std::uint32_t packet,const NativeTu15ConstraintResidualCall&invoke,
  const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintBindingActivity*activity,
  const NativeTu15SystemFiletimeSource&actual_filetime){
  if(packet>0xfffffff0u)throw std::runtime_error("Native TU1.5 constraint builder packet wraps its retained storage");
  const auto call=[&](std::uint32_t owner,std::span<const std::uint32_t>args,
    std::optional<std::uint32_t>receiver,unsigned pop,auto&context){
    if(!invoke)throw std::runtime_error("Native TU1.5 constraint requires its actual residual owner");
    if(activity)++activity->residual_calls;return invoke(owner,args,receiver,pop,context);
  };
  NativeTu15ConstraintStepServices services;
  services.allocate_stack_block=[&](auto allocator,auto owner,auto bytes,auto&context){
    const std::array args{bytes};return call(owner.identity,args,allocator.identity,4,context);
  };
  services.release_stack_block=[&](auto allocator,auto owner,auto block,auto bytes,auto&context){
    const std::array args{block,bytes};call(owner.identity,args,allocator.identity,8,context);
  };
  services.setup_motion=[&](auto motion,auto owner,auto step,auto gravity,auto output,auto&context){
    if(owner.identity==0x325410u){
      if(activity)++activity->native_dynamic_setups;
      NativeTu15DynamicAccumulatorMemory storage(m,motion,step,gravity,output,observe);
      return native_tu15_setup_dynamic_motion_accumulator(storage,context,depth);
    }
    const std::array args{step.identity,gravity.identity,output.identity};
    return NativeTu15ConstraintAccumulator{call(owner.identity,args,motion.identity,12,context)};
  };
  services.build_constraint=[&](auto constraint,auto owner,auto jacobian_input,auto&build,auto&context){
    put(m,packet,build.jacobians);put(m,packet+4u,build.schemas);
    put(m,packet+8u,build.jacobian_limit);put(m,packet+12u,build.secondary_jacobians);
    const auto recover=[&]{build={word(m,packet),word(m,packet+4u),word(m,packet+8u),word(m,packet+12u)};};
    try{
      if(owner.identity==0x31f770u||owner.identity==0x31f860u){
        NativeTu15ConstraintProducerServices producer;
        producer.build_base=[&](const auto&request,auto&base_context){
          native_tu15_build_constraint_base(m,request,base_context,depth,observe,
            activity?&activity->base:nullptr);
        };
        const auto before=activity?activity->producers.emissions:std::array<std::uint64_t,3>{};
        if(owner.identity==0x31f770u)
          native_tu15_build_axis_factor_constraint(m,{constraint.identity},{jacobian_input},{packet},
            context,depth,producer,observe,activity?&activity->producers:nullptr);
        else native_tu15_build_scaled_accumulator_constraint(m,{constraint.identity},{jacobian_input},{packet},
          context,depth,producer,observe,activity?&activity->producers:nullptr);
        if(activity)for(unsigned n=0;n<3;++n)
          activity->completed_producer_emissions[n]+=activity->producers.emissions[n]-before[n];
      }else{
        const std::array args{jacobian_input,packet};
        call(owner.identity,args,constraint.identity,8,context);
      }
    }catch(...){recover();throw;}
    recover();
  };
  services.solve_rows=[&](auto configuration,auto schemas,auto accumulator,auto results,auto&context){
    NativeTu15ConstraintSolverServices solver;
    solver.initialize=[&](auto&initial_context){
      NativeTu15ConstraintSolverInitializerServices initializer;
      initializer.read_time_seconds=[&](auto&clock_context){
        if(actual_filetime){
          if(activity)++activity->native_clock_calls;
          return native_tu15_constraint_clock_seconds(actual_filetime);
        }
        return call(0x37c5c0u,{}, {},0,clock_context);
      };
      initializer.log_failure=[&](auto message,auto&log_context){
        const std::array args{message};call(0x2e3850u,args,{},0,log_context);
      };
      native_tu15_initialize_constraint_solver(m,initial_context,initializer,observe,
        activity?&activity->initializer:nullptr);
    };
    solver.custom_schema=[&](auto owner,auto row,auto payload,auto&custom_context){
      if(native_tu15_owns_constraint_custom_schema(owner)){
        native_tu15_apply_constraint_custom_schema(m,owner,row,payload,custom_context,
          depth,observe,activity?&activity->custom_schemas:nullptr);
        return;
      }
      // The caller's two cdecl arguments are authenticated. An arbitrary
      // registered callee still needs its own body/implicit-input proof.
      if(activity)++activity->custom_schema_refusals;
      std::ostringstream message;message<<"Native TU1.5 solver custom schema owner requires native/ABI qualification: 0x"<<std::hex<<owner.identity;
      throw std::runtime_error(message.str());
    };
    return native_tu15_solve_constraint_rows(m,{configuration,schemas,accumulator,results},
      context,depth,solver,observe,activity?&activity->solver:nullptr);
  };
  services.commit_motion=[&](auto motion,auto owner,auto step,auto parameter,auto output,auto&context){
    std::optional<NativeTu15MotionCommitKind>kind;
    if(owner.identity==0x3235f0u)kind=NativeTu15MotionCommitKind::kinematic;
    else if(owner.identity==0x3249e0u)kind=NativeTu15MotionCommitKind::dynamic;
    else if(owner.identity==0x3255d0u)kind=NativeTu15MotionCommitKind::restore_saved_velocity;
    if(kind){
      if(activity){++activity->native_motion_commits;++activity->native_motion_commit_kinds[unsigned(*kind)];}
      NativeTu15MotionCommitMemory storage(m,*kind,motion,step,output,observe);
      return native_tu15_commit_motion(*kind,storage,parameter,context,depth);
    }
    const std::array args{step.identity,parameter,output.identity};
    return NativeTu15ConstraintAccumulator{call(owner.identity,args,motion.identity,12,context)};
  };
  native_tu15_step_island_constraints(m,input,fp,depth,services,observe,activity?&activity->step:nullptr);
}
} // namespace halo2
