#pragma once
#include "halo2/native_tu15_island_integration.hpp"
namespace halo2 {
enum class NativeTu15DeactivationPeriod {short_period,long_period};
class NativeTu15IslandDeactivationStorage {
public:
    virtual ~NativeTu15IslandDeactivationStorage()=default;
    virtual std::uint32_t seconds()const=0;
    virtual std::uint32_t elapsed(NativeTu15DeactivationPeriod)const=0;
    virtual NativeTu15PhysicsWorld world()const=0;
    virtual std::uint32_t interval(NativeTu15PhysicsWorld,NativeTu15DeactivationPeriod)const=0;
    virtual void elapsed(NativeTu15DeactivationPeriod,std::uint32_t,bool wrapping)=0;
    virtual std::uint32_t body_count()const=0;
    virtual NativeTu15PhysicsBody body(std::uint32_t index)const=0;
    virtual std::uint32_t policy(NativeTu15PhysicsBody)const=0;
    virtual NativeTu15PhysicsVirtualOwner policy_owner(std::uint32_t,NativeTu15DeactivationPeriod)const=0;
    virtual std::uint8_t short_check_count()const=0;
    virtual void short_check_count(std::uint8_t,bool reset)=0;
    virtual void result(bool,std::uint32_t instruction)=0;
};
using NativeTu15DeactivationQuery=std::function<bool(std::uint32_t policy,NativeTu15PhysicsVirtualOwner,
    NativeTu15PhysicsBody,NativeTu15DeactivationPeriod,NativeTu15PhysicsFrameFp&)>;
// Actual3233C0 writes false to its caller's private output byte and returns
// that pointer (ret8). The semantic caller consumes only that byte. This typed
// leaf returns the identical bool without manufacturing a CPU postimage.
bool native_tu15_default_deactivation_policy()noexcept;
// Actual320C80 scheduling and all-body checks. Queries are explicit remaining
// virtual policy owners, not admission results supplied by an offline oracle.
// Masked native x87 arithmetic preserves source rounding and complete status.
// The result is a bool and retained writes; volatile CPU registers are not an
// output of this semantic API. Unmasked faults remain unsupported.
bool native_tu15_check_island_deactivation(NativeTu15IslandDeactivationStorage&,
    NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,const NativeTu15DeactivationQuery&);
class NativeTu15IslandDeactivationMemory final:public NativeTu15IslandDeactivationStorage {
    SourcePhysicsContactMemory&memory_;
    std::uint32_t island_,step_,output_;
    SourcePhysicsContactRecordObserver observer_;
    std::uint32_t read(std::uint32_t,unsigned bytes=4)const;
    void write(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned bytes=4);
public:
    NativeTu15IslandDeactivationMemory(SourcePhysicsContactMemory&,NativeTu15PhysicsIsland,
        NativeTu15PhysicsStepPacket,std::uint32_t output,const SourcePhysicsContactRecordObserver& = {});
    std::uint32_t seconds()const override;
    std::uint32_t elapsed(NativeTu15DeactivationPeriod)const override;
    NativeTu15PhysicsWorld world()const override;
    std::uint32_t interval(NativeTu15PhysicsWorld,NativeTu15DeactivationPeriod)const override;
    void elapsed(NativeTu15DeactivationPeriod,std::uint32_t,bool)override;
    std::uint32_t body_count()const override;
    NativeTu15PhysicsBody body(std::uint32_t)const override;
    std::uint32_t policy(NativeTu15PhysicsBody)const override;
    NativeTu15PhysicsVirtualOwner policy_owner(std::uint32_t,NativeTu15DeactivationPeriod)const override;
    std::uint8_t short_check_count()const override;
    void short_check_count(std::uint8_t,bool)override;
    void result(bool,std::uint32_t)override;
};
} // namespace halo2
