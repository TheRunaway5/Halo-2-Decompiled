#include "halo2/source_sphere_position.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
constexpr float epsilon=.0001f,near_plane=.01f;
constexpr std::array<std::array<unsigned,2>,6> projection{{{2,1},{1,2},{0,2},{2,0},{1,0},{0,1}}};
using V=std::array<float,3>;
V normalize(V v,V zero) {
    const double extended=std::sqrt((double(v[0])*v[0]+double(v[1])*v[1])+double(v[2])*v[2]);
    //30BF0 returns zero for every sub-epsilon norm, even when its input
    //components are nonzero. Both contact callers then choose their source
    //fallback normal; testing only an exactly zero length changes edge hits.
    if(std::abs(extended)<epsilon)return zero;
    const float inverse=1.f/float(extended);for(auto& x:v)x*=inverse;
    return v;
}
struct Round {V center,edge;float radius;};
template<class T>Round decode_round(const T& raw) {
    const Bytes b(raw.bytes);Round p{{b.f32(0x10),b.f32(0x14),b.f32(0x18)},{},b.f32(0x1c)};
    if constexpr(sizeof(T)==44) {p.edge={b.f32(0x1c),b.f32(0x20),b.f32(0x24)};p.radius=b.f32(0x28);}
    return p;
}
bool clip_sphere(const Round& p,const V& origin,const V& ray,float& t,V& normal) {
    const V offset{p.center[0]-origin[0],p.center[1]-origin[1],p.center[2]-origin[2]};
    const float outside=((offset[2]*offset[2]+offset[0]*offset[0])+offset[1]*offset[1])-p.radius*p.radius;
    if(outside>0) {
        const float dot=(ray[0]*offset[0]+offset[2]*ray[2])+offset[1]*ray[1];
        if(dot<=0)return false;
        const float squared=(ray[1]*ray[1]+ray[2]*ray[2])+ray[0]*ray[0];
        const float discriminant=dot*dot-squared*outside;
        if(discriminant<0)return false;
        const double root=double(dot)-std::sqrt(double(discriminant));
        if(squared<root)return false;
        t=float(root)/squared;
    } else t=0;
    normal=normalize({ray[0]*t-offset[0],ray[1]*t-offset[1],ray[2]*t-offset[2]},{0,0,1});
    return true;
}
bool clip_capsule(const Round& p,const V& origin,const V& ray,float& t,V& normal) {
    const auto& e=p.edge;
    const float edge_squared=(e[0]*e[0]+e[1]*e[1])+e[2]*e[2];
    const float slope=(e[2]*ray[2]+e[1]*ray[1])+e[0]*ray[0];
    const float a=((ray[0]*ray[0]+ray[1]*ray[1])+ray[2]*ray[2])*edge_squared-slope*slope;
    if(a==0)return false;
    const V offset{origin[0]-p.center[0],origin[1]-p.center[1],origin[2]-p.center[2]};
    const float along=(e[2]*offset[2]+e[1]*offset[1])+e[0]*offset[0];
    const float b=along*slope-((ray[1]*offset[1]+ray[0]*offset[0])+ray[2]*offset[2])*edge_squared;
    const float d=b*b-((((offset[2]*offset[2]+offset[1]*offset[1])+offset[0]*offset[0])-
        p.radius*p.radius)*edge_squared-along*along)*a;
    if(d<0)return false;
    const float root=float(std::sqrt(double(d))),inverse=1.f/a;
    float lo=(b-root)*inverse,hi=(root+b)*inverse;
    if(lo>1 || hi<0)return false;
    if(lo<0)lo=0;
    if(hi>1)hi=1;
    if(slope==0) {if(along<0 || along>edge_squared)return false;}
    else {
        const float scale=1.f/slope,first=0.f-scale*along,last=(edge_squared-along)*scale;
        if(slope<=0) {if(lo<last)lo=last;if(first<hi)hi=first;}
        else {if(lo<first)lo=first;if(last<hi)hi=last;}
        if(lo>hi)return false;
    }
    t=lo;V position;for(unsigned i=0;i<3;++i)position[i]=ray[i]*lo+offset[i];
    const float projection=0.f-((e[2]*position[2]+e[1]*position[1])+position[0]*e[0])/edge_squared;
    normal=normalize({e[0]*projection+position[0],e[1]*projection+position[1],e[2]*projection+position[2]},{1,0,0});
    return true;
}
struct Polygon {
    V normal;
    float distance,thickness;
    unsigned axis,sign,count;
    std::array<Vec2,8> vertices;
};
Polygon decode(const SourceCollisionPolygon& raw) {
    const Bytes b(raw.bytes);
    Polygon p{{b.f32(0x10),b.f32(0x14),b.f32(0x18)},b.f32(0x1c),b.f32(0x20),
        b.u16(0x24),std::to_integer<unsigned>(raw.bytes[0x26]),b.u32(0x28),{}};
    if(p.axis>2 || p.sign>1 || p.count>8)
        throw FormatError("Sphere-position polygon exceeds original terrain contact layout");
    for(unsigned i=0;i<p.count;++i)p.vertices[i]={b.f32(0x2c+i*8),b.f32(0x30+i*8)};
    return p;
}
//2466B0's x87 PC53 ray/normal dot has one float store. Its remaining
//plane and edge clipping uses scalar SSE; retain those operation groups.
bool clip(const Polygon& p,const V& origin,const V& delta,float& first) {
    const float slope=float((double(p.normal[2])*delta[2]+double(p.normal[1])*delta[1])+
        double(delta[0])*p.normal[0]);
    const float distance=((p.normal[1]*origin[1]+p.normal[2]*origin[2])+
        origin[0]*p.normal[0])-p.distance;
    float lo=0,hi=1;
    if(std::abs(slope)>=epsilon) {
        const float inverse=1.f/slope;
        const float a=0.f-distance*inverse,b=0.f-(distance-p.thickness)*inverse;
        if(slope>0) {if(a>0)lo=a;if(b<hi)hi=b;}
        else {if(b>0)lo=b;if(a<hi)hi=a;}
        if(lo>hi)return false;
    } else if(distance<0 || distance>=p.thickness)return false;
    V projected_origin,projected_delta;
    for(unsigned i=0;i<3;++i) {
        projected_origin[i]=p.normal[i]*(0.f-distance)+origin[i];
        projected_delta[i]=(0.f-slope)*p.normal[i]+delta[i];
    }
    const auto axes=projection[p.axis*2+p.sign];
    for(unsigned i=0;i<p.count;++i) {
        const auto a=p.vertices[i],b=p.vertices[(i+1)%p.count];
        const float dx=b.x-a.x,dy=b.y-a.y;
        const float denominator=dy*projected_delta[axes[0]]-projected_delta[axes[1]]*dx;
        const float numerator=(projected_origin[axes[1]]-a.y)*dx-
            dy*(projected_origin[axes[0]]-a.x);
        if(std::abs(denominator)<epsilon) {if(numerator<0)return false;}
        else {
            const float t=numerator/denominator;
            if(denominator>=0) {if(t<hi)hi=t;}
            else if(lo<t)lo=t;
            if(lo>hi)return false;
        }
    }
    first=lo;return true;
}
//2469B0 selects the earliest admitted polygon and preserves strict tie order.
V segment(std::span<const Round> spheres,std::span<const Round> capsules,
    std::span<const Polygon> bank,const V& origin,const V& delta) {
    float fraction=std::numeric_limits<float>::max();bool found=false;
    const auto consider=[&](float t,const V& normal) {
        if(t<fraction && ((delta[1]*normal[1]+delta[2]*normal[2])+delta[0]*normal[0])<-epsilon)
            {fraction=t;found=true;}
    };
    for(const auto& p:spheres) {float t;V normal;if(clip_sphere(p,origin,delta,t,normal))consider(t,normal);}
    for(const auto& p:capsules) {float t;V normal;if(clip_capsule(p,origin,delta,t,normal))consider(t,normal);}
    for(const auto& p:bank) {
        float t;
        if(clip(p,origin,delta,t) && t<fraction &&
            ((delta[1]*p.normal[1]+delta[2]*p.normal[2])+delta[0]*p.normal[0])<-epsilon) {
            fraction=t;found=true;
        }
    }
    if(!found)fraction=1;
    return {delta[0]*fraction+origin[0],delta[1]*fraction+origin[1],delta[2]*fraction+origin[2]};
}
V vertex(const Polygon& p,unsigned i) {
    V v{};const auto axes=projection[p.axis*2+p.sign];
    v[axes[0]]=p.vertices[i].x;v[axes[1]]=p.vertices[i].y;
    if(std::abs(p.normal[p.axis])>=epsilon)
        v[p.axis]=((p.distance-p.normal[axes[0]]*v[axes[0]])-
            p.normal[axes[1]]*v[axes[1]])/p.normal[p.axis];
    return v;
}
}
SourceSpherePosition source_sphere_position(Vec3 center,float radius,
    const SourceSphereContactBank& raw) {
    SourceSpherePosition out{false,center,radius};
    if(radius<=0)return out;
    if(raw.polygons.size()>256 || raw.spheres.size()>256 || raw.capsules.size()>256)
        throw FormatError("Sphere-position contact count exceeds256");
    std::vector<Polygon> bank;bank.reserve(raw.polygons.size());for(const auto& p:raw.polygons)bank.push_back(decode(p));
    std::vector<Round> spheres,capsules;
    for(const auto& p:raw.spheres)spheres.push_back(decode_round(p));
    for(const auto& p:raw.capsules)capsules.push_back(decode_round(p));
    const V origin{center.x,center.y,center.z};
    V near_sum{},far_sum{};unsigned near_count=0,far_count=0;
    float near_radius=radius,far_radius=radius;
    const float radius_squared=radius*radius;
    for(const auto& p:bank) {
        const float distance=((origin[0]*p.normal[0]+origin[1]*p.normal[1])+
            p.normal[2]*origin[2])-p.distance;
        V projected;for(unsigned i=0;i<3;++i)projected[i]=(0.f-distance)*p.normal[i]+origin[i];
        bool inside=true;float closest_squared=std::numeric_limits<float>::max();V closest{};
        for(unsigned i=0;i<p.count;++i) {
            const V a=vertex(p,i),b=vertex(p,(i+1)%p.count);
            const V offset{projected[0]-a[0],projected[1]-a[1],projected[2]-a[2]},
                edge{b[0]-a[0],b[1]-a[1],b[2]-a[2]};
            const float cross_x=edge[1]*offset[2]-edge[2]*offset[1],
                cross_y=edge[2]*offset[0]-offset[2]*edge[0],
                cross_z=offset[1]*edge[0]-edge[1]*offset[0];
            if((p.normal[2]*cross_z+p.normal[1]*cross_y)+p.normal[0]*cross_x<0) {inside=false;continue;}
            const V side{p.normal[2]*edge[1]-p.normal[1]*edge[2],
                edge[2]*p.normal[0]-p.normal[2]*edge[0],
                p.normal[1]*edge[0]-p.normal[0]*edge[1]};
            const float scale=0.f-((side[0]*offset[0]+side[2]*offset[2])+side[1]*offset[1]);
            V candidate;for(unsigned j=0;j<3;++j)candidate[j]=side[j]*scale+projected[j];
            const float along=((candidate[0]-a[0])*edge[0]+(candidate[2]-a[2])*edge[2])+
                (candidate[1]-a[1])*edge[1];
            if(along<=0)candidate=a;
            else if((edge[1]*edge[1]+edge[0]*edge[0])+edge[2]*edge[2]<=along)candidate=b;
            const float x=candidate[0]-projected[0],y=candidate[1]-projected[1],z=candidate[2]-projected[2];
            const float squared=(x*x+z*z)+y*y;
            if(squared<closest_squared) {closest_squared=squared;closest=candidate;}
        }
        if(inside) {closest_squared=0;closest=projected;}
        if(closest_squared>=radius_squared)continue;
        bool near=true;V positive,negative;
        const V delta{closest[0]-origin[0],closest[1]-origin[1],closest[2]-origin[2]};
        const float length=float(std::sqrt((double(delta[0])*delta[0]+double(delta[2])*delta[2])+double(delta[1])*delta[1]));
        if((std::abs(distance)>=near_plane || !inside) && std::abs(length)>=epsilon && length>near_plane) {
            const float inverse=1.f/length;V ray;
            ray={inverse*delta[0]*radius,delta[1]*inverse*radius,delta[2]*inverse*radius};
            positive=segment(spheres,capsules,bank,origin,ray);
            const float negative_radius=0.f-radius;
            ray={negative_radius*(inverse*delta[0]),(delta[1]*inverse)*negative_radius,(delta[2]*inverse)*negative_radius};
            negative=segment(spheres,capsules,bank,origin,ray);near=false;
        } else {
            V ray{p.normal[0]*radius,p.normal[1]*radius,p.normal[2]*radius};
            positive=segment(spheres,capsules,bank,origin,ray);negative=origin;
        }
        V middle,difference;for(unsigned i=0;i<3;++i) {middle[i]=(negative[i]+positive[i])*.5f;difference[i]=middle[i]-positive[i];}
        const float next_radius=float(std::sqrt((double(difference[0])*difference[0]+double(difference[2])*difference[2])+double(difference[1])*difference[1]));
        auto& sum=near?near_sum:far_sum;auto& count=near?near_count:far_count;auto& bound=near?near_radius:far_radius;
        for(unsigned i=0;i<3;++i)sum[i]+=middle[i];
        if(next_radius<bound)bound=next_radius;
        ++count;
    }
    const auto count=near_count?near_count:far_count;
    if(count) {
        const auto sum=near_count?near_sum:far_sum;const float inverse=1.f/float(count);
        out={true,{sum[0]*inverse,sum[1]*inverse,sum[2]*inverse},near_count?near_radius:far_radius};
    }
    return out;
}
}
