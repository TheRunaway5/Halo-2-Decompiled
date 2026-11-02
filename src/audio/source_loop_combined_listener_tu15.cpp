#include "halo2/source_loop_combined_listener_tu15.hpp"
#include "halo2/native_campaign_audio_listeners.hpp"
#include <bit>
namespace halo2 {namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace native_campaign_listeners_detail;
std::uint32_t word(float value){return std::bit_cast<std::uint32_t>(value);}
NativeCampaignAudioFp context(const SoundDriverFloatingPointContext& fp){return {fp.x87_control_word,0,fp.mxcsr};}
SourceLoopListenerTransformTu15 transform(const SourceObjectAudioViewTu15& view){
    if(!view.orientation)throw FormatError("Combined listener requires its submitted camera orientation");
    const auto& f=view.orientation->forward;const auto& u=view.orientation->up;
    const auto fx=scalar(word(f.x)),fy=scalar(word(f.y)),fz=scalar(word(f.z));
    const auto ux=scalar(word(u.x)),uy=scalar(word(u.y)),uz=scalar(word(u.z));
    //129F10 keeps these particular MULSS operand and subtraction orders.
    const auto x=sub(mul(uy,fz),mul(uz,fy));
    const auto y=sub(mul(uz,fx),mul(fz,ux));
    const auto z=sub(mul(fy,ux),mul(uy,fx));
    return {0x3f800000u,word(f.x),word(f.y),word(f.z),bits(x),bits(y),bits(z),
        word(u.x),word(u.y),word(u.z),word(view.position.x),word(view.position.y),word(view.position.z)};
}
std::array<std::uint32_t,3> local(const SourceLoopListenerTransformTu15& matrix,
    const std::array<std::uint32_t,3>& point){
    const auto scale=scalar(matrix[0]),zero=_mm_setzero_ps(),one=scalar(0x3f800000u);
    if(equal(scale,zero))return {};
    auto x=sub(scalar(point[0]),scalar(matrix[10]));
    auto y=sub(scalar(point[1]),scalar(matrix[11]));
    auto z=sub(scalar(point[2]),scalar(matrix[12]));
    if(!equal(scale,one)){
        const auto reciprocal=div(one,scale);x=mul(reciprocal,x);y=mul(reciprocal,y);z=mul(reciprocal,z);
    }
    std::array<std::uint32_t,3> result{};
    for(unsigned axis=0;axis<3;++axis){const auto at=1+axis*3;
        auto value=mul(scalar(matrix[at+2]),z);
        value=add(value,mul(scalar(matrix[at+1]),y));value=add(value,mul(scalar(matrix[at]),x));result[axis]=bits(value);
    }
    return result;
}
struct Source {
    Bytes bytes;
    std::uint8_t mode()const{return std::to_integer<unsigned>(bytes.slice(3,1)[0])&15u;}
    bool ignore_clusters()const{return (flags()&1u)!=0;}
    bool column()const{return bytes.slice(2,1)[0]!=std::byte{};}
    std::uint16_t flags()const{return bytes.u16(0);}
    std::uint32_t x()const{return bytes.u32(12);}std::uint32_t y()const{return bytes.u32(16);}
    std::uint32_t z()const{return bytes.u32(20);}std::uint32_t height()const{return bytes.u32(0x30);}
    std::int16_t cluster()const{return std::bit_cast<std::int16_t>(bytes.u16(0x2c));}
    std::uint32_t location()const{return bytes.u32(0x28);}
};
struct Row {
    const SourceObjectAudioViewTu15& view;
    std::int16_t cluster()const{return view.cluster;}
    std::uint32_t location()const{return std::bit_cast<std::uint32_t>(view.leaf);}
    std::uint32_t x()const{return word(view.position.x);}std::uint32_t y()const{return word(view.position.y);}
    std::uint32_t z()const{return word(view.position.z);}
};
struct Listeners {
    const SourceObjectAudioViewsTu15& views;
    Row row(unsigned i)const{return {views.at(i).value()};}
    bool active(unsigned i)const{return views.at(i).has_value();}
};
struct Players {const SourceObjectAudioViewsTu15& views;bool present(unsigned i)const{return views.at(i).has_value();}};
struct Table {
    const MultiplayerAudioBsp& bsp;
    std::uint8_t at(std::uint32_t i)const{return bsp.pair_distances.at(i);}
};
struct Scale {
    const MultiplayerAudioBsp& bsp;
    Table bytes()const{return {bsp};}
    std::uint32_t lower()const{return word(bsp.distance_lower);}std::uint32_t upper()const{return word(bsp.distance_upper);}
};
struct Structure {
    const MultiplayerAudioBsp& bsp;
    std::uint32_t cluster_count()const{return std::uint32_t(bsp.clusters.size());}
    std::int32_t scale_count()const{return bsp.encoded_distances?1:0;}
    Table cluster_bytes()const{return {bsp};}Scale scale_table()const{return {bsp};}
};
struct State {
    const MultiplayerAudioBsp& bsp;const SourceObjectAudioViewsTu15& views;
    void require_extended_capacity(unsigned)const{}
    Listeners listeners()const{return {views};}Players players()const{return {views};}
    Structure structure()const{return {bsp};}
};
#endif
}
SourceLoopListenerTransformTu15 source_loop_listener_transform_tu15(const SourceObjectAudioViewTu15& view,
    const SoundDriverFloatingPointContext& controls){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto fp=context(controls);native_campaign_ducking_detail::Environment environment(fp);return transform(view);
#else
    (void)view;(void)controls;throw FormatError("Native combined-listener FP backend unavailable");
#endif
}
std::array<std::uint32_t,3> source_loop_listener_local_point_tu15(const SourceLoopListenerTransformTu15& matrix,
    const std::array<std::uint32_t,3>& point,const SoundDriverFloatingPointContext& controls){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto fp=context(controls);native_campaign_ducking_detail::Environment environment(fp);return local(matrix,point);
#else
    (void)matrix;(void)point;(void)controls;throw FormatError("Native combined-listener FP backend unavailable");
#endif
}
SourceLoopCombinedListenerSelectionTu15 source_loop_combined_listener_tu15(std::span<const std::byte,68> bytes,
    std::uint32_t maximum,const MultiplayerAudioBsp& bsp,const SourceObjectAudioViewsTu15& views,
    const SoundDriverFloatingPointContext& controls){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Source source{Bytes(bytes)};
    if(source.mode()!=1 || std::to_integer<unsigned>(bytes[2])>1)
        throw FormatError("Combined loop requires its remaining source-point mode owner");
    auto fp=context(controls);native_campaign_ducking_detail::Environment environment(fp);
    State state{bsp,views};const auto selected=select(state,source,maximum);
    SourceLoopCombinedListenerSelectionTu15 result{selected.listener,selected.distance,{}};
    if(selected.listener==0xffffffffu)return result;
    const auto& view=views.at(selected.listener).value();
    const std::array point{source.x(),source.y(),source.column()?word(view.position.z):source.z()};
    result.local_point=local(transform(view),point);return result;
#else
    (void)bytes;(void)maximum;(void)bsp;(void)views;(void)controls;
    throw FormatError("Native combined-listener FP backend unavailable");
#endif
}
}
