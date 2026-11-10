SourceBspAdmission source_bsp_admission(const SourceBspAdmissionBsp &bsp,
    const SourcePortalVisibilitySet &visible,const SourceBspAdmissionInput &in) {
    validate(bsp);
    if(visible.clusters.size()>128 || visible.groups.size()>6 || visible.frustums.size()>512 ||
        in.cluster_capacity<1 || in.instance_capacity<1 || in.cluster_capacity>32768 ||
        in.instance_capacity>32768 || in.containing_fog.size()!=bsp.clusters.size() ||
        in.mode>1 || (in.flags&~0x6a1u))throw FormatError("unsupported source BSP admission context");
    SourceBspAdmission out;out.cluster_entries.fill(0xff);
    if(in.flags&0x20)return out;
    const auto spheres=visible.sphere_bank();
    std::vector<int> admitted(bsp.instances.size(),-1);
    for(std::size_t ordinal=0;ordinal<visible.clusters.size();++ordinal) {
        const auto authored=visible.clusters[ordinal].source_cluster;
        if(authored<0 || std::size_t(authored)>=bsp.clusters.size())throw FormatError("source BSP admission constructed cluster outside authored bank");
        const auto &cluster=bsp.clusters[authored];const bool fog=in.containing_fog[authored]!=0;
        out.marked_clusters[authored/32]|=1u<<(authored%32);out.cluster_entries[authored]=std::uint8_t(ordinal);
        const bool main_cluster=!in.auxiliary || !(in.flags&1) || in.auxiliary->geometry_visible(authored);
        if(cluster.geometry_count && main_cluster && out.clusters.size()<in.cluster_capacity-1) {
            out.clusters.push_back({std::uint32_t(authored),std::uint16_t(0x3000|side_flags(cluster_sides(cluster,in))|(fog?0x400:0)),0xffff,0});
        }
        //1345DB's missing auxiliary cluster skips this cluster's instance
        //walk as well. Clusters with no geometry take1345F4 directly.
        if(cluster.geometry_count && !main_cluster)continue;
        for(const auto id:cluster.instances) {
            if(admitted[id]!=-1) {
                auto &record=out.instances[admitted[id]];
                if(!(record.flags&0x800))bit(out.associations[record.association],ordinal);
                if(fog)record.flags|=0x400;
                continue;
            }
            const auto &i=bsp.instances[id];if(!bsp.definitions[i.definition].geometry_count)continue;
            SourceClusterFrustumResult result;
            if(in.mode==1){
                //132E60 keeps the overlapping-sphere predicate in SSE, then
                // uses x87 PC53 for the contained-sphere predicate.
                const float z=i.center.z-in.sphere_center.z,x=i.center.x-in.sphere_center.x,
                    y=i.center.y-in.sphere_center.y,r=i.radius+in.sphere_radius;
                result.visible=!(r*r<(z*z+x*x)+y*y);
                if(result.visible){
                    const double cx=double(in.sphere_center.x)-i.center.x,
                        cy=double(in.sphere_center.y)-i.center.y,cz=double(in.sphere_center.z)-i.center.z;
                    result.contained=!(double(in.sphere_radius)<std::sqrt((cz*cz+cy*cy)+cx*cx)+i.radius);
                }
            }else result=source_cluster_frustum_visibility(spheres,ordinal,i.center,i.radius);
            if(!result.visible && !(in.flags&0x80))continue;
            if(in.mode==0 && result.visible && !result.contained && in.context_2a78==0 && i.radius<=2.5f)result.contained=true;
            std::uint16_t association=0xffff;
            if(!result.contained) {
                if(out.associations.size()==512){out.association_capacity_reached=true;association=0;}
                else {association=std::uint16_t(out.associations.size());out.associations.push_back({});bit(out.associations.back(),ordinal);}
            }
            if(out.instances.size()<in.instance_capacity-1) {
                const bool main_instance=!result.visible || !in.auxiliary || !(in.flags&1) || in.auxiliary->instance(id)!=nullptr;
                const auto flags=std::uint16_t((main_instance?0x3000:0)|side_flags(sides(i.center,double(i.radius),in))|
                    (fog?0x400:0)|(result.contained?0x800:0));
                admitted[id]=int(out.instances.size());out.instances.push_back({id,flags,association,0});
            }
        }
    }
    return out;
}
