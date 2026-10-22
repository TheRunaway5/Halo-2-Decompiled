#include "halo2/source_sphere_position.hpp"
#include "halo2/object_light_spatial.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace halo2 {
namespace {
constexpr std::array<std::array<unsigned,2>,6> projection{{{2,1},{1,2},{0,2},{2,0},{1,0},{0,1}}};
float component(Vec3 p,unsigned i) {return i==0?p.x:i==1?p.y:p.z;}
unsigned axis(Vec3 p) {
    if(std::abs(p.z)<std::abs(p.y) || std::abs(p.z)<std::abs(p.x))
        return std::abs(p.y)<std::abs(p.x)?0:1;
    return 2;
}
template<class T>void put(T& raw,unsigned at,std::uint32_t value) {std::memcpy(raw.bytes.data()+at,&value,4);}
template<class T>void scalar(T& raw,unsigned at,float value) {put(raw,at,std::bit_cast<std::uint32_t>(value));}
template<class T>void vector(T& raw,unsigned at,Vec3 p) {scalar(raw,at,p.x);scalar(raw,at+4,p.y);scalar(raw,at+8,p.z);}
template<class T>void metadata(T& raw,const RawCollisionBsp& bsp,unsigned surface) {
    const auto& s=bsp.surfaces.at(surface);put(raw,0,0xffffffffu);put(raw,4,0xffffffffu);put(raw,8,surface);
    raw.bytes[12]=std::byte(s.flags);raw.bytes[13]=std::byte(s.breakable);
    const auto material=std::uint16_t(s.material);std::memcpy(raw.bytes.data()+14,&material,2);
}
void append(std::vector<unsigned>& indices,unsigned value) {
    if(indices.size()<256 && std::find(indices.begin(),indices.end(),value)==indices.end())indices.push_back(value);
}
}
SourceSphereContactStorage source_light_spawn_contacts(const RawCollisionBsp& bsp,Vec3 center,float radius) {
    if(bsp.nodes.empty() || bsp.vertex_edges.size()!=bsp.vertices.size())
        throw FormatError("Light spawn contact query requires complete source collision BSP");
    const float expanded=radius+.0625f,squared=expanded*expanded;
    std::vector<unsigned> surfaces,edges,vertices;std::vector<std::int16_t> planes;
    std::vector<bool> path(bsp.nodes.size());
    const auto perimeter=[&](unsigned surface,const auto& visit) {
        auto edge=bsp.surfaces.at(surface).first_edge;const auto first=edge;unsigned steps=0;
        do {
            if(++steps>bsp.edges.size())throw FormatError("Light spawn collision surface edge cycle");
            const auto& e=bsp.edges.at(edge);const unsigned side=e.surfaces[1]==surface;
            visit(edge,e,side);edge=e.next[side];
        } while(edge!=first);
    };
    const auto surface=[&](unsigned index,unsigned projection_axis,bool sign,Vec2 point) {
        bool near=false;
        perimeter(index,[&](unsigned,const CollisionEdge& e,unsigned side) {
            const auto delta=bsp.vertices.at(e.vertices[side])-center;
            if((delta.x*delta.x+delta.y*delta.y)+delta.z*delta.z<=squared)
                {append(vertices,e.vertices[side]);near=true;}
        });
        perimeter(index,[&](unsigned edge,const CollisionEdge& e,unsigned side) {
            const auto a=bsp.vertices.at(e.vertices[side]),delta=bsp.vertices.at(e.vertices[!side])-a;
            if(source_object_light_segment_intersects(a,delta,center,expanded)) {append(edges,edge);near=true;}
        });
        if(!near) {
            const auto axes=projection[projection_axis*2+unsigned(sign)];bool contains=true;
            perimeter(index,[&](unsigned,const CollisionEdge& e,unsigned side) {
                const auto a=bsp.vertices.at(e.vertices[side]),b=bsp.vertices.at(e.vertices[!side]);
                if((component(b,axes[1])-point.y)*(component(a,axes[0])-point.x)-
                    (component(a,axes[1])-point.y)*(component(b,axes[0])-point.x)<0)contains=false;
            });
            if(!contains)return;
        }
        append(surfaces,index);
    };
    std::function<void(std::uint16_t,unsigned,bool,Vec2,unsigned)> node2;
    node2=[&](std::uint16_t index,unsigned selected_axis,bool sign,Vec2 point,unsigned depth) {
        if(depth>bsp.nodes2.size())throw FormatError("Light spawn collision2D node cycle");
        if(index&0x8000) {surface(index&0x7fff,selected_axis,sign,point);return;}
        const auto& n=bsp.nodes2.at(index);const float distance=(n.normal.y*point.y+point.x*n.normal.x)-n.distance;
        if(distance<=expanded)node2(std::uint16_t(n.children[0]),selected_axis,sign,point,depth+1);
        if(distance<0.f-expanded)return;
        node2(std::uint16_t(n.children[1]),selected_axis,sign,point,depth+1);
    };
    std::function<void(std::uint32_t)> node3;
    node3=[&](std::uint32_t index) {
        index&=0xffffffu;
        if(index&0x800000u) {
            if(index==0xffffffu)return;
            const auto& leaf=bsp.leaves.at(index&0x7fffffu);
            for(unsigned i=0;i<leaf.count;++i) {
                const auto& r=bsp.references.at(leaf.first+i);
                if(std::find(planes.begin(),planes.end(),r.plane)==planes.end())continue;
                const auto& p=bsp.planes.at(std::uint16_t(r.plane)&0x7fffu);
                const float distance=0.f-(((center.z*p.normal.z+center.y*p.normal.y)+center.x*p.normal.x)-p.distance);
                const Vec3 projected{distance*p.normal.x+center.x,distance*p.normal.y+center.y,p.normal.z*distance+center.z};
                const auto selected_axis=axis(p.normal);const bool sign=(component(p.normal,selected_axis)>0)!=bool(std::uint16_t(r.plane)&0x8000u);
                const auto axes=projection[selected_axis*2+unsigned(sign)];
                node2(std::uint16_t(r.node),selected_axis,sign,{component(projected,axes[0]),component(projected,axes[1])},0);
            }
            return;
        }
        if(index>=bsp.nodes.size() || path[index])throw FormatError("Light spawn collision3D node cycle");
        path[index]=true;const auto& n=bsp.nodes[index];const auto& p=bsp.planes.at(std::uint16_t(n.plane));
        const float distance=((center.z*p.normal.z+center.y*p.normal.y)+p.normal.x*center.x)-p.distance;
        if(distance<=0.f-expanded)node3(std::uint32_t(n.children[0]));
        else if(distance<expanded) {
            if(planes.size()>=128)throw FormatError("Light spawn collision exceeds original plane scratch");
            planes.push_back(std::int16_t(std::uint16_t(n.plane)|0x8000u));node3(std::uint32_t(n.children[0]));
            planes.back()=std::int16_t(std::uint16_t(n.plane)&0x7fffu);node3(std::uint32_t(n.children[1]));planes.pop_back();
        } else node3(std::uint32_t(n.children[1]));
        path[index]=false;
    };
    node3(0);SourceSphereContactStorage out;
    for(auto index:vertices) {
        const auto surface=bsp.edges.at(bsp.vertex_edges.at(index)).surfaces[0];
        SourceCollisionSphere raw{};metadata(raw,bsp,surface);vector(raw,0x10,bsp.vertices.at(index));scalar(raw,0x1c,0);out.spheres.push_back(raw);
    }
    for(auto index:edges) {
        const auto& e=bsp.edges.at(index);const auto& a=bsp.surfaces.at(e.surfaces[0]);const auto& b=bsp.surfaces.at(e.surfaces[1]);
        if(a.plane==b.plane)continue;
        const auto& p=bsp.planes.at(std::uint16_t(a.plane)&0x7fff).normal;
        const auto& q=bsp.planes.at(std::uint16_t(b.plane)&0x7fff).normal;
        const auto start=bsp.vertices.at(e.vertices[0]),delta=bsp.vertices.at(e.vertices[1])-start;
        if((std::uint16_t(a.plane)&0x7fff)!=(std::uint16_t(b.plane)&0x7fff)) {
            const float facing=((q.y*p.x-q.x*p.y)*delta.z+(p.z*q.x-q.z*p.x)*delta.y)+(q.z*p.y-p.z*q.y)*delta.x;
            if(bool(std::uint16_t(a.plane)&0x8000)==bool(std::uint16_t(b.plane)&0x8000)) {if(facing<=-.0001f)continue;}
            else if(facing>=.0001f)continue;
        }
        SourceCollisionCapsule raw{};metadata(raw,bsp,e.surfaces[0]);vector(raw,0x10,start);vector(raw,0x1c,delta);scalar(raw,0x28,0);out.capsules.push_back(raw);
    }
    for(auto index:surfaces) {
        const auto& s=bsp.surfaces.at(index);auto p=bsp.planes.at(std::uint16_t(s.plane)&0x7fff);
        if(s.plane<0) {p.normal={0.f-p.normal.x,0.f-p.normal.y,0.f-p.normal.z};p.distance=0.f-p.distance;}
        SourceCollisionPolygon raw{};metadata(raw,bsp,index);vector(raw,0x10,p.normal);scalar(raw,0x1c,p.distance);scalar(raw,0x20,0);
        const auto selected_axis=axis(p.normal);const unsigned sign=component(p.normal,selected_axis)>0;
        const auto axes=projection[selected_axis*2+sign];
        const auto packed=std::uint32_t(selected_axis)|(sign<<16);put(raw,0x24,packed);
        unsigned count=0;perimeter(index,[&](unsigned,const CollisionEdge& e,unsigned side) {
            if(count>=8)throw FormatError("Light spawn collision polygon exceeds eight authored vertices");
            const auto v=bsp.vertices.at(e.vertices[side]);scalar(raw,0x2c+count*8,component(v,axes[0]));scalar(raw,0x30+count*8,component(v,axes[1]));++count;
        });put(raw,0x28,count);out.polygons.push_back(raw);
    }
    return out;
}
}
