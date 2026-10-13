#include "halo2/multiplayer_audio.hpp"
#include "halo2/menu_music.hpp"
#include "halo2/sound_driver_parameters.hpp"
#include "halo2/sound_variation.hpp"
#include "halo2/sound_spatial.hpp"
#include "halo2/sound_fade.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <exception>
#include <limits>
#include <map>
#include <set>
#include <utility>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
std::int16_t short_at(const Bytes &b,std::size_t at){return std::bit_cast<std::int16_t>(b.u16(at));}
float pin(float x,float lo,float hi){return lo>x?lo:x>hi?hi:x;}
float disk_distance(const BspAtmosphere &b,unsigned index,Vec3 point){
  const auto &d=b.portals.at(index);const auto &p=b.planes.at(d.plane);
  const Vec3 normal{p[0],p[1],p[2]};auto offset=point-d.center;
  const float distance=dot(normal,offset);const auto projection=normal*distance;
  offset=offset-projection;const auto height=dot(projection,normal),squared=dot(offset,offset);
  if(d.radius*d.radius>=squared)return std::abs(height);
  const float radial=std::sqrt(squared)-d.radius;
  return std::sqrt(radial*radial+height*height);
}
float travel(const MultiplayerAudioBsp &b,int a,int z){
  if(a==z)return 0;
  if(a>z)std::swap(a,z);
  if(a<0 || z<0 || std::size_t(z)>=b.clusters.size())throw FormatError("Ambient cluster pair outside BSP");
  const auto index=(b.clusters.size()-1)*std::size_t(a)-(std::size_t(a)+1)*std::size_t(a)/2+std::size_t(z)-1;
  const auto value=b.pair_distances.at(index);
  if(!b.encoded_distances)return float(value&127)*(256.f/127.f);
  if(!value)return 0;
  if(value==255)return 256;
  return (float(value-1)*(1.f/253.f))*(b.distance_upper-b.distance_lower)+b.distance_lower;
}
float path_gain(float distance,float range){
  range=range>.001f?range:.001f;
  const float t=pin((distance-range)/(0.f-range),0,1);
  const float blend=pin(t*t*3.f-t*t*t*2.f,0,1);
  const auto floor=sound_db_to_linear(-64);
  return sound_linear_to_db((1.f-floor)*blend+floor);
}
// Original13DE30/13E0E0. Keep its tie ordering, including invalid palettes:
// equal-distance palette order is observable when only two tags are retained.
void palette_sort(std::vector<unsigned> &order,std::span<const float> distance,
                  std::ptrdiff_t lo,std::ptrdiff_t hi){
  if(hi<=lo)return;
  const auto compare=[&](unsigned a,unsigned b){return distance[a]>distance[b];};
  if(hi-lo+1<=8){
    for(auto end=hi;end>lo;--end){auto maximum=lo;
      for(auto p=lo+1;p<=end;++p)if(compare(order[p],order[maximum]))maximum=p;
      std::swap(order[maximum],order[end]);}
    return;
  }
  std::swap(order[lo+(hi-lo+1)/2],order[lo]);
  auto lower=lo,upper=hi+1;
  for(;;){do{++lower;}while(lower<=hi&&!compare(order[lower],order[lo]));
    do{--upper;}while(upper>lo&&!compare(order[lo],order[upper]));
    if(upper<lower)break;
    std::swap(order[lower],order[upper]);}
  std::swap(order[lo],order[upper]);
  palette_sort(order,distance,lo,upper-1);palette_sort(order,distance,lower,hi);
}
}

