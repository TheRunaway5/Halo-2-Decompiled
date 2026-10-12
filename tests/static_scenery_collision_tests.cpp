#include "halo2/multiplayer.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_object_posed_target.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/math.hpp"
#include "halo2/static_scenery_collision.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
int main(int argc,char**argv){try{
  if(argc!=3&&argc!=4)throw FormatError("Supply COLD_NODE_FIXTURE UPDATE_XBE [baseline]");
  if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
  const bool baseline=argc==4&&std::string_view(argv[3])=="baseline";
  std::ifstream file(argv[1]);std::string schema;unsigned count;file>>schema>>count;
  if(schema!="H2_STATIC_SCENERY_V1"||count!=14)throw FormatError("Invalid original static scenery fixture");
  const auto original=OriginalData::load_xbe(argv[2]);unsigned checked{},missing{},placements{},mover_contacts{},lifetime_checks{};
  for(unsigned row=0;row<count;++row){
    std::string path;std::size_t ordinal;std::uint32_t words[13];file>>path>>std::dec>>ordinal;for(auto&v:words)file>>std::hex>>v;
    if(!file)throw FormatError("Truncated original static scenery fixture");
    Cache cache(path,"assets/disc/maps",original);auto content=cache.load_content(0,true);
    const auto& authored=content.objects.at(ordinal);
    if(authored.type!=6)throw FormatError("Original fixture placement is not scenery");
    const auto model=cache.load_model(authored.definition,false,authored.variant);
    const auto selection=cache.load_initial_model_selection(authored.definition,authored.variant);
    const auto collision=cache.load_model_collision(cache.load_initial_model_definition(authored.definition).model);
    const auto basis=authored_basis(authored.rotation);
    const auto nodes=pose_model_nodes_world(model,source_object_transform(authored.position,basis.forward,basis.up,authored.scale,false));
    if(nodes.size()!=1||std::memcmp(nodes.data(),words,sizeof words))
      throw FormatError("Native static WORLDnode0 differs from unchanged cold constructor: "+path+"/"+std::to_string(ordinal));
    SlayerRules rules;rules.weapon=cache.slayer_starting_weapon(5);
    MultiplayerMatch match(content,0,cache.spawns(),rules);
    bool present{};for(const auto&o:match.simulation().scene().objects())if(o.handle!=null_datum&&o.placement==ordinal)present=true;
    if(!present)throw FormatError("Actual match scene did not instantiate the original static placement");
    unsigned placement_checks{};
    for(const auto&region:collision.regions)for(const auto&permutation:region)for(const auto&bsp:permutation)
      for(unsigned surface=0;surface<bsp.surfaces.size();++surface){
        const auto&s=bsp.surfaces[surface];if(s.flags&16)continue;
        Vec3 center{};unsigned vertices{};auto edge=s.first_edge;
        do{const auto&e=bsp.edges.at(edge);const unsigned side=e.surfaces[0]==surface?0:1;
          if(e.surfaces[side]!=surface||vertices>bsp.edges.size())throw FormatError("Invalid original surface cycle");
          Transform local;local.position=bsp.vertices.at(e.vertices[side]);center=center+compose_transform(nodes.at(unsigned(bsp.node)),local).position;
          ++vertices;edge=e.next[side];
        }while(edge!=s.first_edge);
        center=center/float(vertices);
        const auto plane=source_collision_world_plane(&nodes.at(unsigned(bsp.node)),bsp.planes.at(std::uint16_t(s.plane)&0x7fff),std::uint16_t(s.plane)&0x8000);
        const auto normal=normalized(plane.normal);const auto origin=center+normal*.025f,delta=normal*-.05f;
        const auto expected=source_object_posed_target(collision,nodes,std::span<const std::uint8_t>(selection.permutations),origin,delta,9);
        if(!expected)continue;
        const auto distance=length(delta),expected_distance=expected->fraction*distance;
        const CollisionWorld bsp_world(content.collision);const auto prior=bsp_world.raycast(origin,delta,distance);
        if(prior&&prior->distance<=expected_distance+.00001f)continue;
        const auto actual=match.simulation().collision().raycast(origin,delta,distance);
        ++checked;++placement_checks;
        if(!actual||std::abs(actual->distance-expected_distance)>.00001f){++missing;
          if(!baseline)throw FormatError("Actual match omitted authored scenery collision: "+path+"/"+std::to_string(ordinal));}
        if(!baseline&&normal.z>.9f){
          Body body;body.position=center+Vec3{0,0,.03f};body.velocity={0,0,-3};
          body.height=content.player.height;body.radius=content.player.radius;
          body.maximum_slope_cosine=content.player.maximum_slope_cosine;
          match.simulation().collision().move(body,tick_seconds);
          if(body.support_triangle&&match.simulation().collision().triangles()[*body.support_triangle].source.instance==int(ordinal))
            ++mover_contacts;
        }
      }
    if(!placement_checks)throw FormatError("No unobstructed original scenery query exercised placement");
    if(!baseline&&authored.name>=0){
      SceneObjects scene(content,0,SourceSceneGameContext{2,2});StaticSceneryCollision fixed(content);
      if(!fixed.synchronize(scene)||fixed.triangles().empty())throw FormatError("Initial fixed collision owner missing");
      const auto snapshot=scene;const auto before=fixed.triangles().size();const auto handle=scene.resolve(authored.name);
      scene.destroy(handle);if(!fixed.synchronize(scene)||fixed.triangles().size()>=before)
        throw FormatError("Destroyed scenery retained its collision");
      const auto replacement=scene.create(authored.name,false,99);
      if(replacement==handle||scene.get(replacement).created_tick!=99||!fixed.synchronize(scene)||fixed.triangles().size()!=before)
        throw FormatError("Recreated scenery lost its new generation or collision");
      scene.get(replacement).bsp_hidden=true;
      if(!fixed.synchronize(scene)||fixed.triangles().size()>=before)throw FormatError("BSP-hidden scenery retained collision");
      scene.restore(snapshot);
      if(!fixed.synchronize(scene)||fixed.triangles().size()!=before||scene.resolve(authored.name)!=handle)
        throw FormatError("Scene restoration did not rebuild fixed collision ownership");
      ++lifetime_checks;
    }
    ++placements;
  }
  if(baseline&&missing==0)throw FormatError("Baseline did not reproduce missing scenery collision");
  if(!baseline&&(!mover_contacts||!lifetime_checks))throw FormatError("Fixed scenery mover/lifetime checks were not exercised");
  std::cout<<(baseline?"BASELINE_MISSING ":"PASS ")<<missing<<" missing of "<<checked<<" original posed-surface queries, "<<placements<<" live scene placements; all WORLDnode0 bytes equal original constructors\n";
  std::cout<<"native capsule support contacts "<<mover_contacts<<" destroy/recreate/BSP/restore owner checks "<<lifetime_checks<<'\n';
  std::cout<<"LIMIT fixed nonanimated scenery and existing native triangle backend; not dynamic object/Havok/vehicle parity\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
