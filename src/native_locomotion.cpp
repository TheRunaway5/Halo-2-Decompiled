#include "halo2/native_locomotion.hpp"
#include "halo2/original_data.hpp"
#include "halo2/animation_variant.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/unit_animation.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#include "halo2/unit_constructors.hpp"
#include "halo2/unit_locomotion.hpp"
#include <bit>
#include <algorithm>
#include <cmath>
#include <map>
namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte>b,std::size_t p,unsigned n=4){
  if(p>b.size()||n>b.size()-p)throw FormatError("Native animation authored packet truncated");
  std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[p+i])<<(i*8);return v;}
void put(std::span<std::byte>b,std::size_t p,std::uint32_t v){for(unsigned i=0;i<4;++i)b[p+i]=std::byte(v>>(i*8));}
float fp(std::span<const std::byte>b,std::size_t p){return std::bit_cast<float>(get(b,p));}
float product(float a,float b){volatile float r=a*b;return r;}
float sum(float a,float b){volatile float r=a+b;return r;}
constexpr std::array<std::uint32_t,6> activities{0x0400000c,0x0400000c,0x0a000017,0x09000016,0x0a000014,0x09000015};
}
struct NativeLocomotion::State {
  const Cache &cache;
  // Cache must outlive this adapter; metadata/decoded clips are retained here.
  UnitAnimationMetadataStore metadata;
  std::uint32_t graph,one,deadzone;
  std::array<std::uint32_t,2> rng;
  std::array<std::uint32_t,2> origin_rng;
  std::array<std::byte,0x90> current{},origin{};
  std::map<std::pair<std::uint32_t,std::uint32_t>,AnimationClip> clips;
  std::array<std::uint32_t,2> ids{};
  std::map<std::pair<std::uint32_t,std::uint32_t>,std::uint32_t> bindings;
  std::uint32_t mode=0x06000086;
  AnimationVariantServices variants;
  bool can_interpolate{};
  bool facing_bound{};
  float yaw{},previous_yaw{},turn_rate{},facing_cosine{};
  float facing_epsilon{},reversal_cosine{};
  std::uint32_t biped_flags{},unit_flags{};
  State(const Cache &c,std::uint32_t g,std::uint32_t w,std::array<std::uint32_t,2> random,bool dual)
      :cache(c),metadata(std::shared_ptr<const Cache>(&c,[](const Cache*){})),graph(g),rng(random){
    const auto image=c.original_data();if(!image||!image->unit_animation_constants||!image->unit_locomotion_deadzone_bits)
      throw FormatError("Native locomotion requires original animation/deadzone operands");
    const auto &k=*image->unit_animation_constants;one=k.one_bits;deadzone=*image->unit_locomotion_deadzone_bits;
    variants.memory=metadata.graph_memory();variants.constants={k.one_bits,k.rng_scale_bits,k.unsigned_conversion_correction_bits};
    variants.resource=[this](std::uint32_t address,std::uint32_t packed){
      const auto* graph=cache.query_tag({.group=tag_groups::model_animation_graph, .address=address});
      const std::uint32_t master=graph?graph->datum:0xffffffffu;
      if(master==0xffffffffu)throw FormatError("Native residency lost original master graph");
      // This platform provider actually decodes and retains the selected clip.
      // It is not a fabricated successful guest resource/IO completion.
      const auto &definition=metadata.definition(master,packed);(void)definition;
      const auto inheritance=std::bit_cast<std::int16_t>(std::uint16_t(packed));
      auto resolved=master;
      if(inheritance!=-1){const auto a=variants.memory.graph_address(master);const auto h=variants.memory.read(a,0x54);
        if(inheritance<0||unsigned(inheritance)>=get(h,0x4c))throw FormatError("Native animation inheritance invalid");
        resolved=get(variants.memory.read(get(h,0x50)+unsigned(inheritance)*32,8),4);}
      const auto key=std::pair(master,packed);if(!clips.contains(key))clips.emplace(key,cache.load_animation(resolved,packed>>16));
    };
    construct_source_unit_animation_manager(current,one);
    put(current,0x68,graph); // Explicit native presentation graph binding.
    ids=cache.load_weapon_animation_identifiers(w);
    ids[0]=dual?0x0400054bu:source_animation_weapon_class(variants.memory.graph_address(graph),
                                         ids[0],ids[1],variants.memory);
    const auto selected=select_source_animation_variant(current,binding(mode,activities[0]),rng,variants);
    install_source_unit_animation_channel(current,0,graph,0x3f,selected,activities[0],0xff,0xff,0,one);
    origin=current;
  }
  std::uint32_t binding(std::uint32_t requested_mode,std::uint32_t activity){
      const auto key=std::pair(requested_mode,activity);
      if(const auto found=bindings.find(key);found!=bindings.end())return found->second;
      // Keep the authored signed inheritance word. Elite locomotion lives in
      // inherited graphs; a decoded clip's display index loses that binding.
      // The recovered29c6a0 iterator retains the original any fallback order.
      std::array<std::byte,28> iterator{};
      put(iterator,0,variants.memory.graph_address(graph));
      put(iterator,4,requested_mode);put(iterator,8,ids[0]);put(iterator,12,ids[1]);
      std::array<std::uint32_t,3> resolved{};
      while(const auto weapon_record=next_source_animation_weapon(iterator,resolved,variants.memory)){
        const auto row=find_source_animation_record(weapon_record+4,activity,8,variants.memory);
        if(!row)continue;
        const auto packed=get(variants.memory.read(row+4,4),0);
        if((packed>>16)==0xffff)continue;
        bindings.emplace(key,packed);return packed;
      }
      throw FormatError("Native gait has no authored weapon action: mode="+cache.string_id(requested_mode)+
          " class="+cache.string_id(ids[0])+" weapon="+cache.string_id(ids[1])+" action="+cache.string_id(activity));
  }
  UnitAnimationServices services(std::array<std::uint32_t,2>&random,float seconds){
    UnitAnimationServices s;s.tick_seconds=seconds;s.definition=metadata.definition_provider();
    s.select_variant=[this,&random](auto m,auto packed){return select_source_animation_variant(m,packed,random,variants);};return s;
  }
  NativeLocomotionFrame view(std::span<const std::byte> m)const{
    const auto key=std::pair(get(m,0),get(m,4));const auto f=clips.find(key);
    if(f==clips.end())throw FormatError("Native gait frame lost decoded selected resource");
    return {&f->second,fp(m,0x1c),get(m,8)};
  }
};
NativeLocomotion::NativeLocomotion(const Cache&c,std::uint32_t g,std::uint32_t w,std::array<std::uint32_t,2>rng,bool dual)
  :state_(std::make_unique<State>(c,g,w,rng,dual)){}
