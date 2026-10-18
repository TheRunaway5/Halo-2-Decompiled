#include "halo2/multiplayer_audio.hpp"
#include "halo2/menu_music.hpp"
#include "halo2/sound_driver_parameters.hpp"
#include "halo2/sound_variation.hpp"
#include "halo2/sound_spatial.hpp"
#include "halo2/sound_fade.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/mcc_script_sound_world_access.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <atomic>
#include <thread>
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
struct ScriptAudioEpoch {
  std::atomic_bool alive{true};
  const std::thread::id thread=std::this_thread::get_id();
};
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
  const auto &bsp=bsps_[index];const Bytes file(cache_bytes()),b(file.slice(bsp.file_offset,bsp.size));
  
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

struct MultiplayerPreparedScriptImpulseTu15::Impl {
  enum class Status {Pending,CancelRequested,Consumed};
  std::atomic<Status> status{Status::Pending};
  std::shared_ptr<ScriptAudioEpoch> audio_epoch;
  std::shared_ptr<SourceScriptSoundClockNative64Tu15> clock;
  std::optional<SourceScriptSoundClockAdmission64Tu15> admission;
  std::uint32_t sound=none;
  SourceScriptSoundClockSelectionTu15 selection;
};
MultiplayerPreparedScriptImpulseTu15::MultiplayerPreparedScriptImpulseTu15(std::shared_ptr<Impl>p):impl_(std::move(p)){}
void MultiplayerPreparedScriptImpulseTu15::request_cancel()noexcept{
  if(impl_){auto expected=Impl::Status::Pending;impl_->status.compare_exchange_strong(expected,Impl::Status::CancelRequested);}
}
MultiplayerPreparedScriptImpulseTu15::~MultiplayerPreparedScriptImpulseTu15(){request_cancel();}
MultiplayerPreparedScriptImpulseTu15::MultiplayerPreparedScriptImpulseTu15(MultiplayerPreparedScriptImpulseTu15&&other)noexcept:impl_(std::move(other.impl_)){}
MultiplayerPreparedScriptImpulseTu15&MultiplayerPreparedScriptImpulseTu15::operator=(MultiplayerPreparedScriptImpulseTu15&&other)noexcept{
  if(this!=&other){request_cancel();impl_=std::move(other.impl_);}return *this;
}
std::uint32_t MultiplayerPreparedScriptImpulseTu15::sound()const{
  if(!impl_)throw FormatError("Moved native script sound preparation");
  return impl_->sound;
}
SourceScriptSoundClockSelectionTu15 MultiplayerPreparedScriptImpulseTu15::selection()const{
  if(!impl_)throw FormatError("Moved native script sound preparation");
  return impl_->selection;
}

