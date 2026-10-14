#include "halo2/world_effects.hpp"
#include "halo2/material_effects.hpp"
#include "halo2/projectile_material_response.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/content.hpp"
#include <iostream>
#include <set>
using namespace halo2;
namespace {void check(bool b,const std::string &s){if(!b)throw FormatError(s);}}
int main(int argc,char **argv) {try {
 check(argc==4,"Supply MAP SHARED XBE");
 Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
 const auto content=cache.load_content(0,true);
 const auto globals=cache.query_tag({.group=tag_groups::globals});
 const auto gt=cache.load_source_tag_metadata(globals->datum);
 const Bytes gr(gt.bytes_at(gt.source_address(),0x158));std::vector<std::int16_t> parents;
 for(unsigned n=0;n<gr.u32(0x150);++n)parents.push_back(std::int16_t(Bytes(cache.read_tag_bytes(gr.u32(0x154)+n*0xb4+8,2)).u16(0)));
 unsigned cases=0,draws=0,suppressed=0,default_contacts=0,default_effects=0;
 MuzzleEffectView view{};view.forward={1,0,0};view.up={0,0,1};
 view.width=640;view.height=480;view.orientation={1,0,0,0,1,0,0,0,1};
 for(const auto &[id,w]:content.weapons) {
  if(w.first_person_models.empty() || w.first_person_models.front().model==null_datum ||
      w.first_person_models.front().animation==null_datum || w.projectile==null_datum)continue;
  for(float angle:{1.5707963267948966f,.5235987755982988f}) {
  const auto tag=cache.load_source_tag_metadata(w.projectile);
  const Bytes p(tag.bytes_at(tag.source_address(),0x1a4));
  const auto rows=cache.read_tag_bytes(p.u32(0x1a0),p.u32(0x19c)*0x58);
  auto missing=std::int16_t(-1);
  for(unsigned material=0;material<parents.size();++material)
   if(!source_projectile_material_response(rows,parents,std::int16_t(material))){missing=std::int16_t(material);break;}
  check(missing>=0,"Missing initialized-default material control");
  for(const auto material:{std::int16_t(0),missing}){
  const auto index=source_projectile_material_response(rows,parents,material);std::uint32_t rng=0x12345678;
  const auto response=index?choose_source_projectile_response(rows.subspan(*index*0x58,0x58),
      {1,angle,w.projectile_speed},{1,1.f/65535.f,{},[&]()->std::uint32_t&{return rng;}}):std::int16_t(0);
  const int slot=response>=0&&response<=4?15+response:response==5||response==6?20:-1;
  BulletImpact hit{1,1,0x12340001,id,cache.spawns().front().position,{-1,0,0},{std::sin(angle),std::cos(angle),0},material,true};
  hit.presentation=index?SourceProjectileImpactPresentation{response,1,0,true}:
      source_projectile_default_impact_presentation(hit.normal,hit.incoming*w.projectile_speed,1,0);
  Mesh am,bm;CollisionWorld collision(content.collision);
  NativeWorldEffects actual(cache,am,collision,0x12345678),expected(cache,bm,collision,0x12345678);
  actual.impact(hit);
  // FEB40 explicitly supplies scaleA1/scaleB0 and the selected response's
  // material slot. Its visual requests precede its ordered sound requests.
  const WorldEffectFrame frame{hit.position,hit.normal,hit.incoming,hit.incoming*-1.f,{0,0,1},true,1,0};
  const auto foot=p.u32(0x58);
  if(foot!=null_datum && slot>=0)for(auto effect:source_material_visual_effects(cache,foot,-1,material,slot,0))
   if(effect!=null_datum)expected.spawn(effect,hit.projectile,hit.tick,frame);
  if(p.u32(0x144)!=null_datum)expected.spawn(p.u32(0x144),hit.projectile,hit.tick,frame);
  actual.advance(8,1.f/30.f);expected.advance(8,1.f/30.f);
  const auto a=actual.diagnostics(),b=expected.diagnostics();
  if(!index){++default_contacts;default_effects+=a.effects;}
  std::cout<<w.name<<" response="<<response<<" actual="<<a.systems<<'/'<<a.particles<<'/'<<a.decals
      <<" expected="<<b.systems<<'/'<<b.particles<<'/'<<b.decals<<" incidence="<<angle<<'\n';
  check(a.systems==b.systems && a.particles==b.particles && a.decals==b.decals,
      "Wrong authored material response or effect scales: "+w.name);
  view.position=hit.position-Vec3{3,0,0};
  const auto av=actual.draw(view,.5f,1.f/30.f),bv=expected.draw(view,.5f,1.f/30.f);
  check(av.size()==bv.size(),"Authored contact draw count differs: "+w.name);
  for(unsigned n=0;n<av.size();++n){
   const auto &x=av[n],&y=bv[n];
   check(x.vertex.attributes==y.vertex.attributes && x.vertex.constants==y.vertex.constants &&
       x.model_first==y.model_first && x.model_count==y.model_count,
       "Authored contact particle position/size/color/UV differs: "+w.name);
   check(x.material->pass.name==y.material->pass.name &&
       x.material->vertex.classification==y.material->vertex.classification,
       "Authored contact material differs: "+w.name);
   for(unsigned t=0;t<x.material->bitmaps.size();++t){
    const auto &xb=x.material->bitmaps[t],&yb=y.material->bitmaps[t];
    check(bool(xb)==bool(yb) && (!xb || (xb->datum==yb->datum && xb->index==yb->index)),
        "Authored contact bitmap binding differs: "+w.name);
   }
  }
  draws+=av.size();
  const auto sounds=actual.take_sounds();std::vector<std::uint32_t> material_sounds;
  for(const auto &s:sounds)if(s.kind==WorldEffectSoundKind::Material)material_sounds.push_back(s.sound);
  std::vector<std::uint32_t> wanted;
  if(foot!=null_datum && slot>=0)for(auto sound:source_material_sound_effects(cache,foot,-1,material,slot,0))if(sound!=null_datum)wanted.push_back(sound);
  check(material_sounds==wanted,"Wrong authored contact sound response: "+w.name);++cases;
  // FEB40's approach-speed gate suppresses both the material requests and
  // the projectile's custom impact/explosion graph at a glancing contact.
  Mesh cm;NativeWorldEffects grazing(cache,cm,collision,0x12345678);
  hit.presentation->admitted=false;grazing.impact(hit);grazing.advance(8,1.f/30.f);
  const auto c=grazing.diagnostics();
  check(c.effects==0 && c.systems==0 && c.particles==0 && c.decals==0 && grazing.take_sounds().empty(),
      "Suppressed contact emitted an effect/explosion/sound: "+w.name);++suppressed;
  }
  }
 }
 check(cases>=56 && draws>0 && suppressed==cases && default_contacts>=28 && default_effects>0,"Held projectile coverage incomplete");
 std::cout<<"PASS authored projectile contact effects="<<cases<<" particle draws="<<draws<<" suppressed="<<suppressed
     <<" initialized defaults="<<default_contacts<<" default graph requests="<<default_effects<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
