#include "halo2/source_portal_clip.hpp"
#include <cmath>

namespace halo2 {
namespace {
void bounded(ShaderVector plane,std::span<const Vec3> polygon) {
    if(polygon.size()>64)throw FormatError("source portal polygon exceeds scratch capacity");
    for(float v:plane)if(!std::isfinite(v))throw FormatError("source portal plane is nonfinite");
    for(const auto p:polygon)if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
        throw FormatError("source portal polygon is nonfinite");
}
void finite_eye(Vec3 eye) {
    if(!std::isfinite(eye.x)||!std::isfinite(eye.y)||!std::isfinite(eye.z))
        throw FormatError("source portal eye is nonfinite");
}
float component(Vec3 v,unsigned i){return i==0?v.x:i==1?v.y:v.z;}
}
std::vector<Vec3> source_clip_portal_plane(ShaderVector plane,
    std::span<const Vec3> polygon,bool keep_positive,float epsilon,std::size_t capacity) {
    bounded(plane,polygon);
    if(capacity>64 || !std::isfinite(epsilon))throw FormatError("source portal clipping inputs exceed bounds");
    std::vector<Vec3> output;
    output.reserve(capacity);
    if(polygon.empty())return output;
    const float offset=keep_positive?epsilon:0.f-epsilon;
    auto previous=polygon.back();
    float previous_distance=((previous.z*plane[2]+previous.y*plane[1])+plane[0]*previous.x)-plane[3]-offset;
    bool previous_positive=previous_distance>=0.f;
    const auto unrolled=polygon.size()/4*4;
    for(std::size_t i=0;i<polygon.size();++i) {
        const auto current=polygon[i];
        // The original four-at-a-time loop and scalar tail use distinct
        // ADDSS orders. A uniform dot helper changes crossing/output words.
        float distance;
        if(i>=unrolled || i%4==0)
            distance=((plane[0]*current.x+plane[2]*current.z)+current.y*plane[1])-plane[3]-offset;
        else distance=((plane[0]*current.x+current.y*plane[1])+current.z*plane[2])-plane[3]-offset;
        const bool positive=distance>=0.f;
        if(positive!=previous_positive) {
            if(output.size()>=capacity)return output;
            const float weight=previous_distance/(previous_distance-distance);
            output.push_back({(current.x-previous.x)*weight+previous.x,
                weight*(current.y-previous.y)+previous.y,
                weight*(current.z-previous.z)+previous.z});
        }
        if(positive==keep_positive) {
            if(output.size()>=capacity)return output;
            output.push_back(current);
        }
        previous=current;previous_distance=distance;previous_positive=positive;
    }
    return output;
}
bool source_portal_projected_eye_inside(ShaderVector plane,Vec3 eye,
    std::span<const Vec3> polygon) {
    bounded(plane,polygon);finite_eye(eye);
    unsigned major;
    if(std::abs(plane[1])<=std::abs(plane[2]) && std::abs(plane[0])<=std::abs(plane[2]))major=2;
    else major=std::abs(plane[0])<=std::abs(plane[1])?1:0;
    constexpr unsigned axes[6][2]{{2,1},{1,2},{0,2},{2,0},{1,0},{0,1}};
    const auto &axis=axes[major*2+unsigned(plane[major]>0.f)];
    // 173910 recomputes a stored SSE distance even when 173890 admitted its
    // different, extended-precision near-plane distance.
    const float displacement=0.f-((eye.z*plane[2]+eye.y*plane[1])+eye.x*plane[0]-plane[3]);
    const Vec3 projected{plane[0]*displacement+eye.x,displacement*plane[1]+eye.y,
        plane[2]*displacement+eye.z};
    const float x=component(projected,axis[0]),y=component(projected,axis[1]);
    constexpr float tolerance=0.0001f; // exact source 38D1B717
    for(std::size_t i=0;i<polygon.size();++i) {
        const auto &a=polygon[i],&b=polygon[(i+1)%polygon.size()];
        const float side=(y-component(a,axis[1]))*(component(b,axis[0])-component(a,axis[0]))-
            (x-component(a,axis[0]))*(component(b,axis[1])-component(a,axis[1]));
        if(side<0.f-tolerance)return false;
    }
    return true;
}
SourcePortalFacing source_portal_facing(ShaderVector plane,Vec3 eye,
    std::span<const Vec3> authored_polygon,int clipped_count) {
    bounded(plane,authored_polygon);finite_eye(eye);
    if(clipped_count<0 || clipped_count>64)throw FormatError("source clipped portal count exceeds bounds");
    const double distance=((double(eye.z)*double(plane[2])+double(eye.y)*double(plane[1]))+
        double(plane[0])*double(eye.x))-double(plane[3]);
    SourcePortalFacing result;
    if(std::abs(distance)<=0.015625) {
        result.projected_eye_inside=source_portal_projected_eye_inside(plane,eye,authored_polygon);
        if(result.projected_eye_inside || clipped_count>0)result.classification=3;
    } else if(clipped_count>0) {
        if(clipped_count<3)result.classification=3;
        else result.classification=float(distance)>=0.f?2:1;
    }
    return result;
}
}
