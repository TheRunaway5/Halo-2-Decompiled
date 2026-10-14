#include "halo2/native_player_animation.hpp"
#include "halo2/frontend_model_locals.hpp"
#include "halo2/frontend_model_ik.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool ok,const std::string &why) {if(!ok)throw FormatError(why);}
bool equal(std::span<const BonePose> a,std::span<const BonePose> b) {
 if(a.size()!=b.size())return false;
 for(unsigned n=0;n<a.size();++n)
  if(a[n].rotation!=b[n].rotation || a[n].scale!=b[n].scale ||
     a[n].translation.x!=b[n].translation.x || a[n].translation.y!=b[n].translation.y ||
     a[n].translation.z!=b[n].translation.z)return false;
 return true;
}
// Original1CA950: installation400 admits jmad+7C,800 admits jmad+5C.
// Resolve graph-order masks into the model-order destination bank by name.
std::vector<bool> hand_mask(const Cache &cache,const RenderModel &model,unsigned hand) {
 const auto tag=cache.load_source_tag_metadata(model.animation);
 const Bytes root(tag.bytes_at(tag.source_address(),0x9c));
 std::vector<bool> mask(model.bones.size());
 for(unsigned i=0;i<root.u32(0xc);++i)if(root.u32((hand?0x5c:0x7c)+(i/32)*4)&(1u<<(i%32))) {
  const auto name=Bytes(tag.bytes_at(root.u32(0x10)+i*32,4)).u32(0);
  for(unsigned n=0;n<model.bones.size();++n)if(model.bones[n].name==name)mask[n]=true;
 }
 return mask;
}
void hand_overlay(const RenderModel &model,std::vector<BonePose> &locals,
    const AnimationClip *clip,const std::vector<bool> &mask,float frame=0) {
 if(!clip)return;
 auto applied=locals;apply_native_player_overlay(model,applied,*clip,frame);
 for(unsigned n=0;n<locals.size();++n)if(mask[n])locals[n]=applied[n];
}
// Grid endpoints are read directly from the authored screen. This bypasses
// production screen-coordinate/interpolation code and catches wrong rows,
// columns and compounded raised/lowered alternatives.
void endpoint_screen(const Cache &cache,const RenderModel &model,std::vector<BonePose> &locals,
    const AnimationClip *clip,int yaw,int pitch,float weight) {
 if(!clip || !weight)return;
 const auto tag=cache.load_source_tag_metadata(clip->source_graph);
 const Bytes root(tag.bytes_at(tag.source_address(),0x34));
 const Bytes def(tag.bytes_at(root.u32(0x30)+clip->source_index*0x6c,0x14));
 const int index=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(def.slice(0x12,1)[0]));
 check(index>=0 && unsigned(index)<root.u32(0x24),"Authored screen missing");
 const Bytes screen(tag.bytes_at(root.u32(0x28)+unsigned(index)*0x1c+4,0x18));
 const auto yn=screen.u16(8),yp=screen.u16(10),pn=screen.u16(20),pp=screen.u16(22);
 const unsigned x=yaw<0?0:yaw>0?yn+yp:yn,y=pitch<0?0:pitch>0?pn+pp:pn;
 apply_native_player_overlay(model,locals,*clip,float(x+(yn+yp+1)*y),weight);
}
std::vector<BonePose> reference(const Cache &cache,const RenderModel &model,
 NativePlayerAnimation &body,const AnimationClip &primary,float frame,bool crouch,bool dual,bool moving,
 const AnimationClip *secondary,float secondary_frame,int yaw,int pitch,float lowered,
 NativePlayerAnimation *left=nullptr,bool allow_aim=true,float aim_weight=1,bool support_grip=true) {
 if(!left)left=&body;
 auto locals=frontend_model_locals(model,primary,frame,{},nullptr,true);
 // Literal113830 order: action channels, two masked dual offsets, weighted
 // aim alternatives, then the grip. A reload must not overwrite the grip.
 if(secondary) {
  if(dual)hand_overlay(model,locals,secondary,hand_mask(cache,model,0),secondary_frame);
  else apply_native_player_overlay(model,locals,*secondary,secondary_frame);
 }
 if(dual) {
  const auto *screen=body.action(0x0b00054au,crouch,true);
  hand_overlay(model,locals,screen,hand_mask(cache,model,0));
  hand_overlay(model,locals,left->action(0x0b00054au,crouch,true),hand_mask(cache,model,1));
 }
 const auto *raised=body.action(moving?0x0b000027u:0x0c000026u,crouch,dual);
 const auto *down=body.action(moving?0x0d000029u:0x0e000028u,crouch,dual);
 if(allow_aim) {
  endpoint_screen(cache,model,locals,raised,yaw,pitch,(down?1-lowered:1)*aim_weight);
  endpoint_screen(cache,model,locals,down,yaw,pitch,(raised?lowered:1)*aim_weight);
 }
 if(const auto *grip=body.action(0x0400060bu,crouch,dual)) {
  if(dual) {
   hand_overlay(model,locals,grip,hand_mask(cache,model,0));
   hand_overlay(model,locals,left->action(0x0400060bu,crouch,true),hand_mask(cache,model,1));
  }else if(support_grip)apply_native_player_overlay(model,locals,*grip,0);
  else hand_overlay(model,locals,grip,hand_mask(cache,model,0));
 }
 return locals;
}
}
int main(int argc,char **argv) {try {
 check(argc==4,"Supply MAP SHARED XBE");
 if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
 Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
 const auto content=cache.load_content(0,true);
 unsigned combinations=0,poses=0,actions=0,mixed_dual=0,full_body=0;std::set<std::string> weapons;
 for(unsigned representation:{2u,3u}) {
  const auto unit=content.player_representations.at(representation).biped.datum;
  const auto model=cache.load_model(unit,false);
  for(const auto &[datum,weapon]:content.weapons) {
   if(weapon.first_person_models.empty() || weapon.first_person_models.front().model==null_datum ||
      weapon.first_person_models.front().animation==null_datum)continue;
   weapons.insert(weapon.name);NativePlayerAnimation body(cache,model.animation,datum);
   for(bool dual:{false,true}) {if(dual&&!weapon.dual_wieldable)continue;
    for(bool crouch:{false,true})for(bool moving:{false,true}) {
     const auto label=std::to_string(representation)+" "+weapon.name+" crouch="+std::to_string(crouch)+
         " dual="+std::to_string(dual)+" moving="+std::to_string(moving);
     const auto *base=body.action(moving?0x0a000014u:0x0400000cu,crouch,dual);
     check(base,"Missing authored base: "+label);
     for(int yaw:{-1,0,1})for(int pitch:{-1,0,1})for(float lowered:{0.f,.5f,1.f}) {
      const auto expected=reference(cache,model,body,*base,0,crouch,dual,moving,nullptr,0,yaw,pitch,lowered);
      const auto actual=body.pose(model,*base,0,float(pitch)*10,crouch,dual,moving,nullptr,0,nullptr,0,float(yaw)*10,lowered);
      check(equal(actual,expected),"Authored stance/dual offset composition mismatch: "+label);
      ++poses;
     }
     for(auto activity:{0x06000006u,0x08000002u,0x05000024u,0x08000025u,0x0e000607u,0x0d000021u,
                       0x0b0005b2u,0x140005b3u}) {
      const auto *action=body.action(activity,crouch,dual);
      if(!action)continue;
      for(float frame:{0.f,float(action->frame_count-1)/2,float(action->frame_count-1)}) {
       if(action->animation_type==0) {
        const auto expected=reference(cache,model,body,*action,frame,crouch,dual,false,nullptr,0,1,-1,0,nullptr,false);
        const auto actual=body.pose(model,*action,frame,-10,crouch,dual,false,nullptr,0,nullptr,0,10,0,nullptr,activity);
        check(equal(actual,expected),"Full-body action retained an unrelated aim screen: "+label+" "+cache.string_id(activity));
        ++full_body;continue;
       }
       const auto expected=reference(cache,model,body,*base,0,crouch,dual,moving,action,frame,1,-1,0);
       const auto actual=body.pose(model,*base,0,-10,crouch,dual,moving,action,frame,nullptr,0,10);
       check(equal(actual,expected),"Action overwrote authored aim/grip order: "+label+" "+cache.string_id(activity));
       ++actions;
      }
     }
     if(dual)for(const auto &[left_datum,left_weapon]:content.weapons)if(left_weapon.dual_wieldable) {
      NativePlayerAnimation left(cache,model.animation,left_datum);
      const auto expected=reference(cache,model,body,*base,0,crouch,true,moving,nullptr,0,0,0,0,&left);
      const auto actual=body.pose(model,*base,0,0,crouch,true,moving,nullptr,0,nullptr,0,0,0,&left);
      check(equal(actual,expected),"Mixed dual weapons lost independent offsets: "+label+" left="+left_weapon.name);
      ++mixed_dual;
     }
     // Bit4 disables aim during authored actions (notably Elite reloads).
     for(float weight:{0.f,.25f,.5f,.75f}) {
      const auto expected=reference(cache,model,body,*base,0,crouch,dual,moving,nullptr,0,1,-1,0,nullptr,true,weight);
      const auto actual=body.pose(model,*base,0,-10,crouch,dual,moving,nullptr,0,nullptr,0,10,0,nullptr,
          moving?0x0a000014u:0x0400000cu,weight);
      check(equal(actual,expected),"Authored aim transition weight lost: "+label);++poses;
     }
     if(!dual) {
      const auto expected=reference(cache,model,body,*base,0,crouch,false,moving,nullptr,0,0,0,0,nullptr,true,1,false);
      const auto actual=body.pose(model,*base,0,0,crouch,false,moving,nullptr,0,nullptr,0,0,0,nullptr,
          moving?0x0a000014u:0x0400000cu,1,false);
      check(equal(actual,expected),"Released support-hand grip ignored its source mask: "+label);++poses;
     }
     ++combinations;
    }
   }
  }
 }
 check(weapons.size()>=15 && combinations>=168,"Held weapon coverage incomplete");
 std::cout<<"PASS weapons="<<weapons.size()<<" species=2 stance/movement/dual combinations="<<combinations
     <<" endpoint/weighted poses="<<poses<<" action poses="<<actions<<" full-body="<<full_body
     <<" mixed dual="<<mixed_dual<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
