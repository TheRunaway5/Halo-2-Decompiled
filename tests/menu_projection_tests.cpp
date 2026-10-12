#include "halo2/menu_projection.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>

namespace {
void require(bool value,const char *message) {
  if(!value)throw halo2::FormatError(message);
}
std::array<double,4> project(const halo2::SourceViewProjectionBank &bank,halo2::Vec3 point) {
  const auto &view=bank.inverse_view;
  const std::array<double,4> camera{
      double(view.scale)*(double(view.basis[0].x)*point.x+double(view.basis[1].x)*point.y+
                          double(view.basis[2].x)*point.z)+view.position.x,
      double(view.scale)*(double(view.basis[0].y)*point.x+double(view.basis[1].y)*point.y+
                          double(view.basis[2].y)*point.z)+view.position.y,
      double(view.scale)*(double(view.basis[0].z)*point.x+double(view.basis[1].z)*point.y+
                          double(view.basis[2].z)*point.z)+view.position.z,1};
  std::array<double,4> clip{};
  for(unsigned row=0;row<4;++row)
    for(unsigned column=0;column<4;++column)
      clip[row]+=double(bank.projection[column*4+row])*camera[column];
  return clip;
}
unsigned full_frame_projection() {
  struct Size {unsigned width,height;float x,y;};
  const std::array sizes{
      Size{640,480,1,1},Size{960,720,1,1},Size{1280,1024,1,15.f/16.f},
      Size{1280,720,3.f/4.f,1},Size{1280,800,5.f/6.f,1},
      Size{1680,720,4.f/7.f,1},Size{2560,720,3.f/8.f,1}};
  struct Pose {halo2::Vec3 position,forward,up;};
  const std::array poses{
      Pose{{0,0,0},{1,0,0},{0,0,1}},
      Pose{{2,-5,3},{.6f,.8f,0},{0,0,1}},
      Pose{{27.5f,-84,16},{.36f,.48f,.8f},{-.8f,.6f,0}}};
  unsigned points{};
  for(float degrees:std::array<float,3>{60,70,90})for(const auto &pose:poses) {
    halo2::Camera observer;
    observer.position=pose.position;observer.forward=pose.forward;observer.up=pose.up;
    observer.oriented=true;observer.horizontal_field_of_view=true;observer.field_of_view=degrees;
    const auto original_camera=halo2::menu_render_camera(observer);
    const auto original=halo2::menu_render_view_bank(original_camera);
    const auto constants=halo2::source_view_shader_constants(original,1024,false);
    const auto complete_constants=halo2::source_view_shader_bank(
        original,1024,observer.position,{0,0,640,480},false);
    for(const auto &size:sizes) {
      const auto extended=halo2::menu_full_frame_projection(original_camera,size.width,size.height);
      const auto canvas=halo2::frontend_canvas(size.width,size.height);
      require(extended.scale.x==size.x && extended.scale.y==size.y,
              "full-frame projection has incorrect explicit aspect scales");
      require(std::memcmp(&extended.bank.inverse_view,&original.inverse_view,sizeof(original.inverse_view))==0 &&
              std::memcmp(&extended.bank.camera_to_world,&original.camera_to_world,sizeof(original.camera_to_world))==0 &&
              extended.bank.clipping_rectangle==original.clipping_rectangle &&
              extended.bank.viewport_half_metrics.x==original.viewport_half_metrics.x &&
              extended.bank.viewport_half_metrics.y==original.viewport_half_metrics.y,
              "full-frame projection changes source camera basis or depth/clip metadata");
      for(unsigned column=0;column<4;++column)for(unsigned row=0;row<4;++row) {
        const auto i=column*4+row;
        const float factor=row==0?size.x:row==1?size.y:1.f;
        const float expected=factor==1.f?original.projection[i]:original.projection[i]*factor;
        require(std::bit_cast<std::uint32_t>(extended.bank.projection[i])==
                    std::bit_cast<std::uint32_t>(expected),
                "full-frame projection changes an unscaled axis/depth word or uses wrong XY scaling");
      }
      const auto actual_constants=halo2::source_view_shader_constants(extended.bank,1024,false);
      require(std::memcmp(actual_constants.data()+2,constants.data()+2,5*sizeof(halo2::ShaderVector))==0,
              "full-frame source shader constants change depth or camera basis");
      const auto &camera=extended.camera;
      require(camera.position.x==original_camera.position.x && camera.position.y==original_camera.position.y &&
              camera.position.z==original_camera.position.z && camera.forward.x==original_camera.forward.x &&
              camera.forward.y==original_camera.forward.y && camera.forward.z==original_camera.forward.z &&
              camera.up.x==original_camera.up.x && camera.up.y==original_camera.up.y && camera.up.z==original_camera.up.z &&
              camera.field_of_view==original_camera.field_of_view &&
              camera.horizontal_field_of_view==original_camera.horizontal_field_of_view &&
              camera.projection_clip_scale.x==original_camera.projection_clip_scale.x &&
              camera.projection_clip_scale.y==original_camera.projection_clip_scale.y &&
              camera.projection_clip_offset.x==0 && camera.projection_clip_offset.y==0,
              "full-frame native camera changes source observer/basis/clip fields");
      const double tangent=std::tan(double(*camera.projection_vertical_angle)*.5);
      const double original_tangent=std::tan(double(*original_camera.projection_vertical_angle)*.5);
      require(std::abs(original_tangent/tangent-size.y)<1e-6 &&
              std::abs((4.0/3.0)/(double(size.width)/size.height)*original_tangent/tangent-size.x)<1e-6,
              "full-frame GLM/haze camera disagrees with source bank XY scales");
      if(size.x==1 && size.y==1) {
        require(std::memcmp(&extended.bank,&original,sizeof(original))==0 &&
                std::memcmp(actual_constants.data(),constants.data(),sizeof(constants))==0 &&
                camera.projection_vertical_angle==original_camera.projection_vertical_angle,
                "4:3 full-frame projection changes the original bank or source constants");
        const auto actual_complete=halo2::source_view_shader_bank(
            extended.bank,1024,observer.position,{0,0,640,480},false);
        require(std::memcmp(actual_complete.data(),complete_constants.data(),sizeof(complete_constants))==0,
                "4:3 full-frame projection changes the complete canonical source shader bank");
      }
      for(float depth:std::array<float,6>{.03125f,.25f,2,64,512,900})
        for(float x:std::array<float,3>{-.4f,0,.33f})for(float y:std::array<float,3>{-.22f,0,.19f}) {
          const auto &basis=original.camera_to_world.basis;
          const auto point=observer.position+basis[0]*(x*depth)+basis[1]*(y*depth)-basis[2]*depth;
          const auto before=project(original,point),after=project(extended.bank,point);
          require(before[3]>0 && before[2]/before[3]==after[2]/after[3] && before[3]==after[3],
                  "full-frame world point changes original depth/W");
          const double expected_x=canvas.left+(before[0]/before[3]*.5+.5)*640.0*canvas.scale;
          const double expected_y=canvas.top+(1-before[1]/before[3])*.5*480.0*canvas.scale;
          const double actual_x=(after[0]/after[3]*.5+.5)*size.width;
          const double actual_y=(1-after[1]/after[3])*.5*size.height;
          // The extension scales f32 matrix words once. Allow less than one
          // thousandth of a physical pixel for that rounding, across depth.
          require(std::abs(actual_x-expected_x)<.001 && std::abs(actual_y-expected_y)<.001,
                  "full-frame scenery stretches or moves a centered world point");
          ++points;
        }
    }
  }
  bool rejected=false;
  try{halo2::Camera camera;(void)halo2::menu_full_frame_projection(camera,0,480);}
  catch(const std::invalid_argument &){rejected=true;}
  require(rejected,"full-frame projection admits an empty drawable");
  return points;
}
}

