#include "halo2/world_effects.hpp"
#include "halo2/collision.hpp"
#include "halo2/math.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {constexpr float frame_interval=1.f/30.f;void check(bool value,const char* text){if(!value)throw FormatError(text);}}
int main(int argc,char**argv)try {
 check(argc==5,"Supply MAP SHARED XBE_OR_DASH REPORT");
 if(!std::filesystem::is_regular_file(argv[1]))return 77;
 const auto original=std::string_view(argv[3])=="-"?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(argv[3]);
 Cache cache(argv[1],argv[2],original);const auto content=cache.load_content(0,true);
 const auto graph=cache.query_tag({.group=tag_groups::effect,.name="effects\\impact\\explosion_small\\plasma_grenade\\hard_terrain"});
 check(bool(graph),"Actual authored plasma hard-terrain graph missing");
 const Bytes root(cache.tag_payload(graph->handle()));
 check(std::bit_cast<std::int16_t>(root.u16(4))==-1 && root.u32(0x14)==1,"Authored control graph changed event/loop bank");
 const Bytes event(cache.read_tag_bytes(root.u32(0x18),0x38));
 check(event.f32(4)==0 && event.f32(8)==0 && event.f32(12)==0 &&
       event.f32(16)==frame_interval && event.f32(20)==frame_interval && event.u32(0x30)==0 && event.u32(0x18)==2,
       "Authored control graph is not fixed-clock decal-only");
 const Bytes parts(cache.read_tag_bytes(event.u32(0x1c),2*0x38));
 for(unsigned n=0;n<2;++n) {
  const auto at=n*0x38;
  check(parts.u32(at+8)==0x64656361u && parts.u32(at+0x10)!=null_datum &&
        parts.u16(at)!=2 && !(parts.u16(at+6)&8) && !parts.u32(at+0x30) && !parts.u32(at+0x34) &&
        parts.f32(at+0x28)==parts.f32(at+0x2c),"Authored control part has another owner or radius RNG");
 }
 CollisionWorld collision(content.collision);
 const auto triangle=std::find_if(collision.triangles().begin(),collision.triangles().end(),[](const auto& t){return cross(t.b-t.a,t.c-t.a).z>0.001f;});
 check(triangle!=collision.triangles().end(),"Actual BSP has no real upward-facing contact triangle");
 const Vec3 normal=normalized(cross(triangle->b-triangle->a,triangle->c-triangle->a));
 const auto point=(triangle->a+triangle->b+triangle->c)/3.f;
 const WorldEffectFrame contact{point,normal,normal*-1.f,{1,0,0},{0,0,1},true};
 Mesh mesh;std::uint32_t primary=0x10203040u,auxiliary=0x55667788u;
 NativeWorldEffects effects(cache,mesh,collision,SourceEffectRandom{primary,auxiliary});
 const auto initial=std::array{primary,auxiliary};
 effects.spawn(graph->datum,null_datum,0,contact);effects.advance(0,frame_interval);
 const auto default_vertices=effects.decals(0,0,frame_interval).size();
 check(default_vertices>0 && std::array{primary,auxiliary}!=initial,
       "Default procedural owner did not create actual authored decals and consume their random draws");
 const auto procedural=effects.checkpoint();
 const auto before_takeover=std::array{primary,auxiliary};effects.source_decal_cache(true);
 check(effects.decals(0,0,frame_interval).empty() && effects.diagnostics().decals==0 &&
       std::array{primary,auxiliary}==before_takeover,"Source cache takeover retained prior procedural pages or consumed RNG");
 effects.spawn(graph->datum,null_datum,1,contact);effects.advance(1,frame_interval);
 check(effects.decals(1,0,frame_interval).empty() && std::array{primary,auxiliary}==before_takeover,
       "Delegated deca-only graph created procedural pages or consumed decal part RNG");
 const auto delegated=effects.checkpoint();effects.source_decal_cache(false);
 effects.spawn(graph->datum,null_datum,2,contact);effects.advance(2,frame_interval);
 const auto resumed_vertices=effects.decals(2,0,frame_interval).size();
 check(resumed_vertices>0 && std::array{primary,auxiliary}!=before_takeover,"Releasing source ownership did not resume procedural decals");
 const auto restore=[&](const NativeWorldEffects::Checkpoint& cut) {
  auto prepared=effects.prepare_restore(cut,SourceEffectRandom{primary,auxiliary},{});
  const auto words=cut.random();primary=words[0];auxiliary=words[1];
  check(effects.commit_restore(std::move(prepared)),"Actual effect owner checkpoint did not commit");
 };
 restore(delegated);const auto restored_delegated_random=std::array{primary,auxiliary};
 effects.spawn(graph->datum,null_datum,2,contact);effects.advance(2,frame_interval);
 check(effects.decals(2,0,frame_interval).empty() && std::array{primary,auxiliary}==restored_delegated_random,
       "Checkpoint restore lost delegated source decal ownership or its no-part-RNG behavior");
 restore(procedural);check(!effects.decals(0,0,frame_interval).empty(),"Procedural checkpoint lost its existing pages");
 effects.source_decal_cache(true);check(effects.decals(0,0,frame_interval).empty(),"Restored procedural bank survived source takeover");
 effects.source_decal_cache(false);effects.spawn(graph->datum,null_datum,1,contact);effects.advance(1,frame_interval);
 check(!effects.decals(1,0,frame_interval).empty(),"Procedural owner did not resume after restored takeover");
 nlohmann::json report={{"passed",true},{"graph",graph->name},{"datum",graph->datum},
  {"authored_events",1},{"authored_decal_parts",2},{"default_vertices",default_vertices},{"resumed_vertices",resumed_vertices},
  {"delegated_vertices",0},{"delegated_part_rng_unchanged",true},{"restored_delegation_verified",true},
  {"scope","Actual authored decal-only effect and BSP triangle-derived API contact. Procedural/source ownership, decal-specific RNG, takeover, resume and checkpoint restore. No fabricated blood graph, original projectile result, complete visual RNG parity or GPU acceptance claim."}};
 std::ofstream(argv[4])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