MultiplayerAudioBsp Cache::load_multiplayer_audio_bsp(std::size_t index)const {
  if(index>=bsps_.size())throw FormatError("Audio BSP outside scenario");
  const auto &bsp=bsps_[index];const Bytes file(storage_),b(file.slice(bsp.file_offset,bsp.size));
  
  MultiplayerAudioBsp out;
  const auto [palettes,p]=TagAddressSpace(b, bsp.memory_address).block(16+0xd4, 0x64, 0x100000);
  for(std::size_t i=0;i<palettes;++i){const auto at=p+i*0x64;
    out.palette.push_back({b.u32(at+0x24),b.u32(at+0x2c),b.u32(at+0x48),
      b.f32(at+0x44),b.f32(at+0x4c),b.f32(at+0x50),b.f32(at+0x54),b.f32(at+0x58)});
  }
  if(palettes>64)throw FormatError("Original ambient palette exceeds 64 slots");
  const auto [clusters,c]=TagAddressSpace(b, bsp.memory_address).block(16+0x9c, 0xb0, 0x100000);
  for(std::size_t i=0;i<clusters;++i)out.clusters.push_back({short_at(b,c+i*0xb0+0x72),short_at(b,c+i*0xb0+0x78)});
  const auto [paths,r]=TagAddressSpace(b, bsp.memory_address).block(16+0x148, 0x14, 0x100000);
  for(std::size_t i=0;i<paths;++i){const auto [count,at]=TagAddressSpace(b, bsp.memory_address).block(r+i*0x14+4, 2, 0x100000);auto &path=out.paths.emplace_back();
    for(std::size_t j=0;j<count;++j)path.push_back(b.u16(at+j*2));
  }
  const auto [audibility,a]=TagAddressSpace(b, bsp.memory_address).block(16+0x224, 0x34, 0x100000);
  if(audibility){out.encoded_distances=true;out.distance_lower=b.f32(a+4);out.distance_upper=b.f32(a+8);
    const auto [count,at]=TagAddressSpace(b, bsp.memory_address).block(a+0x24, 1, 0x100000);for(std::size_t i=0;i<count;++i)out.pair_distances.push_back(std::to_integer<std::uint8_t>(b.slice(at+i,1)[0]));
  }else{const auto count=b.u32(16+0xe4),pointer=b.u32(16+0xe8);
    if(count){if(pointer<bsp.memory_address)throw FormatError("Audio PAS pointer outside BSP");
      const auto bytes=b.slice(pointer-bsp.memory_address,count);for(auto x:bytes)out.pair_distances.push_back(std::to_integer<std::uint8_t>(x));}}
  return out;
}