NativeLocomotion::~NativeLocomotion()=default;
void NativeLocomotion::bind_facing(std::uint32_t biped,float yaw) {
  auto &s=*state_;const auto tag=s.cache.load_source_tag_metadata(biped);
  const Bytes d(tag.bytes_at(tag.source_address(),0x250));
  s.turn_rate=d.f32(0x1ec);s.facing_cosine=d.f32(0x24c);
  s.biped_flags=d.u32(0x1f0);s.unit_flags=d.u32(0xbc);
  s.yaw=s.previous_yaw=yaw;s.facing_bound=true;
  const auto image=s.cache.original_data();
  if(!image || !image->unit_facing_constants)throw FormatError("Missing original facing operands");
  s.facing_epsilon=std::bit_cast<float>((*image->unit_facing_constants)[0]);
  s.reversal_cosine=std::bit_cast<float>((*image->unit_facing_constants)[1]);
}
void NativeLocomotion::tick(float forward,float right,bool walking,bool crouch,Vec3 velocity,
    const std::function<void(const NativeFootstepEvent&)>&event,const SourceLandingState *landing,const float *aim_yaw){
  auto&s=*state_;if(!std::isfinite(forward)||!std::isfinite(right))throw FormatError("Nonfinite native gait axes");
  unsigned classification=0;
  if(walking){const float y=-right;const float magnitude=sum(sum(product(forward,forward),product(y,y)),0.f);
    if(magnitude>std::bit_cast<float>(s.deadzone)){
      std::array<std::byte,0x154> request{};std::array<std::byte,0x70> result{};
      put(request,0xdc,std::bit_cast<std::uint32_t>(forward));put(request,0xe0,std::bit_cast<std::uint32_t>(y));
      classify_source_locomotion_request(request,result);classification=get(result,4);
    }}
  // E5700's airborne activity and the graph's actual crouch mode. The native
  // mover supplies contact/crouch ownership, not fabricated Havok records.
  const auto ordinary=activities.at(classification);
  auto activity=walking?(landing?source_landing_activity(*landing,ordinary):ordinary):0x0800001eu;
  s.previous_yaw=s.yaw;
  if(aim_yaw && s.facing_bound) {
    // Native 2D facing adapter of E1800. The real biped definition supplies
    // stationary limits and moving turn rate; the camera never owns the root.
    const auto previous=get(s.current,8);
    const bool was_moving=previous==0x0a000014u || previous==0x09000015u || previous==0x09000016u || previous==0x0a000017u || previous==0x0800001eu;
    float delta=std::remainder(*aim_yaw-s.yaw,6.283185307179586f);
    if(std::cos(delta)<s.reversal_cosine) {
      if(previous==0x0900000eu && delta<0)delta+=6.283185307179586f;
      if(previous==0x0a00000fu && delta>0)delta-=6.283185307179586f;
    }
    if(!walking || was_moving || (s.biped_flags&1))
      s.yaw+=std::clamp(delta,-s.turn_rate/30.f,s.turn_rate/30.f);
    else if(activity==0x0400000cu && !(s.unit_flags&(1u<<20)) && std::cos(delta)<=s.facing_cosine+s.facing_epsilon)
      activity=delta<0?0x0a00000fu:0x0900000eu;
    if(previous==0x0900000eu || previous==0x0a00000fu) {
      const auto selected=s.view(s.current);
      if(!selected.clip->movement_yaw.empty()) {
        const auto n=std::min(unsigned(selected.frame),unsigned(selected.clip->movement_yaw.size()-1));
        // 1CA6F0/1D10B0 ->1E9540: authored per-frame yaw * rate *30*dt.
        const float change=selected.clip->movement_yaw[n];
        if(delta*change>=0)s.yaw+=std::copysign(std::min(std::abs(change),std::abs(delta)),change);
      }
    }
  }
  const auto mode=crouch?0x06000087u:0x06000086u;
  s.origin=s.current;s.origin_rng=s.rng;s.can_interpolate=activity==get(s.current,8)&&mode==s.mode;
  // Source ordinary unit phase advances an existing gait before installing
  // this tick's newly selected gait. New selection is authored frame zero.
  if(s.can_interpolate){auto services=s.services(s.rng,1.f/30.f);
    services.event=[&](auto,float,const auto&p,auto,auto){
      if(get(p,0,2)!=0||!event||!walking)return;
      const auto timing=get(p,8);if(timing!=2&&timing!=3&&timing!=9&&timing!=10)return;
      const auto speed=sum(sum(product(velocity.x,velocity.x),product(velocity.y,velocity.y)),product(velocity.z,velocity.z));
      const unsigned kind=timing==9?3:timing==10?7:speed<=2.25f?0:crouch?6:1;
      event({timing,kind,timing==2?0u:timing==3?1u:2u,float(std::int16_t(get(p,2,2)))});
    };
    advance_source_unit_animation_channel(std::span(s.current).first(32),s.current,{1,0},services);
  }else {
    const auto selected=select_source_animation_variant(s.current,s.binding(mode,activity),s.rng,s.variants);
    install_source_unit_animation_channel(s.current,0,s.graph,0x3f,selected,activity,0xff,0xff,0,s.one);
  }
  s.mode=mode;
}
NativeLocomotionFrame NativeLocomotion::frame()const{return state_->view(state_->current);}
NativeLocomotionFrame NativeLocomotion::sample(float fraction)const{
  auto&s=*state_;if(!s.can_interpolate)return frame();
  if(!(fraction>=0&&fraction<=1))throw FormatError("Native gait interpolation fraction invalid");
  if(fraction==1)return frame();auto m=s.origin;auto rng=s.origin_rng;
  auto services=s.services(rng,(1.f/30.f)*fraction);
  advance_source_unit_animation_channel(std::span(m).first(32),m,{},services);
  // A variant may change at an endpoint. The next discrete selection is held
  // until its actual tick, rather than consuming a second presentation RNG.
  if(get(m,4)!=get(s.current,4))return fraction==0?s.view(s.origin):frame();
  return s.view(m);
}
std::span<const std::byte>NativeLocomotion::manager()const{return state_->current;}
float NativeLocomotion::facing(float fraction)const {
  if(fraction<0 || fraction>1)throw FormatError("Invalid native facing fraction");
  const auto &s=*state_;return s.previous_yaw+std::remainder(s.yaw-s.previous_yaw,6.283185307179586f)*fraction;
}

