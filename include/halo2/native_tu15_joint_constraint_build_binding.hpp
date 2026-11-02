#pragma once
#include "halo2/native_tu15_joint_constraint_build.hpp"
#include "halo2/native_tu15_linear_jacobian_host.hpp"
#include <stdexcept>
namespace halo2 {
struct NativeTu15BoundJointConstraintBuildActivity {
    std::uint64_t attempts{},completed{},refusals{},unsupported_fp{};
    NativeTu15JointConstraintBuildStats parent;
    NativeTu15LinearJacobianEmitterStats linear;
};
// Source31D930 native-only composition. The actual typed constraint/input/
// builder DATA and native FP fields cross this boundary. No executor, guest
// register file, control frame, callback outcome or afterimage is accepted.
inline void native_tu15_build_bound_joint_constraint(SourcePhysicsContactMemory&memory,
    std::uint32_t constraint,NativeTu15ConstraintProducerInput input,
    NativeTu15ConstraintSchemaPacket packet,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15BoundJointConstraintBuildActivity*activity=nullptr){
    NativeTu15BoundJointConstraintBuildActivity local;auto&out=activity?*activity:local;++out.attempts;
    try{
        if(depth||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
            (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
            ++out.unsupported_fp;throw std::runtime_error("Native joint builder requires its qualified FP owner");}
        NativeTu15JointConstraintBuildServices services;
        services.linear=[&](const auto&first,const auto&second,auto producer,auto builder,auto older){
            native_tu15_emit_world_linear_jacobians_active(memory,first,second,
                producer.identity,builder.identity,older,observe,&out.linear);
        };
        native_tu15_build_joint_constraint(memory,constraint,input,packet,fp,depth,
            services,observe,&out.parent);++out.completed;
    }catch(...){++out.refusals;throw;}
}
} // namespace halo2
