#include "halo2/content.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/model.hpp"
#include "halo2/first_person_dual.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <cmath>
#include <iostream>
#include <set>
using namespace halo2;
int main(int argc,char **argv) {try {
  if(argc!=2)throw FormatError("Supply a multiplayer map");
  if(!std::filesystem::exists(argv[1]))return 77;
  Cache cache(argv[1]);const auto content=cache.load_content(0,true);unsigned weapons=0,actions=0,poses=0,dual_poses=0;
  for(const auto &[datum,w]:content.weapons)if(!w.first_person_models.empty() && w.first_person_models.front().model!=null_datum && w.first_person_models.front().animation!=null_datum) {
    cache.load_model(datum,false);
    for(unsigned representation:{2u,3u}) {
      const auto binding=cache.load_first_person_binding(datum,representation);
      const auto model=cache.load_model(binding.weapon_model,false);
      const auto arms=cache.load_model(binding.arms_model,false);
      if(!binding.record_present || binding.idle_index==null_datum)throw FormatError("Missing authored idle binding: "+w.name);
      for(bool dual:{false,true}) {
      FirstPersonViewBinding view;FirstPersonBindingInput input{1,2,int(representation),{3,dual?4u:null_datum}};
      observe_first_person_binding(view,input,[&](auto,auto){return &binding;});
      if(view.hands[0].weapon!=3 || view.hands[1].weapon!=(dual?4u:null_datum))throw FormatError("Both authored hand maps must resolve: "+w.name);
      if(dual && w.dual_wieldable) {
        if(view.hands[0].channel_options!=0x103f || view.hands[1].channel_options!=0x203f)
          throw FormatError("Original dual event ownership changed: "+w.name);
        for(unsigned n=0;n<view.hands[1].arms_node_count;++n) {
          const auto node=view.hands[1].arms_nodes[n];
          if(node>=0 && !(binding.graph_node_flags.at(node)&0x20u))
            throw FormatError("Secondary binding retained an original non-left arm node: "+w.name);
        }
      }
      const auto clips=first_person_clips(view.hands[0],binding);
      for(const auto &clip:clips) {
        const auto animation=cache.load_animation(binding.graph,clip.index);
        for(float frame:{0.f,float(animation.frame_count-1)*.5f,float(animation.frame_count-1)}) {
          const auto weapon_pose=pose_model_frame(model,&animation,frame,&arms);
          const auto arm_pose=pose_model_frame(arms,&animation,frame,&model);
          if(weapon_pose.empty() || arm_pose.empty())throw FormatError("Authored action has no posed geometry: "+w.name);
          for(const auto *vertices:{&weapon_pose,&arm_pose})for(const auto &v:*vertices)
            if(!std::isfinite(v.position.x) || !std::isfinite(v.position.y) || !std::isfinite(v.position.z))
              throw FormatError("Authored action produced nonfinite "+std::string(vertices==&weapon_pose?"weapon":"arm")+
                  " geometry: "+w.name+" clip="+animation.name+" frame="+std::to_string(frame));
          if(dual && w.dual_wieldable) {
            if(first_person_dual_root(animation,model.bones.at(0).name).node<0)
              throw FormatError("Actual dual weapon has no original offhand root: "+w.name);
            const FirstPersonIkDefinition ik;
            const auto primary=pose_first_person_models_world_frame(model,arms,animation,frame,
                w.first_person_offset,Transform{},ik,0);
            const auto secondary=pose_first_person_models_world_frame(model,arms,animation,frame,
                w.first_person_offset,Transform{},ik,0,true,&primary.camera_adjustment);
            for(const auto *vertices:{&secondary.weapon,&secondary.arms})for(const auto &v:*vertices)
              if(!std::isfinite(v.position.x) || !std::isfinite(v.position.y) || !std::isfinite(v.position.z))
                throw FormatError("Original offhand hierarchy produced nonfinite geometry: "+w.name);
            ++dual_poses;
          }
          ++poses;
        }
        first_person_activity(view.hands[0],binding,clip.activity);
        for(int tick=0;tick<clip.frames+3;++tick)first_person_animation_clock(view.hands[0],binding,1.f/30.f);
        if(view.hands[0].activity!=0x0400000c)throw FormatError("Action did not return to idle: "+w.name);
        ++actions;
      }
      std::cout<<w.name<<" representation="<<representation<<" actions="<<clips.size()<<" dual="<<dual<<'\n';
      }
    }
    ++weapons;
  }
  if(!weapons || !actions)throw FormatError("No weapon animations tested");
  std::cout<<"PASS "<<weapons<<" weapon definitions, "<<actions<<" authored actions, "<<poses<<" finite weapon/arm poses, "<<dual_poses<<" actual offhand poses and both hand bindings\n";
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;} }
