#include "halo2/multiplayer_audio.hpp"
#include "halo2/menu_music.hpp"
#include "halo2/sound_driver_parameters.hpp"
#include "halo2/sound_variation.hpp"
#include "halo2/sound_spatial.hpp"
#include "halo2/sound_fade.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/mcc_script_sound_world_access.hpp"
#include "halo2/effect_audio_auxiliary.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_loop_track_control_tu15.hpp"
#include "halo2/source_loop_detail_control_tu15.hpp"
#include "halo2/source_loop_phased_output_tu15.hpp"
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
struct ScriptCancellationDomain {
  enum class Life {Live,Prospective,Retired};
  std::atomic<Life> life{Life::Live};
  std::atomic<std::uint64_t> requests{0}; // Low bit is a publication reservation.
  std::atomic<unsigned> active{0};
  explicit ScriptCancellationDomain(Life value=Life::Live):life(value){}
  static std::uint64_t next(std::uint64_t value)noexcept{
    const auto limit=UINT64_MAX-(value&1u?0u:1u);return value>=limit-2?limit:value+2;
  }
  void note()noexcept{
    auto value=requests.load();while(!requests.compare_exchange_weak(value,next(value))){}
  }
  bool reserve(std::uint64_t value)noexcept{
    if((value&1u) || value>=UINT64_MAX-2 || !requests.compare_exchange_strong(value,value|1u))return false;
    if(active.load()){release();return false;}return true;
  }
  void release()noexcept{requests.fetch_and(~std::uint64_t(1));}
};
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
  std::weak_ptr<ScriptCancellationDomain> cancellation;
  std::atomic_bool late_cancel{false};
};
MultiplayerPreparedScriptImpulseTu15::MultiplayerPreparedScriptImpulseTu15(std::shared_ptr<Impl>p):impl_(std::move(p)){}
void MultiplayerPreparedScriptImpulseTu15::request_cancel()noexcept{
  if(!impl_ || impl_->status.load()!=Impl::Status::Pending)return;
  const auto domain=impl_->cancellation.lock();
  if(!domain){auto expected=Impl::Status::Pending;impl_->status.compare_exchange_strong(expected,Impl::Status::CancelRequested);return;}
  domain->active.fetch_add(1);
  struct Leave {ScriptCancellationDomain&d;~Leave(){d.active.fetch_sub(1);}} leave{*domain};
  auto value=domain->requests.load();
  for(;;){
    if(domain->life.load()==ScriptCancellationDomain::Life::Retired)return;
    if(value&1u){impl_->late_cancel.store(true);domain->note();return;}
    if(domain->requests.compare_exchange_weak(value,ScriptCancellationDomain::next(value))){
      if(domain->life.load()==ScriptCancellationDomain::Life::Prospective)impl_->late_cancel.store(true);
      else{auto expected=Impl::Status::Pending;impl_->status.compare_exchange_strong(expected,Impl::Status::CancelRequested);}
      return;
    }
  }
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
    std::vector<unsigned> chunk_counts;
    std::vector<std::uint16_t> chances;
    unsigned rate_index{},sound_class{};
  };
  struct ScriptVoiceRecord {
    std::uint64_t id{};bool current=false;std::function<bool()> retirement;
    std::weak_ptr<SourceScriptSoundClockNative64Tu15> clock;
    std::optional<SourceScriptSoundClockAdmission64Tu15> admission;
    std::weak_ptr<const SourceScriptSoundClockVoice64Tu15::Impl> source_voice;
  };
  struct Voice {std::uint64_t id{};std::uint32_t sound{},owner{},loop_tag=none;unsigned view{},track{};
    MultiplayerAudioKind kind{};float gain_db{},relative_rate=1,ambient_gain=1;unsigned sound_class{};
    bool loop{};Completion completion;unsigned pitch_range{};std::uint32_t source_pitch_bits{};
    std::int8_t permutation_gain_db{};std::optional<std::uint32_t> permutation_seed;
    std::optional<Vec3> position;std::optional<SoundPlaybackFade> fade;float fade_db{};
    std::shared_ptr<ScriptVoiceRecord> script_record;
    std::optional<unsigned> forced_script_permutation;
    std::optional<SdlSoundPlatform::VoiceReceipt> playback;
    std::shared_ptr<const unsigned char> record;
    std::optional<CompanionCompletion> companion;bool loop_packet{};
    std::map<std::uint64_t,std::int8_t> packet_gains;};
  struct PacketCompletion {
    SdlSoundCompletion value;
    std::optional<SdlSoundPlatform::CompletionReceipt> receipt;
  };
  struct RetiredCompletion {Completion owner;SdlSoundCompletion packet;
    std::optional<CompanionCompletion> companion;VoiceReceipt voice;};
  struct Ambient {float gain{},linear_db{};MultiplayerAmbientTarget target;std::vector<std::uint64_t> voices;};
  struct ObjectLoopTrack {float control{};std::uint64_t voice{};};
  struct ObjectLoopDetailVoice {std::uint64_t voice{};Vec3 offset;};
  struct ObjectLoop {
    LoopingSound definition;std::uint32_t owner=none;
    std::array<std::byte,68> location{};
    std::vector<ObjectLoopTrack> tracks;
    std::vector<std::int32_t> detail_deadlines;
    std::vector<ObjectLoopDetailVoice> detail_voices;
    bool stopping{},seen{},retain{},alternate{},disable_details{};
  };
  std::shared_ptr<const Cache> cache_owner;
  const Cache &cache;SoundLocalRandom random;
  std::shared_ptr<SdlSoundPlatform> platform_storage;SdlSoundPlatform&platform;
  MultiplayerAudioObserver observer;
  std::shared_ptr<ScriptAudioEpoch> script_epoch=std::make_shared<ScriptAudioEpoch>();
  std::atomic_bool script_bound{false};
  std::atomic_bool continuation_enrolled{false};
  std::vector<std::shared_ptr<MultiplayerPreparedScriptImpulseTu15::Impl>> script_preparations;
  std::vector<std::function<void()>> script_closers;
  std::vector<std::shared_ptr<ScriptVoiceRecord>> script_retirements;
  std::vector<std::shared_ptr<SourceScriptSoundClockNative64Tu15>> script_clocks;
  std::optional<MccSoundVariationProvider> native_metadata;
  unsigned play_depth=0,completion_depth=0,operation_depth=0,object_loop_depth=0;
  std::weak_ptr<Core> self;
  const std::thread::id executor=std::this_thread::get_id();
  std::shared_ptr<ScriptCancellationDomain> cancellation=std::make_shared<ScriptCancellationDomain>();
  std::uint64_t revision{},publication_epoch=1;
  bool held=false,public_alive=true,shutdown_requested=false;
  bool transaction=false;
  const void* held_preparation{};
  std::shared_ptr<const unsigned char> publication_record=std::make_shared<const unsigned char>(0);
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
  std::map<std::pair<std::uint32_t,std::uint32_t>,ObjectLoop> object_loops;
  SourceLoopPhasedStateTu15 phased_loops;
  std::function<std::uint32_t()> loop_fresh_secondary;
  std::shared_ptr<const unsigned char> loop_random_domain;
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
  static const Cache& retained_cache(const std::shared_ptr<const Cache>&owner){
    if(!owner || !owner.use_count())throw FormatError("Native Audio requires a retaining Cache owner");
    return *owner;
  }
  Core(std::shared_ptr<const Cache> owner,std::uint32_t seed,
      const SdlSoundPlatformConfiguration&configuration,MultiplayerAudioObserver observer,
      std::uint32_t*shared):Core(retained_cache(owner),seed,configuration,std::move(observer),shared){
    cache_owner=std::move(owner);
  }
  void script_thread()const{
    if(executor!=std::this_thread::get_id())throw FormatError("Native script Audio used from foreign thread");
    if(!script_epoch->alive.load() && !shutdown_requested)throw FormatError("Native script Audio owner unavailable");
  }
  void script_producer()const{
    if(continuation_enrolled.load() || script_bound.load()){
      script_thread();if(held || transaction)throw FormatError("Native Audio publication is held or preparing");
    }
  }
  struct Mutation {
    Core& core;
    explicit Mutation(Core& c):core(c){
      c.script_producer();
      if(c.object_loop_depth && c.play_depth)
        throw FormatError("Native object-loop packet observation cannot mutate its borrowed playback owner");
      if(c.continuation_enrolled.load() || c.script_bound.load())c.merge_script_cancellations();
      ++c.operation_depth;
      if(c.continuation_enrolled.load() && c.revision!=UINT64_MAX)++c.revision;
    }
    ~Mutation(){--core.operation_depth;core.maybe_shutdown();}
  };
  VoiceReceipt receipt(const Voice&v)const{
    VoiceReceipt out;out.owner_=self;out.owner_pointer_=this;out.id_=v.id;
    out.epoch_=publication_epoch;out.record_=v.record;return out;
  }
  bool accepts(const VoiceReceipt&r)const noexcept{
    const auto owner=r.owner_.lock();if(!owner || owner.get()!=this ||
        r.owner_pointer_!=this || r.epoch_!=publication_epoch || !r.record_)return false;
    const auto found=voices.find(r.id_);return found!=voices.end() &&
      found->second.record.get()==r.record_.get() &&
      !found->second.record.owner_before(r.record_) && !r.record_.owner_before(found->second.record);
  }
  void require_receipt(const VoiceReceipt&r)const{
    if(!accepts(r))throw FormatError("Native Audio voice receipt is stale or foreign");
  }
  void raw_mutation()const{
    if(publication_epoch!=1)throw FormatError("Native Audio raw voice mutation retired after publication");
  }
  void parameters(const Voice&voice,const SdlSoundVoiceParameters&value){
    if(voice.playback)platform.parameters(*voice.playback,value);
    else platform.parameters(voice.id,value);
    for(const auto&[context,gain]:voice.packet_gains) {
      const auto volume=parameters(voice,gain).volume_hundredths_db;
      if(voice.playback)platform.packet_volume(*voice.playback,context,volume);
      else platform.packet_volume(voice.id,context,volume);
    }
  }
  SdlSoundPlatform::Submission submit(Voice&voice,SdlSoundPacket packet){
    return voice.playback?platform.submit(*voice.playback,std::move(packet)):
        platform.submit(voice.id,std::move(packet));
  }
  std::optional<PacketCompletion> completion(const Voice&voice)const{
    if(voice.playback){
      if(auto packet=platform.completion(*voice.playback))return PacketCompletion{packet->value(),std::move(packet)};
    }else if(auto packet=platform.completion(voice.id))return PacketCompletion{*packet,{}};
    return {};
  }
  void acknowledge(Voice&voice,const PacketCompletion&packet){
    if(packet.receipt)platform.acknowledge(*packet.receipt);
    else platform.acknowledge(voice.id,packet.value.submission);
    voice.packet_gains.erase(packet.value.packet_context);
  }
  void flush(const Voice&voice){
    if(voice.playback)platform.flush(*voice.playback);else platform.flush(voice.id);
  }
  void destroy_voice(const Voice&voice){
    if(voice.playback)platform.destroy_voice(*voice.playback);else platform.destroy_voice(voice.id);
  }
  void metadata_current()const{if(native_metadata)native_metadata->require_current(cache);}
  SoundVariationDefinition definition(std::uint32_t sound,unsigned pitch=0)const{
    return native_metadata?native_metadata->load(cache,TagHandle{sound},pitch):
        cache.load_sound_variation(sound,pitch);
  }
  void drain_script_cancellations(){
    merge_script_cancellations();
    if(script_preparations.empty())return;
    script_thread();
    using Status=MultiplayerPreparedScriptImpulseTu15::Impl::Status;
    for(auto&p:script_preparations)if(p->status.load()==Status::CancelRequested){
      if(p->admission)(void)p->clock->finish_failure(*p->admission);
      p->status.store(Status::Consumed);
    }
    std::erase_if(script_preparations,[](const auto&p){return p->status.load()==Status::Consumed;});
  }
  void merge_script_cancellations()noexcept{
    for(const auto&p:script_preparations)if(p->late_cancel.exchange(false)){
      auto expected=MultiplayerPreparedScriptImpulseTu15::Impl::Status::Pending;
      p->status.compare_exchange_strong(expected,MultiplayerPreparedScriptImpulseTu15::Impl::Status::CancelRequested);
    }
  }
  void close_script_bindings()noexcept{
    for(auto&p:script_preparations)p->status.store(MultiplayerPreparedScriptImpulseTu15::Impl::Status::Consumed);
    for(auto&close:script_closers)close();
    script_epoch->alive.store(false);script_preparations.clear();script_closers.clear();script_retirements.clear();script_clocks.clear();
  }
  bool classic_sound(std::uint32_t sound)const {
    metadata_current();
    if(!cache.is_mcc())return true;
    const auto *tag=cache.query_tag({.handle=TagHandle{sound},
        .group=TagGroup::from_string("snd!"),.require_address=true,.minimum_size=28});
    if(!tag)throw FormatError("Classic playback lost its resident MCC sound record");
    // Pinned Halo2MCC snd! bit11 is Play Only In Remastered Mode. Apply
    // this before resource loading, variation and voice creation for every
    // native caller, including replicated FP clocks and world effects.
    return !(Bytes(cache.read_tag_bytes(tag->address,28)).u16(0)&0x800u);
  }
  std::uint32_t& common_secondary(){return shared_variation_random?*shared_variation_random:random.seed;}
  Bank &bank(std::uint32_t sound,unsigned pitch=0){
    script_producer();metadata_current();
    const auto key=std::pair{sound,pitch};
    if(auto found=banks.find(key);found!=banks.end())return found->second;
    Bank b;b.definition=definition(sound,pitch);
    b.rate_index=b.definition.sample_rate_index;b.sound_class=std::to_integer<unsigned>(b.definition.sound[2]);
    b.range=imported_sound_range_state(b.definition);b.loop={b.range.used,b.range.last};
    for(unsigned p=0;p<b.definition.permutations.size();++p){
      const auto encoded=cache.load_sound(sound,pitch,p);auto pcm=decode_sound(encoded);
      if(pcm.rate!=48000 || !pcm.frames())throw FormatError("Native authored sound has no complete PCM frames");
      b.pcm.push_back(std::make_shared<const std::vector<float>>(std::move(pcm.stereo)));
      b.chunk_counts.push_back(unsigned(encoded.packet_chunks.size()));
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
  SdlSoundVoiceParameters parameters(const Voice &v,std::optional<std::int8_t> packet_gain={})const {
    metadata_current();
    const float spatial=distance_gain(v);
    const SoundSpatialConstants gain_constants{distance_constants.one_bits,distance_constants.minimum_gain_bits,distance_constants.linear_db_scale_bits};
    const float spatial_db=std::bit_cast<float>(source_sound_spatial_gain_db(std::bit_cast<std::uint32_t>(spatial),gain_constants,{0x027f,0x1f80}));
    const float source_gain=v.gain_db+sound_linear_to_db(v.ambient_gain)+class_gain.at(v.sound_class)+spatial_db+v.fade_db;
    // Original12A603..12A650 adds the selected permutation's signed gain
    // before the driver attenuation conversion. This must follow selection,
    // including every new ambient packet, rather than using permutation0.
    const float attenuation=source_gain+float(packet_gain.value_or(v.permutation_gain_db));
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
    parameters(voice,parameters(voice));
    const auto &pcm=b.pcm.at(p);const auto context=next_packet++;
    const auto volume=voice.loop_packet?std::optional(parameters(voice).volume_hundredths_db):std::nullopt;
    const auto result=submit(voice,{pcm,0,pcm->size()/2,context,volume});
    if(result!=SdlSoundPlatform::Submission::Accepted)throw FormatError("Native sound packet queue unexpectedly full");
    if(voice.loop_packet)voice.packet_gains.emplace(context,voice.permutation_gain_db);
    if(observer)observer({voice.kind,voice.sound,voice.owner,voice.view,voice.pitch_range,p,voice.id,pcm->size()/2,before,after,voice.source_pitch_bits,voice.position});
    selection.stores.clear();
  }
  std::uint64_t play(std::uint32_t sound,float gain_db,MultiplayerAudioKind kind,
      std::uint32_t owner,unsigned view,bool looping=false,std::uint32_t loop_tag=none,unsigned track=0,float ambient_gain=1,
      Completion completion={},float control=1,std::optional<Vec3> position={},
      std::optional<std::uint64_t> presentation_frame={},
      std::optional<SourceScriptSoundClockSelectionTu15> script_selection={},
      std::shared_ptr<ScriptVoiceRecord> script_record={},
      const MccScriptSoundWorldAccess* first_selection=nullptr,
      std::optional<CompanionCompletion> companion={},
      const MultiplayerLoopPacketTu15* loop_packet=nullptr){
    script_producer();
    if(closing)throw FormatError("Native audio is being destroyed");
    Phase phase(play_depth);
    drain_script_cancellations();
    if(!classic_sound(sound))return 0;
    const auto expected_auxiliary_seed=first_selection && shared_variation_random?
        std::optional<std::uint32_t>(*shared_variation_random):std::nullopt;
    // The loop controller has already selected an authored range and packet.
    // Its real output port does not rerun impulse gain/pitch/permutation RNG.
    auto &b=bank(sound,loop_packet?loop_packet->pitch_range:0);float extra=0;
    if(loop_packet && loop_packet->position)(void)bank(sound,0);
    //1269B0/126A40 use the global auxiliary word. Loop-controller and
    // permutation selection keep their separate native owner; sharing that
    // entire stream would incorrectly fold219A90's private1462B0 seed into it.
    float pitch;
    std::optional<std::uint32_t> voice_seed;
    if(loop_packet) {
      pitch=loop_packet->parameters.pitch_cents;
      extra=source_sound_control_gain(b.definition,loop_packet->parameters.gain_control);
    }else {
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
    const auto controlled_pitch=loop_packet?pitch:source_sound_control_pitch(b.definition,pitch,control);
    const bool forced=script_selection && script_selection->pitch_range!=0xff && script_selection->permutation!=0xff;
    const auto range=loop_packet?int(loop_packet->pitch_range):forced?int(script_selection->pitch_range):
        (looping?0:source_sound_pitch_range(b.definition.pitch_ranges,-1,controlled_pitch));
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
    v.record=std::make_shared<const unsigned char>(0);v.companion=std::move(companion);
    v.position=position;
    v.loop_packet=bool(loop_packet);
    v.script_record=std::move(script_record);
    if(forced)v.forced_script_permutation=script_selection->permutation;
    if(loop_packet)v.forced_script_permutation=loop_packet->permutation;
    v.pitch_range=unsigned(range);
    v.source_pitch_bits=std::bit_cast<std::uint32_t>(packet_pitch);
    v.permutation_seed=voice_seed;
    if(loop_packet && loop_packet->fade_in_seconds) {
      v.fade=SoundPlaybackFade{1,-64.f,-1,std::int32_t(*loop_packet->fade_in_seconds*1000.f),true};
      const auto time=std::bit_cast<std::int32_t>(std::uint32_t(platform.elapsed_frames()/48));
      v.fade_db=source_sound_playback_fade(*v.fade,time);
    }
    if(continuation_enrolled.load()){
      v.playback=platform.create_voice_receipt(owner==none?sound:owner,parameters(v),presentation_frame);
      v.id=v.playback->id();
    }else v.id=presentation_frame?platform.create_voice_at(owner==none?sound:owner,parameters(v),*presentation_frame):
        platform.create_voice(owner==none?sound:owner,parameters(v));
    if(v.script_record){v.script_record->id=v.id;v.script_record->current=true;}
    const auto id=v.id;const auto record=v.script_record;const auto playback=v.playback;
    try{
      packet(v,looping,first_selection,expected_auxiliary_seed);if(closing)throw FormatError("Native Audio closed during packet observer");
      const auto[at,inserted]=voices.emplace(id,std::move(v));
      (void)at;if(!inserted)throw FormatError("Native Audio voice publication identity collision");
      return id;
    }catch(...){Voice failed;failed.id=id;failed.playback=playback;flush(failed);
      while(auto c=this->completion(failed))acknowledge(failed,*c);
      destroy_voice(failed);if(record)record->current=false;throw;}
  }
  void erase(std::uint64_t id,std::optional<SdlSoundCompletion> completed={}){
    script_producer();
    auto &voice=voices.at(id);
    if(voice.script_record && voice.script_record->retirement)script_retirements.push_back(voice.script_record);
    flush(voice);
    while(auto c=completion(voice)){
      completed=c->value;acknowledge(voice,*c);
    }
    if((voice.completion || voice.companion) && !completed)
      throw FormatError("Native sound callback has no accounted packet");
    destroy_voice(voice);
    if(voice.script_record)voice.script_record->current=false;
    auto owner=std::move(voice.completion);auto companion=std::move(voice.companion);
    const auto source=receipt(voice);voices.erase(id);
    if(owner || companion)retired.push_back({std::move(owner),*completed,std::move(companion),source});
  }
  void complete(){
    script_producer();
    Phase phase(completion_depth);
    auto pending=std::exchange(retired,{});
    std::exception_ptr failure;
    auto script_pending=std::exchange(script_retirements,{});
    for(auto&record:script_pending)try{if(!record->retirement())script_retirements.push_back(std::move(record));}
      catch(...){script_retirements.push_back(std::move(record));if(!failure)failure=std::current_exception();}
    for(auto &completion:pending)try{
      if(completion.companion){const auto&c=*completion.companion;
        if(const auto owner=c.owner.lock())c.dispatch(owner,c.branch,c.token,c.queued,completion.voice,completion.packet);
      }else completion.owner(completion.packet);
    }
      catch(...){if(!failure)failure=std::current_exception();}
    if(failure)std::rethrow_exception(failure);
  }
  void shutdown()noexcept{
    shutdown_requested=true;closing=true;
    try{while(!voices.empty())erase(voices.begin()->first);ambience.clear();object_loops.clear();phased_loops={};complete();drain_script_cancellations();}catch(...){}
    close_script_bindings();shutdown_requested=false;
  }
  void maybe_shutdown()noexcept{
    if(shutdown_requested && !held && !transaction && !operation_depth && !play_depth && !completion_depth)shutdown();
  }
  ~Core(){if(shutdown_requested)shutdown();}
  struct PhasedOutput final:SourceLoopPhasedOutputTu15 {
    Core& core;MultiplayerAudio& audio;
    PhasedOutput(Core& c,MultiplayerAudio& a):core(c),audio(a){}
    const SoundVariationDefinition& metadata(std::uint32_t sound,unsigned range)override{
      if(!core.classic_sound(sound))throw FormatError("Native synchronized phase lacks its Classic sound route");
      return core.bank(sound,range).definition;
    }
    bool single_chunk(std::uint32_t sound,unsigned range,unsigned permutation)override{
      return core.bank(sound,range).chunk_counts.at(permutation)==1;
    }
    bool active(std::uint64_t id)const override{return core.voices.contains(id);}
    std::uint64_t play(const MultiplayerLoopPacketTu15& input)override{
      const auto voice=audio.play_loop_packet(input);return voice?voice->id():0;
    }
    void parameters(std::uint64_t id,const MultiplayerLoopPacketParametersTu15& input,Vec3 position)override{
      const auto receipt=core.receipt(core.voices.at(id));audio.loop_packet_parameters(receipt,input);audio.emitter(receipt,position);
    }
    bool accepts_packet(std::uint64_t id)const override{return audio.loop_packet_accepts_input(core.receipt(core.voices.at(id)));}
    bool queue(std::uint64_t id,std::uint32_t sound,unsigned range,unsigned permutation)override{
      const auto& voice=core.voices.at(id);const auto receipt=core.receipt(voice);
      if(voice.sound==sound && voice.pitch_range==range)return audio.queue_loop_packet(receipt,permutation);
      return audio.queue_loop_packet_phase(receipt,sound,range,permutation);
    }
    void fade(std::uint64_t id,float seconds,bool rising,std::uint16_t curve)override{
      audio.loop_packet_fade(core.receipt(core.voices.at(id)),seconds,rising,curve);
    }
    void retire(std::uint64_t id)override{core.erase(id);}
    std::uint32_t fresh_secondary()override{
      if(!core.loop_fresh_secondary)throw FormatError("Native synchronized loop lacks its actual platform fresh-random owner");
      return core.loop_fresh_secondary();
    }
  };
 };
 std::shared_ptr<Core> core;
 State(const Cache&cache,std::uint32_t seed,const SdlSoundPlatformConfiguration&configuration,
       MultiplayerAudioObserver observer,std::uint32_t*shared):
       core(std::make_shared<Core>(cache,seed,configuration,std::move(observer),shared)){core->self=core;}
 State(std::shared_ptr<const Cache> cache,std::uint32_t seed,const SdlSoundPlatformConfiguration&configuration,
       MultiplayerAudioObserver observer,std::uint32_t*shared):
       core(std::make_shared<Core>(std::move(cache),seed,configuration,std::move(observer),shared)){core->self=core;}
};

MultiplayerAudio::MultiplayerAudio(const Cache &cache,std::uint32_t seed,
    const SdlSoundPlatformConfiguration &configuration,MultiplayerAudioObserver observer,
    std::uint32_t *shared):
    state_(std::make_unique<State>(cache,seed,configuration,std::move(observer),shared)){}
MultiplayerAudio::MultiplayerAudio(std::shared_ptr<const Cache> cache,std::uint32_t seed,
    const SdlSoundPlatformConfiguration &configuration,MultiplayerAudioObserver observer,
    std::uint32_t *shared):
    state_(std::make_unique<State>(std::move(cache),seed,configuration,std::move(observer),shared)){}
MultiplayerAudio::~MultiplayerAudio(){
  const auto state=state_->core;
  if((state->continuation_enrolled.load() || state->script_bound.load()) &&
      state->executor!=std::this_thread::get_id())std::terminate();
  state->public_alive=false;state->closing=true;state->shutdown_requested=true;state->script_epoch->alive.store(false);
  state->maybe_shutdown();
}
void MultiplayerAudio::enroll_continuation(){
  const auto state=state_->core;state->script_thread();
  if(!state->cache_owner)throw FormatError("Native Audio continuation requires a retaining Cache owner");
  if(state->closing || state->play_depth || state->completion_depth ||
      !state->voices.empty() || !state->ambience.empty() || !state->object_loops.empty() || !state->phased_loops.owners.empty() || !state->phased_loops.synchronization.empty() || !state->retired.empty() ||
      state->script_bound.load() || !state->script_preparations.empty() ||
      !state->script_closers.empty() || !state->script_retirements.empty() || !state->script_clocks.empty())
    throw FormatError("Native Audio continuation requires empty quiescent playback before script binding");
  if(state->continuation_enrolled.load())return;
  state->metadata_current();
  // This no-allocation endpoint operation validates sticky foreign-producer
  // history before the Core publishes the same exclusive producer gate.
  state->platform.enroll_continuation();
  state->continuation_enrolled.store(true);
}
void MultiplayerAudio::bind_native_sound_metadata(const MccSoundVariationProvider&input){
  auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();state->script_thread();
  input.require_current(state->cache);
  using Status=MultiplayerPreparedScriptImpulseTu15::Impl::Status;
  if(state->closing || state->play_depth || state->completion_depth ||
     !state->voices.empty() || !state->ambience.empty() || !state->object_loops.empty() || !state->phased_loops.owners.empty() || !state->phased_loops.synchronization.empty() || !state->retired.empty() ||
     !state->script_retirements.empty() ||
     std::ranges::any_of(state->script_preparations,[](const auto&p){return p->status.load()!=Status::Consumed;}))
    throw FormatError("Native sound metadata binding requires quiescent Audio");
  // Allocate before invalidating old clocks or changing cached definitions.
  const auto provider=input;auto epoch=std::make_shared<ScriptAudioEpoch>();
  state->close_script_bindings();state->banks.clear();
  state->native_metadata=provider;state->script_epoch=std::move(epoch);state->script_bound.store(true);
}
void MultiplayerAudio::prepare(std::uint32_t sound){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();if(sound!=none && state->classic_sound(sound))(void)state->bank(sound);}
namespace {
void validate_loop_packet_parameters(const MultiplayerLoopPacketParametersTu15& value) {
  if(!std::isfinite(value.pitch_cents) || !std::isfinite(value.gain_db) ||
      !std::isfinite(value.gain_control) || value.gain_control<0)
    throw FormatError("Native loop packet requires finite authored pitch/gain and caller control");
}
}
std::optional<MultiplayerAudio::VoiceReceipt> MultiplayerAudio::play_loop_packet(
    const MultiplayerLoopPacketTu15& input,std::optional<std::uint64_t> deadline,Completion completion) {
  const auto state=state_->core;State::Core::Mutation mutation(*state);
  if(input.sound==none)return {};
  validate_loop_packet_parameters(input.parameters);
  if(input.fade_in_seconds && (!std::isfinite(*input.fade_in_seconds) || *input.fade_in_seconds<0 || *input.fade_in_seconds>2147483.f))
    throw FormatError("Native loop initial fade escapes the authored duration domain");
  if(input.view>=4 || (input.position && (!std::isfinite(input.position->x) ||
      !std::isfinite(input.position->y) || !std::isfinite(input.position->z))))
    throw FormatError("Native loop packet has an invalid local view or world position");
  if(!state->classic_sound(input.sound))return {};
  const auto& bank=state->bank(input.sound,input.pitch_range);
  if(input.permutation>=bank.definition.permutations.size())
    throw FormatError("Native loop packet lost its selected authored permutation");
  const auto id=state->play(input.sound,input.parameters.gain_db,MultiplayerAudioKind::Ambient,
      input.owner,input.view,false,none,0,1,std::move(completion),1,input.position,
      deadline,{},{},nullptr,{},&input);
  if(!id)return {};return state->receipt(state->voices.at(id));
}
void MultiplayerAudio::loop_packet_parameters(const VoiceReceipt& receipt,
    const MultiplayerLoopPacketParametersTu15& input) {
  const auto state=state_->core;State::Core::Mutation mutation(*state);
  state->require_receipt(receipt);validate_loop_packet_parameters(input);
  auto& voice=state->voices.at(receipt.id_);
  if(!voice.loop_packet)throw FormatError("Native loop parameters require their actual loop packet voice");
  const auto& bank=state->bank(voice.sound,voice.pitch_range);
  const auto center=std::bit_cast<std::int16_t>(Bytes(bank.definition.pitch_ranges.at(voice.pitch_range).interval).u16(0));
  const float pitch=input.pitch_cents-float(center);
  const auto driver=source_sound_driver_pitch(bank.rate_index,std::bit_cast<std::uint32_t>(pitch),
      state->constants,{0x027f,0x1f80});
  voice.relative_rate=std::bit_cast<float>(source_sound_relative_rate(driver,bank.rate_index,
      state->constants,{0x027f,0x1f80}));
  voice.source_pitch_bits=std::bit_cast<std::uint32_t>(pitch);
  voice.gain_db=input.gain_db+source_sound_control_gain(bank.definition,input.gain_control);
  state->parameters(voice,state->parameters(voice));
}
bool MultiplayerAudio::loop_packet_accepts_input(const VoiceReceipt& receipt)const {
  const auto state=state_->core;state->script_producer();state->require_receipt(receipt);
  const auto& voice=state->voices.at(receipt.id_);
  if(!voice.loop_packet)throw FormatError("Native loop queue requires its actual loop packet voice");
  return (voice.playback?state->platform.status(*voice.playback):state->platform.status(voice.id)).accepts_input;
}
bool MultiplayerAudio::queue_loop_packet(const VoiceReceipt& receipt,unsigned permutation) {
  const auto state=state_->core;State::Core::Mutation mutation(*state);state->require_receipt(receipt);
  auto& voice=state->voices.at(receipt.id_);
  if(!voice.loop_packet)throw FormatError("Native loop queue requires its actual loop packet voice");
  const auto& bank=state->bank(voice.sound,voice.pitch_range);
  if(permutation>=bank.definition.permutations.size())throw FormatError("Native loop queue lost its selected authored permutation");
  if(!(voice.playback?state->platform.status(*voice.playback):state->platform.status(voice.id)).accepts_input)return false;
  voice.forced_script_permutation=permutation;
  try{state->packet(voice,false);}catch(...){state->erase(receipt.id_);state->complete();throw;}
  return true;
}
bool MultiplayerAudio::queue_loop_packet_phase(const VoiceReceipt& receipt,std::uint32_t sound,
    unsigned pitch_range,unsigned permutation) {
  const auto state=state_->core;State::Core::Mutation mutation(*state);state->require_receipt(receipt);
  auto& voice=state->voices.at(receipt.id_);
  if(!voice.loop_packet)throw FormatError("Native loop phase requires its actual loop packet voice");
  if(!(voice.playback?state->platform.status(*voice.playback):state->platform.status(voice.id)).accepts_input)return false;
  if(sound==none || !state->classic_sound(sound))return false;
  const auto& previous=state->bank(voice.sound,voice.pitch_range);
  const auto& next=state->bank(sound,pitch_range);
  if(voice.position)(void)state->bank(sound,0);
  if(permutation>=next.definition.permutations.size())
    throw FormatError("Native loop phase lost its selected authored permutation");
  //222BA0 changes the source sound/range in place. This output port admits
  //the exact-equivalent controls used by the actual Banshee boost phases.
  //It does not infer a new driver rate, gain, curve or spatial predicate.
  const auto& a=previous.definition;const auto& b=next.definition;
  if(previous.rate_index!=next.rate_index || previous.sound_class!=next.sound_class ||
      Bytes(a.sound).u16(0)!=Bytes(b.sound).u16(0) || a.playback!=b.playback ||
      a.scale!=b.scale || a.sound_class!=b.sound_class ||
      a.pitch_ranges.at(voice.pitch_range).interval!=b.pitch_ranges.at(pitch_range).interval ||
      voice.permutation_gain_db!=std::bit_cast<std::int8_t>(b.permutations.at(permutation)[4]))
    throw FormatError("Native loop phase requires its differing in-place driver-control owner");
  voice.sound=sound;voice.pitch_range=pitch_range;voice.forced_script_permutation=permutation;
  try{state->packet(voice,false);}catch(...){state->erase(receipt.id_);state->complete();throw;}
  return true;
}
void MultiplayerAudio::loop_packet_fade(const VoiceReceipt& receipt,float seconds,bool rising,std::uint16_t curve) {
  const auto state=state_->core;State::Core::Mutation mutation(*state);state->require_receipt(receipt);
  auto& voice=state->voices.at(receipt.id_);
  if(!voice.loop_packet)throw FormatError("Native loop fade requires its actual loop packet voice");
  if(!std::isfinite(seconds) || seconds<0 || seconds>2147483.f || curve>1)
    throw FormatError("Native loop fade escapes the authored duration/curve domain");
  const auto time=std::bit_cast<std::int32_t>(std::uint32_t(state->platform.elapsed_frames()/48));
  const auto current=voice.fade?source_sound_playback_fade(*voice.fade,time):(rising?-64.f:0.f);
  const auto duration=std::int32_t(seconds*1000.f);
  voice.fade=SoundPlaybackFade{curve,current,rising?-1:duration,rising?duration:-1,true};
  voice.fade_db=source_sound_playback_fade(*voice.fade,time);
  state->parameters(voice,state->parameters(voice));
}
void MultiplayerAudio::object_loop_request(const SourceLoopSoundRequestTu15& input) {
  const auto state=state_->core;State::Core::Mutation mutation(*state);state->metadata_current();
  State::Core::Phase phase(state->object_loop_depth);
  if(state->cache.is_mcc() && !state->native_metadata)
    throw FormatError("Object loop policy requires its actual converted native metadata owner");
  const auto& args=input.arguments;
  if(args[0]==none)return;
  const Bytes location(input.location);const auto type=std::to_integer<unsigned>(input.location[3])&15u;
  const auto duration=std::bit_cast<float>(args[7]);
  if(args[5]>2 || type>1 || !std::isfinite(location.f32(4)) || location.f32(4)<0 ||
      !std::isfinite(location.f32(8)) || !std::isfinite(duration) || duration<0)
    throw FormatError("Object loop request escapes its original caller domain");
  const auto key=std::pair{args[0],args[2]};auto found=state->object_loops.find(key);
  if(state->phased_loops.owners.contains(key)){
    const auto& definition=state->phased_loops.owners.at(key).definition;
    State::Core::PhasedOutput output(*state,*this);source_loop_phased_request_tu15(state->phased_loops,output,input,definition);return;
  }
  if(found==state->object_loops.end()) {
    if(args[5]==2)return;
    const auto definition=state->cache.load_looping_sound(args[0]);
    //MCC adds lsnd route bits8/9 after the original eight predicates.
    //Keep this raw-cache mode gate separate from Xbox controller flags.
    if(state->cache.is_mcc() && (definition.flags&0x200u))return;
    if(definition.flags&0x10u){
      State::Core::PhasedOutput output(*state,*this);source_loop_phased_request_tu15(state->phased_loops,output,input,definition);return;
    }
    if(definition.flags&0xb2u)
      throw FormatError("Object loop requires its synchronization, aggregation or one-shot owner");
    if(!definition.details.empty() && type!=1)
      throw FormatError("Nonspatial loop details require their actual local-listener selection owner");
    for(const auto& track:definition.tracks) {
      if(track.start!=none || track.end!=none || track.alternate!=none || track.alternate_end!=none ||
          track.alternate_transition_in!=none || track.alternate_transition_out!=none ||
          ((args[6]&1u) && (track.flags&0x14u)))
        throw FormatError("Object loop requires its authored entry/exit/alternate controller");
      if(track.loop==none || !state->classic_sound(track.loop))continue;
      const auto& bank=state->bank(track.loop);
      if(bank.definition.pitch_ranges.size()!=1 || (Bytes(bank.definition.sound).u16(0)&0x2000u))
        throw FormatError("Object loop requires its pitch-range/layer creation owner");
    }
    State::Core::ObjectLoop loop;loop.definition=definition;loop.owner=input.emitter_owner;loop.location=input.location;
    loop.tracks.resize(definition.tracks.size());for(auto& track:loop.tracks)track.control=location.f32(4);
    const auto clock=std::bit_cast<std::int32_t>(std::uint32_t(state->platform.elapsed_frames()/48));
    for(const auto& detail:definition.details)loop.detail_deadlines.push_back(
        source_loop_detail_deadline_tu15(detail.period,detail.flags,clock,state->common_secondary(),true));
    found=state->object_loops.emplace(key,std::move(loop)).first;
  }
  auto& loop=found->second;
  if((args[6]&1u) && std::ranges::any_of(loop.definition.tracks,[](const auto& track){return track.flags&0x14u;}))
    throw FormatError("Object loop requires its authored alternate controller");
  if(!(std::to_integer<unsigned>(input.location[0])&2u)) {
    // The same owner's loop can move between centered first-person output
    // and a spatial world emitter (a held weapon leaving the first-person
    // view during a grenade throw, a dropped or thrown object). A centered
    // voice owns no emitter, so retire its voices; the next frame recreates
    // them in the new mode from the loop's retained gain control.
    if((std::to_integer<unsigned>(loop.location[3])&15u)!=type) {
      for(auto& track:loop.tracks)if(auto voice=state->voices.find(track.voice);voice!=state->voices.end()){
        state->erase(voice->first);track.voice=0;
      }
      for(const auto& detail:loop.detail_voices)if(auto voice=state->voices.find(detail.voice);voice!=state->voices.end())
        state->erase(voice->first);
      loop.detail_voices.clear();
    }
    loop.location=input.location;
  }
  loop.seen=true;loop.retain=(args[6]&2u)!=0;
  loop.alternate=(args[6]&1u)!=0;loop.disable_details=(args[6]&4u)!=0;
  if(args[5]==2 && !loop.stopping) {
    for(unsigned n=0;n<loop.tracks.size();++n)if(auto voice=state->voices.find(loop.tracks[n].voice);voice!=state->voices.end()) {
      const auto& bank=state->bank(voice->second.sound,voice->second.pitch_range);
      const auto curve=std::uint16_t(!(Bytes(bank.definition.sound_class).u16(10)&0x400u));
      loop_packet_fade(state->receipt(voice->second),duration!=0?duration:loop.definition.tracks[n].fade_out,false,curve);
    }
    loop.stopping=true;
  }else if(args[5]!=2)loop.stopping=false;
}
void MultiplayerAudio::object_loop_frame(float elapsed) {
  const auto state=state_->core;State::Core::Mutation mutation(*state);state->metadata_current();
  State::Core::Phase phase(state->object_loop_depth);
  if(!std::isfinite(elapsed) || elapsed<0)throw FormatError("Object loop frame requires a finite elapsed interval");
  State::Core::PhasedOutput phased(*state,*this);source_loop_phased_frame_tu15(state->phased_loops,phased,elapsed);
  for(auto it=state->object_loops.begin();it!=state->object_loops.end();) {
    auto& loop=it->second;const Bytes location(loop.location);bool live=false;
    //221B5D..221B85 retires an untouched owner unless its explicit retain
    //flag and actual playback count keep it alive. Leaving PVS supplies no
    //synthetic stop request. Retire these real native streams with the owner.
    const bool output=std::ranges::any_of(loop.tracks,[&](const auto& track){return state->voices.contains(track.voice);});
    if(!loop.seen && !(loop.retain && output)) {
      for(const auto& track:loop.tracks)if(auto voice=state->voices.find(track.voice);voice!=state->voices.end())
        state->erase(voice->first);
      for(const auto& detail:loop.detail_voices)if(auto voice=state->voices.find(detail.voice);voice!=state->voices.end())
        state->erase(voice->first);
      it=state->object_loops.erase(it);continue;
    }
    std::optional<Vec3> position;
    if((std::to_integer<unsigned>(loop.location[3])&15u)==1)
      position=Vec3{location.f32(0xc),location.f32(0x10),location.f32(0x14)};
    for(unsigned n=0;n<loop.tracks.size();++n) {
      const auto& authored=loop.definition.tracks[n];auto& track=loop.tracks[n];
      if(authored.loop==none || !state->classic_sound(authored.loop))continue;
      const auto& bank=state->bank(authored.loop);auto voice=state->voices.find(track.voice);
      if(voice==state->voices.end())track.voice=0;
      else track.control=source_loop_track_gain_control_tu15(track.control,location.f32(4),Bytes(bank.definition.playback).f32(0xc),elapsed);
      if(!track.voice && loop.stopping)continue;
      const auto base_pitch=short_at(Bytes(bank.definition.playback),0x18);
      const auto pitch=source_sound_control_pitch(bank.definition,float(base_pitch),track.control);
      const MultiplayerLoopPacketParametersTu15 parameters{pitch,authored.gain_db+location.f32(8),track.control};
      const auto choose=[&]{return unsigned(source_looping_sound_permutation(bank.chances,
          state->bank(authored.loop).loop,state->common_secondary()));};
      std::optional<VoiceReceipt> receipt;
      if(!track.voice) {
        //222350 advances the common secondary word once for its constant
        //playback+18 pitch before selecting the first normal permutation.
        auto& seed=state->common_secondary();seed=seed*0x19660du+0x3c6ef35fu;
        MultiplayerLoopPacketTu15 input{authored.loop,loop.owner,0,0,choose(),parameters,position};
        if(authored.flags&1u)input.fade_in_seconds=authored.fade_in;
        receipt=play_loop_packet(input);
        if(!receipt)continue;
        track.voice=receipt->id();
      }else {
        receipt=state->receipt(voice->second);loop_packet_parameters(*receipt,parameters);
        if(position)emitter(*receipt,*position);
      }
      live=true;
      if(loop_packet_accepts_input(*receipt) && !queue_loop_packet(*receipt,choose()))
        throw FormatError("Object loop lost its retained native packet slot");
    }
    //223670/2229C0 retain each detail's offset relative to its actual
    //spatial parent. They do not contribute to the group's track count.
    std::erase_if(loop.detail_voices,[&](const auto& detail){
      const auto voice=state->voices.find(detail.voice);if(voice==state->voices.end())return true;
      if(position)emitter(state->receipt(voice->second),*position+detail.offset);
      return false;
    });
    // Details follow their spatial parent; while that owner is centered
    // (held in first person) none are scheduled.
    if(!loop.stopping && !loop.disable_details && !loop.definition.details.empty() && position) {
      const auto clock=std::bit_cast<std::int32_t>(std::uint32_t(state->platform.elapsed_frames()/48));
      for(unsigned n=0;n<loop.definition.details.size();++n) {
        const auto& detail=loop.definition.details[n];
        if(detail.sound==none || loop.detail_deadlines[n]>=clock ||
            ((detail.flags&1u)&&loop.alternate) || ((detail.flags&2u)&&!loop.alternate))continue;
        const auto spatial=source_loop_detail_spatial_tu15(loop.location,
            {detail.yaw,detail.pitch,detail.distance},state->common_secondary());
        const auto voice=state->play(detail.sound,spatial.gain_db,MultiplayerAudioKind::Ambient,
            loop.owner,0,false,none,0,1,{},spatial.control,spatial.position);
        if(voice)loop.detail_voices.push_back({voice,spatial.offset});
        //222166 reschedules a spatial detail even if native PCM admission
        //failed. Never invent a successful source submission or voice.
        loop.detail_deadlines[n]=source_loop_detail_deadline_tu15(detail.period,detail.flags,
            clock,state->common_secondary(),false);
      }
    }
    loop.seen=false;
    if(loop.stopping && !live) {
      for(const auto& detail:loop.detail_voices)if(auto voice=state->voices.find(detail.voice);voice!=state->voices.end())
        state->erase(voice->first);
      it=state->object_loops.erase(it);
    }else ++it;
  }
  state->complete();
}
void MultiplayerAudio::bind_object_loop_random_source(std::function<std::uint32_t()> source){
  const auto state=state_->core;State::Core::Mutation mutation(*state);
  if(!source || state->closing || state->play_depth || state->completion_depth || !state->voices.empty() ||
      !state->object_loops.empty() || !state->phased_loops.owners.empty() || !state->phased_loops.synchronization.empty())
    throw FormatError("Native loop random binding requires its empty quiescent platform owner");
  auto domain=std::make_shared<const unsigned char>(0);
  state->loop_fresh_secondary=std::move(source);state->loop_random_domain=std::move(domain);
}
void MultiplayerAudio::first_person(unsigned view,const FirstPersonViewBinding &binding,const FirstPersonAnimationEvent &event){
  if(event.type!=1 || event.tag==none || binding.unit==none || binding.hands[0].weapon==none ||
      (binding.hands[0].flags&6)!=6)return;
  impulse(event.tag,1,MultiplayerAudioKind::FirstPerson,binding.hands[0].weapon,view);
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::source_impulse(std::uint32_t sound,
    std::span<const std::byte,160> request,MultiplayerAudioKind kind,unsigned view){
  auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  if(sound==none)return std::nullopt;
  if(!state->classic_sound(sound))return std::nullopt;
  const Bytes raw(request);const auto flags=raw.u32(0);
  if(flags&(0x10u|0x400u))throw FormatError("Source impulse requires its controller/forced-selection owner");
  const auto control=raw.f32(0x10);
  const auto extra=flags&0x80u?raw.f32(0x94):std::bit_cast<float>(state->distance_constants.default_extra_gain_bits);
  const auto gain=raw.f32(0x14)+extra;
  if(!std::isfinite(control)||control<0 || !std::isfinite(gain))
    throw FormatError("Source impulse has invalid authored caller gain");
  const auto location=std::to_integer<unsigned>(request[0xf])&15u;
  std::optional<Vec3> position;
  if(location==1) {
    position=Vec3{raw.f32(0x18),raw.f32(0x1c),raw.f32(0x20)};
    if(!std::isfinite(position->x)||!std::isfinite(position->y)||!std::isfinite(position->z))
      throw FormatError("Source impulse has invalid world position");
  }else if(location)throw FormatError("Source impulse requires its relative-location owner");
  const auto owner=flags&8u?raw.u32(0x50):none;
  const auto voice=state->play(sound,gain,kind,owner,view,false,none,0,1,{},control,position);
  return voice?std::optional<Voice>(voice):std::nullopt;
}
void MultiplayerAudio::impulse(std::uint32_t sound,float gain,MultiplayerAudioKind kind,std::uint32_t owner,unsigned view){
  (void)play_impulse(sound,gain,kind,owner,view);
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::play_impulse(std::uint32_t sound,float gain,
    MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,Completion completion){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  if(sound==none)return std::nullopt;
  if(!std::isfinite(gain)||gain<0)throw FormatError("Invalid native sound caller gain");
  const auto voice=state->play(sound,0,kind,owner,view,false,none,0,1,std::move(completion),gain);
  return voice?std::optional<Voice>(voice):std::nullopt;
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::schedule_impulse(std::uint32_t sound,
    std::uint64_t frame,float control,MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,Completion completion){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  if(sound==none)return std::nullopt;
  if(!std::isfinite(control)||control<0)throw FormatError("Invalid native sound caller gain");
  const auto voice=state->play(sound,0,kind,owner,view,false,none,0,1,std::move(completion),control,{},frame);
  return voice?std::optional<Voice>(voice):std::nullopt;
}
std::shared_ptr<SourceScriptSoundClockNative64Tu15> MultiplayerAudio::make_script_sound_clock(){
  auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();state->script_thread();
  if(state->closing)throw FormatError("Native Audio is being destroyed");
  auto clock=std::make_shared<SourceScriptSoundClockNative64Tu15>();const auto epoch=state->script_epoch;
  clock->bind_audio_owner(epoch,[weak=std::weak_ptr<State::Core>(state),epoch]{
    const auto owner=weak.lock();return owner && owner->public_alive && !owner->held &&
      owner->script_epoch==epoch && epoch->alive.load();
  });
  state->script_clocks.push_back(clock);
  state->script_closers.push_back(clock->close_audio_binding());state->script_bound.store(true);return clock;
}
std::shared_ptr<SourceScriptSoundClockNative64Tu15> MultiplayerAudio::make_script_sound_clock(
    const SourceScriptSoundClockRowsTu15&input){
  auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();state->script_thread();
  state->metadata_current();const auto rows=input;
  using Status=MultiplayerPreparedScriptImpulseTu15::Impl::Status;
  if(state->closing || state->play_depth || state->completion_depth ||
     !state->voices.empty() || !state->ambience.empty() || !state->object_loops.empty() || !state->phased_loops.owners.empty() || !state->phased_loops.synchronization.empty() || !state->retired.empty() ||
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
  clock->bind_audio_owner(epoch,[weak=std::weak_ptr<State::Core>(state),epoch]{
    const auto owner=weak.lock();return owner && owner->public_alive && !owner->held &&
      owner->script_epoch==epoch && epoch->alive.load();
  });
  state->script_clocks.push_back(clock);
  state->script_closers.push_back(clock->close_audio_binding());state->script_bound.store(true);return clock;
}
std::optional<MultiplayerPreparedScriptImpulseTu15> MultiplayerAudio::prepare_script_impulse(
    const std::shared_ptr<SourceScriptSoundClockNative64Tu15>&input_clock,
    std::uint32_t sound,const SourceScriptSoundClockPolicyTu15&policy,
    const SourceScriptSoundClockNative64Tu15::StopRequest&stop,
    const SoundDriverFloatingPointContext&fp){
  auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();const auto clock=input_clock;state->script_thread();
  if(state->closing || !clock || !clock->audio_owner_is(state->script_epoch))
    throw FormatError("Native script preparation requires this actual Audio owner");
  state->drain_script_cancellations();if(sound==none)return {};
  // This is typed resident metadata only. PCM decode/backend admission happens
  // after the committed clock/selection phase in play_prepared_script_impulse.
  const auto definition=state->definition(sound);
  if(definition.datum!=sound)throw FormatError("Native script sound full datum differs");
  auto preparation=std::make_shared<MultiplayerPreparedScriptImpulseTu15::Impl>();
  preparation->audio_epoch=state->script_epoch;preparation->clock=clock;preparation->sound=sound;
  preparation->cancellation=state->cancellation;
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
  auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();state->script_thread();const auto preparation=input.impl_;
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
    const std::weak_ptr<SdlSoundPlatform> weak_platform=platform;
    const std::weak_ptr<State::Core> weak_core=state;
    const auto playback=found->second.playback;
    receipt=SourceScriptSoundClockVoice64Tu15::published(epoch,platform,record,voice,[epoch,weak_core,weak_platform,record,voice,playback]{
      const auto owner=weak_core.lock();if(!owner || !owner->public_alive || owner->held || owner->script_epoch!=epoch)return false;
      if(!epoch->alive.load() || !record->current || record->id!=voice)return false;
      const auto output=weak_platform.lock();if(!output)return false;
      if(playback)(void)output->status(*playback);else (void)output->status(voice);return true;
    });
    record->clock=preparation->clock;record->admission=preparation->admission;
    record->source_voice=receipt->impl_;
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
  auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();state->script_thread();const auto preparation=input.impl_;
  if(!preparation || preparation->audio_epoch!=state->script_epoch)
    throw FormatError("Native script cancellation belongs to another Audio owner");
  input.request_cancel();state->drain_script_cancellations();
}
bool MultiplayerAudio::request_script_stop(const SourceScriptSoundClockVoice64Tu15&input){
  auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();state->script_thread();const auto receipt=input;
  if(state->closing || !receipt.audio_owner_is(state->script_epoch,state->platform_storage))
    throw FormatError("Native script stop requires this actual Audio owner");
  const auto found=state->voices.find(receipt.id());
  if(found==state->voices.end())return false;
  auto&voice=found->second;
  if(!voice.script_record || !voice.script_record->current ||
     !receipt.voice_record_is(voice.script_record))
    throw FormatError("Native script stop lost actual published voice record");
  const auto time=std::bit_cast<std::int32_t>(std::uint32_t(state->platform.elapsed_frames()/48));
  // No observer/backend callback runs while the actual record is borrowed.
  // Sample any active fade at this time, including a repeated stop request.
  auto fade=voice.fade.value_or(SoundPlaybackFade{0,0,0,0,false});
  source_sound_request_stop_tu15(fade,time);voice.fade=fade;
  return true;
}
void MultiplayerAudio::listeners(std::span<const Vec3> positions){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  state->metadata_current();
  if(positions.size()>4)throw FormatError("World audio exceeds original local listener count");
  for(const auto &p:positions)if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
    throw FormatError("World audio listener has nonfinite position");
  state->listeners.assign(positions.begin(),positions.end());
  for(const auto &[id,v]:state->voices)if(v.position)state->parameters(v,state->parameters(v));
}
void MultiplayerAudio::emitter_impl(Voice id,Vec3 position){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  state->metadata_current();
  if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z))
    throw FormatError("World audio emitter has nonfinite position");
  auto &voice=state->voices.at(id);
  if(!voice.position)throw FormatError("Centered audio voice cannot acquire a world emitter");
  voice.position=position;
  state->parameters(voice,state->parameters(voice));
}
void MultiplayerAudio::impulse_at(std::uint32_t sound,Vec3 position,float control,
    MultiplayerAudioKind kind,std::uint32_t owner){
  (void)play_impulse_at(sound,position,control,kind,owner);
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::play_impulse_at(std::uint32_t sound,Vec3 position,float control,
    MultiplayerAudioKind kind,std::uint32_t owner){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  if(sound==none)return {};
  if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z)||!std::isfinite(control)||control<0)
    throw FormatError("Invalid world audio emitter or caller control");
  const auto id=state->play(sound,0,kind,owner,0,false,none,0,1,{},control,position);
  return id?std::optional<Voice>(id):std::nullopt;
}
std::optional<MultiplayerAudio::Voice> MultiplayerAudio::schedule_impulse_at(std::uint32_t sound,Vec3 position,
    std::uint64_t frame,float control,MultiplayerAudioKind kind,std::uint32_t owner){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  if(sound==none)return {};
  if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z)||!std::isfinite(control)||control<0)
    throw FormatError("Invalid world audio emitter or caller control");
  const auto id=state->play(sound,0,kind,owner,0,false,none,0,1,{},control,position,frame);
  return id?std::optional<Voice>(id):std::nullopt;
}
std::vector<MultiplayerAudio::Voice> MultiplayerAudio::interface_loop(std::uint32_t tag,std::uint32_t owner,unsigned view) {auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  state->metadata_current();
  const auto definition=state->cache.load_looping_sound(tag);
  if(state->cache.is_mcc() && (definition.flags&0x200u))return {};
  if(definition.flags&2u)throw FormatError("Interface loop requires ordinary retained looping sound");
  std::vector<Voice> voices;
  try {
    for(unsigned n=0;n<definition.tracks.size();++n) {
      const auto &track=definition.tracks[n];
      if(track.start!=none || track.end!=none || track.fade_in!=0)
        throw FormatError("Interface loop requires additional authored start/end/fade-in owner");
      if(track.loop!=none)if(const auto voice=state->play(track.loop,track.gain_db,
          MultiplayerAudioKind::Shield,owner,view,true,tag,n))voices.push_back(voice);
    }
  }catch(...) {for(const auto id:voices)stop_voice_impl(id);throw;}
  return voices;
}
void MultiplayerAudio::stop_voice_impl(Voice id) {auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
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
void MultiplayerAudio::ambient(std::span<const MultiplayerAmbientTarget> targets,float elapsed){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  state->metadata_current();
  if(!std::isfinite(elapsed)||elapsed<0)throw FormatError("Invalid ambient elapsed time");
  std::set<std::uint32_t> selected;
  for(const auto &target:targets){selected.insert(target.sound);auto found=state->ambience.find(target.sound);
    if(found==state->ambience.end()){
      const auto definition=state->cache.load_looping_sound(target.sound);
      if(state->cache.is_mcc() && (definition.flags&0x200u))continue;
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
        if(const auto voice=state->play(track.loop,track.gain_db,MultiplayerAudioKind::Ambient,none,0,true,target.sound,i,a.gain*a.linear_db))a.voices.push_back(voice);}}
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
    for(const auto id:a.voices){auto &v=state->voices.at(id);v.ambient_gain=a.gain*a.linear_db;state->parameters(v,state->parameters(v));}
    ++it;
  }
}
std::string MultiplayerAudio::speaker_layout(unsigned layout){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();return state->platform.speaker_layout(layout);}
void MultiplayerAudio::paused(bool paused){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  state->metadata_current();
  if(state->paused==paused)return;
  state->paused=paused;
  if(paused)state->platform.paused(true);
  for(const auto &[id,v]:state->voices)state->parameters(v,state->parameters(v));
  if(!paused)state->platform.paused(false);
}
SdlSoundVoiceParameters MultiplayerAudio::voice_parameters(Voice id)const{auto state=state_->core;state->script_producer();
  return state->parameters(state->voices.at(id));
}
SdlSoundPlaybackPosition MultiplayerAudio::playback_position(Voice id)const{auto state=state_->core;state->script_producer();
  if(const auto found=state->voices.find(id);found!=state->voices.end() && found->second.playback)
    return state->platform.playback_position(*found->second.playback);
  // Packet observation precedes Core map publication. Preserve this existing
  // read-only view of the real submitted stream; it cannot mint authority.
  return state->platform.playback_position(id);
}
void MultiplayerAudio::master_volume(float gain){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  state->metadata_current();
  if(!std::isfinite(gain)||gain<0||gain>1)throw FormatError("Invalid native master volume");
  state->master=gain;
  for(const auto &[id,v]:state->voices)state->parameters(v,state->parameters(v));
}
void MultiplayerAudio::class_gains(const std::array<float,sound_class_count> &gains){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  state->metadata_current();
  for(const auto x:gains)if(!std::isfinite(x)||x<-64||x>0)throw FormatError("Invalid native source class gain");
  state->class_gain=gains;for(const auto &[id,v]:state->voices)state->parameters(v,state->parameters(v));
}
void MultiplayerAudio::update(){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();
  state->metadata_current();
  state->drain_script_cancellations();
  const auto time=std::bit_cast<std::int32_t>(std::uint32_t(state->platform.elapsed_frames()/48));
  for(auto it=state->voices.begin();it!=state->voices.end();){auto &v=it->second;
    if(v.fade) {
      v.fade_db=source_sound_playback_fade(*v.fade,time);
      const auto elapsed=std::bit_cast<std::int32_t>(std::uint32_t(time)-std::uint32_t(v.fade->start));
      const bool falling=v.fade->start>v.fade->end;
      if(falling && (v.fade_db+v.gain_db<=-64.f || elapsed>=0)) {
        const auto id=v.id;++it;state->erase(id);continue;
      }
      if(!falling && std::bit_cast<std::int32_t>(std::uint32_t(time)-std::uint32_t(v.fade->end))>=0)
        v.fade.reset();
      state->parameters(v,state->parameters(v));
    }
    bool completed=false,failed=false;
    std::optional<SdlSoundCompletion> packet;
    while(auto c=state->completion(v)){completed=true;packet=c->value;failed|=c->value.status!=SdlSoundCompletionStatus::Success;state->acknowledge(v,*c);}
    if(completed){if(v.loop_packet && !failed &&
          (v.playback?state->platform.status(*v.playback):state->platform.status(v.id)).pending_packets){++it;}
      else if(v.loop&&!failed){state->packet(v,true);if(state->closing)break;++it;}
      else{const auto id=v.id;++it;state->erase(id,packet);}}
    else ++it;
  }
  state->complete();
}
void MultiplayerAudio::stop(){auto state=state_->core;State::Core::Mutation mutation(*state);state->script_producer();while(!state->voices.empty())state->erase(state->voices.begin()->first);state->ambience.clear();state->object_loops.clear();state->phased_loops={};state->complete();state->drain_script_cancellations();}
std::size_t MultiplayerAudio::voices()const{auto state=state_->core;state->script_producer();return state->voices.size();}
std::vector<float> MultiplayerAudio::render(std::size_t frames){auto state=state_->core;state->script_producer();state->metadata_current();return state->platform.render(frames);}
std::uint64_t MultiplayerAudio::mixed_frames()const{auto state=state_->core;state->script_producer();return state->platform.rendered_frames();}
std::uint64_t MultiplayerAudio::presentation_frames()const{auto state=state_->core;state->script_producer();return state->platform.presentation_frames();}
bool MultiplayerAudio::continuation_enrolled()const noexcept{
  const auto&c=*state_->core;return c.executor==std::this_thread::get_id() && c.public_alive && !c.closing &&
    c.continuation_enrolled.load() && c.script_epoch->alive.load();
}
bool MultiplayerAudio::VoiceReceipt::same(const VoiceReceipt&b)const noexcept{
  return owner_pointer_==b.owner_pointer_ && !owner_.owner_before(b.owner_) && !b.owner_.owner_before(owner_) &&
    id_==b.id_ && epoch_==b.epoch_ && record_.get()==b.record_.get() &&
    !record_.owner_before(b.record_) && !b.record_.owner_before(record_);
}
std::optional<MultiplayerAudio::VoiceReceipt> MultiplayerAudio::play_impulse_receipt(std::uint32_t sound,float control,
    MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,Completion completion){
  const auto c=state_->core;State::Core::Mutation operation(*c);
  if(!c->continuation_enrolled.load())throw FormatError("Native Audio typed playback requires explicit enrollment");
  const auto id=play_impulse(sound,control,kind,owner,view,std::move(completion));
  if(!id)return {};return state_->core->receipt(state_->core->voices.at(*id));
}
std::optional<MultiplayerAudio::VoiceReceipt> MultiplayerAudio::schedule_impulse_receipt(std::uint32_t sound,std::uint64_t frame,
    float control,MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,Completion completion){
  const auto c=state_->core;State::Core::Mutation operation(*c);
  if(!c->continuation_enrolled.load())throw FormatError("Native Audio typed playback requires explicit enrollment");
  const auto id=schedule_impulse(sound,frame,control,kind,owner,view,std::move(completion));
  if(!id)return {};return c->receipt(c->voices.at(*id));
}
std::optional<MultiplayerAudio::VoiceReceipt> MultiplayerAudio::play_impulse_at_receipt(std::uint32_t sound,Vec3 point,
    float control,MultiplayerAudioKind kind,std::uint32_t owner){
  const auto c=state_->core;State::Core::Mutation operation(*c);
  if(!c->continuation_enrolled.load())throw FormatError("Native Audio typed playback requires explicit enrollment");
  const auto id=play_impulse_at(sound,point,control,kind,owner);
  if(!id)return {};return state_->core->receipt(state_->core->voices.at(*id));
}
std::vector<MultiplayerAudio::VoiceReceipt> MultiplayerAudio::interface_loop_receipts(std::uint32_t sound,
    std::uint32_t owner,unsigned view){
  const auto c=state_->core;State::Core::Mutation operation(*c);
  if(!c->continuation_enrolled.load())throw FormatError("Native Audio typed playback requires explicit enrollment");
  const auto ids=interface_loop(sound,owner,view);std::vector<VoiceReceipt> out;
  try{out.reserve(ids.size());
    for(const auto id:ids)out.push_back(c->receipt(c->voices.at(id)));return out;
  }catch(...){
    const auto failure=std::current_exception();
    for(const auto id:ids)if(c->voices.contains(id))try{c->erase(id);}catch(...){}
    try{c->complete();}catch(...){}std::rethrow_exception(failure);
  }
}
std::optional<MultiplayerAudio::VoiceReceipt> MultiplayerAudio::play_companion_impulse(std::uint32_t sound,float control,
    MultiplayerAudioKind kind,std::uint32_t owner,unsigned view,CompanionCompletion companion){
  const auto state=state_->core;State::Core::Mutation operation(*state);
  if(sound==none)return {};
  if(!state->continuation_enrolled.load() || companion.owner.expired() || !companion.branch ||
      !companion.branch.use_count() || !companion.dispatch)
    throw FormatError("Native Audio companion requires its real enrolled owner and branch");
  if(!std::isfinite(control)||control<0)throw FormatError("Invalid native sound caller control");
  const auto id=state->play(sound,0,kind,owner,view,false,none,0,1,{},control,{},{},{},{},nullptr,std::move(companion));
  if(!id)return {};return state->receipt(state->voices.at(id));
}
void MultiplayerAudio::emitter(Voice id,Vec3 point){
  const auto c=state_->core;c->script_producer();c->raw_mutation();emitter_impl(id,point);
}
void MultiplayerAudio::emitter(const VoiceReceipt&r,Vec3 point){
  const auto c=state_->core;c->script_producer();c->require_receipt(r);emitter_impl(r.id_,point);
}
void MultiplayerAudio::stop_voice(Voice id){
  const auto c=state_->core;c->script_producer();c->raw_mutation();stop_voice_impl(id);
}
void MultiplayerAudio::stop_voice(const VoiceReceipt&r){
  const auto c=state_->core;c->script_producer();c->require_receipt(r);stop_voice_impl(r.id_);
}
bool MultiplayerAudio::voice_active(const VoiceReceipt&r)const{
  const auto c=state_->core;c->script_producer();return c->accepts(r);
}
SdlSoundVoiceParameters MultiplayerAudio::voice_parameters(const VoiceReceipt&r)const{
  const auto c=state_->core;c->script_producer();c->require_receipt(r);return c->parameters(c->voices.at(r.id_));
}
SdlSoundPlaybackPosition MultiplayerAudio::playback_position(const VoiceReceipt&r)const{
  const auto c=state_->core;c->script_producer();c->require_receipt(r);
  const auto&v=c->voices.at(r.id_);return c->platform.playback_position(*v.playback);
}

