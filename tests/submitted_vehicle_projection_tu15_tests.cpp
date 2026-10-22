#include "helpers/submitted_vehicle_projection_tu15.hpp"
#include <iostream>
using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
test::SubmittedVehicleProjection project(std::array<Vec3,3> points,float w=1) {
    std::array<ShaderVector,7> bank{};bank[0][0]=bank[1][1]=bank[2][2]=1;bank[3][3]=w;
    std::array<Vertex,3> vertices{};for(unsigned i=0;i<3;++i)vertices[i].position=points[i];
    test::SubmittedVehicleProjection result;result.observe(vertices,bank,{640,360});return result;
}
}
int main(){try {
    require(project({Vec3{-.2f,-.2f,0},Vec3{.2f,-.2f,0},Vec3{0,.2f,0}}).readable(),"On-screen triangle rejected");
    require(!project({Vec3{2,2,0},Vec3{3,2,0},Vec3{2,3,0}}).readable(),"Offscreen draw admitted");
    require(!project({Vec3{-.2f,-.2f,0},Vec3{.2f,-.2f,0},Vec3{0,.2f,0}},-1).readable(),"Behind-camera draw admitted");
    require(!project({Vec3{0,0,0},Vec3{.001f,0,0},Vec3{0,.001f,0}}).readable(),"Subpixel draw admitted");
    require(!project({Vec3{-.2f,0,0},Vec3{0,0,0},Vec3{.2f,0,0}}).readable(),"Degenerate draw admitted");
    require(project({Vec3{-3,-3,0},Vec3{3,-3,0},Vec3{0,3,0}}).readable(),"Viewport-crossing triangle rejected");
    std::cout<<"PASS actual submitted projection clipping and negative display cases\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