std::array<std::uint32_t,3> authored_footstep_sounds(const Cache&cache,std::uint32_t foot,
    std::int16_t material,unsigned kind,int variant){
  const auto matg=cache.query_tag({.group=tag_groups::globals});
  if(!matg)throw FormatError("Footstep material globals missing");
  const auto globals=cache.load_source_tag_metadata(matg->datum);const auto header=globals.bytes_at(globals.source_address(),0x158);
  const auto count=get(header,0x150),pointer=get(header,0x154);
  const auto source_bytes=[&](std::uint32_t address,std::size_t extent)->std::span<const std::byte>{
    // Cache serialization may deduplicate entry arrays into another tag.
    // Resolve the literal owner; never relocate or copy a guessed foot row.
    for(const auto&t:cache.select_tags({.contains=TagAddressRange{address,extent}})){
      (void)cache.tag_payload(t.handle());
      return cache.read_tag_bytes(address,extent);
    }
    throw FormatError("Footstep pointer has no resident cache owner");
  };
  auto row=[&](int m){if(m<0||unsigned(m)>=count)throw FormatError("Footstep global material invalid");return globals.bytes_at(pointer+unsigned(m)*0xb4,0xb4);};
  if(material<0||unsigned(material)>=count)return {0xffffffffu,0xffffffffu,0xffffffffu};
  auto find=[&](std::uint32_t datum,unsigned index)->std::vector<std::byte>{
    if(datum==0xffffffffu)return {};const auto tag=cache.load_source_tag_metadata(datum);
    if(tag.group()!="foot")throw FormatError("Footstep material-effects group mismatch");
    const auto root=tag.bytes_at(tag.source_address(),8);
    for(unsigned depth=0;depth<32;++depth){
      if(index<get(root,0)){const auto action=tag.bytes_at(get(root,4)+index*0x18,0x18);
        if(get(action,8)>0){const auto entries=source_bytes(get(action,12),std::size_t(get(action,8))*0x18);
          int m=material;for(unsigned parents=0;m!=-1;++parents){if(parents>=count)throw FormatError("Footstep material parent cycle");
            for(unsigned i=0;i<get(action,8);++i)if(std::bit_cast<std::int16_t>(std::uint16_t(get(entries,i*0x18+0x14,2)))==m)
              return {entries.begin()+i*0x18,entries.begin()+(i+1)*0x18};
            m=std::bit_cast<std::int16_t>(std::uint16_t(get(row(m),8,2)));}
          return {};}}
      switch(index){case 13:index=14;break;case 16:case 17:index=15;break;case 19:index=16;break;case 20:index=19;break;default:return {};}
    }throw FormatError("Footstep action fallback cycle");
  };
  auto entry=find(foot,kind==13?14:kind);
  int m=material;for(unsigned depth=0;entry.empty()&&m!=-1;++depth){if(depth>=count)throw FormatError("Footstep fallback material cycle");
    const auto r=row(m);entry=find(get(r,0xb0),kind==13?14:kind);m=std::bit_cast<std::int16_t>(std::uint16_t(get(r,8,2)));}
  std::array<std::uint32_t,3>sounds{0xffffffffu,0xffffffffu,0xffffffffu};if(entry.empty())return sounds;
  sounds[0]=get(entry,4);sounds[2]=get(entry,12);
  if(get(entry,22,1)==1&&variant!=-1){const int v=kind>=13&&kind<=14?3:variant;
    if(v<0||v>=7)throw FormatError("Footstep sweetener variant outside authored slots");sounds[1]=get(row(material),0x3c+unsigned(v)*8);}
  return sounds;
}
} // namespace halo2