struct MultiplayerAudio::ContinuationSnapshot {
  using Core=State::Core;
  struct Values {
    decltype(Core::banks) banks;
    decltype(Core::voices) voices;
    decltype(Core::ambience) ambience;
    decltype(Core::object_loops) object_loops;
    SourceLoopPhasedStateTu15 phased_loops;
    decltype(Core::retired) retired;
    decltype(Core::listeners) listeners;
    SoundLocalRandom random{0};
    std::array<float,sound_class_count> class_gain{};
    float master{};std::uint64_t next_packet{};bool paused{},script_bound{};
    std::optional<MccSoundVariationProvider> native_metadata;
    std::shared_ptr<ScriptAudioEpoch> script_epoch;
    decltype(Core::script_preparations) preparations;
    decltype(Core::script_closers) closers;
    decltype(Core::script_retirements) retirements;
    decltype(Core::script_clocks) clocks;
    std::shared_ptr<ScriptCancellationDomain> cancellation;
  } values;
  struct Record {
    std::shared_ptr<const Core::ScriptVoiceRecord> source;
    std::uint64_t id{};bool current{},retirement{};
    std::weak_ptr<SourceScriptSoundClockNative64Tu15> clock;
    std::optional<SourceScriptSoundClockAdmission64Tu15> admission;
    std::optional<SourceScriptSoundClockVoice64Tu15> voice;
  };
  struct Clock {
    std::shared_ptr<SourceScriptSoundClockNative64Tu15> owner;
    SourceScriptSoundClockNative64Tu15::Checkpoint checkpoint;
  };
  struct Preparation {
    std::shared_ptr<const MultiplayerPreparedScriptImpulseTu15::Impl> source;
    MultiplayerPreparedScriptImpulseTu15::Impl::Status status{};
    std::shared_ptr<SourceScriptSoundClockNative64Tu15> clock;
    std::optional<SourceScriptSoundClockAdmission64Tu15> admission;
    std::uint32_t sound{};SourceScriptSoundClockSelectionTu15 selection;
  };
  std::weak_ptr<Core> owner;const Core* owner_pointer{};
  std::shared_ptr<const Cache> cache;
  SdlSoundPlatform::Checkpoint platform;
  std::vector<Record> records;
  std::vector<Clock> clocks;
  std::vector<Preparation> preparations;
  std::vector<CompanionVoice> companions;
  std::uint32_t* auxiliary{};std::optional<std::uint32_t> auxiliary_value;
  std::shared_ptr<const unsigned char> loop_random_domain;
  std::uint64_t source_epoch{};
};
struct MultiplayerAudio::ContinuationPrepared {
  using Core=State::Core;
  enum class Phase {Pending,CancelRequested,Committing,Consumed};
  std::atomic<Phase> phase{Phase::Pending};
  const std::thread::id executor=std::this_thread::get_id();
  std::weak_ptr<Core> owner;const Core* owner_pointer{};
  std::shared_ptr<const ContinuationSnapshot> source;
  ContinuationSnapshot::Values values;
  SdlSoundPlatform::PreparedReplacement platform;
  struct Clock {
    std::shared_ptr<SourceScriptSoundClockNative64Tu15> owner;
    SourceScriptSoundClockNative64Tu15::PreparedReplacement prepared;
  };
  struct Receipt {VoiceReceipt source,target;};
  struct ScriptPreparation {
    std::shared_ptr<const MultiplayerPreparedScriptImpulseTu15::Impl> source;
    std::shared_ptr<MultiplayerPreparedScriptImpulseTu15::Impl> target;
    bool issued{};
  };
  std::vector<Clock> clocks;
  std::vector<Receipt> receipts;
  std::vector<ScriptPreparation> preparations;
  std::optional<EffectAudioAuxiliary> auxiliary;
  // A transferable preparation never owns a native/timing exclusion. The
  // actual prospective aggregate retains the creator-owned native scope.
  std::optional<MccSoundVariationProvider::NativeBinding> native_target;
  std::shared_ptr<const unsigned char> publication_record=std::make_shared<const unsigned char>(0);
  std::shared_ptr<const unsigned char> swap_publication_record=publication_record;
  std::uint64_t expected_revision{},target_epoch{},installed_revision{};
  std::shared_ptr<ScriptCancellationDomain> source_cancellation;
  std::uint64_t expected_cancellation{};
  const PublicationHold* intent{};
  void cancel()noexcept{
    auto expected=Phase::Pending;
    if(phase.compare_exchange_strong(expected,Phase::CancelRequested) && values.cancellation)
      values.cancellation->life.store(ScriptCancellationDomain::Life::Retired);
  }
  bool cancellation_ready()const noexcept{
    return source_cancellation && values.cancellation &&
      source_cancellation->requests.load()==expected_cancellation && !source_cancellation->active.load() &&
      values.cancellation->requests.load()==0 && !values.cancellation->active.load();
  }
};
struct MultiplayerAudio::PublicationHold {
  std::shared_ptr<State::Core> core;
  std::shared_ptr<ContinuationPrepared> prepared;
  const std::thread::id thread=std::this_thread::get_id();
  struct Busy {
    State::Core&core;
    ContinuationPrepared&prepared;
    explicit Busy(State::Core&c,ContinuationPrepared&p):core(c),prepared(p){core.held=true;core.held_preparation=&p;}
    ~Busy(){
      // SDL exclusion has ended. Only now release the cancellation reservation;
      // queued requests are drained by the next ordinary producer operation.
      if(prepared.intent){
        if(prepared.phase.load()==ContinuationPrepared::Phase::Consumed)core.cancellation->release();
        else{
          prepared.phase.store(ContinuationPrepared::Phase::CancelRequested);
          prepared.source_cancellation->release();
          prepared.values.cancellation->life.store(ScriptCancellationDomain::Life::Retired);
          prepared.values.cancellation->release();
        }
        prepared.intent=nullptr;
      }
      core.held=false;core.held_preparation=nullptr;core.maybe_shutdown();
    }
  } busy;
  // Member order releases SDL before timing, then the producer exclusion and
  // retained outgoing Audio storage. Native exclusion remains aggregate-owned.
  std::unique_ptr<MccSoundVariationProvider::HeldNativeBinding> native;
  SdlSoundPlatform::HeldPublication platform;
  PublicationHold(std::shared_ptr<State::Core>c,std::shared_ptr<ContinuationPrepared>p):
    core(std::move(c)),prepared(std::move(p)),busy(*core,*prepared),
    native(prepared->native_target?new MccSoundVariationProvider::HeldNativeBinding(
      MccSoundVariationProvider::hold_native_binding(*prepared->native_target)):nullptr),
    platform(core->platform.hold_publication(prepared->platform)){}
};
namespace {
template<class A,class B>bool audio_same_owner(const A&a,const B&b)noexcept{
  return a.get()==b.get() && !a.owner_before(b) && !b.owner_before(a);
}
bool audio_same_authored_definition(const SoundVariationDefinition &a,const SoundVariationDefinition &b){
  if(a.datum!=b.datum || a.sound!=b.sound || a.playback!=b.playback ||
      a.sound_class!=b.sound_class || a.scale!=b.scale || a.sample_rate_index!=b.sample_rate_index ||
      a.permutations!=b.permutations || a.pitch_ranges.size()!=b.pitch_ranges.size() ||
      a.used.size()!=b.used.size())return false;
  for(unsigned n=0;n<a.range.size();++n)if(n!=4&&n!=5&&a.range[n]!=b.range[n])return false;
  for(unsigned n=0;n<a.pitch_ranges.size();++n)if(a.pitch_ranges[n].interval!=b.pitch_ranges[n].interval ||
      a.pitch_ranges[n].permutations!=b.pitch_ranges[n].permutations)return false;
  return true;
}
}
std::optional<std::size_t> MultiplayerAudio::Checkpoint::voices()const noexcept{
  if(!data_)return {};return data_->values.voices.size();
}
std::vector<MultiplayerAudio::CompanionVoice> MultiplayerAudio::Checkpoint::companion_completions()const{
  if(!data_)throw FormatError("Empty Native Audio checkpoint");return data_->companions;
}
MultiplayerAudio::PreparedReplacement::~PreparedReplacement(){if(data_)data_->cancel();}
MultiplayerAudio::PreparedReplacement::PreparedReplacement(PreparedReplacement&&b)noexcept:data_(std::move(b.data_)){}
MultiplayerAudio::PreparedReplacement& MultiplayerAudio::PreparedReplacement::operator=(PreparedReplacement&&b)noexcept{
  if(this!=&b){if(data_)data_->cancel();data_=std::move(b.data_);}return *this;
}
std::optional<MultiplayerAudio::VoiceReceipt> MultiplayerAudio::PreparedReplacement::voice(const VoiceReceipt&source)const noexcept{
  if(!data_)return {};const auto phase=data_->phase.load();
  if(phase==ContinuationPrepared::Phase::CancelRequested)return {};
  for(const auto&r:data_->receipts)if(r.source.same(source))return r.target;return {};
}
std::optional<std::size_t> MultiplayerAudio::PreparedReplacement::voices()const noexcept{
  if(!data_)return {};const auto phase=data_->phase.load();
  if(phase==ContinuationPrepared::Phase::CancelRequested)return {};
  return data_->source->values.voices.size();
}
bool MultiplayerAudio::PreparedReplacement::for_audio(const MultiplayerAudio&audio)const noexcept{
  if(!data_ || data_->executor!=std::this_thread::get_id())return false;
  const auto c=data_->owner.lock();const auto phase=data_->phase.load();
  return c && c.get()==audio.state_->core.get() && c.get()==data_->owner_pointer && c->public_alive &&
    !c->closing && c->script_epoch->thread==std::this_thread::get_id() &&
    phase!=ContinuationPrepared::Phase::CancelRequested &&
    (phase==ContinuationPrepared::Phase::Consumed || !data_->native_target || data_->native_target->current());
}
bool MultiplayerAudio::PreparedReplacement::published()const noexcept{
  if(!data_ || data_->executor!=std::this_thread::get_id() || data_->phase.load()!=ContinuationPrepared::Phase::Consumed)return false;
  const auto c=data_->owner.lock();return c && c->public_alive && !c->closing &&
    c->script_epoch->thread==std::this_thread::get_id() && c->publication_epoch==data_->target_epoch &&
    c->revision==data_->installed_revision && audio_same_owner(c->publication_record,data_->publication_record) &&
    ((c->held_preparation==data_.get() && data_->intent) ||
      (!c->cancellation->requests.load() && !c->cancellation->active.load())) &&
    (!data_->auxiliary || data_->auxiliary->committed(data_->source->auxiliary,*data_->source->auxiliary_value));
}
std::optional<MultiplayerPreparedScriptImpulseTu15> MultiplayerAudio::PreparedReplacement::script(
    const MultiplayerPreparedScriptImpulseTu15&old){
  if(!data_ || data_->executor!=std::this_thread::get_id() || !old.impl_)return {};
  if(data_->phase.load()==ContinuationPrepared::Phase::CancelRequested)return {};
  for(auto&p:data_->preparations)if(audio_same_owner(p.source,old.impl_)){
    if(p.issued)return {};p.issued=true;return MultiplayerPreparedScriptImpulseTu15(p.target);
  }
  return {};
}
MultiplayerAudio::HeldPublication::HeldPublication(std::unique_ptr<PublicationHold>p):data_(std::move(p)){}
MultiplayerAudio::HeldPublication::~HeldPublication(){
  if(data_ && data_->thread!=std::this_thread::get_id())std::terminate();
}
MultiplayerAudio::Checkpoint MultiplayerAudio::checkpoint()const{
  const auto c=state_->core;c->script_producer();
  if(!c->continuation_enrolled.load() || !c->cache_owner || !c->public_alive || c->closing ||
      c->operation_depth || c->play_depth || c->completion_depth || c->revision==UINT64_MAX)
    throw FormatError("Native Audio checkpoint requires enrolled quiescent owned playback");
  struct Transaction {State::Core&c;explicit Transaction(State::Core&v):c(v){c.transaction=true;}~Transaction(){c.transaction=false;c.maybe_shutdown();}} transaction(*c);
  c->merge_script_cancellations();
  const auto cancellation=c->cancellation;
  if(!cancellation->reserve(cancellation->requests.load()))
    throw FormatError("Native Audio script cancellation is in progress or exhausted");
  struct Release {ScriptCancellationDomain&d;~Release(){d.release();}} release{*cancellation};
  c->metadata_current();
  auto image=std::make_shared<ContinuationSnapshot>();image->owner=c;image->owner_pointer=c.get();image->cache=c->cache_owner;
  image->source_epoch=c->publication_epoch;
  image->loop_random_domain=c->loop_random_domain;
  image->auxiliary=c->shared_variation_random;
  if(image->auxiliary)image->auxiliary_value=*image->auxiliary;
  const auto append_record=[&](std::shared_ptr<const State::Core::ScriptVoiceRecord> record){
    if(!record)throw FormatError("Native Audio has an absent script record");
    for(const auto&old:image->records)if(audio_same_owner(old.source,record))return;
    ContinuationSnapshot::Record value;value.source=record;value.id=record->id;value.current=record->current;
    value.retirement=bool(record->retirement);value.clock=record->clock;value.admission=record->admission;
    if(const auto voice=record->source_voice.lock())value.voice=SourceScriptSoundClockVoice64Tu15(voice);
    image->records.push_back(std::move(value));
  };
  for(const auto&[id,v]:c->voices){
    if(v.completion)throw FormatError("Native Audio continuation needs the actual opaque completion owner");
    if(!v.playback || !v.record || !v.record.use_count())throw FormatError("Native Audio voice lacks typed retained ownership");
    (void)c->platform.status(*v.playback);
    if(v.script_record)append_record(v.script_record);
    if(v.companion)image->companions.push_back({c->receipt(v),*v.companion});
  }
  for(const auto&r:c->retired){
    if(r.owner)throw FormatError("Native Audio continuation needs its actual retired completion owner");
    if(r.companion)image->companions.push_back({r.voice,*r.companion});
  }
  for(const auto&r:c->script_retirements)append_record(r);
  for(const auto&clock:c->script_clocks){
    std::vector<SourceScriptSoundClockAdmission64Tu15> admissions;
    std::vector<SourceScriptSoundClockVoice64Tu15> voices;
    for(const auto&r:image->records)if(const auto owner=r.clock.lock();owner && audio_same_owner(owner,clock)){
      if(r.admission)admissions.push_back(*r.admission);if(r.voice)voices.push_back(*r.voice);
    }
    for(const auto&p:c->script_preparations)if(audio_same_owner(p->clock,clock) && p->admission)admissions.push_back(*p->admission);
    auto cp=clock->checkpoint(admissions,voices);
    for(const auto&voice:cp.voices()){
      if(!voice.audio_owner_is(c->script_epoch,c->platform_storage))throw FormatError("Native Audio clock has a foreign voice owner");
      append_record(std::static_pointer_cast<const State::Core::ScriptVoiceRecord>(voice.voice_record()));
    }
    image->clocks.push_back({clock,std::move(cp)});
  }
  for(const auto&p:c->script_preparations)image->preparations.push_back({p,p->status.load(),p->clock,p->admission,p->sound,p->selection});
  auto&v=image->values;v.banks=c->banks;v.voices=c->voices;v.ambience=c->ambience;v.object_loops=c->object_loops;v.phased_loops=c->phased_loops;v.retired=c->retired;
  v.listeners=c->listeners;v.random=c->random;v.class_gain=c->class_gain;v.master=c->master;v.next_packet=c->next_packet;
  v.paused=c->paused;v.script_bound=c->script_bound.load();v.native_metadata=c->native_metadata;v.script_epoch=c->script_epoch;
  v.clocks=c->script_clocks;
  v.retirements=c->script_retirements;
  image->platform=c->platform.checkpoint();
  Checkpoint out;out.data_=std::move(image);return out;
}