std::vector<MultiplayerAmbientTarget> multiplayer_ambient_targets(const MultiplayerAudioBsp &b,
    const BspAtmosphere &geometry,Vec3 point,int cluster,float weather,bool weather_enabled){
  if(cluster<0)return {};
  const auto &current=b.clusters.at(std::size_t(cluster));const auto count=b.palette.size();
  struct Candidate{bool valid{};float distance{};int portal=-1,cluster=-1;};
  std::vector<Candidate> candidates(count);
  if(current.palette>=0)candidates.at(unsigned(current.palette))={true,0,-1,cluster};
  float nearest=std::numeric_limits<float>::max();
  if(current.path>=0)for(const auto encoded:b.paths.at(unsigned(current.path))){
    const auto portal=unsigned(encoded&0x7fff);const auto &p=geometry.portals.at(portal);
    const int other=(encoded&0x8000)?p.front:p.back;const auto &adjacent=b.clusters.at(std::size_t(other));
    if((p.flags&0x28)||adjacent.palette==current.palette)continue;
    const float path=travel(b,cluster,other),distance=disk_distance(geometry,portal,point)+path;
    if(nearest>=distance && current.palette>=0){candidates.at(unsigned(current.palette)).portal=int(portal);nearest=distance;}
    if(adjacent.palette>=0){auto &candidate=candidates.at(unsigned(adjacent.palette));
      float limit=b.palette.at(unsigned(adjacent.palette)).cutoff;
      if(limit>=path){if(candidate.valid)limit=candidate.distance;
        if(limit>=distance)candidate={true,distance,int(portal),other};}}
  }
  std::vector<unsigned> ordered;std::vector<float> distances;
  for(unsigned i=0;i<count;++i){ordered.push_back(i);distances.push_back(candidates[i].valid?candidates[i].distance:std::numeric_limits<float>::max());}
  palette_sort(ordered,distances,0,std::ptrdiff_t(count)-1);
  ordered.erase(std::find_if(ordered.begin(),ordered.end(),[&](unsigned i){return !candidates[i].valid;}),ordered.end());
  std::vector<MultiplayerAmbientTarget> out;
  const bool inside=!ordered.empty()&&cluster==candidates[ordered.front()].cluster;
  for(const auto index:ordered){const auto &s=b.palette[index];const auto &candidate=candidates[index];
    if(s.sound==none || std::any_of(out.begin(),out.end(),[&](const auto &v){return v.sound==s.sound;}))continue;
    float inner=1,portal=1,outer=1;
    if(s.flags&1){inner=s.inside;portal=s.portal;outer=s.outside;}
    bool muted=false;
    if(s.flags&8){inner*=weather;portal*=weather;outer*=weather;muted=!weather_enabled;}
    float distance=candidate.portal<0?0:disk_distance(geometry,unsigned(candidate.portal),point);
    if(out.empty() && inside)for(const auto adjacent_index:ordered){
      if(adjacent_index!=index && b.palette[adjacent_index].sound==s.sound){
        const auto &adjacent=candidates[adjacent_index];
        if(adjacent.portal>=0)distance=disk_distance(geometry,unsigned(adjacent.portal),point);
        if(s.flags&4)outer=(b.palette[adjacent_index].flags&1)?b.palette[adjacent_index].inside:1;
        if(s.flags&2)portal=(b.palette[adjacent_index].flags&1)?b.palette[adjacent_index].inside:1;
        break;
      }}
    const float fraction=std::abs(s.cutoff)<.0001f?(distance<0?0.f:1.f):pin(distance/s.cutoff,0,1);
    const float blend=pin(fraction*fraction*3.f-fraction*fraction*fraction*2.f,0,1);
    const float gain=((inside?inner:outer)-portal)*blend+portal;
    out.push_back({s.sound,index,muted?0.f:gain,path_gain(candidate.distance,s.cutoff),
        std::abs(s.fade_seconds)<.0001f?2.f:s.fade_seconds});
    if(out.size()==2)break;
  }
  return out;
}

