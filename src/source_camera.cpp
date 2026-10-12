#include "halo2/source_camera.hpp"
#include <cmath>

namespace halo2 {
SourceViewGrid source_view_grid(unsigned players,unsigned view,bool wide) {
  if(players<1 || players>4 || view>=players)throw FormatError("invalid source split view");
  SourceViewGrid result;int columns=1,rows=1;
  while(columns*rows<int(players)) {
    if(wide?columns<=rows:columns<rows)++columns;
    else {columns=1;++rows;}
  }
  result.grid={std::int16_t(columns),std::int16_t(rows)};
  unsigned offset=0;
  if(players<unsigned(columns*rows)) {
    if(!wide) {
      if(!view)result.span[0]=2;
      else offset=1;
    } else {
      if(!view)result.span[1]=2;
      else offset=view/unsigned(columns);
    }
  }
  result.position={std::int16_t((view+offset)%unsigned(columns)),
                   std::int16_t((view+offset)/unsigned(columns))};
  return result;
}
SourceCameraAngles source_camera_angles(float horizontal,float aspect,
    std::array<std::int16_t,2> grid,std::array<std::int16_t,2> span,float baseline) {
  if(!(aspect>0 && baseline>0 && grid[0]>0 && grid[1]>0 && span[0]>0 && span[1]>0))
    throw FormatError("source camera aspect, baseline and split grid must be positive");
  const int ratio=(int(span[1])*int(grid[0])*100)/(int(grid[1])*int(span[0]));
  const float factor=ratio==200?.5f:ratio==50?.800000011920929f:.7850000262260437f;
  const float observer_vertical=float(2*std::atan(std::tan(double(horizontal)*.5)/double(aspect)));
  return {observer_vertical*factor,(horizontal/baseline)*factor};
}
namespace {
Vec3 rotate_observer(Vec3 v,Vec3 a,float sine,float cosine,bool second) {
  // 171D90 retains these scalar operation orders on both rotations.
  const float xx=(1-a.x*a.x)*cosine+a.x*a.x;
  const float xy=(1-cosine)*a.x*a.y;
  const float zz=(1-a.z*a.z)*cosine+a.z*a.z;
  const float yy=(1-a.y*a.y)*cosine+a.y*a.y;
  const float z=(1-cosine)*a.z;
  const float zx=z*a.x,zy=z*a.y;
  const float m01=xy-sine*a.z,m10=xy+sine*a.z;
  const float m02=zx+sine*a.y,m20=zx-sine*a.y;
  const float m12=zy-sine*a.x,m21=zy+sine*a.x;
  if(second)return {m02*v.z+m01*v.y+xx*v.x,
      m12*v.z+yy*v.y+m10*v.x,m21*v.y+zz*v.z+m20*v.x};
  return {v.x*xx+m02*v.z+m01*v.y,
      m12*v.z+yy*v.y+v.x*m10,m21*v.y+zz*v.z+v.x*m20};
}
Vec3 normalize_source(Vec3 v) {
  const float length=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
  if(std::abs(length)>=.00009999999747378752f) {
    const float inverse=1/length;v={inverse*v.x,inverse*v.y,inverse*v.z};
  }
  return v;
}
}
SourceObserverAim source_observer_aim(Vec3 forward,Vec3 up,float horizontal,
    Vec2 offset,float aspect) {
  if(!(aspect>0))throw FormatError("source observer aspect must be positive");
  const auto original=forward;
  const long double vertical=2*std::atan(std::tan(static_cast<long double>(horizontal)*.5L)/aspect);
  const long double half_tangent=std::tan(vertical*.5L);
  const auto horizontal_rotation=std::atan(half_tangent*offset.x);
  const float vertical_rotation=float(std::atan(half_tangent*offset.y));
  const float first_sine=float(std::sin(horizontal_rotation));
  const float first_cosine=float(std::cos(horizontal_rotation));
  forward=rotate_observer(forward,{0,0,1},first_sine,first_cosine,false);
  up=rotate_observer(up,{0,0,1},first_sine,first_cosine,false);
  const Vec3 axis{forward.y*up.z-up.y*forward.z,
    forward.z*up.x-forward.x*up.z,up.y*forward.x-forward.y*up.x};
  const float sine=float(std::sin(static_cast<long double>(vertical_rotation)));
  const float cosine=float(std::cos(static_cast<long double>(vertical_rotation)));
  forward=rotate_observer(forward,axis,sine,cosine,true);
  up=rotate_observer(up,axis,sine,cosine,true);
  float dot=forward.y*original.y+forward.z*original.z+forward.x*original.x;
  if(dot<=.00009999999747378752f)dot=.00009999999747378752f;
  const float reciprocal=1/dot;
  const Vec3 difference{reciprocal*original.x-forward.x,
    original.y*reciprocal-forward.y,original.z*reciprocal-forward.z};
  return {forward,up,std::sqrt(difference.x*difference.x+
      difference.y*difference.y+difference.z*difference.z)};
}
ShaderVector source_camera_frustum(RenderViewport outer,RenderViewport safe) {
  const int left=int(outer.left),right=int(outer.left+outer.width),
    top=int(outer.top),bottom=int(outer.top+outer.height),
    safe_left=int(safe.left),safe_right=int(safe.left+safe.width),
    safe_top=int(safe.top),safe_bottom=int(safe.top+safe.height);
  if(right<=left || bottom<=top || safe_bottom<=safe_top)
    throw FormatError("source camera rectangles must be positive");
  const float y_scale=2.f/float(safe_bottom-safe_top);
  const float x_scale=(float(bottom-top)/float(right-left))*y_scale;
  const float x_center=float(safe_right+safe_left)*.5f;
  const float y_center=float(safe_top+safe_bottom)*.5f;
  return {(float(left)-x_center)*x_scale,(float(right)-x_center)*x_scale,
    (y_center-float(bottom))*y_scale,(y_center-float(top))*y_scale};
}
SourceCameraProjection source_camera_projection(float angle,RenderViewport outer,
    RenderViewport safe,bool wide) {
  const auto bounds=source_camera_frustum(outer,safe);
  const float half_x=(bounds[1]-bounds[0])*.5f,
    half_y=(bounds[3]-bounds[2])*.5f;
  const float center_x=((bounds[1]+bounds[0])/half_x)*-.5f,
    center_y=((bounds[2]+bounds[3])/half_y)*-.5f;
  float aspect=outer.width/outer.height;
  if(wide)aspect*=1.3333333730697632f;
  const float tangent=float(std::tan(static_cast<long double>(angle)*.5L));
  return {1/(half_x*aspect*tangent),1/(half_y*tangent),-center_x,-center_y,
    float(2*std::atan(static_cast<long double>(half_y*tangent))),
    {1/(half_x*(wide?1.3333333730697632f:1.f)),1/half_y},{center_x,center_y}};
}
Vec2 source_hud_aim_anchor(const SourceHudAimCamera &camera,RenderViewport outer,
    RenderViewport safe,float displacement,bool active) {
  const int top=int(safe.top),left=int(safe.left),
    bottom=int(safe.top+safe.height),right=int(safe.left+safe.width);
  Vec2 result{float((left+right)/2),float((top+bottom)/2)};
  if(active) {
    if(!(outer.width>0 && outer.height>0 && camera.projection_angle>0))
      throw FormatError("source HUD aim camera projection must be positive");
    const Vec3 point{camera.position.x+camera.forward.x-camera.up.x*displacement,
      camera.position.y+camera.forward.y-camera.up.y*displacement,
      camera.position.z+camera.forward.z-camera.up.z*displacement};
    const Vec3 r=normalize_source({camera.up.z*camera.forward.y-camera.up.y*camera.forward.z,
      camera.forward.z*camera.up.x-camera.up.z*camera.forward.x,
      camera.up.y*camera.forward.x-camera.forward.y*camera.up.x});
    const Vec3 u=normalize_source({camera.forward.z*r.y-camera.forward.y*r.z,
      camera.forward.x*r.z-camera.forward.z*r.x,camera.forward.y*r.x-camera.forward.x*r.y});
    const Vec3 b=normalize_source({-camera.forward.x,-camera.forward.y,-camera.forward.z});
    const Vec3 negative{-camera.position.x,-camera.position.y,-camera.position.z};
    const auto transform=[&](Vec3 axis,bool final) {
      const float translation=final?
        axis.x*negative.x+axis.z*negative.z+axis.y*negative.y:
        axis.z*negative.z+axis.x*negative.x+axis.y*negative.y;
      return axis.z*point.z+axis.y*point.y+axis.x*point.x+translation;
    };
    const Vec3 view{transform(r,false),transform(u,false),transform(b,true)};
    if(view.z<0) {
      const auto projection=source_camera_projection(camera.projection_angle,outer,safe,camera.widescreen);
      const float reciprocal=-1/view.z;
      const float x=(projection.horizontal_scale*view.x+projection.horizontal_offset*view.z)*reciprocal;
      const float y=0.f-(projection.vertical_scale*view.y+projection.vertical_offset*view.z)*reciprocal;
      if(x>=-1 && x<=1 && y>=-1 && y<=1)
        result.y=(y+1)*.5f*float(int(outer.top+outer.height)-int(outer.top))+float(int(outer.top));
    }
  }
  result.x=std::floor(result.x+.5f);result.y=std::floor(result.y+.5f);
  return result;
}
} // namespace halo2
