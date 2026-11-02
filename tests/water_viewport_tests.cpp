#include "halo2/water.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/gameplay_fov.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
using namespace halo2;
namespace {
float side(Vec2 a,Vec2 b,Vec2 c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);}
bool contains(Vec2 p,Vec2 a,Vec2 b,Vec2 c){
    const auto x=side(a,b,p),y=side(b,c,p),z=side(c,a,p);
    return (x>=0&&y>=0&&z>=0)||(x<=0&&y<=0&&z<=0);
}
void check(bool value,const char*message){if(!value)throw FormatError(message);}
}
int main(int argc,char**argv)try {
    check(argc==2 || (argc==3 && std::string_view(argv[2])=="--legacy-ui"),"Supply prepared MCC maps [--legacy-ui]");
    const std::filesystem::path maps=argv[1];if(!std::filesystem::exists(maps/"zanzibar.map"))return 77;
    const Cache cache(maps/"zanzibar.map",maps);const auto water=cache.load_bsp_water(0);
    check(!water.definitions.empty(),"Zanzibar infinite water absent");
    const bool legacy=argc==3;unsigned samples{};
    // Include a translated split viewport and a very wide desktop. All use
    // the same short outer bank as the actual source scene projection.
    for(const std::array<std::int16_t,4> outer:{std::array<std::int16_t,4>{0,0,720,1280},
        {0,0,1080,1920},{0,0,1080,2560},{360,0,720,1280}}) {
        const int width=int(outer[3])-outer[1],height=int(outer[2])-outer[0];
        SourceFrustumProjectionInput input;input.outer=outer;
        input.safe={std::int16_t(outer[0]+int(height*.075f)),std::int16_t(outer[1]+int(width*.075f)),
            std::int16_t(outer[2]-int(height*.075f)),std::int16_t(outer[3]-int(width*.075f))};
        input.scenario_present=true;input.scenario_type=cache.load_source_geometry_scenario_type();
        input.projection_angle=gameplay_camera_angles(gameplay_field_of_view(78),gameplay_field_of_view(78),float(width)/height).projection_angle;
        const auto projection=source_frustum_projection(input);
        for(float pitch:{-.15f,0.f,.03f}) {
            const Vec3 eye{-10,28,1},forward{0,std::cos(pitch),std::sin(pitch)},up{0,-std::sin(pitch),std::cos(pitch)};
            const auto camera=source_view_projection({eye,forward,up,.015625f,1024.f,{},projection.projection_program});
            const auto constants=source_view_shader_constants(camera,1024.f,false);
            for(const auto&definition:water.definitions) {
                const auto state=legacy?water_frame(definition,eye,forward,input.projection_angle,4.f/3.f,true):
                    water_frame(definition,eye,forward,input.projection_angle,outer,true);
                const auto reference=water_frame(definition,eye,forward,input.projection_angle,float(width)/height,true);
                if(!legacy)check(state.local_to_world==reference.local_to_world && state.constants==reference.constants &&
                    state.bump_matrices==reference.bump_matrices,"Camera rectangle changed original water-state math");
                std::vector<std::optional<Vec2>> points;
                for(const auto&v:definition.geometry.vertices){const auto p=water_world_position(state,v.position);ShaderVector clip{};
                    for(unsigned n=0;n<4;++n)clip[n]=((constants[n][0]*p.x+constants[n][1]*p.y)+constants[n][2]*p.z)+constants[n][3];
                    points.push_back(clip[3]>0?std::optional<Vec2>{{clip[0]/clip[3],clip[1]/clip[3]}}:std::nullopt);
                }
                for(float y:{-.2f,-.4f})for(float x:{-.999f,-.98f,0.f,.98f,.999f}) {
                    bool covered{};for(std::size_t i=0;i+2<points.size();i+=3)
                        if(points[i]&&points[i+1]&&points[i+2]&&contains({x,y},*points[i],*points[i+1],*points[i+2])){covered=true;break;}
                    if(!covered){std::cerr<<"Water edge uncovered: "<<width<<'x'<<height<<" pitch="<<pitch<<" ndc="<<x<<','<<y<<'\n';return 1;}
                    ++samples;
                }
            }
        }
    }
    bool rejected{};try{(void)water_frame(water.definitions.front(),{},{0,1,0},1.f,std::array<std::int16_t,4>{0,0,0,1280},true);}
    catch(const FormatError&){rejected=true;}check(rejected,"Empty physical viewport was admitted");
    std::cout<<"PASS submitting camera water coverage: "<<samples<<" projected samples\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
