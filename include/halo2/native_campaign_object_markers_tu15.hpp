#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
namespace halo2 {
enum class NativeCampaignObjectMarkerPoseTu15 {local,world};
struct NativeCampaignObjectMarkerValueTu15 {
    std::uint16_t node{};
    std::array<std::uint32_t,13> local{},world{};
    std::uint32_t radius{};
};
// A real marker record or a private native value. Domain fields retain
// publication order without assigning a source address to private storage.
class NativeCampaignObjectMarkerStorageTu15 {
public:
    virtual ~NativeCampaignObjectMarkerStorageTu15()=default;
    virtual void select(std::uint32_t index)=0;
    virtual std::uint16_t node()const=0;
    virtual void node(std::uint16_t)=0;
    virtual std::uint32_t scalar(NativeCampaignObjectMarkerPoseTu15,unsigned field)const=0;
    virtual void scalar(NativeCampaignObjectMarkerPoseTu15,unsigned field,std::uint32_t)=0;
    virtual void vector(NativeCampaignObjectMarkerPoseTu15,unsigned first,const std::array<std::uint32_t,3>&)=0;
    virtual void radius(std::uint32_t)=0;
};
class NativeCampaignObjectMarkerRetainedTu15 final:public NativeCampaignObjectMarkerStorageTu15 {
    SourcePhysicsContactMemory &memory_;
    std::uint32_t base_,record_;
public:
    NativeCampaignObjectMarkerRetainedTu15(SourcePhysicsContactMemory &,std::uint32_t base);
    void select(std::uint32_t)override;
    std::uint16_t node()const override;
    void node(std::uint16_t)override;
    std::uint32_t scalar(NativeCampaignObjectMarkerPoseTu15,unsigned)const override;
    void scalar(NativeCampaignObjectMarkerPoseTu15,unsigned,std::uint32_t)override;
    void vector(NativeCampaignObjectMarkerPoseTu15,unsigned,const std::array<std::uint32_t,3>&)override;
    void radius(std::uint32_t)override;
};
class NativeCampaignObjectMarkerLocalTu15 final:public NativeCampaignObjectMarkerStorageTu15 {
    NativeCampaignObjectMarkerValueTu15 &value_;
public:
    explicit NativeCampaignObjectMarkerLocalTu15(NativeCampaignObjectMarkerValueTu15 &);
    void select(std::uint32_t)override;
    std::uint16_t node()const override;
    void node(std::uint16_t)override;
    std::uint32_t scalar(NativeCampaignObjectMarkerPoseTu15,unsigned)const override;
    void scalar(NativeCampaignObjectMarkerPoseTu15,unsigned,std::uint32_t)override;
    void vector(NativeCampaignObjectMarkerPoseTu15,unsigned,const std::array<std::uint32_t,3>&)override;
    void radius(std::uint32_t)override;
};
// Native authored render-model marker lookup and original marker transforms.
// All addresses refer to real retained data; private host temporaries never
// acquire synthetic source addresses.
std::uint32_t native_campaign_object_marker_index_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t model,std::uint32_t marker);
void native_campaign_object_quaternion_basis_tu15(SourcePhysicsContactMemory &,
    std::uint32_t quaternion,std::uint32_t destination,NativeTu15PhysicsFrameFp &);
void native_campaign_object_transform_product_tu15(SourcePhysicsContactMemory &,
    std::uint32_t left,std::uint32_t right,std::uint32_t destination,NativeTu15PhysicsFrameFp &);
std::uint32_t native_campaign_object_marker_transforms_tu15(SourcePhysicsContactMemory &,
    std::uint32_t model,std::uint32_t group,std::uint32_t variants,std::uint32_t node_indices,
    std::uint32_t node_transforms,bool mirrored,std::uint32_t destination,
    std::int32_t capacity,NativeTu15PhysicsFrameFp &);
std::uint32_t native_campaign_object_marker_transforms_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t model,std::uint32_t group,std::uint32_t variants,std::uint32_t node_indices,
    std::uint32_t node_transforms,bool mirrored,NativeCampaignObjectMarkerStorageTu15 &,
    std::int32_t capacity,NativeTu15PhysicsFrameFp &);
}