MultiplayerAudio::PreparationIdentity MultiplayerAudio::PreparedReplacement::identity()const noexcept{
  PreparationIdentity out;
  if(data_){out.owner_=data_;out.pointer_=data_.get();}return out;
}
bool MultiplayerAudio::PreparedReplacement::matches(const PreparationIdentity&i)const noexcept{
  if(!data_ || data_->executor!=std::this_thread::get_id())return false;const auto owner=i.owner_.lock();
  if(!owner || i.pointer_!=data_.get() || !audio_same_owner(owner,data_))return false;
  const auto c=data_->owner.lock();if(!c || !c->public_alive || c->closing ||
      c->script_epoch->thread!=std::this_thread::get_id())return false;
  const auto phase=data_->phase.load();
  if(phase==ContinuationPrepared::Phase::Consumed)return published();
  return (phase==ContinuationPrepared::Phase::Pending ||
      (phase==ContinuationPrepared::Phase::Committing && data_->intent && c->held_preparation==data_.get())) &&
    (phase==ContinuationPrepared::Phase::Committing || data_->cancellation_ready()) &&
    (!data_->native_target || data_->native_target->current()) &&
    c->revision==data_->expected_revision &&
    (!data_->auxiliary || data_->auxiliary->ready(data_->source->auxiliary,*data_->source->auxiliary_value));
}
bool MultiplayerAudio::PreparedReplacement::auxiliary_agrees(const std::uint32_t*binding,std::uint32_t value)const noexcept{
  return data_ && data_->source->auxiliary==binding && data_->source->auxiliary_value==value;
}
MultiplayerAudio::PreparedReplacement MultiplayerAudio::prepare_restore(const Checkpoint&cp)const{
  return prepare_restore(cp,CompanionBinder{},nullptr);
}
MultiplayerAudio::PreparedReplacement MultiplayerAudio::prepare_restore(const Checkpoint&cp,const CompanionBinder&binder)const{
  return prepare_restore(cp,binder,nullptr);
}
MultiplayerAudio::PreparedReplacement MultiplayerAudio::prepare_restore(const Checkpoint&cp,const EffectAudioAuxiliary&auxiliary)const{
  return prepare_restore(cp,CompanionBinder{},&auxiliary);
}
MultiplayerAudio::PreparedReplacement MultiplayerAudio::prepare_restore(const Checkpoint&cp,
    const MccSoundVariationProvider::NativeBinding&target)const{
  return prepare_restore(cp,CompanionBinder{},nullptr,&target);
}
MultiplayerAudio::PreparedReplacement MultiplayerAudio::prepare_restore(const Checkpoint&cp,const EffectAudioAuxiliary&auxiliary,
    const MccSoundVariationProvider::NativeBinding&target)const{
  return prepare_restore(cp,CompanionBinder{},&auxiliary,&target);
}
MultiplayerAudio::PreparedReplacement MultiplayerAudio::prepare_restore(const Checkpoint&cp,const CompanionBinder&binder,
    const EffectAudioAuxiliary*auxiliary)const{
  return prepare_restore(cp,binder,auxiliary,nullptr);
}
MultiplayerAudio::PreparedReplacement MultiplayerAudio::prepare_restore(const Checkpoint&cp,const CompanionBinder&binder,
    const EffectAudioAuxiliary*auxiliary,const MccSoundVariationProvider::NativeBinding*target)const{
  const auto c=state_->core;c->script_producer();const auto source=cp.data_;
  if(!source || source->owner.lock().get()!=c.get() || source->owner_pointer!=c.get() ||
      !audio_same_owner(source->cache,c->cache_owner) || !c->continuation_enrolled.load() ||
      !c->public_alive || c->closing || c->operation_depth || c->play_depth || c->completion_depth ||
      c->revision>=UINT64_MAX-1 || c->publication_epoch==UINT64_MAX)
    throw FormatError("Native Audio replacement requires its exact owned quiescent endpoint");
  for(const auto&[id,v]:c->voices)if(v.completion)
    throw FormatError("Native Audio outgoing playback requires its actual opaque completion owner");
  if(!audio_same_owner(source->loop_random_domain,c->loop_random_domain))
    throw FormatError("Native Audio replacement changed its actual loop random service owner");
  for(const auto&r:c->retired)if(r.owner)
    throw FormatError("Native Audio outgoing retirement requires its actual opaque completion owner");
  // These owners must provide an actual staged target binding. Never infer a
  // future native lease or restore an effects-owned word from a caller scalar.
  std::optional<MccSoundVariationProvider> target_metadata;
  if(target)target_metadata=MccSoundVariationProvider::binding_provider(*target,source->cache);
  else if(source->values.native_metadata || c->native_metadata)
    throw FormatError("Native Audio replacement requires its genuine target metadata binding");
  if(source->auxiliary!=c->shared_variation_random ||
      (source->auxiliary && (!auxiliary || !source->auxiliary_value ||
        !auxiliary->ready(source->auxiliary,*source->auxiliary_value))) ||
      (!source->auxiliary && auxiliary))
    throw FormatError("Native Audio replacement requires its genuine effects target agreement");
  struct Transaction {State::Core&c;explicit Transaction(State::Core&v):c(v){c.transaction=true;}~Transaction(){c.transaction=false;c.maybe_shutdown();}} transaction(*c);
  auto out=std::make_shared<ContinuationPrepared>();out->owner=c;out->owner_pointer=c.get();out->source=source;
  out->expected_revision=c->revision;out->target_epoch=c->publication_epoch+1;
  out->source_cancellation=c->cancellation;out->expected_cancellation=c->cancellation->requests.load();
  if((out->expected_cancellation&1u) || c->cancellation->active.load() || out->expected_cancellation>=UINT64_MAX-2)
    throw FormatError("Native Audio script cancellation is in progress or exhausted");
  if(auxiliary)out->auxiliary=*auxiliary;
  if(target)out->native_target=*target;
  out->values=source->values;out->values.script_epoch=std::make_shared<ScriptAudioEpoch>();
  if(target_metadata){
    for(auto &[key,bank]:out->values.banks){
      auto definition=target_metadata->load(c->cache,TagHandle{key.first},key.second);
      if(!audio_same_authored_definition(bank.definition,definition))
        throw FormatError("Native Audio target authored metadata or PCM ordering differs");
      // Native last/forced/used snapshots may differ. The captured Audio
      // range/loop histories remain separate real owners and are not reset.
      bank.definition=std::move(definition);
    }
    out->values.native_metadata=std::move(target_metadata);
  }
  out->values.cancellation=std::make_shared<ScriptCancellationDomain>(ScriptCancellationDomain::Life::Prospective);
  out->values.preparations.clear();out->values.closers.clear();out->values.retirements.clear();
  out->platform=c->platform.prepare_restore(source->platform);
  const auto count=out->platform.voices();
  if(!count || *count!=source->values.voices.size())throw FormatError("Native Audio platform/Core voice census differs");
  const auto target_receipt=[&](const VoiceReceipt&old,std::shared_ptr<const unsigned char>record){
    for(const auto&r:out->receipts)if(r.source.same(old))return r.target;
    VoiceReceipt fresh;fresh.owner_=c;fresh.owner_pointer_=c.get();fresh.id_=old.id_;fresh.epoch_=out->target_epoch;
    fresh.record_=std::move(record);out->receipts.push_back({old,fresh});return fresh;
  };
  for(auto&[id,v]:out->values.voices){
    const auto&old=source->values.voices.at(id);
    if(!old.playback || !old.record)throw FormatError("Native Audio checkpoint lost an actual stream record");
    auto playback=out->platform.voice(old.playback->id());
    if(!playback || playback->id()!=id)throw FormatError("Native Audio target stream membership differs");
    v.playback=std::move(playback);v.record=std::make_shared<const unsigned char>(0);
    VoiceReceipt saved;saved.owner_=source->owner;saved.owner_pointer_=source->owner_pointer;
    saved.id_=id;saved.epoch_=source->source_epoch;
    saved.record_=old.record;
    (void)target_receipt(saved,v.record);
  }
  // The checkpoint contains exact Core receipt identities independently of
  // script epochs. They include pending typed completions whose stream retired.
  for(const auto&saved:source->companions){
    std::shared_ptr<const unsigned char> record;
    if(const auto found=out->values.voices.find(saved.voice.id());found!=out->values.voices.end() &&
        audio_same_owner(source->values.voices.at(saved.voice.id()).record,saved.voice.record_))record=found->second.record;
    else record=std::make_shared<const unsigned char>(0);
    (void)target_receipt(saved.voice,std::move(record));
  }
  struct RecordMap {
    const ContinuationSnapshot::Record* saved{};
    std::shared_ptr<State::Core::ScriptVoiceRecord> target;
  };
  std::vector<RecordMap> records;records.reserve(source->records.size());
  for(const auto&r:source->records){
    auto fresh=std::make_shared<State::Core::ScriptVoiceRecord>();fresh->id=r.id;fresh->current=r.current;
    fresh->clock=r.clock;records.push_back({&r,std::move(fresh)});
  }
  const auto record_for=[&](const std::shared_ptr<const State::Core::ScriptVoiceRecord>&old)->RecordMap&{
    const auto found=std::find_if(records.begin(),records.end(),[&](const auto&r){return audio_same_owner(r.saved->source,old);});
    if(found==records.end())throw FormatError("Native Audio source script record census is incomplete");return *found;
  };
  for(auto&[id,v]:out->values.voices)if(v.script_record)v.script_record=record_for(v.script_record).target;
  const auto new_epoch=out->values.script_epoch;
  const std::weak_ptr<State::Core> weak_core=c;
  const std::weak_ptr<SdlSoundPlatform> weak_platform=c->platform_storage;
  std::vector<std::pair<SourceScriptSoundClockVoice64Tu15,SourceScriptSoundClockVoice64Tu15>> mapped_voices;
  const auto mapper=[&](const SourceScriptSoundClockVoice64Tu15&old){
    for(const auto&pair:mapped_voices)if(audio_same_owner(pair.first.impl_,old.impl_))return pair.second;
    if(!old.audio_owner_is(source->values.script_epoch,c->platform_storage))
      throw FormatError("Native Audio saved script voice belongs to a foreign binding");
    auto&record=record_for(std::static_pointer_cast<const State::Core::ScriptVoiceRecord>(old.voice_record()));
    if(old.id()!=record.saved->id)throw FormatError("Native Audio saved script voice/record identity differs");
    std::optional<SdlSoundPlatform::VoiceReceipt> playback;
    if(const auto found=out->values.voices.find(old.id());found!=out->values.voices.end() &&
        audio_same_owner(found->second.script_record,record.target))playback=found->second.playback;
    if(record.saved->current && !playback)throw FormatError("Native Audio current script voice has no actual target stream");
    const auto target_record=record.target;const auto id=old.id();
    auto fresh=SourceScriptSoundClockVoice64Tu15::published(new_epoch,c->platform_storage,target_record,id,
      [weak_core,new_epoch,weak_platform,target_record,id,playback]{
        const auto core=weak_core.lock();if(!core || !core->public_alive || core->held || core->script_epoch!=new_epoch ||
            !new_epoch->alive.load() || !target_record->current || target_record->id!=id)return false;
        const auto platform=weak_platform.lock();if(!platform || !playback)return false;
        (void)platform->status(*playback);return true;
      });
    if(record.saved->voice && audio_same_owner(record.saved->voice->impl_,old.impl_))record.target->source_voice=fresh.impl_;
    mapped_voices.emplace_back(old,fresh);return fresh;
  };
  for(const auto&clock:source->clocks){
    auto target=clock.owner->prepare_restore(clock.checkpoint,new_epoch,[weak_core,new_epoch]{
      const auto core=weak_core.lock();return core && core->public_alive && !core->held && core->script_epoch==new_epoch && new_epoch->alive.load();
    },mapper);
    out->values.closers.push_back(target.close_audio_binding());out->clocks.push_back({clock.owner,std::move(target)});
  }
  const auto clock_for=[&](const std::shared_ptr<SourceScriptSoundClockNative64Tu15>&owner)->ContinuationPrepared::Clock&{
    const auto found=std::find_if(out->clocks.begin(),out->clocks.end(),[&](const auto&v){return audio_same_owner(v.owner,owner);});
    if(found==out->clocks.end())throw FormatError("Native Audio script clock census is incomplete");return *found;
  };
  for(auto&r:records){
    const auto owner=r.saved->clock.lock();
    if(r.saved->admission && owner){
      auto&clock=clock_for(owner);r.target->admission=clock.prepared.admission(*r.saved->admission);
      if(!r.target->admission)throw FormatError("Native Audio saved script admission was not mapped");
      if(r.saved->voice){
        auto voice=clock.prepared.voice(*r.saved->voice);
        if(!voice)throw FormatError("Native Audio saved script voice was not mapped");
        r.target->source_voice=voice->impl_;
        if(r.saved->retirement)r.target->retirement=clock.prepared.retirement_callback(*r.target->admission,*voice);
      }
    }
    if(r.saved->retirement && !r.target->retirement)r.target->retirement=[]{return true;};
  }
  for(const auto&old:source->values.retirements)out->values.retirements.push_back(record_for(old).target);
  for(const auto&old:source->preparations){
    auto target=std::make_shared<MultiplayerPreparedScriptImpulseTu15::Impl>();target->status.store(old.status);
    target->audio_epoch=new_epoch;target->clock=old.clock;target->sound=old.sound;target->selection=old.selection;
    target->cancellation=out->values.cancellation;
    if(old.admission){target->admission=clock_for(old.clock).prepared.admission(*old.admission);
      if(!target->admission)throw FormatError("Native Audio pending script admission was not mapped");}
    out->preparations.push_back({old.source,target});out->values.preparations.push_back(std::move(target));
  }
  const auto bind=[&](const CompanionCompletion&old,const VoiceReceipt&voice){
    if(!binder)throw FormatError("Native Audio replacement requires its genuine typed completion companion");
    auto fresh=binder(old,voice);const auto a=old.owner.lock(),b=fresh.owner.lock();
    if(!a || !b || !audio_same_owner(a,b) || !fresh.branch || !fresh.branch.use_count() ||
        old.branch.get()==fresh.branch.get() ||
        (!old.branch.owner_before(fresh.branch) && !fresh.branch.owner_before(old.branch)) ||
        fresh.token!=old.token || fresh.queued!=old.queued || fresh.dispatch!=old.dispatch)
      throw FormatError("Native Audio companion target owner, branch or descriptor differs");
    return fresh;
  };
  for(auto&[id,v]:out->values.voices)if(v.companion){
    const auto found=std::find_if(source->companions.begin(),source->companions.end(),[&](const auto&x){return x.voice.id()==id && audio_same_owner(x.voice.record_,source->values.voices.at(id).record);});
    if(found==source->companions.end())throw FormatError("Native Audio companion source census differs");
    const auto fresh=std::find_if(out->receipts.begin(),out->receipts.end(),[&](const auto&x){return x.source.same(found->voice);});
    v.companion=bind(*v.companion,fresh->target);
  }
  for(auto&r:out->values.retired)if(r.companion){
    const auto fresh=std::find_if(out->receipts.begin(),out->receipts.end(),[&](const auto&x){return x.source.same(r.voice);});
    if(fresh==out->receipts.end())throw FormatError("Native Audio retired companion source census differs");
    r.companion=bind(*r.companion,fresh->target);r.voice=fresh->target;
  }
  PreparedReplacement result;result.data_=std::move(out);return result;
}

