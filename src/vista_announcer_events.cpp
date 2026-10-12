#include "halo2/vista_announcer_events.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_king.hpp"
#include <bit>
#include <cfenv>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Scope {
  std::fenv_t saved_;unsigned mx_;
public:
  explicit Scope(const VistaLoadingFloatingPoint& f):mx_(_mm_getcsr()) {
    if((f.x87_control_word&0x3f)!=0x3f || (f.x87_control_word&0x300)==0x100 ||
        (f.mxcsr&0x1f80)!=0x1f80 || (f.mxcsr&0xffff0000))throw FormatError("Unsupported Vista announcer FP context");
    if(std::fegetenv(&saved_))throw FormatError("Vista announcer FP environment unavailable");
    _mm_setcsr(f.mxcsr);asm volatile("fldcw %0"::"m"(f.x87_control_word):"memory");
  }
  ~Scope(){std::fesetenv(&saved_);_mm_setcsr(mx_);}
};
std::int32_t ticks(float value,std::int16_t rate){
  const auto converted=_mm_cvtsi32_ss(_mm_setzero_ps(),rate);
  const auto product=_mm_cvtss_f32(_mm_mul_ss(converted,_mm_set_ss(value)));
  std::int32_t out;asm volatile("flds %1; fistpl %0":"=m"(out):"m"(product):"st");return out;
}
#endif
std::int16_t short_at(const Bytes& b,unsigned at){return std::bit_cast<std::int16_t>(b.u16(at));}
bool eligible(const VistaAnnouncerResponse& response,const VistaAnnouncerEvent& e,
    const VistaAnnouncerLocalPlayer& p,unsigned audience){
  const Bytes b(response);
  if(short_at(b,4)!=std::bit_cast<std::int32_t>(e.words[1]) || short_at(b,6)!=int(audience))return false;
  const auto required=short_at(b,0x10),excluded=short_at(b,0x12);
  bool present=true;
  if(required>=1 && required<=4)present=e.words[unsigned(required)+2]!=0xffffffffu;
  if(!present)return false;
  switch(excluded){
    case 1:return e.words[3]!=p.datum;
    case 2:return e.words[4]!=std::uint32_t(std::int32_t(p.membership));
    case 3:return e.words[5]!=p.datum;
    case 4:return e.words[6]!=std::uint32_t(std::int32_t(p.membership));
    default:return true;
  }
}
}
VistaAnnouncerEvent vista_announcer_event(std::span<const std::byte,34> b){
  const Bytes bytes(b);VistaAnnouncerEvent e;
  for(unsigned i=0;i<8;++i)e.words[i]=bytes.u32(i*4);
  e.data=bytes.u16(32);return e;
}
VistaAnnouncerEvent vista_announcer_event_construct(std::uint32_t type,std::uint32_t subtype){
  return {{type,subtype,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0},0xffff};
}
VistaAnnouncerEvent vista_announcer_king_notice(const SourceKingNotice& notice){
  auto out=vista_announcer_event_construct(6,std::uint32_t(notice.type));
  out.words[3]=notice.player;out.words[4]=std::uint32_t(notice.membership);return out;
}
std::optional<VistaAnnouncerEvent> vista_announcer_game_over_event(std::uint8_t& published,
    bool engine,std::uint8_t mode,std::uint8_t over){
  if(!engine || mode==4 || !over || published)return {};
  published=1;return vista_announcer_event_construct(0,19);
}
void vista_announcer_events_initialize(VistaAnnouncerEventState& s,std::int16_t rate){
  s={};vista_announcer_queue_initialize(s.queue,rate);
  for(auto& r:s.recent)r.tick=std::uint32_t(-1000);
}
std::optional<VistaAnnouncerAudioSelection> vista_announcer_event_audio(
    VistaAnnouncerEventState& s,const VistaAnnouncerResponses& responses,const VistaAnnouncerEvent& e,
    const VistaAnnouncerLocalPlayer& p,const VistaAnnouncerEventContext& in,
    const VistaLoadingFloatingPoint& fp,const VistaAnnouncerPlayback& ordinary){
  if(in.world_mode==4 || (e.words[2]!=0xffffffffu && e.words[2]!=p.datum) ||
      (e.words[2]==0xffffffffu && (p.flags&2)) || p.output_user==-1 ||
      (in.game_over && (e.words[0]!=0 || e.words[1]!=19)))return {};
  const std::vector<VistaAnnouncerResponse>* selected_bank=nullptr;
  switch(e.words[0]){
    case 0:selected_bank=&responses.general;break;
    case 2:selected_bank=&responses.slayer;break;
    case 6:selected_bank=&responses.king;break;
    default:throw FormatError("Unsupported Vista announcer event bank");
  }
  const auto& bank=*selected_bank;
  std::array<unsigned,5> audiences{};unsigned count=0;
  if(e.words[3]==p.datum)audiences[count++]=0;
  if(e.words[5]==p.datum)audiences[count++]=2;
  if(e.words[4]==std::uint32_t(std::int32_t(p.membership)))audiences[count++]=1;
  if(e.words[6]==std::uint32_t(std::int32_t(p.membership)))audiences[count++]=3;
  audiences[count++]=4;
  const VistaAnnouncerResponse* selected=nullptr;unsigned audience{},ordinal{};
  for(unsigned a=0;a<count && !selected;++a)for(unsigned i=0;i<bank.size();++i)
    if(eligible(bank[i],e,p,audiences[a])){selected=&bank[i];audience=audiences[a];ordinal=i;break;}
  if(!selected)return {};
  const Bytes r(*selected);
  if(std::bit_cast<std::int32_t>(r.u32(0xa0))>0)throw FormatError("Vista weighted announcer response owner unavailable");
  VistaAnnouncerAudioSelection out;out.announcer=(r.u16(0x40)&1)!=0;
  const auto language=out.announcer?in.language:0;
  if(language>=9)throw FormatError("Vista announcer language is outside actual response fields");
  out.sound=r.u32(0x48+language*8);out.caption_key=r.u32(0xc);out.display_key=r.u32(0x14);out.audience=audience;out.response_ordinal=ordinal;
  if(out.sound==0xffffffffu)return out;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Scope scope(fp);const auto delay=r.f32(0x3c);out.delay_ticks=delay==0.f?0:ticks(delay,in.tick_rate);
  const auto hash=((((e.words[1]<<12)^e.words[4])<<4)^~(e.words[6]<<8)^~e.words[5]^e.words[3]^e.words[0]);
  const auto threshold=ticks(std::bit_cast<float>(0x3f666666u),in.tick_rate);
  std::optional<unsigned> chosen;std::int32_t best=-1;bool fresh=true;
  for(unsigned i=0;i<4;++i){const auto delta=std::bit_cast<std::int32_t>(in.tick-s.recent[i].tick);
    if(s.recent[i].hash==hash && delta<threshold){chosen=i;fresh=false;break;}
    if(delta>best){best=delta;chosen=i;}
  }
  if(!chosen)throw FormatError("Vista recent event owner has no source-admitted replacement slot");
  auto& recent=s.recent[*chosen];recent.tick=in.tick;
  if(fresh){recent.hash=hash;recent.sounds.fill(0xffffffffu);}
  bool added=false;
  for(auto& cue:recent.sounds){if(cue==out.sound)break;if(cue==0xffffffffu){cue=out.sound;added=true;break;}}
  if(!added && !(e.words[0]==0 && (e.words[1]==20 || e.words[1]==21)))return out;
  if(out.announcer){const auto before=s.queue.count;vista_announcer_queue_enqueue(s.queue,out.sound,out.delay_ticks);out.requested=s.queue.count!=before;}
  else {if(!ordinary)throw FormatError("Vista ordinary event audio owner unavailable");ordinary(out.sound,1.f);out.requested=true;}
  return out;
#else
  (void)s;(void)fp;(void)ordinary;throw FormatError("Vista announcer SSE backend unavailable");
#endif
}
} // namespace halo2
