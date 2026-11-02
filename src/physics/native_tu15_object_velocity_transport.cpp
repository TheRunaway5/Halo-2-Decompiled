#include "halo2/native_tu15_object_velocity_transport.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
namespace halo2 {
namespace {
void require(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t address){std::array<std::byte,4>bytes{};memory.read(address,bytes);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;}
void word(SourcePhysicsContactMemory&memory,std::uint32_t address,std::uint32_t value){std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));memory.write(address,bytes);}
Vec3 vector(const SourcePhysicsContactMemory&memory,std::uint32_t address){return{std::bit_cast<float>(word(memory,address)),std::bit_cast<float>(word(memory,address+4u)),std::bit_cast<float>(word(memory,address+8u))};}
bool qualified_owner(std::uint32_t owner){return owner==0x22d460u||owner==0x31b730u||owner==0x281f10u||owner==0x3232e0u;}
}
void native_tu15_run_object_velocity_handoff(SourcePhysicsContactMemory&memory,
 const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&outer,
 const NativeTu15ObjectVelocityTransportPolicy&policy,
 const NativeTu15ObjectVelocityOwnerExecutor&execute,
 const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectVelocityTransportActivity*activity){
    if(activity)++activity->attempts;
    NativeTu15PhysicsFrameFp fp{};bool numerical_owner{};
    const auto publish_fp=[&]{if(numerical_owner){outer.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};outer.x87_status=fp.status;}};
    try{
        require(call.instruction==0xf2091u&&call.target==0xb9800u&&!call.tail,
            "Native TU1.5 velocity transport requires the qualified F2091 call");
        require(bool(execute)&&bool(policy.publish_phase),
            "Native TU1.5 velocity transport requires genuine owners and phase publication");
        const auto admission=[&]{
            require(!(policy.first_trace&&*policy.first_trace)&&!(policy.second_trace&&*policy.second_trace)&&
                (!observer||policy.observation_only_stores),
                "Native TU1.5 velocity transport refuses full tracing or mutable observation");
        };
        admission();
        require(outer.floating_point.has_value()&&!outer.active_mmx&&outer.x87_depth<=5,
            "Native TU1.5 velocity transport requires its qualified incoming x87 boundary");
        const auto sp=outer.gp[4];const auto scratch_end=std::uint64_t(policy.scratch_first)+policy.scratch_bytes;
        require(policy.caller_stack_begin<policy.caller_stack_end&&sp>=policy.caller_stack_begin&&
            std::uint64_t(sp)+0x40u<=policy.caller_stack_end&&policy.scratch_bytes>=0x2000u&&
            policy.scratch_first>=policy.caller_stack_begin&&scratch_end<=sp&&scratch_end<=policy.caller_stack_end,
            "Native TU1.5 velocity transport requires caller-owned argument and service storage");
        require(word(memory,sp)==0xf2096u&&word(memory,sp+4u)==sp+0x1cu&&outer.gp[0]==sp+0x34u,
            "Native TU1.5 velocity transport requires the authored F2096/linear/angular packet");
        // The source F2091 caller owns these fixed local vectors. They cannot
        // alias persistent object fields or the service arena below entrySP.
        const auto linear=vector(memory,sp+0x1cu),angular=vector(memory,sp+0x34u);
        const auto entry=outer;fp={outer.floating_point->x87_control_word,outer.x87_status,outer.floating_point->mxcsr};numerical_owner=true;
        const auto end=std::uint32_t(scratch_end),service_stack=end-0x400u;
        const auto policy_bounds=((end-0x60u)&~15u)-32u,shape_end=(end-0x100u)&~15u;
        const NativeTu15ObjectActivationResidualCall remaining=[&](auto owner,auto arguments,auto receiver,unsigned pop,auto&context){
            admission();
            if(!qualified_owner(owner)){
                if(activity)++activity->unqualified_owner_calls;
                throw std::runtime_error("Native TU1.5 velocity transport requires additional genuine-owner ABI qualification");
            }
            require(arguments.size()<=4,"Native TU1.5 velocity service argument packet is outside its qualified domain");
            auto actual=entry;actual.gp[4]=service_stack;if(receiver)actual.gp[1]=*receiver;
            actual.floating_point=SourcePhysicsFactoryFloatingPoint{context.control_word,context.mxcsr};actual.x87_status=context.status;
            word(memory,service_stack,0xf2096u);for(unsigned n=0;n<arguments.size();++n)word(memory,service_stack+4u+n*4u,arguments[n]);
            try{execute(owner,actual,observer);}catch(...){if(actual.floating_point){context={actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr};}throw;}
            require(actual.floating_point.has_value()&&!actual.active_mmx&&actual.x87_depth==entry.x87_depth&&
                actual.gp[4]==service_stack+4u+pop,
                "Native TU1.5 velocity service changed its real RET/FP contract");
            for(unsigned n=0;n<entry.x87_depth;++n)require(std::memcmp(&actual.x87[n],&entry.x87[n],10)==0,
                "Native TU1.5 velocity service changed a preserved incoming extended value");
            context={actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr};
            return actual.gp[0];
        };
        SourcePhysicsContactRecordObserver observed;
        if(observer)observed=[&](const auto&store){admission();policy.publish_phase(store.instruction);observer(store);};
        policy.publish_phase(0xb9800u);
        native_tu15_set_object_velocities_with_residuals(memory,entry.gp[6],&linear,&angular,fp,entry.x87_depth,
            {end-0x20u,end-0x1cu,policy_bounds,shape_end-0x200u,0x200u},remaining,observed,activity?&activity->binding:nullptr);
        publish_fp();outer.gp[4]=sp+8u;
        if(activity)++activity->completed;
    }catch(...){publish_fp();if(activity)++activity->refusals;throw;}
}
} // namespace halo2
