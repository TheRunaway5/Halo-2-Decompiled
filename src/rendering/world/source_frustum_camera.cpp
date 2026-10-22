#include "halo2/source_frustum_camera.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
Vec3 normalized_source(Vec3 v) {
    // 30BF0 evaluates X²+Y²+Z² and sqrt with x87 PC=53, then stores the
    // length to float before the SSE reciprocal and component products.
    const double x=v.x,y=v.y,z=v.z;
    const double length=std::sqrt((x*x+y*y)+z*z);
    if (!(length<std::bit_cast<float>(0x38d1b717u))) {
        const float scalar=1.f/static_cast<float>(length);
        v={v.x*scalar,scalar*v.y,scalar*v.z};
    }
    return v;
}
Vec3 source_point(const Transform& t,Vec3 v) {
    // 1429D0 pre-scales each basis column, then adds (X+Y)+(Z+translation).
    const Vec3 a=t.basis[0]*t.scale,b=t.basis[1]*t.scale,c=t.basis[2]*t.scale;
    return {(v.x*a.x+v.y*b.x)+(v.z*c.x+t.position.x),
            (v.x*a.y+v.y*b.y)+(v.z*c.y+t.position.y),
            (v.x*a.z+v.y*b.z)+(v.z*c.z+t.position.z)};
}
ShaderVector source_plane(const Transform& t,ShaderVector p,bool zy_order) {
    const Vec3 n{(t.basis[2].x*p[2]+t.basis[1].x*p[1])+t.basis[0].x*p[0],
                 (t.basis[2].y*p[2]+t.basis[1].y*p[1])+t.basis[0].y*p[0],
                 (t.basis[2].z*p[2]+t.basis[1].z*p[1])+t.basis[0].z*p[0]};
    const float scaled=p[3]*t.scale;
    const float d=zy_order
        ? ((t.position.z*n.z+t.position.y*n.y)+scaled)+n.x*t.position.x
        : ((t.position.y*n.y+scaled)+t.position.z*n.z)+t.position.x*n.x;
    return {n.x,n.y,n.z,d};
}
bool inconsistent(const SourceCameraFrustum& f) {
    const auto outside=[&](unsigned p,Vec3 v) {
        const auto& n=f.planes[p];
        const double d=((double(n[2])*v.z+double(n[1])*v.y)+double(n[0])*v.x)-n[3];
        // Retail FCOMPI/JBE also admits unordered values to the diagnostic.
        return !(std::fabs(d)<std::bit_cast<float>(0x3c23d70au));
    };
    return outside(0,f.corners[1])||outside(2,f.corners[1])||outside(5,f.corners[1])
        ||outside(1,f.corners[3])||outside(2,f.corners[3])||outside(5,f.corners[3])
        ||outside(0,f.corners[0])||outside(3,f.corners[0])||outside(5,f.corners[0])
        ||outside(1,f.corners[2])||outside(3,f.corners[2])||outside(5,f.corners[2])
        ||outside(0,f.eye)||outside(1,f.eye)||outside(2,f.eye)||outside(3,f.eye);
}
}

bool source_frustum_rectangle(const SourceFrustumCamera& camera,ShaderVector rectangle,
    std::uint32_t group,SourceCameraFrustum& out,bool* plane_inconsistent) {
    if (plane_inconsistent) *plane_inconsistent=false;
    if (!(rectangle[1]>rectangle[0])||!(rectangle[3]>rectangle[2])) return false;
    out.rectangle=rectangle;
    out.source_group=group;
    const float xmin=rectangle[0]*camera.far_distance;
    const float xmax=rectangle[1]*camera.far_distance;
    const float ymin=rectangle[2]*camera.far_distance;
    const float ymax=rectangle[3]*camera.far_distance;
    const float z=0.f-camera.far_distance;
    out.corners={source_point(camera.camera_to_world,{xmin,ymax,z}),
                 source_point(camera.camera_to_world,{xmin,ymin,z}),
                 source_point(camera.camera_to_world,{xmax,ymax,z}),
                 source_point(camera.camera_to_world,{xmax,ymin,z})};
    out.eye=camera.camera_to_world.position;
    const float largest=std::numeric_limits<float>::max();
    out.bounds={largest,-largest,largest,-largest,largest,-largest};
    // 11F770 receives the four contiguous corner points followed by the eye.
    const auto accumulate=[&](Vec3 p) {
        if (out.bounds[0]>p.x) out.bounds[0]=p.x;
        if (p.x>out.bounds[1]) out.bounds[1]=p.x;
        if (out.bounds[2]>p.y) out.bounds[2]=p.y;
        if (p.y>out.bounds[3]) out.bounds[3]=p.y;
        if (out.bounds[4]>p.z) out.bounds[4]=p.z;
        if (p.z>out.bounds[5]) out.bounds[5]=p.z;
    };
    for (const auto& p:out.corners) accumulate(p);
    accumulate(out.eye);
    const std::array<Vec3,4> side{{{-1.f,0.f,0.f-rectangle[0]},
                                  {1.f,0.f,rectangle[1]},
                                  {0.f,-1.f,0.f-rectangle[2]},
                                  {0.f,1.f,rectangle[3]}}};
    for (unsigned i=0;i<4;++i) {
        const Vec3 n=normalized_source(side[i]);
        out.planes[i]=source_plane(camera.camera_to_world,{n.x,n.y,n.z,0.f},i>=2);
    }
    if (camera.near_plane_enabled) {
        for (unsigned i=0;i<4;++i) out.planes[4][i]=0.f-camera.near_plane[i];
    } else {
        out.planes[4]=source_plane(camera.camera_to_world,{0.f,0.f,1.f,0.f},false);
    }
    out.planes[5]=source_plane(camera.camera_to_world,{0.f,0.f,-1.f,camera.far_distance},false);
    for (unsigned i=0;i<4;++i) out.rays[i]=out.corners[i]-out.eye;
    if (plane_inconsistent) *plane_inconsistent=inconsistent(out);
    return true;
}