MultiplayerAudio::HeldPublication MultiplayerAudio::hold_publication(PreparedReplacement&p){
  const auto c=state_->core;c->script_producer();const auto data=p.data_;
  if(!data || data->owner.lock().get()!=c.get() || data->owner_pointer!=c.get() ||
      data->phase.load()!=ContinuationPrepared::Phase::Pending || !c->public_alive || c->closing ||
      c->operation_depth || c->play_depth || c->completion_depth || c->revision!=data->expected_revision ||
      (data->native_target&&!data->native_target->current()))
    throw FormatError("Native Audio replacement is stale, busy or foreign");
  return HeldPublication(std::make_unique<PublicationHold>(c,data));
}
bool MultiplayerAudio::ready(const PreparedReplacement&p,const HeldPublication&h)const noexcept{
  if(!p.data_ || !h.data_ || h.data_->thread!=std::this_thread::get_id() || h.data_->prepared!=p.data_)return false;
  const auto&c=h.data_->core;const auto&data=*p.data_;
  if(c.get()!=state_->core.get() || data.owner_pointer!=c.get() || data.owner.lock().get()!=c.get() ||
      !c->public_alive || c->closing || !c->held || c->transaction || c->operation_depth ||
      c->play_depth || c->completion_depth || c->script_epoch->thread!=std::this_thread::get_id() ||
      !c->script_epoch->alive.load() || c->revision!=data.expected_revision || c->revision>=UINT64_MAX-1 ||
      !audio_same_owner(c->cache_owner,data.source->cache))return false;
  const auto phase=data.phase.load();
  if(phase==ContinuationPrepared::Phase::Pending){if(!data.cancellation_ready())return false;}
  else if(phase!=ContinuationPrepared::Phase::Committing || data.intent!=h.data_.get() ||
      !(data.source_cancellation->requests.load()&1u) || !(data.values.cancellation->requests.load()&1u))return false;
  if(!c->platform.ready(data.platform,h.data_->platform))return false;
  if(data.native_target&&(!h.data_->native||!h.data_->native->matches(*data.native_target)))return false;
  if(data.auxiliary && !data.auxiliary->ready(data.source->auxiliary,*data.source->auxiliary_value))return false;
  for(const auto&clock:data.clocks)if(!clock.owner->ready(clock.prepared))return false;
  return true;
}
bool MultiplayerAudio::begin_commit(PreparedReplacement&p,HeldPublication&h)noexcept{
  if(!ready(p,h))return false;
  const auto data=p.data_;auto expected=ContinuationPrepared::Phase::Pending;
  if(data->phase.load()==ContinuationPrepared::Phase::Committing)return data->intent==h.data_.get();
  if(!data->phase.compare_exchange_strong(expected,ContinuationPrepared::Phase::Committing))return false;
  if(!data->source_cancellation->reserve(data->expected_cancellation)){
    data->phase.store(ContinuationPrepared::Phase::CancelRequested);return false;
  }
  if(!data->values.cancellation->reserve(0)){
    data->source_cancellation->release();data->phase.store(ContinuationPrepared::Phase::CancelRequested);return false;
  }
  data->intent=h.data_.get();return true;
}
bool MultiplayerAudio::commit(PreparedReplacement&p,HeldPublication&h)noexcept{
  if(!begin_commit(p,h))return false;
  const auto data=p.data_;
  if(data->auxiliary && !data->auxiliary->committed(data->source->auxiliary,*data->source->auxiliary_value))return false;
  const auto c=h.data_->core;
  if(!c->platform.commit(data->platform,h.data_->platform))std::terminate();
  // Child wrappers remain private and strongly held. No callback or producer
  // can change their direct witnesses between the full preflight and swaps.
  c->script_epoch->alive.store(false);
  for(auto&clock:data->clocks)if(!clock.owner->commit(clock.prepared))std::terminate();
  auto&v=data->values;
  c->banks.swap(v.banks);c->voices.swap(v.voices);c->ambience.swap(v.ambience);c->object_loops.swap(v.object_loops);std::swap(c->phased_loops,v.phased_loops);c->retired.swap(v.retired);
  c->listeners.swap(v.listeners);std::swap(c->random.seed,v.random.seed);c->random.stores.swap(v.random.stores);
  std::swap(c->class_gain,v.class_gain);std::swap(c->master,v.master);std::swap(c->next_packet,v.next_packet);
  std::swap(c->paused,v.paused);c->native_metadata.swap(v.native_metadata);c->script_epoch.swap(v.script_epoch);
  c->script_preparations.swap(v.preparations);c->script_closers.swap(v.closers);c->script_retirements.swap(v.retirements);
  c->script_clocks.swap(v.clocks);v.script_bound=c->script_bound.exchange(v.script_bound);
  c->cancellation->life.store(ScriptCancellationDomain::Life::Retired);
  c->cancellation.swap(v.cancellation);c->cancellation->life.store(ScriptCancellationDomain::Life::Live);
  c->publication_record.swap(data->swap_publication_record);c->publication_epoch=data->target_epoch;
  ++c->revision;data->installed_revision=c->revision;data->phase.store(ContinuationPrepared::Phase::Consumed);
  return true;
}
} // namespace halo2
