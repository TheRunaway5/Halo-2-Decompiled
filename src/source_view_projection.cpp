#include "halo2/source_view_projection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
Vec3 normalized_render(Vec3 v) {
    // 2FD90 evaluates (Z²+Y²)+X² and sqrt with x87 PC=53. The
    // threshold uses the double length, but SSE divides by its float store.
    const double x=v.x,y=v.y,z=v.z;
    const double magnitude=std::sqrt((z*z+y*y)+x*x);
    if(!(magnitude<std::bit_cast<float>(0x38d1b717u))) {
        const float scalar=1.f/static_cast<float>(magnitude);
        v={scalar*v.x,v.y*scalar,v.z*scalar};
    }
    return v;
}
}
SourceViewProjectionBank source_view_projection(const SourceViewProjectionInput& in) {
    SourceViewProjectionBank out;
    const auto f=in.forward,u=in.up;
    const Vec3 right{u.z*f.y-u.y*f.z,f.z*u.x-u.z*f.x,u.y*f.x-f.y*u.x};
    const Vec3 up{f.z*right.y-f.y*right.z,f.x*right.z-f.z*right.x,f.y*right.x-f.x*right.y};
    const Vec3 backward{0.f-f.x,0.f-f.y,0.f-f.z};
    out.camera_to_world={1,{normalized_render(right),normalized_render(up),normalized_render(backward)},in.position};
    out.inverse_view=inverse_transform(out.camera_to_world);
    const auto& p=in.projection_program;
    std::copy_n(p.begin()+12,4,out.clipping_rectangle.begin());
    out.viewport_half_metrics={(p[10]*p[4])*.5f,(p[11]*p[5])*.5f};
    float nx=0,ny=0,nz=1,d=0.f-in.near_distance;
    if(in.near_distance==0) {
        const auto& b=out.inverse_view.basis;const auto& plane=in.oblique_near_plane;
        nx=(b[2].x*plane[2]+b[1].x*plane[1])+b[0].x*plane[0];
        ny=(b[2].y*plane[2]+b[1].y*plane[1])+b[0].y*plane[0];
        nz=(b[2].z*plane[2]+b[1].z*plane[1])+b[0].z*plane[0];
        const auto position=out.inverse_view.position;
        d=((plane[3]*out.inverse_view.scale+position.z*nz)+position.y*ny)+nx*position.x;
    }
    const float reciprocal=1.f/nz;
    const float distance=0.f-reciprocal*d;
    const double plane_magnitude=(std::fabs(double(reciprocal)*ny)+std::fabs(double(reciprocal)*nx))+1.;
    float depth_scale=float(double(in.far_distance)/((double(in.far_distance)-distance)*plane_magnitude));
    float depth_x=(reciprocal*depth_scale)*nx;
    float depth_y=(reciprocal*depth_scale)*ny;
    float depth_offset=0.f-depth_scale*distance;
    if(depth_offset>0 && in.near_distance==0) {
        depth_x=0.f-depth_x;depth_y=0.f-depth_y;
        depth_offset=0.f-depth_offset;depth_scale=0.f-depth_scale;
    }
    auto& projection=out.projection;
    projection[0]=p[10];projection[5]=p[11];
    projection[2]=0.f-depth_x;projection[6]=0.f-depth_y;
    projection[8]=0.f-p[8];projection[9]=0.f-p[9];
    projection[10]=0.f-depth_scale;projection[11]=-1;
    projection[14]=depth_offset;
    return out;
}
std::array<ShaderVector,7> source_view_shader_constants(
    const SourceViewProjectionBank& bank,float actual_w_distance,bool first_person) {
    std::array<ShaderVector,7> result;
    const float scalar=16777215.f/actual_w_distance;
    const auto& b=bank.inverse_view.basis;const auto& p=bank.projection;
    const auto t=bank.inverse_view.position;
    for(unsigned row=0;row<4;++row) {
        const float x=p[row],y=p[4+row],z=p[8+row],w=p[12+row];
        auto& c=result[row];
        c[0]=((z*b[0].z+b[0].x*x)+y*b[0].y)*scalar;
        c[1]=((z*b[1].z+x*b[1].x)+y*b[1].y)*scalar;
        c[2]=((z*b[2].z+b[2].y*y)+x*b[2].x)*scalar;
        c[3]=((z*t.z+t.y*y)+x*t.x)*scalar;
        c[3]=w*scalar+c[3];
        if(first_person)for(auto& v:c)v*=std::bit_cast<float>(0x3e19999au);
    }
    for(unsigned row=0;row<3;++row) {
        const auto v=bank.camera_to_world.basis[row];
        result[4+row]={v.x,v.y,v.z,row==0?1.f:row==1?.5f:2.f};
    }
    return result;
}
std::array<ShaderVector,12> source_view_shader_bank(
    const SourceViewProjectionBank& bank,float actual_w_distance,Vec3 submitting_eye,
    RenderViewport viewport,bool first_person) {
    std::array<ShaderVector,12> result;
    const auto view=source_view_shader_constants(bank,actual_w_distance,first_person);
    std::copy(view.begin(),view.end(),result.begin());
    // Literal45E460, copied by12FA0 independently of camera distance.
    result[7]={submitting_eye.x,submitting_eye.y,submitting_eye.z,
               std::bit_cast<float>(0x443ff400u)};
    const auto physical=viewport_constants(viewport);
    std::copy(physical.begin(),physical.end(),result.begin()+8);
    return result;
}
}
