#include "halo2/multiplayer.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/transform.hpp"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

// CPU-only measurement of the actual runtime's undecorated body/child-weapon
// path. This is a proposed cache comparison, not an actual renderer/FPS test.
// View-specific shader selection, lighting and admission stay with callers.
namespace {
using namespace halo2;
using Clock=std::chrono::steady_clock;
struct Pose {
  std::vector<Transform> nodes;
  std::vector<Vertex> body,gun;
};
struct Input {Transform root;float seconds{};};
struct Models {
  RenderModel body,gun;
  std::optional<AnimationClip> body_clip,gun_clip;
  WeaponAttachmentMarkers attachment;
};
Pose build(const Models &models,const Input &input,bool reuse_nodes) {
  Pose output;
  output.nodes=pose_model_nodes_world(models.body,input.root,
      models.body_clip?&*models.body_clip:nullptr,input.seconds);
  output.body=reuse_nodes?skin_model_world_nodes(models.body,output.nodes):
      pose_model_world(models.body,input.root,models.body_clip?&*models.body_clip:nullptr,input.seconds);
  const auto parent=output.nodes.at(models.attachment.parent.node);
  const auto marker=compose_transform(parent,models.attachment.parent.local);
  const auto aligned=source_object_attachment_frame({{}, {1,0,0}, {0,0,1}},
      marker,models.attachment.child.local,false);
  const auto local=source_object_attachment_local_frame(aligned,parent);
  const auto gun_root=source_object_transform(local.position,local.forward,local.up,1,false,&parent);
  output.gun=pose_model_world(models.gun,gun_root,
      models.gun_clip?&*models.gun_clip:nullptr,input.seconds);
  return output;
}
std::vector<Pose> evaluate(const Models &models,const std::array<Input,4> &inputs,
    unsigned views,unsigned mode) {
  std::array<std::optional<Pose>,4> posed;
  std::vector<Pose> outputs;
  for(unsigned view=0;view<views;++view)for(unsigned player=0;player<4;++player)if(player!=view) {
    if(mode==2 && views>1) {
      if(!posed[player])posed[player]=build(models,inputs[player],true);
      // Every view keeps its mutable copy for shader erase/material selection.
      outputs.push_back(*posed[player]);
    } else outputs.push_back(build(models,inputs[player],mode!=0));
  }
  return outputs;
}
template<class T>void equal(const std::vector<T>&a,const std::vector<T>&b) {
  if(a.size()!=b.size() || std::memcmp(a.data(),b.data(),a.size()*sizeof(T)))
    throw FormatError("Proposed body cache changed a complete output buffer");
}
void equal(const std::vector<Pose>&a,const std::vector<Pose>&b) {
  if(a.size()!=b.size())throw FormatError("Proposed body cache changed draw order");
  for(unsigned i=0;i<a.size();++i){equal(a[i].nodes,b[i].nodes);equal(a[i].body,b[i].body);equal(a[i].gun,b[i].gun);}
}
}
int main(int argc,char **argv) {
  try {
    static_assert(sizeof(halo2::Vertex)==132 && sizeof(halo2::Transform)==52);
    if(argc!=6)throw halo2::FormatError("Supply MAP SHARED UPDATE_XBE FRAMES OUTPUT_TSV");
    const unsigned frames=unsigned(std::stoul(argv[4]));
    if(frames<16)throw halo2::FormatError("Measure at least sixteen frame inputs");
    const halo2::Cache cache(argv[1],argv[2],halo2::OriginalData::load_xbe(argv[3]));
    const auto content=cache.load_content(0,true);
    const auto weapon=cache.slayer_starting_weapon(5);
    auto rules=halo2::slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=weapon;
    halo2::MultiplayerMatch match(content,0,cache.spawns(),rules);
    std::array<halo2::Controls,halo2::multiplayer_capacity> commands{};
    for(unsigned player=0;player<4;++player){const auto id=match.join("Profile "+std::to_string(player));commands[id].yaw=match.unit(id).yaw;}
    Models models;
    models.body=cache.load_model(content.player.datum,true);
    models.gun=cache.load_model(weapon,true);
    models.gun_clip=cache.load_initialized_object_idle(weapon);
    const auto identifiers=cache.load_weapon_animation_identifiers(weapon);
    models.body_clip=cache.load_unit_weapon_action(models.body.animation,0x06000086,
        identifiers[0],identifiers[1],0x0400000c);
    models.attachment=halo2::select_weapon_attachment(cache.load_weapon_attachment(content.player.datum,weapon,0),{},{});
    if(models.attachment.named_child || models.attachment.parent.region!=-1)
      throw halo2::FormatError("Runtime cache slice requires unnamed child and unfiltered original parent marker");
    std::ofstream out(argv[5]);if(!out)throw halo2::FormatError("Open body profiling output");
    out<<"views\tframe\tbaseline_ms\tnodes_reused_ms\tcached_ms\tdraws\tbaseline_body_skeletons\tnodes_reused_body_skeletons\tcached_body_skeletons\tbody_vertices\tgun_vertices\n"<<std::setprecision(12);
    std::size_t compared{};
    for(unsigned frame=0;frame<frames+12;++frame) {
      if(!(frame%2))match.step(commands);
      const auto state=match.state();std::array<Input,4> inputs;
      const float seconds=(float(state.tick)+float(frame%2)*.5f)/30.f;
      for(unsigned player=0;player<4;++player) {
        const auto &p=state.players[player];const auto basis=halo2::authored_basis({p.yaw,0,0});
        inputs[player]={halo2::source_object_transform(p.position,basis.forward,basis.up,1,false),seconds};
      }
      for(unsigned views:{1u,4u}) {
        double baseline_ms{},nodes_ms{},cached_ms{};
        std::vector<Pose> baseline,nodes,cached;
        const auto timed=[&](unsigned mode,std::vector<Pose>&output,double &ms) {
          const auto start=Clock::now();output=evaluate(models,inputs,views,mode);
          ms=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
        };
        // Alternate order to avoid always giving one variant the warm cache.
        if(frame%3==0){timed(0,baseline,baseline_ms);timed(1,nodes,nodes_ms);timed(2,cached,cached_ms);}
        else if(frame%3==1){timed(1,nodes,nodes_ms);timed(2,cached,cached_ms);timed(0,baseline,baseline_ms);}
        else{timed(2,cached,cached_ms);timed(0,baseline,baseline_ms);timed(1,nodes,nodes_ms);}
        equal(baseline,nodes);equal(baseline,cached);compared+=baseline.size()*2;
        if(frame>=12)out<<views<<'\t'<<frame-12<<'\t'<<baseline_ms<<'\t'<<nodes_ms<<'\t'<<cached_ms<<'\t'<<baseline.size()<<'\t'
          <<baseline.size()*2<<'\t'<<baseline.size()<<'\t'<<(views==4?4:3)<<'\t'<<models.body.mesh.vertices.size()<<'\t'<<models.gun.mesh.vertices.size()<<'\n';
      }
    }
    if(!out)throw halo2::FormatError("Write body profiling output");
    std::cout<<"PASS "<<compared<<" complete native body+node+attached-SMG buffers (all bytes/order); four source-created native players.\n"
        <<"LIMIT CPU kernel comparison; no GPU, shader/LOD choice, lighting or actual frame-rate claim.\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
