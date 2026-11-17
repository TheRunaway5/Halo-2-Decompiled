#pragma once
#include "halo2/native_tu15_bsp_admission_kernel.hpp"
#include "halo2/native_tu15_geometry_visibility_value.hpp"
#include "halo2/native_tu15_visibility_association_value.hpp"
#include "halo2/source_bsp_admission.hpp"
#include <array>
#include <utility>
#include <vector>

namespace halo2::native_bsp_admission_detail {
// The public predecessor validates authored banks and uses its established
// floating comparison policy. Its side computations are deliberately lazy
// until a record can be published; the raw view separately owns the CPU's
// earlier scratch/FP acquisitions and COMISS unordered decisions.
template<class ClusterSides,class InstanceSides>class HostView {
    const SourceBspAdmissionBsp&bsp_;const SourcePortalVisibilitySet&visible_;const SourceBspAdmissionInput&in_;
    ClusterSides cluster_sides_;InstanceSides instance_sides_;
    SourceBspAdmission out_;SourceClusterFrustumSet spheres_;std::vector<int>admitted_;
    std::size_t ordinal_{},cursor_{};std::uint32_t authored_{},id_{};bool fog_{},main_{};
    const SourceBspAdmissionCluster*cluster_{};SourceClusterFrustumResult result_{};std::uint16_t association_{};
    static void bit(std::array<std::uint32_t,4>&mask,std::size_t ordinal){mask[ordinal/32]|=1u<<(ordinal%32);}
    static std::uint16_t side_flags(const auto&s){return std::uint16_t((s.negative?0x8000u:0u)|(s.positive?0x4000u:0u));}
public:
    HostView(const SourceBspAdmissionBsp&bsp,const SourcePortalVisibilitySet&visible,const SourceBspAdmissionInput&input,
        ClusterSides cluster_sides,InstanceSides instance_sides):bsp_(bsp),visible_(visible),in_(input),cluster_sides_(cluster_sides),instance_sides_(instance_sides){out_.cluster_entries.fill(0xff);}
    bool begin(){if(in_.flags&0x20)return false;spheres_=visible_.sphere_bank();admitted_.assign(bsp_.instances.size(),-1);return !visible_.clusters.empty();}
    void cluster(){
        const auto authored=visible_.clusters[ordinal_].source_cluster;
        if(authored<0||std::size_t(authored)>=bsp_.clusters.size())throw FormatError("source BSP admission constructed cluster outside authored bank");
        authored_=std::uint32_t(authored);cluster_=&bsp_.clusters[authored_];fog_=in_.containing_fog[authored_]!=0;
    }
    void mark_cluster(){out_.marked_clusters[authored_/32]|=1u<<(authored_%32);out_.cluster_entries[authored_]=std::uint8_t(ordinal_);
        main_=!in_.auxiliary||!(in_.flags&1)||in_.auxiliary->geometry_visible(authored_);}
    bool cluster_has_geometry()const{return cluster_->geometry_count!=0;}
    void cluster_sides()const{}bool cluster_allowed()const{return main_;}void cluster_fog()const{}
    bool cluster_room()const{return out_.clusters.size()<in_.cluster_capacity-1;}
    void append_cluster(){out_.clusters.push_back({authored_,std::uint16_t(0x3000u|side_flags(cluster_sides_(*cluster_,in_))|(fog_?0x400u:0u)),0xffff,0});}
    bool begin_instances(){cursor_=0;return !cluster_->instances.empty();}
    bool instance_seen(){id_=cluster_->instances[cursor_];return admitted_[id_]!=-1;}
    void merge_instance(){auto&record=out_.instances[admitted_[id_]];if(!(record.flags&0x800))bit(out_.associations[record.association],ordinal_);if(fog_)record.flags|=0x400;}
    bool instance_has_geometry()const{return bsp_.definitions[bsp_.instances[id_].definition].geometry_count!=0;}
    bool instance_visible(){const auto&i=bsp_.instances[id_];result_=native_geometry_visibility_detail::geometry(spheres_,ordinal_,i.center,i.radius,in_.mode,false,in_.context_2a78!=0,in_.sphere_center,in_.sphere_radius);return result_.visible;}
    bool force_instance()const{return(in_.flags&0x80)!=0;}
    void instance_sides()const{}void instance_fog()const{}void instance_flags()const{}
    bool contained()const{return result_.contained;}void none_association(){association_=0xffff;}
    void allocate_association(){association_=native_visibility_association_detail::reserve_mask(out_.associations,out_.association_capacity_reached,ordinal_);}
    bool instance_room()const{return out_.instances.size()<in_.instance_capacity-1;}
    void append_instance(){
        const bool main_instance=!result_.visible||!in_.auxiliary||!(in_.flags&1)||in_.auxiliary->instance(id_)!=nullptr;
        const auto&i=bsp_.instances[id_];const auto flags=std::uint16_t((main_instance?0x3000u:0u)|side_flags(instance_sides_(i.center,double(i.radius),in_))|(fog_?0x400u:0u)|(result_.contained?0x800u:0u));
        admitted_[id_]=int(out_.instances.size());out_.instances.push_back({id_,flags,association_,0});
    }
    bool next_instance(){return ++cursor_<cluster_->instances.size();}
    bool next_cluster(){return ++ordinal_<visible_.clusters.size();}
    SourceBspAdmission finish(){return std::move(out_);}
};
template<class ClusterSides,class InstanceSides>SourceBspAdmission values(const SourceBspAdmissionBsp&bsp,const SourcePortalVisibilitySet&visible,
    const SourceBspAdmissionInput&input,ClusterSides cluster_sides,InstanceSides instance_sides){
    HostView view{bsp,visible,input,cluster_sides,instance_sides};return collect(view);
}
} // namespace halo2::native_bsp_admission_detail
