#include "halo2/native_grenade.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/simulation.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/math.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/projectile_material_response.hpp"
#include "world_continuation_bindings.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <set>

namespace halo2 {
namespace {
template<class T>T get(std::span<const std::byte> b,std::size_t p){
  if(p>b.size()||sizeof(T)>b.size()-p)throw FormatError("Truncated grenade state");
  T v;std::memcpy(&v,b.data()+p,sizeof v);return v;}
template<class T>void put(std::span<std::byte> b,std::size_t p,T v){
  if(p>b.size()||sizeof(T)>b.size()-p)throw FormatError("Truncated grenade state");
  std::memcpy(b.data()+p,&v,sizeof v);}
Bytes root(const SourceTagMetadata&t){return Bytes(t.bytes_at(t.source_address(),t.size()));}
float add(float a,float b){volatile float r=a+b;return r;}
float mul(float a,float b){volatile float r=a*b;return r;}
float sub(float a,float b){volatile float r=a-b;return r;}
float div(float a,float b){volatile float r=a/b;return r;}
Vec3 source_cross(Vec3 a,Vec3 b){return {sub(mul(a.y,b.z),mul(a.z,b.y)),
  sub(mul(a.z,b.x),mul(a.x,b.z)),sub(mul(a.x,b.y),mul(a.y,b.x))};}
Vec3 source_scale(Vec3 a,float b){return {mul(a.x,b),mul(a.y,b),mul(a.z,b)};}
Vec3 source_add(Vec3 a,Vec3 b){return {add(a.x,b.x),add(a.y,b.y),add(a.z,b.z)};}
float squared(Vec3 v){return add(add(mul(v.x,v.x),mul(v.y,v.y)),mul(v.z,v.z));}
Vec3 creation_up(Vec3 aim,float one,float epsilon){
  // Complete11E720/30BD0 direction arithmetic used by E8D60's request patch.
  const auto x=std::abs(aim.x),y=std::abs(aim.y),z=std::abs(aim.z);
  Vec3 up=x<=y&&x<=z?Vec3{0,aim.z,sub(0,aim.y)}:
      y<=z?Vec3{sub(0,aim.z),0,aim.x}:Vec3{aim.y,sub(0,aim.x),0};
  const double xy=double(up.x)*double(up.x)+double(up.y)*double(up.y);
  const double length=std::sqrt(xy+double(up.z)*double(up.z));
  if(!(double(epsilon)>std::abs(length)))up=source_scale(up,div(one,float(length)));
  return up;
}
std::uint8_t release_ticks(const AnimationClip&clip){
  // 1CA8D0 first timing type0 -> seconds, then148160 nearest-even to low8.
  for(const auto&e:clip.frame_events)if(e.type==0){
    const float seconds=mul(float(e.frame),tick_seconds);
    const float ticks=mul(30.f,seconds);
    if(!std::isfinite(ticks)||ticks<0||ticks>127)throw FormatError("Unsupported signed grenade release duration");
    return std::uint8_t(std::lrint(ticks));}
  throw FormatError("Grenade clip has no original release event");
}
}
NativeGrenadeAction::NativeGrenadeAction(){initialize_source_grenade_action(source);}
std::uint32_t NativeGrenadeAction::active_actions()const{return get<std::uint32_t>(source,4);}
std::uint8_t NativeGrenadeAction::phase()const{return get<std::uint8_t>(source,0xc);}
std::uint8_t NativeGrenadeAction::counter()const{return get<std::uint8_t>(source,0xe);}
std::uint32_t NativeGrenadeAction::held()const{return get<std::uint32_t>(source,0x10);}
void advance_native_grenade_timer(NativeGrenadeProjectile&p,std::int16_t start,float one){
  p.age=std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(p.age)+1));
  p.arming=add(p.arming,p.arming_step);
  const bool ready=start==1?(p.flags&4):start==2?(p.flags&16):start==3?(p.flags&0x800):true;
  if((p.flags&0x28)||ready){p.flags|=0x20;p.timer=add(p.timer,p.timer_step);if(p.timer>=one&&p.state<1)p.state=1;}
}
struct NativeGrenadeSandbox::Definition {
  struct Grenade {std::uint32_t datum,effect,detonation_effect;SourceTagMetadata tag;
    std::shared_ptr<const PropAreaDamageDefinition> damage;};
  std::vector<Grenade> grenades;
  SourceTagMetadata globals;
  Vec3 offset,up;
  float one{},frame_subtractor{},up_threshold{},rest_squared{};
  std::vector<std::int16_t> parents;
  explicit Definition(SourceTagMetadata g):globals(std::move(g)){}
};
struct NativeGrenadeSandbox::UnitDefinition {
  RenderModel model;
  std::vector<ObjectAttachmentMarker> marker;
  std::vector<std::uint8_t> permutations;
  float speed{};
};
struct NativeGrenadeSandbox::WeaponClips {AnimationClip ordinary;std::optional<AnimationClip> preferred;};
NativeGrenadeSandbox::NativeGrenadeSandbox(std::shared_ptr<const Cache> cache,
    std::uint32_t unit,NativeGrenadeConstants constants):initial_unit_(unit),constants_(constants),cache_(std::move(cache)){
  if(!cache_)throw FormatError("Grenade adapter requires a retained Cache");
  const auto original=cache_->original_data();
  if(!original||!original->unit_animation_constants||!original->sound_driver_startup_operands)
    throw FormatError("Grenade adapter requires selected executable operands");
  const auto it=cache_->query_tag({.group=tag_groups::globals});
  if(!it)throw FormatError("Missing grenade globals");
  auto d=std::make_shared<Definition>(cache_->load_source_tag_metadata(it->datum));
  d->one=std::bit_cast<float>(original->unit_animation_constants->one_bits);
  d->frame_subtractor=std::bit_cast<float>(original->unit_animation_constants->frame_subtractor_bits);
  d->up_threshold=std::bit_cast<float>(constants.contact_up_threshold_bits);
  d->rest_squared=std::bit_cast<float>(constants.rest_speed_squared_bits);
  std::memcpy(&d->up,original->sound_driver_startup_operands->basis_up.data(),12);
  const auto b=root(d->globals);const auto info=b.u32(0x134);
  const Bytes p(d->globals.bytes_at(info,0x6c));d->offset={p.f32(0x60),p.f32(0x64),p.f32(0x68)};
  const auto count=b.u32(0x100);if(count>2)throw FormatError("Unsupported grenade definition bank");
  const Bytes rows(d->globals.bytes_at(b.u32(0x104),std::size_t(count)*0x2c));
  for(unsigned i=0;i<count;++i){const auto at=i*0x2c,datum=rows.u32(at+0x28);
    if(datum==null_datum)throw FormatError("Missing authored grenade projectile");
    auto tag=cache_->load_source_tag_metadata(datum);if(tag.group()!="proj")throw FormatError("Invalid grenade projectile group");
    const auto q=root(tag);if(q.f32(0xd4)!=q.f32(0xd8))throw FormatError("Grenade timer requires original RNG range owner");
    const auto damage=q.u32(0x104);
    auto area=damage==null_datum?nullptr:load_prop_area_damage_definition(*cache_,damage);
    d->grenades.push_back({datum,rows.u32(at+8),q.u32(0xfc),std::move(tag),std::move(area)});
  }
  const auto mn=b.u32(0x150);if(mn>65536)throw FormatError("Excessive grenade material bank");
  const Bytes materials(d->globals.bytes_at(b.u32(0x154),std::size_t(mn)*0xb4));
  for(unsigned i=0;i<mn;++i)d->parents.push_back(std::bit_cast<std::int16_t>(materials.u16(i*0xb4+8)));
  definition_=std::move(d);
  unit_definition(unit);
  binding_=world_continuation::grenade_binding(*cache_,unit,constants);
}
const NativeGrenadeSandbox::UnitDefinition &NativeGrenadeSandbox::unit_definition(std::uint32_t unit) {
  if(const auto i=units_.find(unit);i!=units_.end())return *i->second;
  auto d=std::make_shared<UnitDefinition>();
  const auto unit_tag=cache_->load_source_tag_metadata(unit);d->speed=root(unit_tag).f32(0x1b0);
  const auto modeldef=cache_->load_initial_model_definition(unit);
  d->model=cache_->load_model(unit,false);const auto selection=cache_->load_initial_model_selection(unit);
  if(!selection.resolved())throw FormatError("Grenade marker needs actual model permutation owner");
  d->permutations=selection.permutations;
  d->marker=source_model_markers(cache_->load_source_tag_metadata(modeldef.render_model),0x09000100);
  if(d->marker.empty())throw FormatError("Player has no authored grenade marker");
  return *units_.emplace(unit,std::move(d)).first->second;
}
std::shared_ptr<const NativeGrenadeSandbox::WeaponClips> NativeGrenadeSandbox::weapon_clips(
    std::uint32_t unit,std::uint32_t weapon){
  const auto key=std::pair{unit,weapon};
  if(const auto i=clips_.find(key);i!=clips_.end())return i->second;
  const auto &body=unit_definition(unit);
  const auto ids=cache_->load_weapon_animation_identifiers(weapon);
  auto c=std::make_shared<WeaponClips>();
  c->ordinary=cache_->load_unit_weapon_action(body.model.animation,0x06000086,ids[0],ids[1],0x0d000021,true);
  try{c->preferred=cache_->load_unit_weapon_action(body.model.animation,0x06000086,ids[0],ids[1],0x1000006c,true);}
  catch(const FormatError&){/* E95E0 explicitly falls back to the ordinary channel. */}
  release_ticks(c->ordinary);clips_[key]=c;return c;
}
const AnimationClip* NativeGrenadeSandbox::action_clip(std::uint32_t graph,std::uint32_t clip)const{
  NativeAuthorityBoundary::Call call(authority_);
  for(const auto&[_,c]:clips_){if(c->ordinary.source_graph==graph&&c->ordinary.source_index==clip)return &c->ordinary;
    if(c->preferred&&c->preferred->source_graph==graph&&c->preferred->source_index==clip)return &*c->preferred;}
  if(graph==null_datum||clip==null_datum)return nullptr;
  const auto key=std::pair{graph,clip};
  if(const auto i=selected_clips_.find(key);i!=selected_clips_.end())return &i->second;
  // A native replica carries the authoritative selected graph/index, not a
  // request for reselection. Decoding real Cache backing is its residency.
  ++authority_.mutations;return &selected_clips_.emplace(key,cache_->load_animation(graph,clip)).first->second;
}
void NativeGrenadeSandbox::update_unit(Unit&u,bool pressed,int owner,const Allocate&allocate,const Activity&activity,
    const Constructed&constructed){
  NativeAuthorityBoundary::Call call(authority_);++authority_.mutations;
  auto&s=u.grenade_action;
  if(pressed&&!u.vitality.dead&&u.source_parent==null_datum&&!(u.source_unit_10a&4)&&
      !(u.source_action_word&0x400001u)&&u.grenade_current>=0&&
      std::size_t(u.grenade_current)<definition_->grenades.size()&&u.grenades[unsigned(u.grenade_current)]>0){
    const auto w=cache_->load_source_tag_metadata(u.weapon.definition);const auto wb=root(w);
    //1055F0 blocks authored flag6 in signed weapon-state5..10.
    const auto ws=std::bit_cast<std::int8_t>(std::uint8_t(u.weapon.smg.weapon_state));
    if(!((wb.u32(0x12c)&0x40u)&&ws>=5&&ws<=10)){
      const auto clips=weapon_clips(u.definition.datum,u.weapon.definition);
      const bool prefer=(u.weapon.smg.own_flags&2)&&!(u.weapon.smg.own_flags&4)&&clips->preferred.has_value();
      const auto&clip=prefer?*clips->preferred:clips->ordinary;
      s.graph=clip.source_graph;s.clip=clip.source_index;s.activity=prefer?0x1000006c:0x0d000021;
      s.frame=0;s.animation_active=true;
      source_grenade_begin_channel_prefix(s.source,0,release_ticks(clip));
      source_zoom_reset(u.source_zoom);++u.zoom_reset_serial;
      if(activity)activity(u,s.activity);
      const auto&g=definition_->grenades[unsigned(u.grenade_current)];
      events_.push_back({NativeGrenadeEvent::Begin,u.source_datum,null_datum,g.datum,g.effect,u.body.position,{},s.activity,s.graph,s.clip,0,1});
      u.source_action_word|=0x400000u;
    }
  }
  if(!(u.source_action_word&0x400000u))return;
  if(const auto*clip=action_clip(s.graph,s.clip)){
    const float end=sub(float(clip->frame_count),definition_->frame_subtractor);
    s.frame=std::min(add(s.frame,mul(mul(tick_seconds,definition_->one),30.f)),end);
    s.animation_active=s.frame<end;
  }else throw FormatError("Retained grenade animation clip disappeared");
  const auto next=source_grenade_update(s.source);
  if(next.callback==GrenadeCallback::prepare_projectile)prepare(u,owner,allocate,constructed);
  else if(next.callback==GrenadeCallback::release_projectile)release(u);
  else if(next.callback==GrenadeCallback::animation_query){
    auto reply=source_grenade_animation_reply(s.source,0x0d000021,s.animation_active&&s.activity==0x0d000021);
    if(reply.callback==GrenadeCallback::animation_query)
      reply=source_grenade_animation_reply(s.source,0x1000006c,s.animation_active&&s.activity==0x1000006c);
    if(reply.returned==false)u.source_action_word&=~0x400000u;
  }
  // Attachment consumes the same actual decoded body pose as rendering.
  if(s.held()!=null_datum){
    const auto &body=unit_definition(u.definition.datum);
    const auto clip=action_clip(s.graph,s.clip);
    const auto basis=authored_basis({u.yaw,0,0});
    const auto transform=source_object_transform(u.body.position,basis.forward,basis.up,definition_->one,false);
    const auto nodes=pose_model_nodes_world_frame(body.model,transform,clip,s.frame);
    const auto markers=pose_model_markers(body.marker,nodes,body.permutations,{},1);
    if(markers.empty())throw FormatError("Grenade marker rejected current authored permutations");
    for(auto&p:projectiles_)if(p.datum==s.held()){
      const auto a=attachments_.find(p.datum);
      if(a==attachments_.end()||a->second.first>=nodes.size())
        throw FormatError("Held grenade lost its source attachment node/frame");
      const auto&local=a->second.second;
      const auto frame=compose_transform(nodes[a->second.first],source_object_transform(
          local.position,local.forward,local.up,definition_->one,false));
      p.position=frame.position;p.forward=frame.basis[0];p.up=frame.basis[2];break;
    }
  }
}
void NativeGrenadeSandbox::adopt_source_projectiles(std::vector<NativeGrenadeProjectile> next,
    std::span<const PresentationOwner> units) {
  NativeAuthorityBoundary::Call call(authority_);
  for(auto &projectile:next) {
    // Full source creator identity comes from body+C4/+C8/+C2, independently
    // of attachment (a plasma grenade can attach to an opposing unit). Only
    // this native participant index needs an explicit presentation association.
    const auto old=std::find_if(projectiles_.begin(),projectiles_.end(),[&](const auto &p){
      return p.datum==projectile.datum && p.player==projectile.player;
    });
    if(old!=projectiles_.end())projectile.native_owner=old->native_owner;
    for(const auto &owner:units)if(projectile.player!=null_datum && owner.unit &&
        projectile.player==owner.unit->source_player)projectile.native_owner=owner.native_owner;
  }
  ++authority_.mutations;projectiles_=std::move(next);source_driven_=true;
}
void NativeGrenadeSandbox::adopt_source_detonations(std::uint64_t tick,
    std::span<const SourceProjectileDetonationTu15> completed) {
  NativeAuthorityBoundary::Call call(authority_);++authority_.mutations;source_driven_=true;
  for(const auto &event:completed) {
    // Original101330 owns damage, retirement and its selected effect request.
    // This bridge only publishes the completed effect's actual location frame.
    events_.push_back({NativeGrenadeEvent::Detonated,event.unit,event.projectile,
        event.definition,event.effect});
    if(event.effect_instance==null_datum)continue; // Actual source admission rejected it.
    if(event.effect_locations.empty())
      throw FormatError("Completed TU1.5 explosion effect has no retained presentation location");
    if(detonation_serial_==UINT64_MAX)throw FormatError("Native grenade detonation serial exhausted");
    const auto &frame=event.effect_locations.front().frame;
    detonations_.push_back({++detonation_serial_,tick,event.projectile,event.definition,
        event.effect,event.player,event.unit,frame.position,frame.basis[0],frame.basis[2],event.effect_locations,event.colors,event.controls,event.effect_instance});
    if(detonations_.size()>64)detonations_.erase(detonations_.begin());
  }
}
void NativeGrenadeSandbox::prepare(Unit&u,int owner,const Allocate&allocate,const Constructed&constructed){
  const auto &body=unit_definition(u.definition.datum);
  auto&s=u.grenade_action;const auto result=source_grenade_prepare_prefix(s.source,
      {null_datum,u.grenade_current,u.grenades,true});
  if(result.stop!=GrenadePreparationStop::marker_request)return;
  const auto*clip=action_clip(s.graph,s.clip);
  if(!clip)throw FormatError("Grenade preparation lost its actual animation");
  const auto basis=authored_basis({u.yaw,0,0});
  const auto transform=source_object_transform(u.body.position,basis.forward,basis.up,definition_->one,false);
  const auto nodes=pose_model_nodes_world_frame(body.model,transform,clip,s.frame);
  const auto markers=pose_model_markers(body.marker,nodes,body.permutations,{},1);
  if(markers.empty())throw FormatError("Grenade preparation has no current authored marker");
  if(!allocate)throw FormatError("Grenade requires actual shared object allocator");
  const auto datum=allocate(5);
  if(datum==null_datum){s.source[0xc]=std::byte{3};return;}
  const auto&g=definition_->grenades[unsigned(u.grenade_current)];const auto q=root(g.tag);
  NativeGrenadeProjectile p;
  p.datum=datum;p.definition=g.datum;p.parent=u.source_datum;p.player=u.source_player;p.unit=u.source_datum;
  p.designator=u.source_membership;p.native_owner=owner;p.type=u.grenade_current;
  p.position=markers.front().world.position;
  p.forward=direction(u.yaw,u.pitch);
  p.up=creation_up(p.forward,definition_->one,definition_->frame_subtractor);
  attachments_.emplace(datum,std::pair{markers.front().node,
      source_object_attachment_local_frame({p.position,p.forward,p.up},nodes[markers.front().node])});
  const float timer=mul(30.f,q.f32(0xd4)),arming=mul(30.f,q.f32(0xcc));
  p.timer_step=timer>=definition_->one?div(definition_->one,timer):0.f;
  p.arming_step=arming>=definition_->one?div(definition_->one,arming):0.f;
  projectiles_.push_back(p);
  put(s.source,0x10,datum);put(s.source,0x14,std::int16_t(u.grenade_current));s.source[0xc]=std::byte{2};
  if(constructed)constructed(datum,g.datum);
  events_.push_back({NativeGrenadeEvent::Prepared,u.source_datum,datum,g.datum,null_datum,p.position, {},s.activity,s.graph,s.clip,s.counter(),2});
}
void NativeGrenadeSandbox::release(Unit&u){
  auto&s=u.grenade_action;if(source_grenade_release_empty(s.source))return;
  const auto held=s.held();
  const auto i=std::find_if(projectiles_.begin(),projectiles_.end(),[&](const auto&p){return p.datum==held;});
  if(i==projectiles_.end())throw FormatError("Grenade action lost full salted held object");
  put(s.source,0x10,null_datum);put(s.source,0x14,std::int16_t(-1));
  i->parent=null_datum;attachments_.erase(held);
  const auto aim=direction(u.yaw,u.pitch);
  auto side=source_cross(definition_->up,aim);if(original_vector_normalize(side)==0)side=definition_->up;
  auto up=source_cross(aim,side);original_vector_normalize(up);
  auto point=ordinary_unit_eye(u.body.position,u.definition.camera_height,u.definition.crouch_camera_height,u.crouch);
  point=source_add(point,source_scale(aim,definition_->offset.x));
  point=source_add(point,source_scale(side,definition_->offset.y));
  point=source_add(point,source_scale(up,definition_->offset.z));
  i->position=point;i->velocity=source_scale(aim,unit_definition(u.definition.datum).speed);
  // Source positive10DF90 returnedAL0. Native movement velocity is not an
  // invented support-object velocity provider and is therefore not added.
  i->flags|=1;s.source[0xc]=std::byte{3};
  events_.push_back({NativeGrenadeEvent::Released,u.source_datum,held,i->definition,null_datum,
      i->position,i->velocity,s.activity,s.graph,s.clip,s.counter(),3});
}
void NativeGrenadeSandbox::cancel_unit(Unit&u){
  NativeAuthorityBoundary::Call call(authority_);
  if(!(u.source_action_word&0x400000u))return;
  ++authority_.mutations;
  // E9A50 invokes E8F30 even with heldNONE, then clears phase. In the
  // ordinary player-owner release branch E8F30 clears the early flag before
  // random non-player launch adjustment, so the same player launch applies.
  release(u);u.grenade_action.source[0xc]=std::byte{};
  u.grenade_action.animation_active=false;u.source_action_word&=~0x400000u;
}
void NativeGrenadeSandbox::update_projectiles(std::uint64_t tick,const CollisionWorld&world,const Area&area,
    const std::function<void(std::uint32_t)>&release_object,const CollisionObjectType&object_type){
  NativeAuthorityBoundary::Call call(authority_);
  if(!projectiles_.empty())++authority_.mutations;
  const auto&d=*definition_;
  for(auto i=projectiles_.begin();i!=projectiles_.end();){
    auto&p=*i;const auto&g=d.grenades.at(unsigned(p.type));const auto q=root(g.tag);
    const auto start=std::bit_cast<std::int16_t>(q.u16(0xc0));
    advance_native_grenade_timer(p,start,d.one);
    if(p.parent==null_datum&&p.state<2&&!(p.flags&0x4000)&&
        !(p.state==1&&(p.arming_step==0||p.arming>=d.one))){
      // Existing source ballistic segment, with actual native terrain casts.
      const auto flight=projectile_flight(p.position,p.velocity,mul(retail_gravity,q.f32(0x164)),tick_seconds,p.traveled,q.f32(0xe0));
      const auto distance=length(flight.displacement);const auto hit=distance>0?world.raycast(p.position,flight.displacement/distance,distance):std::nullopt;
      p.velocity=flight.velocity;p.traveled=flight.traveled;
      if(!hit)p.position=flight.position;
      else{
        p.position=hit->position;p.flags|=0x800;if(hit->normal.z>d.up_threshold)p.flags|=4;
        const auto count=q.u32(0x19c);if(count>65536)throw FormatError("Excessive projectile responses");
        const Bytes rows(g.tag.bytes_at(q.u32(0x1a0),std::size_t(count)*0x58));
        const auto backing=g.tag.bytes_at(q.u32(0x1a0),std::size_t(count)*0x58);
        const auto found=source_projectile_material_response(backing,d.parents,std::bit_cast<std::int16_t>(hit->material));
        if(!found)throw FormatError("Projectile requires original default material-response owner");
        const auto at=*found*0x58;
        if(rows.f32(at+0x38)!=0||rows.f32(at+0x3c)!=0)
          throw FormatError("Projectile response requires original impact perturbation owner");
        // This world contains BSP triangles and posed scenario object triangles,
        // never unit shapes. Preserve the actual producer's kind/type input.
        // A future unit collision owner must supply its real header and frame;
        // it cannot acquire sticking behavior by being labeled terrain here.
        const bool object=hit->source.bsp<0 && hit->source.instance>=0;
        std::function<std::uint8_t()> target_type;
        if(object)target_type=[&]{
          if(!object_type)throw FormatError("Projectile collision object type owner unavailable");
          return object_type(std::size_t(hit->source.instance));
        };
        const auto response=choose_source_projectile_response(backing.subspan(at,0x58),
          {object?4:1,{},{}},{d.one,0.f,target_type,{}});
        if(response==4||response==5){
          const auto n=hit->normal,v=p.velocity;
          const auto dotn=add(add(mul(v.y,n.y),mul(n.x,v.x)),mul(v.z,n.z));
          const auto parallel=sub(d.one,rows.f32(at+0x50)),perpendicular=sub(d.one,rows.f32(at+0x54));
          p.velocity={sub(mul(sub(v.x,mul(dotn,n.x)),parallel),mul(mul(dotn,n.x),perpendicular)),
            sub(mul(sub(v.y,mul(dotn,n.y)),parallel),mul(mul(dotn,n.y),perpendicular)),
            sub(mul(sub(v.z,mul(dotn,n.z)),parallel),mul(mul(dotn,n.z),perpendicular))};
          if(squared(p.velocity)<mul(q.f32(0xdc),q.f32(0xdc))&&p.state<1)p.state=1;
          if(squared(p.velocity)<d.rest_squared){p.flags|=0x10;if(hit->normal.z>d.up_threshold){p.velocity={};p.flags|=0x4000;}}
        }else if(response==0)p.state=2;
        else throw FormatError("Projectile contact response is outside native ordinary bounce scope");
        events_.push_back({NativeGrenadeEvent::Contact,p.unit,p.datum,p.definition,null_datum,p.position,p.velocity});
      }
    }
    const bool detonate=p.state==2||(p.state==1&&(p.arming_step==0||p.arming>=d.one));
    if(detonate){
      if(detonation_serial_==UINT64_MAX)throw FormatError("Native grenade detonation serial exhausted");
      if(g.damage){if(!area)throw FormatError("Grenade detonation requires real area-damage owner");
        area(*g.damage,p.position,p.player,p.unit,p.designator,p.native_owner,p.definition);}
      events_.push_back({NativeGrenadeEvent::Detonated,p.unit,p.datum,p.definition,g.detonation_effect,p.position,p.velocity});
      detonations_.push_back({++detonation_serial_,tick,p.datum,p.definition,g.detonation_effect,
          p.player,p.unit,p.position,p.forward,p.up});
      if(detonations_.size()>64)detonations_.erase(detonations_.begin());
      attachments_.erase(p.datum);if(release_object)release_object(p.datum);i=projectiles_.erase(i);
    }else ++i;
  }
}
std::vector<NativeGrenadeEvent> NativeGrenadeSandbox::take_events(){NativeAuthorityBoundary::Call call(authority_);++authority_.mutations;auto r=std::move(events_);events_.clear();return r;}

