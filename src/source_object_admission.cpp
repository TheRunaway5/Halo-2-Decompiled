#include "halo2/source_object_admission.hpp"
#include "halo2/source_object_spatial_pool.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
const SourceBspAdmissionRecord *SourceObjectAdmission::object(std::uint32_t id) const {
    const auto found=std::find_if(objects.begin(),objects.end(),
        [&](const auto &r){return r.id==id;});
    return found==objects.end()?nullptr:&*found;
}
namespace {
template<class Lists>
SourceObjectAdmission admit(const SourcePortalVisibilitySet &visible,
    const Lists &collidable,const Lists &noncollidable,
    std::span<const SourceObjectAdmissionObject> objects,const SourceObjectAdmissionInput &in,
    std::span<const std::array<std::uint32_t,4>> prior) {
    if(collidable.size()!=noncollidable.size() || collidable.size()>512 ||
        visible.clusters.size()>128 || visible.groups.size()>6 || visible.frustums.size()>512 ||
        in.containing_fog.size()!=collidable.size() || in.capacity<1 || in.capacity>32768 ||
        prior.size()>512 || (in.flags&~0x6d1u) || in.mode>1 || in.sky_cluster<-1 ||
        (in.sky_cluster>=0 && std::size_t(in.sky_cluster)>=collidable.size()))
        throw FormatError("unsupported source object admission context");
    SourceObjectAdmission out;out.associations.assign(prior.begin(),prior.end());
    std::vector<int> registry(65536,-1),admitted(65536,-1);
    for(std::size_t i=0;i<objects.size();++i) {
        const auto &object=objects[i];const auto slot=object.full_handle&0xffff;
        if(object.full_handle==0xffffffffu || registry[slot]!=-1)
            throw FormatError("source object admission registry has ambiguous full handles");
        registry[slot]=int(i);
    }
    const auto spheres=visible.sphere_bank();
    const auto associate=[&](std::uint16_t association,std::size_t ordinal) {
        if(ordinal>=128)throw FormatError("source sky bypass requires negative association-bank write");
        out.associations.at(association)[ordinal/32]|=1u<<(ordinal%32);
    };
    const auto submit=[&](std::uint32_t id,std::size_t ordinal,bool fog,bool skip) {
        const auto slot=id&0xffff;const auto index=registry[slot];
        if(index==-1 || objects[index].full_handle!=id)
            throw FormatError("source cluster list refers to absent object generation");
        if(admitted[slot]!=-1) {
            auto &record=out.objects[admitted[slot]];
            if(!(record.flags&0x800) && record.association!=0xffff)associate(record.association,ordinal);
            if(fog)record.flags|=0x400;
            if(skip)record.flags|=0x200;
            return;
        }
        const auto &object=objects[index];
        if(in.context_2a78 && !object.lighting_sphere)
            throw FormatError("source object light admission lacks its retained lighting sphere");
        const auto &sphere=in.context_2a78?*object.lighting_sphere:object.rendering_sphere;
        SourceClusterFrustumResult result;
        if(in.mode==1) {
            // TU1.5 135970: SSE overlap, then x87 PC53 containment.
            const float z=sphere.center.z-in.sphere_center.z,
                x=sphere.center.x-in.sphere_center.x,y=sphere.center.y-in.sphere_center.y,
                r=sphere.radius+in.sphere_radius;
            result.visible=!(r*r<(z*z+x*x)+y*y);
            if(result.visible) {
                const double dx=double(in.sphere_center.x)-sphere.center.x,
                    dy=double(in.sphere_center.y)-sphere.center.y,
                    dz=double(in.sphere_center.z)-sphere.center.z;
                result.contained=!(double(in.sphere_radius)<std::sqrt((dz*dz+dy*dy)+dx*dx)+sphere.radius);
            }
        } else result=skip?SourceClusterFrustumResult{true,false}:
            source_cluster_frustum_visibility(spheres,ordinal,sphere.center,sphere.radius);
        if(!result.visible && !(in.flags&0x80))return;
        //132E60 only reaches the small-sphere shortcut after successful
        // frustum admission (or the explicit sky skip), before plane flags.
        if(in.mode==0 && in.context_2a78==0 && result.visible && !result.contained && sphere.radius<=2.5f)result.contained=true;
        bool negative=true,positive=true;
        if(in.flags&0x200) {
            const auto &p=in.clipping_plane;
            const float distance=(p[2]*sphere.center.z+p[1]*sphere.center.y)+
                sphere.center.x*p[0]-p[3];
            if(sphere.radius<std::abs(distance)){negative=distance<0.f;positive=distance>0.f;}
        }
        const bool main_owner=!result.visible || !in.auxiliary || !(in.flags&1) ||
            in.auxiliary->object(id)!=nullptr;
        const bool secondary=main_owner && (!in.context_2a78 || !(object.flags&0x10000u));
        const auto flags=std::uint16_t((main_owner?0x1000:0)|(secondary?0x2000:0)|
            (negative?0x8000:0)|(positive?0x4000:0)|
            (result.contained?0x800:0)|(fog?0x400:0)|(skip?0x200:0));
        std::uint16_t association=0xffff;
        if(!result.contained && (object.flags&0x80000000u)) {
            if(ordinal>=128)throw FormatError("source sky bypass requires negative association-bank write");
            if(out.associations.size()==512){association=0;out.association_capacity_reached=true;}
            else {
                association=std::uint16_t(out.associations.size());out.associations.push_back({});
                associate(association,ordinal);
            }
        }
        //163C80 reserves the last queue slot. Allocation happens first and
        // survives failure; BEC70 stamps only a successful append.
        if(out.objects.size()<in.capacity-1) {
            admitted[slot]=int(out.objects.size());out.objects.push_back({id,flags,association,0});
        }
    };
    const auto visit=[&](std::int16_t cluster,std::size_t ordinal,bool fog,bool skip) {
        for(const auto &record:collidable.cluster(cluster)) {
            const auto index=registry[record.object&0xffff];
            if(index==-1 || objects[index].full_handle!=record.object)
                throw FormatError("source collidable list refers to absent object generation");
            //132220 filters the regular collidable traversal only; its sky
            // traversal and all noncollidable owners bypass flag400.
            if(!skip && (in.flags&0x400) && objects[index].type!=7)continue;
            submit(record.object,ordinal,fog,skip);
        }
        for(const auto &record:noncollidable.cluster(cluster))submit(record.object,ordinal,fog,skip);
    };
    if(!(in.flags&0x40)) {
        for(std::size_t ordinal=0;ordinal<visible.clusters.size();++ordinal) {
            const auto cluster=visible.clusters[ordinal].source_cluster;
            if(cluster<0 || std::size_t(cluster)>=collidable.size())
                throw FormatError("source constructed cluster outside object registration bank");
            visit(cluster,ordinal,in.containing_fog[cluster]!=0,false);
        }
        if(in.context_2a78==0 && in.sky_cluster!=-1 && !visible.clusters.empty())visit(in.sky_cluster,std::size_t(-1),false,true);
    }
    return out;
}
struct PoolView {
    const SourceObjectSpatialPool &pool;
    std::size_t size() const {return pool.heads.size();}
    std::vector<ObjectLightSpatialRecord> cluster(std::int16_t id) const {
        return pool.cluster(id);
    }
};
}
SourceObjectAdmission source_object_admission(const SourcePortalVisibilitySet &visible,
    const ObjectLightSpatialLists &collidable,const ObjectLightSpatialLists &noncollidable,
    std::span<const SourceObjectAdmissionObject> objects,const SourceObjectAdmissionInput &in,
    std::span<const std::array<std::uint32_t,4>> prior) {
    return admit(visible,collidable,noncollidable,objects,in,prior);
}
SourceObjectAdmission source_object_admission(const SourcePortalVisibilitySet &visible,
    const SourceObjectSpatialPool &collidable,const SourceObjectSpatialPool &noncollidable,
    std::span<const SourceObjectAdmissionObject> objects,const SourceObjectAdmissionInput &in,
    std::span<const std::array<std::uint32_t,4>> prior) {
    return admit(visible,PoolView{collidable},PoolView{noncollidable},objects,in,prior);
}
}
