#pragma once
#include "halo2/native_tu15_cluster_frustum_kernel.hpp"
#include "halo2/source_visibility.hpp"

namespace halo2::native_cluster_frustum_value_detail {
class View {
    const SourceClusterFrustumSet&set_;std::size_t section_;Vec3 center_;float radius_;
    SourceClusterFrustumResult result_;int group_{};
public:
    View(const SourceClusterFrustumSet&set,std::size_t section,Vec3 center,float radius):
        set_(set),section_(section),center_(center),radius_(radius){}
    int begin(){
        if(set_.group_count>0&&(set_.group_count>6||section_>=set_.clusters.size()))
            throw FormatError("source cluster frustum group/entry unavailable");
        return set_.group_count;
    }
    void open_groups(){}void close_groups(){}
    native_cluster_frustum_detail::Range range(){return{0,set_.clusters[section_].counts[group_]};}
    bool contained()const{return result_.contained;}
    unsigned classify(const native_cluster_frustum_detail::Range&range){
        const auto index=int(set_.clusters[section_].first_indices[group_])+range.index;
        if(index<0||std::size_t(index)>=set_.frustums.size())
            throw FormatError("source cluster frustum range unavailable");
        return unsigned(source_sphere_frustum_visibility(set_.frustums[index],center_,radius_));
    }
    void accept(bool contained){result_.visible=true;result_.contained=contained;}
    native_cluster_frustum_detail::Range next(native_cluster_frustum_detail::Range range){++range.index;return range;}
    bool next_group(){return ++group_<set_.group_count;}
    SourceClusterFrustumResult finish()const{return result_;}
};
inline SourceClusterFrustumResult visible(const SourceClusterFrustumSet&set,std::size_t section,Vec3 center,float radius){
    View view{set,section,center,radius};return native_cluster_frustum_detail::aggregate(view);
}
} // namespace halo2::native_cluster_frustum_value_detail
