#include "halo2/source_portal_frustum.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
constexpr float edge_epsilon=0.0001f;
void bounded(std::span<const Vec2> polygon) {
    if(polygon.size()>64)throw FormatError("source projected portal exceeds64 vertices");
    for(const auto p:polygon)if(!std::isfinite(p.x)||!std::isfinite(p.y))
        throw FormatError("source projected portal is nonfinite");
}
void finite(float value) {
    if(!std::isfinite(value))throw FormatError("source portal projection input is nonfinite");
}
Vec3 source_transform_point(const Transform &t,Vec3 p) {
    // 1429D0 pre-scales the basis and groups two ADDPS pairs. Late scaling or
    // a conventional affine dot product changes independently captured words.
    const auto a=t.basis[0]*t.scale,b=t.basis[1]*t.scale,c=t.basis[2]*t.scale;
    return {(p.x*a.x+p.y*b.x)+(p.z*c.x+t.position.x),
        (p.x*a.y+p.y*b.y)+(p.z*c.y+t.position.y),
        (p.x*a.z+p.y*b.z)+(p.z*c.z+t.position.z)};
}
}
std::vector<Vec2> source_clip_portal_polygon_2d(std::array<float,3> plane,
    std::span<const Vec2> polygon,float epsilon,std::size_t capacity) {
    bounded(polygon);for(const auto v:plane)finite(v);finite(epsilon);
    if(capacity>64)throw FormatError("source2D portal output exceeds64 vertices");
    std::vector<int> side;side.reserve(polygon.size());unsigned positive=0,negative=0;
    for(const auto p:polygon) {
        const float distance=(p.y*plane[1]+p.x*plane[0])-plane[2];
        if(distance>epsilon){side.push_back(1);++positive;}
        else if(distance<0.f-epsilon){side.push_back(0);++negative;}
        else side.push_back(-1);
    }
    if(!negative)return {polygon.begin(),polygon.end()};
    if(!positive)return {};
    std::vector<Vec2> out;out.reserve(capacity);
    auto emit=[&](Vec2 p){if(out.size()<capacity)out.push_back(p);};
    for(std::size_t i=0;i<polygon.size();++i) {
        const auto previous=(i+polygon.size()-1)%polygon.size();
        const auto p=polygon[i],q=polygon[previous];
        if(side[i]==0) {
            if(side[previous]==1) {
                const float dx=p.x-q.x,dy=p.y-q.y;
                const float denominator=plane[0]*dx+plane[1]*dy;
                if(denominator<0.f-edge_epsilon || denominator>edge_epsilon) {
                    const float weight=((p.y*plane[1]+p.x*plane[0])-plane[2])/denominator;
                    emit({p.x-dx*weight,p.y-dy*weight});
                }
            }
        } else {
            if(side[i]==1 && side[previous]==0) {
                const float dx=q.x-p.x,dy=q.y-p.y;
                const float denominator=plane[0]*dx+plane[1]*dy;
                if(denominator<0.f-edge_epsilon || denominator>edge_epsilon) {
                    const float weight=((q.x*plane[0]+q.y*plane[1])-plane[2])/denominator;
                    emit({q.x-dx*weight,q.y-dy*weight});
                }
            }
            emit(p);
        }
    }
    return out;
}
std::vector<Vec2> source_intersect_portal_polygons(std::span<const Vec2> subject,
    std::span<const Vec2> clipping,float epsilon,std::size_t capacity) {
    bounded(subject);bounded(clipping);finite(epsilon);
    if(capacity>64)throw FormatError("source portal intersection output exceeds64 vertices");
    if(!subject.empty()&&clipping.empty())throw FormatError("source portal intersection needs clipping edges");
    std::vector<Vec2> out(subject.begin(),subject.end());
    for(std::size_t i=0;i<clipping.size() && !out.empty();++i) {
        const auto current=clipping[i],previous=clipping[(i+clipping.size()-1)%clipping.size()];
        // 23A2B0 retains the unrounded x difference on the x87 stack, then
        // multiplies it by its stored float. Its y difference is already float.
        const float dy=float(double(previous.y)-double(current.y));
        const double extended_dx=double(current.x)-double(previous.x);
        const float dx=float(extended_dx);
        const double norm=std::sqrt(extended_dx*double(dx)+double(dy)*double(dy));
        if(std::abs(norm)<double(edge_epsilon) || float(norm)==0.f)continue;
        const float reciprocal=1.f/float(norm),nx=dy*reciprocal,ny=dx*reciprocal;
        const float d=ny*current.y+nx*current.x;
        out=source_clip_portal_polygon_2d({nx,ny,d},out,epsilon,capacity);
    }
    return out;
}
SourcePortalProjection source_project_portal(const SourcePortalProjectionView &view,
    const SourcePortalGeometry &portal,ShaderVector plane) {
    if(portal.vertices.size()>64)throw FormatError("source authored portal exceeds64 vertices");
    bounded(view.initial_polygon);finite(view.far_distance);
    for(const auto v:view.rectangle)finite(v);
    finite(view.world_to_view.scale);
    for(const auto b:view.world_to_view.basis){finite(b.x);finite(b.y);finite(b.z);}
    finite(view.world_to_view.position.x);finite(view.world_to_view.position.y);finite(view.world_to_view.position.z);
    std::vector<Vec3> local;local.reserve(portal.vertices.size());
    for(const auto p:portal.vertices){finite(p.x);finite(p.y);finite(p.z);local.push_back(source_transform_point(view.world_to_view,p));}
    const auto clipped=source_clip_portal_plane(view.clipping_plane,local,true,.0078125f,64);
    const auto facing=source_portal_facing(plane,view.eye,portal.vertices,int(clipped.size()));
    SourcePortalProjection out;
    out.flags=portal.flags;out.back=portal.back;out.front=portal.front;
    out.classification=facing.classification;out.projected_eye_inside=facing.projected_eye_inside;
    out.clipped_count=std::uint16_t(clipped.size());out.arena_entries=64;
    constexpr float largest=std::numeric_limits<float>::max();
    out.minimum_depth=largest;out.rectangle={largest,-largest,largest,-largest};
    if(!out.classification)return out;
    out.polygon.resize(clipped.size());
    for(std::size_t i=0;i<clipped.size();++i) {
        const auto p=clipped[i];const float reciprocal=-1.f/p.z;
        const Vec2 projected{p.x*reciprocal,p.y*reciprocal};
        out.polygon[out.classification==1?clipped.size()-1-i:i]=projected;
        if(projected.x<out.rectangle[0])out.rectangle[0]=projected.x;
        if(projected.x>out.rectangle[1])out.rectangle[1]=projected.x;
        if(projected.y<out.rectangle[2])out.rectangle[2]=projected.y;
        if(projected.y>out.rectangle[3])out.rectangle[3]=projected.y;
        const float depth=0.f-p.z;
        if(out.minimum_depth>depth)out.minimum_depth=depth;
    }
    if(out.classification==3) {
        if(out.projected_eye_inside) {
            if(out.minimum_depth>.015625f)out.minimum_depth=.015625f;
            out.rectangle=view.rectangle;out.polygon=view.initial_polygon;
        } else {
            out.rectangle[0]-=.00390625f;out.rectangle[1]+=.00390625f;
            out.rectangle[2]-=.00390625f;out.rectangle[3]+=.00390625f;
            const auto &r=out.rectangle;
            out.polygon={{r[0],r[2]},{r[1],r[2]},{r[1],r[3]},{r[0],r[3]}};
        }
    }
    out.clipped_count=std::uint16_t(out.polygon.size());out.arena_entries=out.clipped_count;
    if(view.far_enabled && out.minimum_depth>view.far_distance)out.classification=0;
    return out;
}
}
