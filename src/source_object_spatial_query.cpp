#include "halo2/source_object_spatial_query.hpp"
#include "halo2/math.hpp"
#include <cmath>
#include <functional>

namespace halo2 {
std::vector<std::uint32_t> source_object_spatial_query(
    const ObjectLightSpatialBsp &bsp,const SourceObjectSpatialPool &collidable,
    const SourceObjectSpatialPool &noncollidable,
    const SourceObjectSpatialQueryInput &in,SourceObjectSpatialQueryState &state) {
    if(bsp.clusters.size()>512 || collidable.heads.size()!=bsp.clusters.size() ||
       noncollidable.heads.size()!=bsp.clusters.size() || in.capacity>32767 ||
       in.location.cluster<-1 || (in.location.cluster>=0 &&
       std::size_t(in.location.cluster)>=bsp.clusters.size()) ||
       !std::isfinite(in.center.x)||!std::isfinite(in.center.y)||
       !std::isfinite(in.center.z)||!std::isfinite(in.radius))
        throw FormatError("invalid source spatial query bank/input");
    std::vector<std::int16_t> clusters;
    if(in.location.cluster!=-1) {
        if(in.radius<=0)clusters.push_back(in.location.cluster);
        else {
            ++state.cluster_epoch;
            //14A5B0 always appends the entry cluster, even when its existing
            // stamp equals the wrapped epoch. Only adjacent clusters test it.
            std::function<void(std::int16_t)> visit=[&](std::int16_t c) {
                if(clusters.size()<512)clusters.push_back(c);
                state.clusters[c]=state.cluster_epoch;
                for(const auto index:bsp.clusters[c]) {
                    if(index>=bsp.portals.size())throw FormatError("source spatial query portal outside BSP");
                    const auto &p=bsp.portals[index];const auto next=p.back==c?p.front:p.back;
                    if(next<0 || std::size_t(next)>=bsp.clusters.size())
                        throw FormatError("source spatial query adjacent cluster outside BSP");
                    if(state.clusters[next]!=state.cluster_epoch &&
                       source_object_light_portal_intersects(bsp,index,in.center,in.radius))visit(next);
                }
            };
            visit(in.location.cluster);
        }
    }
    ++state.object_epoch;
    const auto lists=in.lists?in.lists:~0u,types=in.types?in.types:~0u;
    std::vector<std::uint32_t> out;
    for(const auto c:clusters)for(unsigned kind=0;kind<2;++kind) {
        if(!(lists&(1u<<kind)))continue;
        const auto &pool=kind?noncollidable:collidable;
        for(const auto &record:pool.cluster(c)) {
            const auto slot=record.object&0xffffu;
            if(record.object==0xffffffffu || slot>=state.objects.size())
                throw FormatError("source spatial query primitive has invalid owner slot");
            const auto &p=record.primitive;
            if(!(types&(1u<<(std::uint8_t(p.type)&31))) ||
               state.objects[slot]==state.object_epoch)continue;
            state.objects[slot]=state.object_epoch;
            const auto d=p.center-in.center;
            const float squared=kind?(d.z*d.z+d.x*d.x)+d.y*d.y:
                                     (d.z*d.z+d.y*d.y)+d.x*d.x;
            const float radius=p.radius+in.radius;
            if(!(radius*radius>=squared))continue;
            if(out.size()>=in.capacity)return out;
            out.push_back(record.object);
        }
    }
    return out;
}
}