struct MultiplayerAudio::State {
  struct Bank {
    SoundVariationDefinition definition;
    SoundRangeState range;
    LoopingSoundSelection loop;
    std::vector<std::shared_ptr<const std::vector<float>>> pcm;
    std::vector<std::uint16_t> chances;
    unsigned rate_index{},sound_class{};
  };
  struct Voice {std::uint64_t id{};std::uint32_t sound{},owner{},loop_tag=none;unsigned view{},track{};
    MultiplayerAudioKind kind{};float gain_db{},relative_rate=1,ambient_gain=1;unsigned sound_class{};
    bool loop{};Completion completion;unsigned pitch_range{};std::uint32_t source_pitch_bits{};
    std::int8_t permutation_gain_db{};std::optional<std::uint32_t> permutation_seed;
    std::optional<Vec3> position;std::optional<SoundPlaybackFade> fade;float fade_db{};};
  struct RetiredCompletion {Completion owner;SdlSoundCompletion packet;};
  struct Ambient {float gain{},linear_db{};MultiplayerAmbientTarget target;std::vector<std::uint64_t> voices;};
  const Cache &cache;SoundLocalRandom random;SdlSoundPlatform platform;MultiplayerAudioObserver observer;
  std::uint32_t *shared_variation_random{};
  SoundDriverParameterConstants constants;
  SoundCreateConstants distance_constants;
  std::vector<Vec3> listeners;
  std::map<std::pair<std::uint32_t,unsigned>,Bank> banks;
  std::map<std::uint64_t,Voice> voices;
  std::map<std::uint32_t,Ambient> ambience;
  std::vector<RetiredCompletion> retired;
  std::array<float,sound_class_count> class_gain{};
  float master=1.f;
  std::uint64_t next_packet=1;
  bool closing=false;
  State(const Cache &c,std::uint32_t seed,const SdlSoundPlatformConfiguration &configuration,MultiplayerAudioObserver o,
      std::uint32_t *shared):
      cache(c),random(seed),platform(configuration),observer(std::move(o)),shared_variation_random(shared){
    if(!cache.original_data()->sound_driver_parameter_constants)throw FormatError("Authored multiplayer audio requires selected original driver constants");
    constants=*cache.original_data()->sound_driver_parameter_constants;
    if(!cache.original_data()->sound_create_constants)throw FormatError("Authored world audio requires selected original distance constants");
    distance_constants=*cache.original_data()->sound_create_constants;
  }
  Bank &bank(std::uint32_t sound,unsigned pitch=0){
    const auto key=std::pair{sound,pitch};
    if(auto found=banks.find(key);found!=banks.end())return found->second;
    Bank b;b.definition=cache.load_sound_variation(sound,pitch);
    b.rate_index=std::to_integer<unsigned>(b.definition.sound[3]);b.sound_class=std::to_integer<unsigned>(b.definition.sound[2]);
    b.range=imported_sound_range_state(b.definition);b.loop={b.range.used,b.range.last};
    for(unsigned p=0;p<b.definition.permutations.size();++p){
      auto pcm=decode_sound(cache.load_sound(sound,pitch,p));
      if(pcm.rate!=48000 || !pcm.frames())throw FormatError("Native authored sound has no complete PCM frames");
      b.pcm.push_back(std::make_shared<const std::vector<float>>(std::move(pcm.stereo)));
      b.chances.push_back(Bytes(b.definition.permutations[p]).u16(2));
    }
    return banks.emplace(key,std::move(b)).first->second;
  }
  float distance_gain(const Voice &v)const {
    if(!v.position)return 1.f;
    if(listeners.empty())return 0.f;
    auto nearest=std::bit_cast<float>(distance_constants.maximum_float_bits);
    for(const auto &listener:listeners){
      //129570 computes (z*z+y*y)+x*x before129450's square root.
      const auto delta=listener-*v.position;
      const float squared=(delta.z*delta.z+delta.y*delta.y)+delta.x*delta.x;
      if(nearest>squared)nearest=squared;
    }
    const auto &definition=banks.at({v.sound,0}).definition;
    const Bytes playback(definition.playback),cls(definition.sound_class);
    const auto flags=std::to_integer<unsigned>(definition.sound[1]);
    const auto epsilon=std::bit_cast<float>(distance_constants.fade_epsilon_bits);
    const auto authored_minimum=flags&4u?cls.f32(0x18):playback.f32(0);
    const auto minimum=authored_minimum>epsilon?authored_minimum:epsilon;
    const auto maximum=flags&8u?cls.f32(0x1c):playback.f32(4);
    return std::bit_cast<float>(source_sound_distance_gain(std::bit_cast<std::uint32_t>(minimum),
        std::bit_cast<std::uint32_t>(maximum),std::bit_cast<std::uint32_t>(std::sqrt(nearest)),constants,{0x027f,0x1f80}));
  }
  SdlSoundVoiceParameters parameters(const Voice &v)const {
    const float spatial=distance_gain(v);
    const SoundSpatialConstants gain_constants{distance_constants.one_bits,distance_constants.minimum_gain_bits,distance_constants.linear_db_scale_bits};
    const float spatial_db=std::bit_cast<float>(source_sound_spatial_gain_db(std::bit_cast<std::uint32_t>(spatial),gain_constants,{0x027f,0x1f80}));
    const float source_gain=v.gain_db+sound_linear_to_db(v.ambient_gain)+class_gain.at(v.sound_class)+spatial_db+v.fade_db;
    // Original12A603..12A650 adds the selected permutation's signed gain
    // before the driver attenuation conversion. This must follow selection,
    // including every new ambient packet, rather than using permutation0.
    const float attenuation=source_gain+float(v.permutation_gain_db);
    const auto volume=source_sound_driver_volume(std::bit_cast<std::uint32_t>(attenuation),constants,{0x027f,0x1f80});
    // Explicit native stereo output. Xbox mix-bin/DSP routing is a separate
    // platform contract; neither channel is silently dropped on a stereo host.
    const auto output_gain=spatial>0.f?master:0.f;
    return {std::bit_cast<std::uint32_t>(v.relative_rate),volume,false,{{0,0,output_gain},{1,1,output_gain}}};
  }
  void packet(Voice &voice,bool loop_selection){
    auto &b=bank(voice.sound,voice.pitch_range);
    SoundLocalRandom per_voice(voice.permutation_seed.value_or(0));
    auto &selection=voice.permutation_seed?per_voice:random;
    const auto before=selection.seed;
    const unsigned p=unsigned(loop_selection?source_looping_sound_permutation(b.chances,b.loop,selection.seed):
        source_sound_permutation(b.definition,b.range,selection));
    if(voice.permutation_seed)voice.permutation_seed=selection.seed;
    voice.permutation_gain_db=std::bit_cast<std::int8_t>(b.definition.permutations.at(p)[4]);
    platform.parameters(voice.id,parameters(voice));
    const auto &pcm=b.pcm.at(p);const auto result=platform.submit(voice.id,{pcm,0,pcm->size()/2,next_packet++});
    if(result!=SdlSoundPlatform::Submission::Accepted)throw FormatError("Native sound packet queue unexpectedly full");
    if(observer)observer({voice.kind,voice.sound,voice.owner,voice.view,voice.pitch_range,p,voice.id,pcm->size()/2,before,selection.seed,voice.source_pitch_bits,voice.position});
    selection.stores.clear();
  }
  std::uint64_t play(std::uint32_t sound,float gain_db,MultiplayerAudioKind kind,
      std::uint32_t owner,unsigned view,bool looping=false,std::uint32_t loop_tag=none,unsigned track=0,float ambient_gain=1,
      Completion completion={},float control=1,std::optional<Vec3> position={}){
    if(closing)throw FormatError("Native audio is being destroyed");
    auto &b=bank(sound);float extra=0;
    //1269B0/126A40 use the global auxiliary word. Loop-controller and
    // permutation selection keep their separate native owner; sharing that
    // entire stream would incorrectly fold219A90's private1462B0 seed into it.
    float pitch;
    std::optional<std::uint32_t> voice_seed;
    {
      SoundLocalRandom borrowed(shared_variation_random?*shared_variation_random:0);
      auto &variation=shared_variation_random?borrowed:random;
      struct Publish {
        std::uint32_t *owner;SoundLocalRandom &value;
        ~Publish(){if(owner)*owner=value.seed;}
      } publish{shared_variation_random,variation};
      if(!looping){
        extra=source_sound_gain(b.definition,variation)+source_sound_control_gain(b.definition,control);
        if(gain_db+extra+class_gain.at(b.sound_class)<=-64.f)return 0;
      }
      pitch=float(source_sound_pitch(b.definition,variation));
      //126000 retains the global word immediately after pitch as this
      // impulse voice's independent subsequent permutation stream.
      if(shared_variation_random && !looping)voice_seed=variation.seed;
    }
    const auto controlled_pitch=source_sound_control_pitch(b.definition,pitch,control);
    const auto range=looping?0:source_sound_pitch_range(b.definition.pitch_ranges,-1,
        controlled_pitch);
    if(range<0)throw FormatError("Authored sound has no selectable pitch range");
    // Original12A482..12A4A8 retains caller-adjusted pitch for playback,
    // not just range selection.12A551..12A563 subtracts the selected sample
    // range's signed base pitch before the actual driver conversion. Ambient
    // caller-control ownership remains separate; retain its current input.
    const Bytes interval(b.definition.pitch_ranges.at(unsigned(range)).interval);
    const auto center=std::bit_cast<std::int16_t>(interval.u16(0));
    const float packet_pitch=(looping?pitch:controlled_pitch)-float(center);
    const auto driver_pitch=source_sound_driver_pitch(b.rate_index,std::bit_cast<std::uint32_t>(packet_pitch),constants,{0x027f,0x1f80});
    const auto relative=std::bit_cast<float>(source_sound_relative_rate(driver_pitch,b.rate_index,constants,{0x027f,0x1f80}));
    Voice v{0,sound,owner,loop_tag,view,track,kind,gain_db+extra,relative,ambient_gain,b.sound_class,looping,std::move(completion)};
    v.position=position;
    v.pitch_range=unsigned(range);
    v.source_pitch_bits=std::bit_cast<std::uint32_t>(packet_pitch);
    v.permutation_seed=voice_seed;
    v.id=platform.create_voice(owner==none?sound:owner,parameters(v));
    try{packet(v,looping);}catch(...){platform.flush(v.id);
      while(auto c=platform.completion(v.id))platform.acknowledge(v.id,c->submission);
      platform.destroy_voice(v.id);throw;}
    const auto id=v.id;voices.emplace(id,std::move(v));return id;
  }
  void erase(std::uint64_t id,std::optional<SdlSoundCompletion> completed={}){
    platform.flush(id);
    while(auto c=platform.completion(id)){
      completed=*c;platform.acknowledge(id,c->submission);
    }
    auto &voice=voices.at(id);
    if(voice.completion && !completed)
      throw FormatError("Native sound callback has no accounted packet");
    platform.destroy_voice(id);
    auto owner=std::move(voice.completion);voices.erase(id);
    if(owner)retired.push_back({std::move(owner),*completed});
  }
  void complete(){
    auto pending=std::exchange(retired,{});
    std::exception_ptr failure;
    for(auto &completion:pending)try{completion.owner(completion.packet);}
      catch(...){if(!failure)failure=std::current_exception();}
    if(failure)std::rethrow_exception(failure);
  }
};