SourceFrustumCamera source_frustum_camera(const SourceFrustumCameraInput& input) {
    SourceFrustumCamera camera{};
    const Vec3 up=normalized_source(input.up);
    const Vec3 backward=normalized_source({0.f-input.forward.x,0.f-input.forward.y,0.f-input.forward.z});
    const Vec3 right=normalized_source({up.y*backward.z-up.z*backward.y,
                                       up.z*backward.x-backward.z*up.x,
                                       backward.y*up.x-up.y*backward.x});
    camera.camera_to_world={1.f,{right,up,backward},input.position};
    camera.inverse_view=inverse_transform(camera.camera_to_world);
    camera.near_plane_enabled=1;
    camera.near_distance=input.near_distance;
    camera.near_plane={input.forward.x,input.forward.y,input.forward.z,
        ((input.forward.z*input.position.z+input.forward.y*input.position.y)
            +input.forward.x*input.position.x)+input.near_distance};
    camera.far_plane_enabled=1;
    camera.far_distance=input.far_distance;
    camera.full_frustum_enabled=1;
    camera.polygon_count=4;
    const auto& r=input.rectangle;
    camera.polygon={Vec2{r[0],r[2]},Vec2{r[1],r[2]},Vec2{r[1],r[3]},Vec2{r[0],r[3]}};
    source_frustum_rectangle(camera,r,0,camera.full_frustum);
    return camera;
}

SourceFrustumProjection source_frustum_projection(const SourceFrustumProjectionInput& input) {
    SourceFrustumProjection result;
    const auto& o=input.outer;const auto& s=input.safe;
    const float height=float(int(o[2])-int(o[0]));
    const float width=float(int(o[3])-int(o[1]));
    const float y_scale=2.f/float(int(s[2])-int(s[0]));
    const float x_scale=(height/width)*y_scale;
    const float x_center=float(int(s[3])+int(s[1]))*.5f;
    const float y_center=float(int(s[0])+int(s[2]))*.5f;
    auto& bounds=result.projection_bounds;
    bounds={(float(o[1])-x_center)*x_scale,(float(o[3])-x_center)*x_scale,
            (y_center-float(o[2]))*y_scale,(y_center-float(o[0]))*y_scale};
    result.view_modifier=input.rectangle_override_enabled||input.screen_offset_enabled||
        (input.scenario_present&&input.scenario_type==2);
    if(input.rectangle_override_enabled) {
        const float dx=bounds[1]-bounds[0],dy=bounds[3]-bounds[2];
        const float cx=((input.rectangle_center.x+1.f)*.5f)*dx+bounds[0];
        const float cy=((input.rectangle_center.y+1.f)*.5f)*dy+bounds[2];
        const float sx=(input.rectangle_scale*dx)*.5f;
        const float sy=(input.rectangle_scale*dy)*.5f;
        bounds={cx-sx,sx+cx,cy-sy,sy+cy};
    } else if(input.screen_offset_enabled) {
        const float x=((bounds[1]-bounds[0])*std::bit_cast<float>(0x3acccccdu))*input.screen_offset.x;
        const float y=((bounds[3]-bounds[2])*std::bit_cast<float>(0x3b088889u))*input.screen_offset.y;
        bounds={bounds[0]+x,bounds[1]+x,bounds[2]-y,bounds[3]-y};
    }
    auto& p=result.projection_program;
    const ShaderVector b=input.use_projection_bounds?bounds:ShaderVector{-1,1,-1,1};
    std::copy(b.begin(),b.end(),p.begin());p[4]=width;p[5]=height;
    const float half_x=(p[1]-p[0])*.5f,half_y=(p[3]-p[2])*.5f;
    const float cx=((p[1]+p[0])/half_x)*-.5f;
    const float cy=((p[2]+p[3])/half_y)*-.5f;
    p[6]=half_x;p[7]=half_y;p[8]=cx;p[9]=cy;
    float tangent=float(std::tan(static_cast<long double>(input.projection_angle)*.5L));
    float aspect=width/height;
    const bool auxiliary=input.auxiliary_state_present&&input.auxiliary_state_flag;
    const bool excluded=input.layout_state_present&&input.layout_state_flag&&
        input.layout_state_index!=-1&&input.layout_state_mode==3;
    if(!excluded&&input.widescreen) {
        aspect*=std::bit_cast<float>(0x3faaaaabu);
        if(auxiliary)tangent=(tangent*std::bit_cast<float>(0x3faaaaabu))*std::bit_cast<float>(0x3f100000u);
    } else if(auxiliary) {
        const float scale=height*std::bit_cast<float>(0x3b088889u);
        p[2]*=scale;p[3]=scale*p[3];
    }
    const float horizontal=1.f/((half_x*aspect)*tangent);
    const float horizontal_inverse=1.f/horizontal;
    const float vertical=1.f/(half_y*tangent);
    const float vertical_inverse=1.f/vertical;
    p[10]=horizontal;p[11]=vertical;
    p[12]=0.f-(cx+1.f)*horizontal_inverse;
    p[13]=0.f-(cx-1.f)*horizontal_inverse;
    p[14]=0.f-(cy+1.f)*vertical_inverse;
    p[15]=0.f-(cy-1.f)*vertical_inverse;
    return result;
}
}