struct MultiplayerAudio::State {
 struct Core {
  struct Bank {
    SoundVariationDefinition definition;
    SoundRangeState range;
    LoopingSoundSelection loop;
    std::vector<std::shared_ptr<const std::vector<float>>> pcm;
    std::vector<std::uint16_t> chances;
    unsigned rate_index{},sound_class{};
  };
  struct ScriptVoiceRecord {std::uint64_t id{};bool current=false;std::function<bool()> retirement;};
  struct Voice {std::uint64_t id{};std::uint32_t sound{},owner{},loop_tag=none;unsigned view{},track{};
    MultiplayerAudioKind kind{};float gain_db{},relative_rate=1,ambient_gain=1;unsigned sound_class{};
    bool loop{};Completion completion;unsigned pitch_range{};std::uint32_t source_pitch_bits{};
    std::int8_t permutation_gain_db{};std::optional<std::uint32_t> permutation_seed;
    std::optional<Vec3> position;std::optional<SoundPlaybackFade> fade;float fade_db{};
    std::shared_ptr<ScriptVoiceRecord> script_record;
    std::optional<unsigned> forced_script_permutation;};
  struct RetiredCompletion {Completion owner;SdlSoundCompletion packet;};
  struct Ambient {float gain{},linear_db{};MultiplayerAmbientTarget target;std::vector<std::uint64_t> voices;};
  const Cache &cache;SoundLocalRandom random;
  std::shared_ptr<SdlSoundPlatform> platform_storage;SdlSoundPlatform&platform;
  MultiplayerAudioObserver observer;
  std::shared_ptr<ScriptAudioEpoch> script_epoch=std::make_shared<ScriptAudioEpoch>();
  std::atomic_bool script_bound{false};
  std::vector<std::shared_ptr<MultiplayerPreparedScriptImpulseTu15::Impl>> script_preparations;
  std::vector<std::function<void()>> script_closers;
  std::vector<std::function<bool()>> script_retirements;
  std::optional<MccSoundVariationProvider> native_metadata;
  unsigned play_depth=0,completion_depth=0;
  struct Phase {
    unsigned& depth;
    explicit Phase(unsigned& value):depth(value){++depth;}
    ~Phase(){--depth;}
  };
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
  bool closing=false,paused=false;
  Core(const Cache &c,std::uint32_t seed,const SdlSoundPlatformConfiguration &configuration,MultiplayerAudioObserver o,
      std::uint32_t *shared):
      cache(c),random(seed),platform_storage(std::make_shared<SdlSoundPlatform>(configuration)),platform(*platform_storage),observer(std::move(o)),shared_variation_random(shared){
    if(!cache.original_data()->sound_driver_parameter_constants)throw FormatError("Authored multiplayer audio requires selected original driver constants");
    constants=*cache.original_data()->sound_driver_parameter_constants;
    if(!cache.original_data()->sound_create_constants)throw FormatError("Authored world audio requires selected original distance constants");
    distance_constants=*cache.original_data()->sound_create_constants;
  }
  void script_thread()const{
    if(script_epoch->thread!=std::this_thread::get_id())throw FormatError("Native script Audio used from foreign thread");
    if(!script_epoch->alive.load())throw FormatError("Native script Audio owner unavailable");
  }
  void script_producer()const{if(script_bound.load())script_thread();}
  void metadata_current()const{if(native_metadata)native_metadata->require_current(cache);}
  SoundVariationDefinition definition(std::uint32_t sound,unsigned pitch=0)const{
    return native_metadata?native_metadata->load(cache,TagHandle{sound},pitch):
        cache.load_sound_variation(sound,pitch);
  }
  void drain_script_cancellations(){
    if(script_preparations.empty())return;
    script_thread();
    using Status=MultiplayerPreparedScriptImpulseTu15::Impl::Status;
    for(auto&p:script_preparations)if(p->status.load()==Status::CancelRequested){
      if(p->admission)(void)p->clock->finish_failure(*p->admission);
      p->status.store(Status::Consumed);
    }
    std::erase_if(script_preparations,[](const auto&p){return p->status.load()==Status::Consumed;});
  }
  void close_script_bindings()noexcept{
    for(auto&p:script_preparations)p->status.store(MultiplayerPreparedScriptImpulseTu15::Impl::Status::Consumed);
    for(auto&close:script_closers)close();
    script_epoch->alive.store(false);script_preparations.clear();script_closers.clear();script_retirements.clear();
  }
  Bank &bank(std::uint32_t sound,unsigned pitch=0){
    script_producer();metadata_current();
    const auto key=std::pair{sound,pitch};
    if(auto found=banks.find(key);found!=banks.end())return found->second;
    Bank b;b.definition=definition(sound,pitch);
    b.rate_index=b.definition.sample_rate_index;b.sound_class=std::to_integer<unsigned>(b.definition.sound[2]);
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
    metadata_current();
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
    return {std::bit_cast<std::uint32_t>(v.relative_rate),volume,paused,{{0,0,output_gain},{1,1,output_gain}}};
  }
  void packet(Voice &voice,bool loop_selection,
      const MccScriptSoundWorldAccess* first_selection=nullptr,
      std::optional<std::uint32_t> expected_auxiliary_seed={}){
    auto &b=bank(voice.sound,voice.pitch_range);
    SoundLocalRandom per_voice(voice.permutation_seed.value_or(0));
    auto &selection=voice.permutation_seed?per_voice:random;
    const auto before=selection.seed;
    unsigned p;auto after=selection.seed;
    if(first_selection && !voice.forced_script_permutation){
      if(loop_selection || !voice.script_record || !voice.permutation_seed ||
         !expected_auxiliary_seed || !shared_variation_random ||
         *shared_variation_random!=*voice.permutation_seed || voice.pitch_range>UINT16_MAX)
        throw FormatError("Native first permutation requires its successful prepared impulse phase");
      const auto result=first_selection->first_permutation_after_pitch(cache,
          {TagHandle{voice.sound},std::uint16_t(voice.pitch_range),
           *expected_auxiliary_seed,*voice.permutation_seed});
      if(result.relative_permutation<0 ||
         unsigned(result.relative_permutation)>=b.definition.permutations.size())
        throw FormatError("Native first permutation returned an unsupported packet ordinal");
      p=unsigned(result.relative_permutation);after=result.auxiliary_seed_after;
      *shared_variation_random=after;
      // Actual128C38 captured the private subsequent-packet seed before
      // ordinary21FA80. Initial shared selection must not advance that copy
      // or substitute the cached Bank range for the actual World owner.
    }else{
      p=voice.forced_script_permutation?*voice.forced_script_permutation:
          unsigned(loop_selection?source_looping_sound_permutation(b.chances,b.loop,selection.seed):
          source_sound_permutation(b.definition,b.range,selection));
      after=selection.seed;
      if(voice.permutation_seed)voice.permutation_seed=selection.seed;
    }
    voice.permutation_gain_db=std::bit_cast<std::int8_t>(b.definition.permutations.at(p)[4]);
    platform.parameters(voice.id,parameters(voice));
    const auto &pcm=b.pcm.at(p);const auto result=platform.submit(voice.id,{pcm,0,pcm->size()/2,next_packet++});
    if(result!=SdlSoundPlatform::Submission::Accepted)throw FormatError("Native sound packet queue unexpectedly full");
    if(observer)observer({voice.kind,voice.sound,voice.owner,voice.view,voice.pitch_range,p,voice.id,pcm->size()/2,before,after,voice.source_pitch_bits,voice.position});
    selection.stores.clear();
  }
  std::uint64_t play(std::uint32_t sound,float gain_db,MultiplayerAudioKind kind,
      std::uint32_t owner,unsigned view,bool looping=false,std::uint32_t loop_tag=none,unsigned track=0,float ambient_gain=1,
      Completion completion={},float control=1,std::optional<Vec3> position={},
      std::optional<std::uint64_t> presentation_frame={},
      std::optional<SourceScriptSoundClockSelectionTu15> script_selection={},
      std::shared_ptr<ScriptVoiceRecord> script_record={},
      const MccScriptSoundWorldAccess* first_selection=nullptr){
    script_producer();
    if(closing)throw FormatError("Native audio is being destroyed");
    Phase phase(play_depth);
    drain_script_cancellations();
    const auto expected_auxiliary_seed=first_selection && shared_variation_random?
        std::optional<std::uint32_t>(*shared_variation_random):std::nullopt;
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
      // TU1.5 128C38 retains the global word immediately after pitch as
      // this impulse voice's independent subsequent permutation stream.
      if(shared_variation_random && !looping)voice_seed=variation.seed;
    }
    const auto controlled_pitch=source_sound_control_pitch(b.definition,pitch,control);
    const bool forced=script_selection && script_selection->pitch_range!=0xff && script_selection->permutation!=0xff;
    const auto range=forced?int(script_selection->pitch_range):(looping?0:source_sound_pitch_range(b.definition.pitch_ranges,-1,controlled_pitch));
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
    v.script_record=std::move(script_record);
    if(forced)v.forced_script_permutation=script_selection->permutation;
    v.pitch_range=unsigned(range);
    v.source_pitch_bits=std::bit_cast<std::uint32_t>(packet_pitch);
    v.permutation_seed=voice_seed;
    v.id=presentation_frame?platform.create_voice_at(owner==none?sound:owner,parameters(v),*presentation_frame):
        platform.create_voice(owner==none?sound:owner,parameters(v));
    if(v.script_record){v.script_record->id=v.id;v.script_record->current=true;}
    const auto id=v.id;const auto record=v.script_record;
    try{
      packet(v,looping,first_selection,expected_auxiliary_seed);if(closing)throw FormatError("Native Audio closed during packet observer");
      const auto[at,inserted]=voices.emplace(id,std::move(v));
      (void)at;if(!inserted)throw FormatError("Native Audio voice publication identity collision");
      return id;
    }catch(...){platform.flush(id);
      while(auto c=platform.completion(id))platform.acknowledge(id,c->submission);
      platform.destroy_voice(id);if(record)record->current=false;throw;}
  }
  void erase(std::uint64_t id,std::optional<SdlSoundCompletion> completed={}){
    script_producer();
    auto &voice=voices.at(id);
    if(voice.script_record && voice.script_record->retirement)script_retirements.push_back(voice.script_record->retirement);
    platform.flush(id);
    while(auto c=platform.completion(id)){
      completed=*c;platform.acknowledge(id,c->submission);
    }
    if(voice.completion && !completed)
      throw FormatError("Native sound callback has no accounted packet");
    platform.destroy_voice(id);
    if(voice.script_record)voice.script_record->current=false;
    auto owner=std::move(voice.completion);voices.erase(id);
    if(owner)retired.push_back({std::move(owner),*completed});
  }
  void complete(){
    script_producer();
    Phase phase(completion_depth);
    auto pending=std::exchange(retired,{});
    std::exception_ptr failure;
    auto script_pending=std::exchange(script_retirements,{});
    for(auto&notification:script_pending)try{if(!notification())script_retirements.push_back(std::move(notification));}
      catch(...){script_retirements.push_back(std::move(notification));if(!failure)failure=std::current_exception();}
    for(auto &completion:pending)try{completion.owner(completion.packet);}
      catch(...){if(!failure)failure=std::current_exception();}
    if(failure)std::rethrow_exception(failure);
  }
 };
 std::shared_ptr<Core> core;
 State(const Cache&cache,std::uint32_t seed,const SdlSoundPlatformConfiguration&configuration,
       MultiplayerAudioObserver observer,std::uint32_t*shared):
       core(std::make_shared<Core>(cache,seed,configuration,std::move(observer),shared)){}
};