void NativeGrenadeSandbox::validate_authority(const AuthorityCheckpoint &saved,std::span<const Unit> units)const {
  using namespace world_continuation;
  if(saved.version!=1 || saved.initial_unit!=initial_unit_ || saved.binding!=binding_ || saved.projectiles.size()>2048 ||
      saved.detonations.size()>64 || saved.detonation_serial==UINT64_MAX || !saved.unit_bindings.contains(initial_unit_))
    throw FormatError("Grenade continuation has incompatible bindings or frontier");
  std::map<std::uint32_t,AuthorityDigest> unit_bindings;
  for(const auto &[id,d]:units_)unit_bindings.emplace(id,grenade_unit(d->model,d->marker,d->permutations,d->speed));
  std::map<std::pair<std::uint32_t,std::uint32_t>,AuthorityDigest> weapon_bindings,selected_bindings;
  for(const auto &[key,c]:clips_)weapon_bindings.emplace(key,grenade_clips(c->ordinary,c->preferred));
  for(const auto &[key,c]:selected_clips_)selected_bindings.emplace(key,animation_clip(c));
  if(unit_bindings!=saved.unit_bindings || weapon_bindings!=saved.weapon_clip_bindings || selected_bindings!=saved.selected_clip_bindings)
    throw FormatError("Grenade continuation decoded bindings differ from local resources");
  const auto handle=[](std::uint32_t h){return h!=null_datum && (h>>16) && (h&65535u)<2048;};
  const auto reference=[&](std::uint32_t h){return h==null_datum || handle(h);};
  std::set<std::uint32_t> handles,slots;
  for(const auto &p:saved.projectiles) {
    if(!handle(p.datum) || !handles.insert(p.datum).second || !slots.insert(p.datum&65535u).second ||
        p.type<0 || std::size_t(p.type)>=definition_->grenades.size() || p.definition!=definition_->grenades[unsigned(p.type)].datum ||
        !reference(p.parent) || !reference(p.unit) || !finite(p.position) || !finite(p.velocity) || !finite(p.forward) || !finite(p.up) ||
        !std::isfinite(p.timer) || !std::isfinite(p.timer_step) || !std::isfinite(p.arming) || !std::isfinite(p.arming_step) || !std::isfinite(p.traveled))
      throw FormatError("Grenade continuation has invalid projectile identity/state");
    if(!saved.source_driven && p.parent!=null_datum && !saved.attachments.contains(p.datum))
      throw FormatError("Native held grenade continuation lost its attachment frame");
  }
  for(const auto &[h,a]:saved.attachments) {
    const auto p=std::ranges::find(saved.projectiles,h,&NativeGrenadeProjectile::datum);
    if(p==saved.projectiles.end() || p->parent==null_datum || !finite(a.second.position) || !finite(a.second.forward) || !finite(a.second.up))
      throw FormatError("Grenade continuation has an orphaned or invalid attachment");
    const auto unit=std::ranges::find(units,p->parent,&Unit::source_datum);
    if(unit==units.end() || unit->grenade_action.held()!=h || !units_.contains(unit->definition.datum) ||
        a.first>=units_.at(unit->definition.datum)->model.bones.size())
      throw FormatError("Grenade continuation attachment differs from its restored unit");
  }
  if(!saved.source_driven)for(const auto &u:units)if(u.grenade_action.held()!=null_datum) {
    const auto p=std::ranges::find(saved.projectiles,u.grenade_action.held(),&NativeGrenadeProjectile::datum);
    if(p==saved.projectiles.end() || p->parent!=u.source_datum || !saved.attachments.contains(p->datum))
      throw FormatError("Restored unit grenade action lost its held projectile");
  }
  for(const auto &e:saved.pending_events)if(e.kind<NativeGrenadeEvent::Begin || e.kind>NativeGrenadeEvent::Unavailable ||
      !reference(e.unit) || !reference(e.projectile) || !finite(e.position) || !finite(e.velocity))
    throw FormatError("Grenade continuation has an invalid pending event");
  std::uint64_t previous=0;
  for(const auto &d:saved.detonations) {
    if(!d.serial || d.serial<=previous || d.serial>saved.detonation_serial || !handle(d.projectile) || !reference(d.unit) ||
        !finite(d.position) || !finite(d.forward) || !finite(d.up) || !std::isfinite(d.controls.a) || !std::isfinite(d.controls.b) ||
        (d.source_effect_instance!=null_datum && (!saved.source_driven || !(d.source_effect_instance>>16) || (d.source_effect_instance&0xffffu)>=256u)))
      throw FormatError("Grenade continuation has an invalid detonation journal");
    for(const auto &location:d.source_locations)if(!finite(location.frame))
      throw FormatError("Grenade continuation has an invalid source effect location");
    previous=d.serial;
  }
  if(previous!=saved.detonation_serial)throw FormatError("Grenade continuation lost its retained detonation frontier");
}
struct NativeGrenadeSandbox::PreparedAuthority::Impl {
  NativeGrenadeSandbox *owner{};bool *staging{};
  std::unique_ptr<NativeGrenadeSandbox> next;
  ~Impl(){if(staging)*staging=false;}
};
NativeGrenadeSandbox::PreparedAuthority::PreparedAuthority()=default;
NativeGrenadeSandbox::PreparedAuthority::~PreparedAuthority()=default;
NativeGrenadeSandbox::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
NativeGrenadeSandbox::PreparedAuthority &NativeGrenadeSandbox::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool NativeGrenadeSandbox::PreparedAuthority::for_owner(const NativeGrenadeSandbox &owner)const noexcept{return impl_ && impl_->owner==&owner;}
NativeGrenadeSandbox::AuthorityCheckpoint NativeGrenadeSandbox::authority_checkpoint(std::span<const Unit> units,bool qualified_source_world)const {
  authority_.capture();if(source_driven_ && !qualified_source_world)throw FormatError("Grenade presentation requires its qualified complete source world");
  AuthorityCheckpoint s;s.initial_unit=initial_unit_;s.binding=binding_;s.source_driven=source_driven_;
  for(const auto &[id,d]:units_)s.unit_bindings.emplace(id,world_continuation::grenade_unit(d->model,d->marker,d->permutations,d->speed));
  for(const auto &[key,c]:clips_)s.weapon_clip_bindings.emplace(key,world_continuation::grenade_clips(c->ordinary,c->preferred));
  for(const auto &[key,c]:selected_clips_)s.selected_clip_bindings.emplace(key,world_continuation::animation_clip(c));
  s.projectiles=projectiles_;s.pending_events=events_;s.detonations=detonations_;s.detonation_serial=detonation_serial_;s.attachments=attachments_;
  validate_authority(s,units);return s;
}
NativeGrenadeSandbox::PreparedAuthority NativeGrenadeSandbox::prepare_authority(const AuthorityCheckpoint &saved,std::span<const Unit> units,bool qualified_source_world) {
  authority_.prepare();
  if(saved.version!=1 || saved.initial_unit!=initial_unit_ || saved.binding!=binding_ || (saved.source_driven && !qualified_source_world))
    throw FormatError("Grenade continuation import has incompatible local bindings");
  PreparedAuthority p;p.impl_=std::make_unique<PreparedAuthority::Impl>();
  p.impl_->owner=this;p.impl_->staging=&authority_.staging;authority_.staging=true;
  auto &next=p.impl_->next;next=std::make_unique<NativeGrenadeSandbox>(cache_,initial_unit_,constants_);
  if(next->binding_!=saved.binding)throw FormatError("Grenade resident cache changed after admission");
  for(const auto &[id,digest]:saved.unit_bindings){(void)digest;next->unit_definition(id);}
  for(const auto &[key,digest]:saved.weapon_clip_bindings){(void)digest;next->weapon_clips(key.first,key.second);}
  for(const auto &[key,digest]:saved.selected_clip_bindings){(void)digest;
    next->selected_clips_.emplace(key,cache_->load_animation(key.first,key.second));}
  next->validate_authority(saved,units);
  next->projectiles_=saved.projectiles;next->events_=saved.pending_events;next->detonations_=saved.detonations;
  next->attachments_=saved.attachments;next->detonation_serial_=saved.detonation_serial;next->source_driven_=saved.source_driven;return p;
}
bool NativeGrenadeSandbox::commit_authority(PreparedAuthority &&p) noexcept {
  if(!p.for_owner(*this) || !authority_.staging || authority_.depth)return false;
  auto &n=*p.impl_->next;units_.swap(n.units_);clips_.swap(n.clips_);selected_clips_.swap(n.selected_clips_);
  projectiles_.swap(n.projectiles_);events_.swap(n.events_);detonations_.swap(n.detonations_);attachments_.swap(n.attachments_);
  detonation_serial_=n.detonation_serial_;source_driven_=n.source_driven_;authority_.fault=false;p.impl_.reset();return true;
}
} // namespace halo2