int main(int argc, char **argv) {
  try {
    if (argc != 2 && argc != 3) throw halo2::FormatError("menu projection test requires original instruction fixtures");
    std::ifstream file(argv[1], std::ios::binary);
    char magic[8]{};
    std::uint32_t count{};
    if (!file.read(magic,8) || std::memcmp(magic,"H2MPROJ1",8) ||
        !file.read(reinterpret_cast<char*>(&count),4) || count!=3)
      throw halo2::FormatError("invalid original menu projection fixture");
    float maximum_error=0;
    for (unsigned i=0;i<count;++i) {
      std::array<float,6> row{};
      if (!file.read(reinterpret_cast<char*>(row.data()),sizeof(row)))
        throw halo2::FormatError("truncated menu projection fixture");
      halo2::Camera observer;
      observer.horizontal_field_of_view=true;
      observer.field_of_view=row[0];
      const auto render=halo2::menu_render_camera(observer);
      if (render.field_of_view!=observer.field_of_view || !render.horizontal_field_of_view)
        throw halo2::FormatError("render projection changed the authoritative observer FOV");
      const float angle=halo2::camera_vertical_field_of_view(render,4.f/3.f);
      const float tangent=float(std::tan(double(angle)*.5));
      const std::array<float,5> actual{angle,render.projection_clip_scale.x,
          render.projection_clip_scale.y,render.projection_clip_scale.x/((4.f/3.f)*tangent),
          render.projection_clip_scale.y/tangent};
      for (unsigned j=0;j<actual.size();++j)
        maximum_error=std::max(maximum_error,std::abs(actual[j]-row[j+1]));
    }
    if (maximum_error>1e-6f) throw halo2::FormatError("menu lens differs from original instructions");
    if (file.peek()!=std::char_traits<char>::eof()) throw halo2::FormatError("unexpected projection fixture data");
    unsigned display_cases=0;
    if (argc==3) {
      std::ifstream display(argv[2],std::ios::binary);
      if (!display.read(magic,8) || std::memcmp(magic,"H2MDISP2",8) ||
          !display.read(reinterpret_cast<char*>(&count),4) || count!=48)
        throw halo2::FormatError("invalid original display projection fixture");
      for (unsigned i=0;i<count;++i) {
        std::array<std::uint32_t,4> mode{};
        std::array<float,4> row{};
        if (!display.read(reinterpret_cast<char*>(mode.data()),sizeof(mode)) ||
            !display.read(reinterpret_cast<char*>(row.data()),sizeof(row)))
          throw halo2::FormatError("truncated display projection fixture");
        const auto lens=halo2::menu_projection_lens(row[0],{bool(mode[2]),bool(mode[1]),bool(mode[3]),bool(mode[0])});
        const std::array<float,3> actual{lens.render_angle,lens.scale.x,lens.scale.y};
        for (unsigned j=0;j<actual.size();++j)
          maximum_error=std::max(maximum_error,std::abs(actual[j]-row[j+1]));
        ++display_cases;
      }
      if (display.peek()!=std::char_traits<char>::eof()) throw halo2::FormatError("unexpected display projection data");
      if (maximum_error>1e-6f) throw halo2::FormatError("display lens differs from original instructions");
    }
    halo2::Camera ordinary;
    ordinary.field_of_view=75;
    if (ordinary.projection_clip_scale.x!=1 || ordinary.projection_clip_scale.y!=1 ||
        ordinary.projection_vertical_angle ||
        std::abs(halo2::camera_vertical_field_of_view(ordinary,4.f/3.f)-75*.017453292f)>1e-7f)
      throw halo2::FormatError("ordinary camera projection changed");
    const auto world_points=full_frame_projection();
    std::cout << "Three original frontend projections and " << display_cases
              << " display branches; maximum angle/scale error " << maximum_error
              << "; " << world_points << " full-frame world-point checks" << '\n';
    return 0;
  } catch (const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
}