MultiplayerAudio::MultiplayerAudio(const Cache &cache,std::uint32_t seed,
    const SdlSoundPlatformConfiguration &configuration,MultiplayerAudioObserver observer,
    std::uint32_t *shared):
    state_(std::make_unique<State>(cache,seed,configuration,std::move(observer),shared)){}
MultiplayerAudio::~MultiplayerAudio(){auto state=state_->core;state->script_producer();state->closing=true;try{stop();}catch(...){}state->close_script_bindings();}
void MultiplayerAudio::bind_native_sound_metadata(const MccSoundVariationProvider&input){
  auto state=state_->core;state->script_producer();state->script_thread();
  input.require_current(state->cache);
  using Status=MultiplayerPreparedScriptImpulseTu15::Impl::Status;
  if(state->closing || state->play_depth || state->completion_depth ||
     !state->voices.empty() || !state->ambience.empty() || !state->retired.empty() ||
     !state->script_retirements.empty() ||
     std::ranges::any_of(state->script_preparations,[](const auto&p){return p->status.load()!=Status::Consumed;}))
    throw FormatError("Native sound metadata binding requires quiescent Audio");
  // Allocate before invalidating old clocks or changing cached definitions.
  const auto provider=input;auto epoch=std::make_shared<ScriptAudioEpoch>();
  state->close_script_bindings();state->banks.clear();
  state->native_metadata=provider;state->script_epoch=std::move(epoch);state->script_bound.store(true);
}
void MultiplayerAudio::prepare(std::uint32_t sound){auto state=state_->core;state->script_producer();if(sound!=none)(void)state->bank(sound);}
void MultiplayerAudio::first_person(unsigned view,const FirstPersonViewBinding &binding,const FirstPersonAnimationEvent &event){
  if(event.type!=1 || event.tag==none || binding.unit==none || binding.hands[0].weapon==none ||
      (binding.hands[0].flags&6)!=6)return;
  impulse(event.tag,1,MultiplayerAudioKind::FirstPerson,binding.hands[0].weapon,view);
}
void MultiplayerAudio::impulse(std::uint32_t sound,float gain,MultiplayerAudioKind kind,std::uint32_t owner,unsigned view){
  (void)play_impulse(sound,gain,kind,owner,view);
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::play_impulse(std::uint32_t sound,float gain,
    MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,Completion completion){auto state=state_->core;state->script_producer();
  if(sound==none)return std::nullopt;
  if(!std::isfinite(gain)||gain<0)throw FormatError("Invalid native sound caller gain");
  const auto voice=state->play(sound,0,kind,owner,view,false,none,0,1,std::move(completion),gain);
  return voice?std::optional<Voice>(voice):std::nullopt;
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::schedule_impulse(std::uint32_t sound,
    std::uint64_t frame,float control,MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,Completion completion){auto state=state_->core;state->script_producer();
  if(sound==none)return std::nullopt;
  if(!std::isfinite(control)||control<0)throw FormatError("Invalid native sound caller gain");
  const auto voice=state->play(sound,0,kind,owner,view,false,none,0,1,std::move(completion),control,{},frame);
  return voice?std::optional<Voice>(voice):std::nullopt;
}
std::shared_ptr<SourceScriptSoundClockNative64Tu15> MultiplayerAudio::make_script_sound_clock(){
  auto state=state_->core;state->script_producer();state->script_thread();
  if(state->closing)throw FormatError("Native Audio is being destroyed");
  auto clock=std::make_shared<SourceScriptSoundClockNative64Tu15>();const auto epoch=state->script_epoch;
  clock->bind_audio_owner(epoch,[epoch]{return epoch->alive.load();});
  state->script_closers.push_back(clock->close_audio_binding());state->script_bound.store(true);return clock;
}
std::shared_ptr<SourceScriptSoundClockNative64Tu15> MultiplayerAudio::make_script_sound_clock(
    const SourceScriptSoundClockRowsTu15&input){
  auto state=state_->core;state->script_producer();state->script_thread();
  state->metadata_current();const auto rows=input;
  using Status=MultiplayerPreparedScriptImpulseTu15::Impl::Status;
  if(state->closing || state->play_depth || state->completion_depth ||
     !state->voices.empty() || !state->ambience.empty() || !state->retired.empty() ||
     !state->script_retirements.empty() || !state->script_closers.empty() ||
     std::ranges::any_of(state->script_preparations,[](const auto&p){return p->status.load()!=Status::Consumed;}))
    throw FormatError("Restored script metadata requires quiescent Audio with a fresh clock epoch");
  // Authenticate the complete salted sound identities before registering an
  // imported clock. Selector/predicate bytes may retain a partial original
  // admission; the producer qualifies that state rather than normalizing it.
  for(const auto&row:rows)if(row.sound!=none){
    const auto definition=state->definition(row.sound);
    if(definition.datum!=row.sound)throw FormatError("Restored script metadata has a foreign sound");
  }
  auto clock=std::make_shared<SourceScriptSoundClockNative64Tu15>();
  clock->initialize_metadata(rows);const auto epoch=state->script_epoch;
  clock->bind_audio_owner(epoch,[epoch]{return epoch->alive.load();});
  state->script_closers.push_back(clock->close_audio_binding());state->script_bound.store(true);return clock;
}
std::optional<MultiplayerPreparedScriptImpulseTu15> MultiplayerAudio::prepare_script_impulse(
    const std::shared_ptr<SourceScriptSoundClockNative64Tu15>&input_clock,
    std::uint32_t sound,const SourceScriptSoundClockPolicyTu15&policy,
    const SourceScriptSoundClockNative64Tu15::StopRequest&stop,
    const SoundDriverFloatingPointContext&fp){
  auto state=state_->core;state->script_producer();const auto clock=input_clock;state->script_thread();
  if(state->closing || !clock || !clock->audio_owner_is(state->script_epoch))
    throw FormatError("Native script preparation requires this actual Audio owner");
  state->drain_script_cancellations();if(sound==none)return {};
  // This is typed resident metadata only. PCM decode/backend admission happens
  // after the committed clock/selection phase in play_prepared_script_impulse.
  const auto definition=state->definition(sound);
  if(definition.datum!=sound)throw FormatError("Native script sound full datum differs");
  auto preparation=std::make_shared<MultiplayerPreparedScriptImpulseTu15::Impl>();
  preparation->audio_epoch=state->script_epoch;preparation->clock=clock;preparation->sound=sound;
  state->script_preparations.push_back(preparation);
  try{
    preparation->admission=clock->begin(sound,policy,stop,fp);
    if(!preparation->admission){preparation->status.store(MultiplayerPreparedScriptImpulseTu15::Impl::Status::Consumed);return {};}
    preparation->selection=preparation->admission->selection();
    const auto pair=preparation->selection;
    if(pair.pitch_range!=0xff && pair.permutation!=0xff &&
       (definition.pitch_ranges.size()!=1 || pair.pitch_range!=0 || pair.permutation>=definition.permutations.size()))
      throw FormatError("Native script selector outside actual original213410 namespace");
    return MultiplayerPreparedScriptImpulseTu15(std::move(preparation));
  }catch(...){
    if(preparation->admission && state->script_epoch->alive.load())(void)clock->finish_failure(*preparation->admission);
    preparation->status.store(MultiplayerPreparedScriptImpulseTu15::Impl::Status::Consumed);throw;
  }
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::play_prepared_script_impulse(
    MultiplayerPreparedScriptImpulseTu15&input,float control,MultiplayerAudioKind kind,
    std::uint32_t owner,unsigned view,std::optional<Vec3>position,
    std::optional<std::uint64_t>frame,Completion completion){
  return play_prepared_script_impulse_impl(input,nullptr,control,kind,owner,view,position,frame,std::move(completion));
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::play_prepared_script_impulse(
    MultiplayerPreparedScriptImpulseTu15&input,const MccScriptSoundWorldAccess&access,
    float control,MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,
    std::optional<Vec3>position,std::optional<std::uint64_t>frame,Completion completion){
  const auto retained=access;
  return play_prepared_script_impulse_impl(input,&retained,control,kind,owner,view,position,frame,std::move(completion));
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::play_prepared_script_impulse_impl(
    MultiplayerPreparedScriptImpulseTu15&input,const MccScriptSoundWorldAccess*first_selection,
    float control,MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,
    std::optional<Vec3>position,std::optional<std::uint64_t>frame,Completion completion){
  auto state=state_->core;state->script_producer();state->script_thread();const auto preparation=input.impl_;
  using Status=MultiplayerPreparedScriptImpulseTu15::Impl::Status;
  if(!preparation || preparation->audio_epoch!=state->script_epoch || !preparation->admission)
    throw FormatError("Native script preparation belongs to another Audio owner");
  state->metadata_current();
  if(first_selection){
    if(!state->native_metadata || !state->shared_variation_random)
      throw FormatError("Native first permutation requires converted metadata and shared creation RNG");
    first_selection->require_metadata_owner(state->cache,*state->native_metadata);
  }
  // Before resource decode, gain/pitch RNG, observer or backend calls.
  if(!preparation->clock->admission_pending(*preparation->admission))
    throw FormatError("Native script preparation has stale clock generation");
  auto expected=Status::Pending;
  if(!preparation->status.compare_exchange_strong(expected,Status::Consumed)){
    state->drain_script_cancellations();throw FormatError("Native script preparation already consumed or canceled");
  }
  std::optional<Voice> id;std::optional<SourceScriptSoundClockVoice64Tu15> receipt;
  try{
    if(!std::isfinite(control)||control<0 ||
       (position && (!std::isfinite(position->x)||!std::isfinite(position->y)||!std::isfinite(position->z))))
      throw FormatError("Invalid prepared native script sound input");
    auto record=std::make_shared<State::Core::ScriptVoiceRecord>();
    const auto voice=state->play(preparation->sound,0,kind,owner,view,false,none,0,1,
        std::move(completion),control,position,frame,preparation->selection,record,first_selection);
    if(!voice){(void)preparation->clock->finish_failure(*preparation->admission);return {};}
    id=voice;
    const auto found=state->voices.find(voice);
    if(found==state->voices.end() || found->second.script_record!=record || !record->current)
      throw FormatError("Native script voice not published by actual Audio owner");
    const auto epoch=state->script_epoch;const auto platform=state->platform_storage;
    receipt=SourceScriptSoundClockVoice64Tu15::published(epoch,platform,record,voice,[epoch,platform,record,voice]{
      if(!epoch->alive.load() || !record->current || record->id!=voice)return false;
      (void)platform->status(voice);return true;
    });
    if(!preparation->clock->finish_voice(*preparation->admission,*receipt))
      throw FormatError("Native script voice lost its actual clock admission");
    record->retirement=preparation->clock->retirement_callback(*preparation->admission,*receipt);
    return voice;
  }catch(...){
    const auto failure=std::current_exception();
    if(id && state->voices.contains(*id))try{state->erase(*id);}catch(...){}
    if(state->script_epoch->alive.load())try{
      if(receipt)(void)preparation->clock->retired(*preparation->admission,*receipt);
      (void)preparation->clock->finish_failure(*preparation->admission);
    }catch(...){}
    try{state->complete();}catch(...){}
    std::rethrow_exception(failure);
  }
}
void MultiplayerAudio::cancel_prepared_script_impulse(MultiplayerPreparedScriptImpulseTu15&input){
  auto state=state_->core;state->script_producer();state->script_thread();const auto preparation=input.impl_;
  if(!preparation || preparation->audio_epoch!=state->script_epoch)
    throw FormatError("Native script cancellation belongs to another Audio owner");
  input.request_cancel();state->drain_script_cancellations();
}
bool MultiplayerAudio::request_script_stop(const SourceScriptSoundClockVoice64Tu15&input){
  auto state=state_->core;state->script_producer();state->script_thread();const auto receipt=input;
  if(state->closing || !receipt.audio_owner_is(state->script_epoch,state->platform_storage))
    throw FormatError("Native script stop requires this actual Audio owner");
  const auto found=state->voices.find(receipt.id());
  if(found==state->voices.end())return false;
  auto&voice=found->second;
  if(!voice.script_record || !voice.script_record->current ||
     !receipt.voice_record_is(voice.script_record))
    throw FormatError("Native script stop lost actual published voice record");
  const auto time=std::bit_cast<std::int32_t>(std::uint32_t(state->platform.rendered_frames()/48));
  // No observer/backend callback runs while the actual record is borrowed.
  // Sample any active fade at this time, including a repeated stop request.
  auto fade=voice.fade.value_or(SoundPlaybackFade{0,0,0,0,false});
  source_sound_request_stop_tu15(fade,time);voice.fade=fade;
  return true;
}
void MultiplayerAudio::listeners(std::span<const Vec3> positions){auto state=state_->core;state->script_producer();
  state->metadata_current();
  if(positions.size()>4)throw FormatError("World audio exceeds original local listener count");
  for(const auto &p:positions)if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
    throw FormatError("World audio listener has nonfinite position");
  state->listeners.assign(positions.begin(),positions.end());
  for(const auto &[id,v]:state->voices)if(v.position)state->platform.parameters(id,state->parameters(v));
}
void MultiplayerAudio::emitter(Voice id,Vec3 position){auto state=state_->core;state->script_producer();
  state->metadata_current();
  if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z))
    throw FormatError("World audio emitter has nonfinite position");
  auto &voice=state->voices.at(id);
  if(!voice.position)throw FormatError("Centered audio voice cannot acquire a world emitter");
  voice.position=position;
  state->platform.parameters(id,state->parameters(voice));
}
void MultiplayerAudio::impulse_at(std::uint32_t sound,Vec3 position,float control,
    MultiplayerAudioKind kind,std::uint32_t owner){
  (void)play_impulse_at(sound,position,control,kind,owner);
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::play_impulse_at(std::uint32_t sound,Vec3 position,float control,
    MultiplayerAudioKind kind,std::uint32_t owner){auto state=state_->core;state->script_producer();
  if(sound==none)return {};
  if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z)||!std::isfinite(control)||control<0)
    throw FormatError("Invalid world audio emitter or caller control");
  const auto id=state->play(sound,0,kind,owner,0,false,none,0,1,{},control,position);
  return id?std::optional<Voice>(id):std::nullopt;
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::schedule_impulse_at(std::uint32_t sound,Vec3 position,
    std::uint64_t frame,float control,MultiplayerAudioKind kind,std::uint32_t owner){auto state=state_->core;state->script_producer();
  if(sound==none)return {};
  if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z)||!std::isfinite(control)||control<0)
    throw FormatError("Invalid world audio emitter or caller control");
  const auto id=state->play(sound,0,kind,owner,0,false,none,0,1,{},control,position,frame);
  return id?std::optional<Voice>(id):std::nullopt;
}
std::vector<MultiplayerAudio::Voice> MultiplayerAudio::interface_loop(std::uint32_t tag,std::uint32_t owner,unsigned view) {auto state=state_->core;state->script_producer();
  state->metadata_current();
  const auto definition=state->cache.load_looping_sound(tag);
  if(definition.flags&2u)throw FormatError("Interface loop requires ordinary retained looping sound");
  std::vector<Voice> voices;
  try {
    for(unsigned n=0;n<definition.tracks.size();++n) {
      const auto &track=definition.tracks[n];
      if(track.start!=none || track.end!=none || track.fade_in!=0)
        throw FormatError("Interface loop requires additional authored start/end/fade-in owner");
      if(track.loop!=none)voices.push_back(state->play(track.loop,track.gain_db,
          MultiplayerAudioKind::Shield,owner,view,true,tag,n));
    }
  }catch(...) {for(const auto id:voices)stop_voice(id);throw;}
  return voices;
}
void MultiplayerAudio::stop_voice(Voice id) {auto state=state_->core;state->script_producer();
  if(auto found=state->voices.find(id);found!=state->voices.end()) {
    auto &v=found->second;
    if(v.fade)return;
    if(v.loop && v.loop_tag!=none) {
      const auto track=state->cache.load_looping_sound(v.loop_tag).tracks.at(v.track);
      if((track.flags&2u) && track.fade_out>0) {
        //21A250 phase2 retains the authored track fade.12A810 chooses the
        //playback curve from sound-class bit10 and uses the sound clock.
        const auto &bank=state->bank(v.sound,v.pitch_range);
        const auto curve=std::uint16_t(!(Bytes(bank.definition.sound_class).u16(10)&0x400u));
        v.fade=SoundPlaybackFade{curve,v.fade_db,std::int32_t(track.fade_out*1000.f),-1,true};
        return;
      }
    }
    state->erase(id);
  }
  state->complete();
}
bool MultiplayerAudio::voice_active(Voice id) const {auto state=state_->core;state->script_producer();return state->voices.contains(id);}
void MultiplayerAudio::ambient(std::span<const MultiplayerAmbientTarget> targets,float elapsed){auto state=state_->core;state->script_producer();
  state->metadata_current();
  if(!std::isfinite(elapsed)||elapsed<0)throw FormatError("Invalid ambient elapsed time");
  std::set<std::uint32_t> selected;
  for(const auto &target:targets){selected.insert(target.sound);auto found=state->ambience.find(target.sound);
    if(found==state->ambience.end()){
      const auto definition=state->cache.load_looping_sound(target.sound);
      if(definition.flags&2)throw FormatError("One-shot ambient collection requires its original owner");
      State::Core::Ambient a;a.gain=target.gain;a.linear_db=sound_db_to_linear(-64);a.target=target;
      try{for(unsigned i=0;i<definition.tracks.size();++i){const auto &track=definition.tracks[i];
        if(track.start!=none || track.end!=none)
          throw FormatError("Ambient start/end transition owner is unavailable");
        // Normal lane only. Original21A250's phase0/alternate0 traversal
        // selects track+20, or NONE. Populated alternate rain does not admit
        // a voice without the separate actual alternate/weather producer.
        // Unchanged Base/1.1/1.5 slices cover every resident track on23 maps.
        if(track.loop==none)continue;
        a.voices.push_back(state->play(track.loop,track.gain_db,MultiplayerAudioKind::Ambient,none,0,true,target.sound,i,a.gain*a.linear_db));}}
      catch(...){for(auto v:a.voices)state->erase(v);throw;}
      found=state->ambience.emplace(target.sound,std::move(a)).first;
    }
    found->second.target=target;
  }
  for(auto it=state->ambience.begin();it!=state->ambience.end();){auto &a=it->second;
    const float step=elapsed/a.target.fade_seconds;
    const bool retain=selected.contains(it->first);const float target_db=retain?sound_db_to_linear(a.target.gain_db):0;
    a.linear_db+=pin(target_db-a.linear_db,-step,step);
    a.gain+=pin((retain?a.target.gain:0)-a.gain,-step,step);
    if(!retain&&std::abs(a.linear_db)<.0001f){for(auto v:a.voices)state->erase(v);it=state->ambience.erase(it);continue;}
    for(const auto id:a.voices){auto &v=state->voices.at(id);v.ambient_gain=a.gain*a.linear_db;state->platform.parameters(id,state->parameters(v));}
    ++it;
  }
}
std::string MultiplayerAudio::speaker_layout(unsigned layout){auto state=state_->core;state->script_producer();return state->platform.speaker_layout(layout);}
void MultiplayerAudio::paused(bool paused){auto state=state_->core;state->script_producer();
  state->metadata_current();
  if(state->paused==paused)return;
  state->paused=paused;
  if(paused)state->platform.paused(true);
  for(const auto &[id,v]:state->voices)state->platform.parameters(id,state->parameters(v));
  if(!paused)state->platform.paused(false);
}
SdlSoundVoiceParameters MultiplayerAudio::voice_parameters(Voice id)const{auto state=state_->core;state->script_producer();
  return state->parameters(state->voices.at(id));
}
SdlSoundPlaybackPosition MultiplayerAudio::playback_position(Voice id)const{auto state=state_->core;state->script_producer();
  return state->platform.playback_position(id);
}
void MultiplayerAudio::master_volume(float gain){auto state=state_->core;state->script_producer();
  state->metadata_current();
  if(!std::isfinite(gain)||gain<0||gain>1)throw FormatError("Invalid native master volume");
  state->master=gain;
  for(const auto &[id,v]:state->voices)state->platform.parameters(id,state->parameters(v));
}
void MultiplayerAudio::class_gains(const std::array<float,sound_class_count> &gains){auto state=state_->core;state->script_producer();
  state->metadata_current();
  for(const auto x:gains)if(!std::isfinite(x)||x<-64||x>0)throw FormatError("Invalid native source class gain");
  state->class_gain=gains;for(const auto &[id,v]:state->voices)state->platform.parameters(id,state->parameters(v));
}
void MultiplayerAudio::update(){auto state=state_->core;state->script_producer();
  state->metadata_current();
  state->drain_script_cancellations();
  const auto time=std::bit_cast<std::int32_t>(std::uint32_t(state->platform.rendered_frames()/48));
  for(auto it=state->voices.begin();it!=state->voices.end();){auto &v=it->second;
    if(v.fade) {
      v.fade_db=source_sound_playback_fade(*v.fade,time);
      const auto elapsed=std::bit_cast<std::int32_t>(std::uint32_t(time)-std::uint32_t(v.fade->start));
      if(v.fade_db+v.gain_db<=-64.f || elapsed>=0) {
        const auto id=v.id;++it;state->erase(id);continue;
      }
      state->platform.parameters(v.id,state->parameters(v));
    }
    bool completed=false,failed=false;
    std::optional<SdlSoundCompletion> packet;
    while(auto c=state->platform.completion(v.id)){completed=true;packet=*c;failed|=c->status!=SdlSoundCompletionStatus::Success;state->platform.acknowledge(v.id,c->submission);}
    if(completed){if(v.loop&&!failed){state->packet(v,true);++it;}
      else{const auto id=v.id;++it;state->erase(id,packet);}}
    else ++it;
  }
  state->complete();
}
void MultiplayerAudio::stop(){auto state=state_->core;state->script_producer();while(!state->voices.empty())state->erase(state->voices.begin()->first);state->ambience.clear();state->complete();state->drain_script_cancellations();}
std::size_t MultiplayerAudio::voices()const{auto state=state_->core;state->script_producer();return state->voices.size();}
std::vector<float> MultiplayerAudio::render(std::size_t frames){auto state=state_->core;state->script_producer();state->metadata_current();return state->platform.render(frames);}
std::uint64_t MultiplayerAudio::mixed_frames()const{auto state=state_->core;state->script_producer();return state->platform.rendered_frames();}
std::uint64_t MultiplayerAudio::presentation_frames()const{auto state=state_->core;state->script_producer();return state->platform.presentation_frames();}
} // namespace halo2
