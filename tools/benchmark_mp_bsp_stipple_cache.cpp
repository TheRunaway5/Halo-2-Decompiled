#include "halo2/multiplayer.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_bsp_shader_scene.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/hud.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include "bsp_shader_scene_reference.hpp"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

// Compare the complete existing selection result with the isolated candidate,
// using real Lockout data and the same native camera/admission producers.
int main(int argc,char **argv) {
  using namespace halo2;
  try {
    if(argc!=5)throw FormatError("Supply MAP SHARED UPDATE_XBE OUTPUT_TSV");
    std::array<SourceShaderStipple,256> memo;
    for(unsigned alpha=0;alpha<256;++alpha)memo[alpha]=source_shader_instance_stipple(std::uint8_t(alpha));
    for(unsigned repeat=0;repeat<3;++repeat)for(unsigned alpha=0;alpha<256;++alpha) {
      const auto direct=source_shader_instance_stipple(std::uint8_t(alpha));
      if(direct.level!=memo[alpha].level || direct.rows!=memo[alpha].rows)throw FormatError("Stipple cache changed complete source-helper output");
    }
    const Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    const auto content=cache.load_content(0,true);
    auto mesh=cache.load_bsp(0),candidate_mesh=mesh;
    SourceBspShaderSceneReference baseline(cache,mesh,0);
    SourceBspShaderScene candidate(cache,candidate_mesh,0);
    if(mesh.materials.size()!=candidate_mesh.materials.size())throw FormatError("Stipple cache changed resident material slots");
    const auto portals=cache.load_source_portal_bsp(0);
    const auto geometry=cache.load_source_bsp_admission(0);
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    SourcePortalMaskState mask;mask.initialize(cache.load_source_portal_mask_definition(0),{});
    auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=cache.slayer_starting_weapon(5);
    MultiplayerMatch match(content,0,cache.spawns(),rules);
    std::array<Controls,multiplayer_capacity> controls{};
    for(unsigned player=0;player<4;++player){const auto id=match.join("Profile "+std::to_string(player));controls[id].yaw=match.unit(id).yaw;}
    std::ofstream out(argv[4]);if(!out)throw FormatError("Open BSP cache comparison output");
    out<<"case\tviews\tview\tbaseline_ms\tcandidate_ms\tinput_vertices\toutput_vertices\tparts\tclusters\tinstances\n"<<std::setprecision(12);
    std::size_t compared{};
    for(unsigned test=0;test<16;++test) {
      match.step(controls);const auto state=match.state();
      for(unsigned views:{1u,4u})for(unsigned view=0;view<views;++view) {
        const auto &p=state.players[view];const auto forward=direction(p.yaw,p.pitch);
        const auto aim=source_observer_aim(forward,cross(forward,{-std::sin(p.yaw),std::cos(p.yaw),0}),content.player.camera_field_of_view,content.player_screen_offset);
        const auto position=ordinary_unit_eye(p.position,content.player.camera_height,content.player.crouch_camera_height,p.crouch);
        const auto grid=source_view_grid(views,view,true);
        const auto angles=source_camera_angles(content.player.camera_field_of_view,1.3333333730697632f,grid.grid,grid.span);
        const auto viewport=hud_viewport(views,view,true);
        const auto rectangle=[](UiRect r){return std::array<std::int16_t,4>{std::int16_t(r.top*1.5f),std::int16_t(r.left*2.f),std::int16_t(r.bottom*1.5f),std::int16_t(r.right*2.f)};};
        SourceFrustumProjectionInput projection_input;
        projection_input.projection_angle=angles.projection_angle;
        projection_input.outer=rectangle(viewport.outer);projection_input.safe=rectangle(viewport.safe);
        projection_input.scenario_present=true;projection_input.scenario_type=cache.load_source_geometry_scenario_type();
        const auto projection=source_frustum_projection(projection_input);
        const auto camera=source_view_projection({position,aim.forward,aim.up,.015625f,1024.f,{},projection.projection_program});
        const auto frustum=source_frustum_camera({position,aim.forward,aim.up,.015625f,1024.f,projection.clipping_rectangle()});
        const auto fog=atmosphere.sample(position,aim.forward);
        const auto visibility=source_portal_visibility(portals,std::span{&frustum,1},fog.cluster,mask.words);
        const auto fog_admission=source_bsp_fog_admission(atmosphere,{true,true,fog.planar_datum,0});
        SourceBspAdmissionInput admission_input;admission_input.containing_fog=fog_admission;
        const auto admission=source_bsp_admission(geometry,visibility,admission_input);
        const SourceBspShaderView input{camera,admission,projection_input.outer[0],projection_input.outer[2],views,projection.view_modifier};
        auto before=mesh.vertices,after=candidate_mesh.vertices;
        using Clock=std::chrono::steady_clock;
        const auto start=Clock::now();const auto before_parts=baseline.select(before,input);
        const auto mid=Clock::now();const auto after_parts=candidate.select(after,input);
        const auto end=Clock::now();
        if(before.size()!=after.size() || std::memcmp(before.data(),after.data(),before.size()*sizeof(Vertex)))
          throw FormatError("Stipple cache changed complete selected vertex output");
        if(before_parts.size()!=after_parts.size())throw FormatError("Stipple cache changed selected part count");
        for(unsigned i=0;i<before_parts.size();++i) {
          const auto &a=before_parts[i],&b=after_parts[i];
          if(std::tie(a.geometry,a.section,a.part,a.shader_lod,a.material,a.coverage_byte,a.instance_alpha)!=
              std::tie(b.geometry,b.section,b.part,b.shader_lod,b.material,b.coverage_byte,b.instance_alpha))
            throw FormatError("Stipple cache changed complete ordered part output");
        }
        out<<test<<'\t'<<views<<'\t'<<view<<'\t'<<std::chrono::duration<double,std::milli>(mid-start).count()<<'\t'
          <<std::chrono::duration<double,std::milli>(end-mid).count()<<'\t'<<mesh.vertices.size()<<'\t'<<after.size()<<'\t'
          <<after_parts.size()<<'\t'<<admission.clusters.size()<<'\t'<<admission.instances.size()<<'\n';++compared;
        if(test==15 && views==4 && view==3) {
          std::ofstream fixture(std::string(argv[4])+".vertices.bin",std::ios::binary);
          fixture.write(reinterpret_cast<const char*>(after.data()),std::streamsize(after.size()*sizeof(Vertex)));
          if(!fixture)throw FormatError("Write actual selected BSP grouping fixture");
        }
      }
    }
    if(!out)throw FormatError("Write BSP cache comparison");
    std::cout<<"PASS all256 complete stipple levels/32rowpatterns and "<<compared<<" complete native BSP vertex/orderedpart outputs.\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