MultiplayerAudio::MultiplayerAudio(const Cache &cache,std::uint32_t seed,
    const SdlSoundPlatformConfiguration &configuration,MultiplayerAudioObserver observer,
    std::uint32_t *shared):
    state_(std::make_unique<State>(cache,seed,configuration,std::move(observer),shared)){}
MultiplayerAudio::~MultiplayerAudio(){state_->closing=true;try{stop();}catch(...){} }
void MultiplayerAudio::prepare(std::uint32_t sound){if(sound!=none)(void)state_->bank(sound);}
void MultiplayerAudio::first_person(unsigned view,const FirstPersonViewBinding &binding,const FirstPersonAnimationEvent &event){
  if(event.type!=1 || event.tag==none || binding.unit==none || binding.hands[0].weapon==none ||
      (binding.hands[0].flags&6)!=6)return;
  impulse(event.tag,1,MultiplayerAudioKind::FirstPerson,binding.hands[0].weapon,view);
}
void MultiplayerAudio::impulse(std::uint32_t sound,float gain,MultiplayerAudioKind kind,std::uint32_t owner,unsigned view){
  (void)play_impulse(sound,gain,kind,owner,view);
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::play_impulse(std::uint32_t sound,float gain,
    MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,Completion completion){
  if(sound==none)return std::nullopt;
  if(!std::isfinite(gain)||gain<0)throw FormatError("Invalid native sound caller gain");
  const auto voice=state_->play(sound,0,kind,owner,view,false,none,0,1,std::move(completion),gain);
  return voice?std::optional<Voice>(voice):std::nullopt;
}
void MultiplayerAudio::listeners(std::span<const Vec3> positions){
  if(positions.size()>4)throw FormatError("World audio exceeds original local listener count");
  for(const auto &p:positions)if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
    throw FormatError("World audio listener has nonfinite position");
  state_->listeners.assign(positions.begin(),positions.end());
  for(const auto &[id,v]:state_->voices)if(v.position)state_->platform.parameters(id,state_->parameters(v));
}
void MultiplayerAudio::impulse_at(std::uint32_t sound,Vec3 position,float control,
    MultiplayerAudioKind kind,std::uint32_t owner){
  (void)play_impulse_at(sound,position,control,kind,owner);
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::play_impulse_at(std::uint32_t sound,Vec3 position,float control,
    MultiplayerAudioKind kind,std::uint32_t owner){
  if(sound==none)return {};
  if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z)||!std::isfinite(control)||control<0)
    throw FormatError("Invalid world audio emitter or caller control");
  const auto id=state_->play(sound,0,kind,owner,0,false,none,0,1,{},control,position);
  return id?std::optional<Voice>(id):std::nullopt;
}
std::vector<MultiplayerAudio::Voice> MultiplayerAudio::interface_loop(std::uint32_t tag,std::uint32_t owner,unsigned view) {
  const auto definition=state_->cache.load_looping_sound(tag);
  if(definition.flags&2u)throw FormatError("Interface loop requires ordinary retained looping sound");
  std::vector<Voice> voices;
  try {
    for(unsigned n=0;n<definition.tracks.size();++n) {
      const auto &track=definition.tracks[n];
      if(track.start!=none || track.end!=none || track.fade_in!=0)
        throw FormatError("Interface loop requires additional authored start/end/fade-in owner");
      if(track.loop!=none)voices.push_back(state_->play(track.loop,track.gain_db,
          MultiplayerAudioKind::Shield,owner,view,true,tag,n));
    }
  }catch(...) {for(const auto id:voices)stop_voice(id);throw;}
  return voices;
}
void MultiplayerAudio::stop_voice(Voice id) {
  if(auto found=state_->voices.find(id);found!=state_->voices.end()) {
    auto &v=found->second;
    if(v.fade)return;
    if(v.loop && v.loop_tag!=none) {
      const auto track=state_->cache.load_looping_sound(v.loop_tag).tracks.at(v.track);
      if((track.flags&2u) && track.fade_out>0) {
        //21A250 phase2 retains the authored track fade.12A810 chooses the
        //playback curve from sound-class bit10 and uses the sound clock.
        const auto &bank=state_->bank(v.sound,v.pitch_range);
        const auto curve=std::uint16_t(!(Bytes(bank.definition.sound_class).u16(10)&0x400u));
        v.fade=SoundPlaybackFade{curve,v.fade_db,std::int32_t(track.fade_out*1000.f),-1,true};
        return;
      }
    }
    state_->erase(id);
  }
  state_->complete();
}
bool MultiplayerAudio::voice_active(Voice id) const {return state_->voices.contains(id);}
void MultiplayerAudio::ambient(std::span<const MultiplayerAmbientTarget> targets,float elapsed){
  if(!std::isfinite(elapsed)||elapsed<0)throw FormatError("Invalid ambient elapsed time");
  std::set<std::uint32_t> selected;
  for(const auto &target:targets){selected.insert(target.sound);auto found=state_->ambience.find(target.sound);
    if(found==state_->ambience.end()){
      const auto definition=state_->cache.load_looping_sound(target.sound);
      if(definition.flags&2)throw FormatError("One-shot ambient collection requires its original owner");
      State::Ambient a;a.gain=target.gain;a.linear_db=sound_db_to_linear(-64);a.target=target;
      try{for(unsigned i=0;i<definition.tracks.size();++i){const auto &track=definition.tracks[i];
        if(track.start!=none || track.end!=none)
          throw FormatError("Ambient start/end transition owner is unavailable");
        // Normal lane only. Original21A250's phase0/alternate0 traversal
        // selects track+20, or NONE. Populated alternate rain does not admit
        // a voice without the separate actual alternate/weather producer.
        // Unchanged Base/1.1/1.5 slices cover every resident track on23 maps.
        if(track.loop==none)continue;
        a.voices.push_back(state_->play(track.loop,track.gain_db,MultiplayerAudioKind::Ambient,none,0,true,target.sound,i,a.gain*a.linear_db));}}
      catch(...){for(auto v:a.voices)state_->erase(v);throw;}
      found=state_->ambience.emplace(target.sound,std::move(a)).first;
    }
    found->second.target=target;
  }
  for(auto it=state_->ambience.begin();it!=state_->ambience.end();){auto &a=it->second;
    const float step=elapsed/a.target.fade_seconds;
    const bool retain=selected.contains(it->first);const float target_db=retain?sound_db_to_linear(a.target.gain_db):0;
    a.linear_db+=pin(target_db-a.linear_db,-step,step);
    a.gain+=pin((retain?a.target.gain:0)-a.gain,-step,step);
    if(!retain&&std::abs(a.linear_db)<.0001f){for(auto v:a.voices)state_->erase(v);it=state_->ambience.erase(it);continue;}
    for(const auto id:a.voices){auto &v=state_->voices.at(id);v.ambient_gain=a.gain*a.linear_db;state_->platform.parameters(id,state_->parameters(v));}
    ++it;
  }
}
std::string MultiplayerAudio::speaker_layout(unsigned layout){return state_->platform.speaker_layout(layout);}
void MultiplayerAudio::master_volume(float gain){
  if(!std::isfinite(gain)||gain<0||gain>1)throw FormatError("Invalid native master volume");
  state_->master=gain;
  for(const auto &[id,v]:state_->voices)state_->platform.parameters(id,state_->parameters(v));
}
void MultiplayerAudio::class_gains(const std::array<float,sound_class_count> &gains){
  for(const auto x:gains)if(!std::isfinite(x)||x<-64||x>0)throw FormatError("Invalid native source class gain");
  state_->class_gain=gains;for(const auto &[id,v]:state_->voices)state_->platform.parameters(id,state_->parameters(v));
}
void MultiplayerAudio::update(){
  const auto time=std::bit_cast<std::int32_t>(std::uint32_t(state_->platform.rendered_frames()/48));
  for(auto it=state_->voices.begin();it!=state_->voices.end();){auto &v=it->second;
    if(v.fade) {
      v.fade_db=source_sound_playback_fade(*v.fade,time);
      const auto elapsed=std::bit_cast<std::int32_t>(std::uint32_t(time)-std::uint32_t(v.fade->start));
      if(v.fade_db+v.gain_db<=-64.f || elapsed>=0) {
        const auto id=v.id;++it;state_->erase(id);continue;
      }
      state_->platform.parameters(v.id,state_->parameters(v));
    }
    bool completed=false,failed=false;
    std::optional<SdlSoundCompletion> packet;
    while(auto c=state_->platform.completion(v.id)){completed=true;packet=*c;failed|=c->status!=SdlSoundCompletionStatus::Success;state_->platform.acknowledge(v.id,c->submission);}
    if(completed){if(v.loop&&!failed){state_->packet(v,true);++it;}
      else{const auto id=v.id;++it;state_->erase(id,packet);}}
    else ++it;
  }
  state_->complete();
}
void MultiplayerAudio::stop(){if(state_){while(!state_->voices.empty())state_->erase(state_->voices.begin()->first);state_->ambience.clear();state_->complete();}}
std::size_t MultiplayerAudio::voices()const{return state_->voices.size();}
std::vector<float> MultiplayerAudio::render(std::size_t frames){return state_->platform.render(frames);}
std::uint64_t MultiplayerAudio::mixed_frames()const{return state_->platform.rendered_frames();}
} // namespace halo2
