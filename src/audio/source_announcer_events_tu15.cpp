#include "halo2/source_announcer_events_tu15.hpp"
#include <stdexcept>
#include <bit>
#include <cfenv>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class FloatingPoint {
    std::fenv_t saved_;unsigned mxcsr_;
public:
    explicit FloatingPoint(const VistaLoadingFloatingPoint& input):mxcsr_(_mm_getcsr()) {
        if((input.x87_control_word&0x3f)!=0x3f || (input.x87_control_word&0x300)==0x100 ||
            (input.mxcsr&0x1f80)!=0x1f80 || (input.mxcsr&0xffff0000) || std::fegetenv(&saved_))
            throw std::runtime_error("Unsupported TU1.5 announcer floating-point owner");
        _mm_setcsr(input.mxcsr);asm volatile("fldcw %0"::"m"(input.x87_control_word):"memory");
    }
    ~FloatingPoint(){std::fesetenv(&saved_);_mm_setcsr(mxcsr_);}
};
std::int32_t ticks(float seconds,std::int16_t rate) {
    const auto value=_mm_cvtss_f32(_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),rate),_mm_set_ss(seconds)));
    std::int32_t out;asm volatile("flds %1; fistpl %0":"=m"(out):"m"(value):"st");return out;
}
#endif
struct Response {
    std::span<const std::byte> bytes;
    explicit Response(const VistaAnnouncerResponse& input):bytes(input){}
    std::uint16_t u16(unsigned at)const {
        return std::uint16_t(std::to_integer<unsigned>(bytes[at]) | (std::to_integer<unsigned>(bytes[at+1])<<8));
    }
    std::uint32_t u32(unsigned at)const {return std::uint32_t(u16(at)) | (std::uint32_t(u16(at+2))<<16);}
    float f32(unsigned at)const{return std::bit_cast<float>(u32(at));}
};
std::int16_t signed_half(const Response& bytes,unsigned offset){return std::bit_cast<std::int16_t>(bytes.u16(offset));}
bool eligible(const VistaAnnouncerResponse& raw,const VistaAnnouncerEvent& event,
    const VistaAnnouncerLocalPlayer& player,unsigned audience) {
    const Response row(raw);
    if(signed_half(row,4)!=std::bit_cast<std::int32_t>(event.words[1]) || signed_half(row,6)!=int(audience))return false;
    const auto required=signed_half(row,0x10),excluded=signed_half(row,0x12);
    const bool present=required<1 || required>4 || event.words[unsigned(required)+2]!=0xffffffffu;
    if(!present)return false;
    switch(excluded) {
    case 1:return event.words[3]!=player.datum;
    case 2:return event.words[4]!=std::uint32_t(std::int32_t(player.membership));
    case 3:return event.words[5]!=player.datum;
    case 4:return event.words[6]!=std::uint32_t(std::int32_t(player.membership));
    default:return true;
    }
}
}
std::optional<SourceAnnouncerRecipientSelectionTu15> source_announcer_recipient_tu15(
    std::span<const VistaAnnouncerResponse> rows,const VistaAnnouncerEvent& event,
    const VistaAnnouncerLocalPlayer& player) {
    std::array<unsigned,5> audiences{};unsigned count{};
    if(event.words[3]==player.datum)audiences[count++]=0;
    if(event.words[5]==player.datum)audiences[count++]=2;
    const auto member=std::uint32_t(std::int32_t(player.membership));
    if(event.words[4]==member)audiences[count++]=1;
    if(event.words[6]==member)audiences[count++]=3;
    audiences[count++]=4;
    for(unsigned n=0;n<count;++n)for(unsigned row=0;row<rows.size();++row)
        if(eligible(rows[row],event,player,audiences[n]))return SourceAnnouncerRecipientSelectionTu15{row,audiences[n]};
    return {};
}
SourceAnnouncerResponseSelectionTu15 source_announcer_response_tu15(
    const VistaAnnouncerResponse& raw,std::span<const SourceAnnouncerAlternativeTu15> alternatives,
    unsigned language,std::int16_t rate,std::uint32_t& random,const VistaLoadingFloatingPoint& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    FloatingPoint scope(fp);const Response row(raw);
    SourceAnnouncerResponseSelectionTu15 out;out.announcer=(row.u16(0x40)&1u)!=0;
    const auto selected_language=out.announcer?language:0u;
    if(selected_language>=9)throw std::runtime_error("TU1.5 announcer language has no authored sound slot");
    out.sound=row.u32(0x48+selected_language*8);
    const auto delay=row.f32(0x3c);out.delay_ticks=delay==0.f?0:ticks(delay,rate);
    const auto count=std::bit_cast<std::int32_t>(row.u32(0xa0));
    if(count>0) {
        if(std::size_t(count)!=alternatives.size())throw std::runtime_error("TU1.5 alternate sound block is incomplete");
        random=random*0x19660du+0x3c6ef35fu;
        // Original1A8328..1A8344 uses x87, with a single spill before the
        // SSE cumulative comparison. Preserve the original greater-than edge.
        float choice;const auto upper=std::int32_t(random>>16);
        constexpr float scale=std::bit_cast<float>(0x37800080u);
        asm volatile("fildl %1; fmuls %2; fstps %0":"=m"(choice):"m"(upper),"m"(scale):"st");
        auto cumulative=_mm_setzero_ps();
        for(const auto& alternative:alternatives) {
            cumulative=_mm_add_ss(cumulative,_mm_set_ss(alternative.weight));
            if(_mm_comigt_ss(_mm_set_ss(choice),cumulative)) {
                out.sound=alternative.sounds[selected_language];out.announcer=(alternative.flags&1u)!=0;break;
            }
        }
    }
    return out;
#else
    (void)raw;(void)alternatives;(void)language;(void)rate;(void)random;(void)fp;
    throw std::runtime_error("TU1.5 announcer requires the verified x87/SSE scalar backend");
#endif
}
bool source_announcer_remember_tu15(VistaAnnouncerEventState& state,const VistaAnnouncerEvent& event,
    std::uint32_t sound,std::uint32_t tick,std::int16_t rate,const VistaLoadingFloatingPoint& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    FloatingPoint scope(fp);
    const auto& w=event.words;
    const auto hash=((((w[1]<<12)^w[4])<<4)^~(w[6]<<8)^~w[5]^w[3]^w[0]);
    const auto window=ticks(std::bit_cast<float>(0x3f666666u),rate);
    std::optional<unsigned> selected;std::int32_t oldest=-1;bool fresh=true;
    for(unsigned i=0;i<state.recent.size();++i) {
        const auto delta=std::bit_cast<std::int32_t>(tick-state.recent[i].tick);
        if(state.recent[i].hash==hash && delta<window){selected=i;fresh=false;break;}
        if(delta>oldest){oldest=delta;selected=i;}
    }
    if(!selected)throw std::runtime_error("TU1.5 recent audio bank has no source-admitted replacement");
    auto& recent=state.recent[*selected];recent.tick=tick;
    if(fresh){recent.hash=hash;recent.sounds.fill(0xffffffffu);}
    bool added=false;
    for(auto& cue:recent.sounds) {
        if(cue==sound)break;
        if(cue==0xffffffffu){cue=sound;added=true;break;}
    }
    return added || (w[0]==0 && (w[1]==20 || w[1]==21));
#else
    (void)state;(void)event;(void)sound;(void)tick;(void)rate;(void)fp;
    throw std::runtime_error("TU1.5 announcer requires the verified x87/SSE scalar backend");
#endif
}
} // namespace halo2
