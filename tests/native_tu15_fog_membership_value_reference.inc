bool BspAtmosphere::contains_planar(int cluster,std::uint32_t datum) const {
    if(cluster<=-2) {
        const auto instance=std::size_t(-(std::int64_t(cluster)+2));
        if(instance>=instance_membership.size())throw FormatError("planar instance outside BSP");
        return std::any_of(instance_membership[instance].begin(),instance_membership[instance].end(),
            [&](unsigned member){return contains_planar(int(member),datum);});
    }
    if(cluster<0)return false;
    if(std::size_t(cluster)>=clusters.size())throw FormatError("planar cluster outside BSP");
    const int designator=clusters[cluster].planar_designator;
    if(designator==-1)return false;
    if(datum==0xffffffffu)return true;
    int palette=designator;
    if(designator<0) {
        const unsigned plane=unsigned(designator)&127;
        if(plane>=fog_planes.size())throw FormatError("containing fog plane outside BSP");
        palette=fog_planes[plane].palette;
    }
    return palette>=0 && std::size_t(palette)<fog_palette.size() && fog_palette[palette].datum==datum;
}

bool source_bsp_cluster_contains_fog(const BspAtmosphere &atmosphere,int cluster,
    const SourceBspFogAdmissionInput &in) {
    if(!in.scenario_loaded || !in.bsp_loaded || cluster==-1)return false;
    if(in.flags_4e64c8&0x10)return true;
    if(cluster<0 || std::size_t(cluster)>=atmosphere.clusters.size())
        throw FormatError("source16E210 cluster outside active BSP");
    return atmosphere.contains_planar(cluster,in.datum);
}
