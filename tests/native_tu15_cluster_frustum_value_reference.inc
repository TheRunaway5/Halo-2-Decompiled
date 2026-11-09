SourceClusterFrustumResult source_cluster_frustum_visibility_reference(
    const SourceClusterFrustumSet& set,std::size_t section_index,Vec3 center,float radius) {
    SourceClusterFrustumResult result;
    if(set.group_count<=0)return result;
    if(set.group_count>6 || section_index>=set.clusters.size())
        throw FormatError("source cluster frustum group/entry unavailable");
    const auto& ranges=set.clusters[section_index];
    for(int group=0;group<set.group_count;++group) {
        for(int i=0;i<ranges.counts[group] && !result.contained;++i) {
            const auto index=int(ranges.first_indices[group])+i;
            if(index<0 || std::size_t(index)>=set.frustums.size())
                throw FormatError("source cluster frustum range unavailable");
            const auto classification=source_sphere_frustum_visibility(set.frustums[index],center,radius);
            if(classification!=SourceSphereVisibility::outside) {
                result.visible=true;
                result.contained=classification==SourceSphereVisibility::contained;
            }
        }
    }
    return result;
}
