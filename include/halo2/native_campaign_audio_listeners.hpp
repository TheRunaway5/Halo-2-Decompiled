#pragma once
#include "halo2/native_campaign_audio_ducking.hpp"

namespace halo2 {
struct NativeCampaignListenerSelection {std::uint32_t listener,distance;};
namespace native_campaign_listeners_detail {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace native_campaign_ducking_detail;
inline constexpr std::uint32_t none=0xffffffffu,maximum_distance=0x7f7fffffu;
inline bool equal(__m128 first,__m128 second) {
    unsigned char ordered,same;
    asm volatile("ucomiss %3,%2; setnp %0; sete %1":"=qm"(ordered),"=qm"(same):"x"(first),"x"(second):"cc","memory");
    return ordered&&same;
}
// Offset in the strictly triangular cluster table. The signed product's
// division truncates toward zero, including wrapped 32-bit products.
inline std::uint32_t pair_offset(std::uint32_t low,std::uint32_t high,std::uint32_t clusters) {
    const auto product=(low+1u)*low;
    const auto triangle=std::uint32_t(std::bit_cast<std::int32_t>(product)/2);
    return (clusters-1u)*low-triangle+high-1u;
}
template<class Structure> std::uint8_t cluster_byte(Structure structure,std::int16_t first,std::int16_t second) {
    if(first==second)return 0;
    const auto low=std::uint32_t(std::int32_t(first<second?first:second));
    const auto high=std::uint32_t(std::int32_t(first<second?second:first));
    const auto count=structure.cluster_count();auto table=structure.cluster_bytes();
    return table.at(pair_offset(low,high,count));
}
template<class Structure> std::uint32_t cluster_scale(Structure structure,std::int32_t first,std::int32_t second) {
    if(first==second)return 0u;
    if(structure.scale_count()<=0) {
        const auto cell=cluster_byte(structure,std::int16_t(first),std::int16_t(second));
        return bits(mul(signed_milliseconds(cell&0x7fu),scalar(0x40010204u)));
    }
    auto scale=structure.scale_table();const auto count=structure.cluster_count();auto table=scale.bytes();
    const auto low=std::uint32_t(first<second?first:second),high=std::uint32_t(first<second?second:first);
    const auto cell=table.at(pair_offset(low,high,count));
    if(cell==0)return 0u;if(cell==255)return 0x43800000u;
    const auto lower=scalar(scale.lower());const auto upper=scalar(scale.upper());
    auto position=mul(signed_milliseconds(cell-1u),scalar(0x3b81848eu));
    const auto span=sub(upper,lower);position=mul(position,span);
    return bits(add(position,lower));
}
template<class State,class Source> bool cut_off(State& state,Source source,std::uint32_t listener,std::uint32_t ratio) {
    if(listener==none)return true;
    if(source.ignore_clusters())return false;
    if(source.mode()!=1u)return false;
    auto listeners=state.listeners();auto row=listeners.row(listener);
    const auto source_cluster=source.cluster();if(source_cluster==-1)return true;
    const auto listener_cluster=row.cluster();if(listener_cluster==-1)return true;
    const auto location=row.location();if(location==source.location() || listener_cluster==source_cluster)return false;
    auto structure=state.structure();const auto scale=cluster_scale(structure,listener_cluster,source_cluster);
    const auto attenuated=mul(scalar(scale),scalar(ratio));
    return !above(scalar(0x43800000u),attenuated);
}
inline std::uint32_t absolute_distance(std::uint32_t z,std::uint32_t y,std::uint32_t x) {
    std::uint32_t result;
    // Physical extended sum x*x+y*y+z*z, with its original operand and NaN
    // precedence. The temporary FPU operands are never virtualized.
    asm volatile("flds %1; flds %2; flds %3; fld %%st(0); fmul %%st(1),%%st; fld %%st(2); "
        "fmul %%st(3),%%st; faddp; fld %%st(3); fmul %%st(4),%%st; faddp; fsqrt; "
        "fstp %%st(3); fstp %%st(0); fstp %%st(0); fstps %0"
        :"=m"(result):"m"(z),"m"(y),"m"(x):"memory","st","st(1)","st(2)","st(3)","st(4)");
    return result;
}
template<class State,class Source> std::uint32_t distance(State& state,Source source,std::uint32_t listener) {
    const auto mode=source.mode();if(mode==0u)return 0u;
    if(mode!=1u) {
        const auto z=source.z();const auto y=source.y();const auto x=source.x();state.require_extended_capacity(5);return absolute_distance(z,y,x);
    }
    auto listeners=state.listeners();auto row=listeners.row(listener);
    const auto listener_z=scalar(row.z());const auto dz=sub(listener_z,scalar(source.z()));
    if(!source.column()) {
        const auto listener_y=scalar(row.y());const auto dy=sub(listener_y,scalar(source.y()));
        const auto listener_x=scalar(row.x());const auto dx=sub(listener_x,scalar(source.x()));
        const auto dy2=mul(dy,dy);auto sum=mul(dz,dz);sum=add(sum,dy2);const auto dx2=mul(dx,dx);
        return bits(add(sum,dx2));
    }
    auto clamped=_mm_setzero_ps();
    if(!above(clamped,dz))clamped=above(dz,scalar(source.height()))?scalar(source.height()):dz;
    if(!equal(clamped,dz))return maximum_distance;
    const auto source_y=scalar(source.y());const auto dy=sub(source_y,scalar(row.y()));
    const auto source_x=scalar(source.x());auto sum=sub(source_x,scalar(row.x()));
    sum=mul(sum,sum);const auto dy2=mul(dy,dy);return bits(add(sum,dy2));
}
template<class State> std::uint32_t first_player(State& state) {
    auto players=state.players();for(unsigned index=0;index<4;++index)if(players.present(index))return index;return none;
}
inline std::pair<std::uint32_t,std::uint32_t> distance_and_ratio(std::uint32_t squared,std::uint32_t limit) {
    std::uint32_t distance,ratio;
    asm volatile("flds %2; fsqrt; fsts %0; fdivs %3; fstps %1"
        :"=m"(distance),"=m"(ratio):"m"(squared),"m"(limit):"memory","st");return {distance,ratio};
}
template<class State,class Source> NativeCampaignListenerSelection select(State& state,Source source,std::uint32_t limit) {
    if(source.mode()!=1u)return {first_player(state),0u};
    auto listeners=state.listeners();auto best=scalar(maximum_distance);auto chosen=none;
    for(unsigned index=0;index<4;++index)if(listeners.active(index)) {
        const auto measure=scalar(distance(state,source,index));if(above(best,measure)){chosen=index;best=measure;}
    }
    state.require_extended_capacity(1);const auto [distance,ratio]=distance_and_ratio(bits(best),limit);
    if(cut_off(state,source,chosen,ratio))chosen=none;
    const auto flags=source.flags();
    if(!(flags&0x100u) && !(flags&0x800u)) {
        const auto range=scalar(limit);const auto squared_limit=mul(range,range);
        if(above(best,squared_limit))return {none,maximum_distance};
    } else if(chosen==none)chosen=first_player(state);
    return {chosen,distance};
}
#endif
}
// Typed numerical/query entry points, each with the caller's actual masked
// FP controls. All state remains in source/listener/structure services.
template<class State,class Source> NativeCampaignListenerSelection native_campaign_select_listener(State& state,Source source,
    std::uint32_t limit,NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    native_campaign_listeners_detail::Environment environment(fp);return native_campaign_listeners_detail::select(state,source,limit);
#else
    (void)state;(void)source;(void)limit;(void)fp;throw std::runtime_error("Exact native listener FP backend unavailable");
#endif
}
template<class State,class Source> std::uint32_t native_campaign_listener_distance(State& state,Source source,
    std::uint32_t listener,NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    native_campaign_listeners_detail::Environment environment(fp);return native_campaign_listeners_detail::distance(state,source,listener);
#else
    (void)state;(void)source;(void)listener;(void)fp;throw std::runtime_error("Exact native listener FP backend unavailable");
#endif
}
template<class State,class Source> bool native_campaign_listener_cut_off(State& state,Source source,std::uint32_t listener,
    std::uint32_t ratio,NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    native_campaign_listeners_detail::Environment environment(fp);return native_campaign_listeners_detail::cut_off(state,source,listener,ratio);
#else
    (void)state;(void)source;(void)listener;(void)ratio;(void)fp;throw std::runtime_error("Exact native listener FP backend unavailable");
#endif
}
template<class Structure> std::uint32_t native_campaign_cluster_scale(Structure structure,std::int32_t first,std::int32_t second,
    NativeCampaignAudioFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    native_campaign_listeners_detail::Environment environment(fp);return native_campaign_listeners_detail::cluster_scale(structure,first,second);
#else
    (void)structure;(void)first;(void)second;(void)fp;throw std::runtime_error("Exact native listener FP backend unavailable");
#endif
}
template<class Structure> std::uint8_t native_campaign_cluster_byte(Structure structure,std::int16_t first,std::int16_t second) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return native_campaign_listeners_detail::cluster_byte(structure,first,second);
#else
    (void)structure;(void)first;(void)second;throw std::runtime_error("Exact native listener backend unavailable");
#endif
}
} // namespace halo2
