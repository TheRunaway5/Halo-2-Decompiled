#pragma once
#include "halo2/native_tu15_island_deactivation.hpp"
namespace halo2 {
class NativeTu15BodyDeactivationPolicyStorage {
public:
    virtual ~NativeTu15BodyDeactivationPolicyStorage()=default;
    virtual std::uint32_t distance_threshold(NativeTu15DeactivationPeriod)const=0;
    virtual NativeTu15PhysicsMotion motion()const=0;
    virtual NativeTu15PhysicsVector position(NativeTu15PhysicsMotion)const=0;
    virtual NativeTu15PhysicsVector previous_position(NativeTu15DeactivationPeriod)const=0;
    virtual NativeTu15PhysicsVector previous_quaternion(NativeTu15DeactivationPeriod)const=0;
    virtual NativeTu15PhysicsVector quaternion(NativeTu15PhysicsMotion)const=0;
    virtual std::uint32_t angular_threshold(NativeTu15DeactivationPeriod)const=0;
    virtual std::uint32_t angular_scale()const=0;
    virtual void previous_position(NativeTu15DeactivationPeriod,NativeTu15PhysicsVector)=0;
    virtual void previous_quaternion(NativeTu15DeactivationPeriod,NativeTu15PhysicsVector)=0;
    virtual void result(NativeTu15DeactivationPeriod,bool)=0;
};
// Complete actual31E5C0/31E690. Binary32 SIMD operations retain source operand
// order/grouping; thresholds/comparisons use real native x87 intermediates.
// Return value equals the original private output byte; no GP/flags postimage
// is fabricated. The roots have no unconverted dependencies. Their original
// unmasked platform trap ownership remains an explicit unsupported context.
bool native_tu15_body_deactivation_policy(NativeTu15BodyDeactivationPolicyStorage&,
    NativeTu15DeactivationPeriod,NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth);
class NativeTu15BodyDeactivationPolicyMemory final:public NativeTu15BodyDeactivationPolicyStorage {
    SourcePhysicsContactMemory&memory_;std::uint32_t policy_,body_,output_;
    SourcePhysicsContactRecordObserver observer_;
    std::uint32_t scalar(std::uint32_t)const;
    NativeTu15PhysicsVector vector(std::uint32_t)const;
    void scalar(std::uint32_t,std::uint32_t,std::uint32_t);
    void vector(std::uint32_t,std::uint32_t,const NativeTu15PhysicsVector&);
public:
    NativeTu15BodyDeactivationPolicyMemory(SourcePhysicsContactMemory&,std::uint32_t policy,
        NativeTu15PhysicsBody,std::uint32_t private_output,const SourcePhysicsContactRecordObserver& = {});
    std::uint32_t distance_threshold(NativeTu15DeactivationPeriod)const override;
    NativeTu15PhysicsMotion motion()const override;
    NativeTu15PhysicsVector position(NativeTu15PhysicsMotion)const override;
    NativeTu15PhysicsVector previous_position(NativeTu15DeactivationPeriod)const override;
    NativeTu15PhysicsVector previous_quaternion(NativeTu15DeactivationPeriod)const override;
    NativeTu15PhysicsVector quaternion(NativeTu15PhysicsMotion)const override;
    std::uint32_t angular_threshold(NativeTu15DeactivationPeriod)const override;
    std::uint32_t angular_scale()const override;
    void previous_position(NativeTu15DeactivationPeriod,NativeTu15PhysicsVector)override;
    void previous_quaternion(NativeTu15DeactivationPeriod,NativeTu15PhysicsVector)override;
    void result(NativeTu15DeactivationPeriod,bool)override;
};
} // namespace halo2
