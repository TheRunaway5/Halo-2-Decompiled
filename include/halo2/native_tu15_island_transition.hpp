#pragma once
#include "halo2/native_tu15_island_integration.hpp"
namespace halo2 {
struct NativeTu15PhysicsArrayHeader {std::uint32_t identity{};};
struct NativeTu15PhysicsListener {std::uint32_t identity{};};
enum class NativeTu15DeactivationNotification {island,body};
class NativeTu15IslandTransitionStorage {
public:
    virtual ~NativeTu15IslandTransitionStorage()=default;
    virtual NativeTu15PhysicsArrayHeader inactive_islands(NativeTu15PhysicsWorld)const=0;
    virtual NativeTu15PhysicsArrayHeader active_islands(NativeTu15PhysicsWorld)const=0;
    virtual NativeTu15PhysicsArrayHeader island_listeners(NativeTu15PhysicsWorld)const=0;
    virtual NativeTu15PhysicsArrayHeader bodies(NativeTu15PhysicsIsland)const=0;
    virtual NativeTu15PhysicsArrayHeader body_listeners(NativeTu15PhysicsBody)const=0;
    virtual std::uint32_t capacity(NativeTu15PhysicsArrayHeader)const=0;
    virtual std::uint32_t count(NativeTu15PhysicsArrayHeader)const=0;
    virtual NativeTu15PhysicsArray data(NativeTu15PhysicsArrayHeader)const=0;
    virtual std::uint32_t element(NativeTu15PhysicsArray,std::uint32_t)const=0;
    virtual void element(NativeTu15PhysicsArray,std::uint32_t,std::uint32_t,std::uint32_t site)=0;
    virtual void count(NativeTu15PhysicsArrayHeader,std::uint32_t,std::uint32_t site)=0;
    virtual std::uint32_t index(NativeTu15PhysicsIsland)const=0;
    virtual void index(NativeTu15PhysicsIsland,std::uint32_t,std::uint32_t site)=0;
    virtual void inactive(NativeTu15PhysicsIsland)=0;
    virtual NativeTu15PhysicsVirtualOwner listener_owner(NativeTu15PhysicsListener,NativeTu15DeactivationNotification)const=0;
};
struct NativeTu15IslandTransitionServices {
    // Actual2E3250 remains a named dependency. It receives the actual header
    // identity and stride4, and must publish its real allocation/count state.
    std::function<void(NativeTu15PhysicsArrayHeader,std::uint32_t stride,NativeTu15PhysicsFrameFp&)> grow_array;
    std::function<void(NativeTu15PhysicsListener,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&)> island_deactivated;
    std::function<void(NativeTu15PhysicsListener,NativeTu15PhysicsVirtualOwner,NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&)> body_deactivated;
};
// Complete actual322740 notification dispatch, reverse callbacks, mutable
// cohorts, null compaction and retained source read/write ordering.
void native_tu15_notify_island_deactivation(NativeTu15IslandTransitionStorage&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandTransitionServices&);
// Complete actual316CD0 transition. No CPU postimage is manufactured. These
// integer roots leave FP untouched except for genuine dependency callbacks.
void native_tu15_deactivate_physics_island(NativeTu15IslandTransitionStorage&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandTransitionServices&);
class NativeTu15IslandTransitionMemory final:public NativeTu15IslandTransitionStorage {
    SourcePhysicsContactMemory&memory_;
    SourcePhysicsContactRecordObserver observer_;
    std::uint32_t read(std::uint32_t,unsigned bytes=4)const;
    void write(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned bytes=4);
public:
    explicit NativeTu15IslandTransitionMemory(SourcePhysicsContactMemory&,const SourcePhysicsContactRecordObserver& = {});
    NativeTu15PhysicsArrayHeader inactive_islands(NativeTu15PhysicsWorld)const override;
    NativeTu15PhysicsArrayHeader active_islands(NativeTu15PhysicsWorld)const override;
    NativeTu15PhysicsArrayHeader island_listeners(NativeTu15PhysicsWorld)const override;
    NativeTu15PhysicsArrayHeader bodies(NativeTu15PhysicsIsland)const override;
    NativeTu15PhysicsArrayHeader body_listeners(NativeTu15PhysicsBody)const override;
    std::uint32_t capacity(NativeTu15PhysicsArrayHeader)const override;
    std::uint32_t count(NativeTu15PhysicsArrayHeader)const override;
    NativeTu15PhysicsArray data(NativeTu15PhysicsArrayHeader)const override;
    std::uint32_t element(NativeTu15PhysicsArray,std::uint32_t)const override;
    void element(NativeTu15PhysicsArray,std::uint32_t,std::uint32_t,std::uint32_t)override;
    void count(NativeTu15PhysicsArrayHeader,std::uint32_t,std::uint32_t)override;
    std::uint32_t index(NativeTu15PhysicsIsland)const override;
    void index(NativeTu15PhysicsIsland,std::uint32_t,std::uint32_t)override;
    void inactive(NativeTu15PhysicsIsland)override;
    NativeTu15PhysicsVirtualOwner listener_owner(NativeTu15PhysicsListener,NativeTu15DeactivationNotification)const override;
};
} // namespace halo2
