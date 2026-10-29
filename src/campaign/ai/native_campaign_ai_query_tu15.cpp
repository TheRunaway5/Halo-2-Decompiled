#include "halo2/native_campaign_ai_query_tu15.hpp"
#include <array>
#include <bit>
#include <optional>
#include <stdexcept>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
class QueryEnvironment {
  std::uint32_t saved_;
  NativeCampaignAiQueryFp &state_;
public:
  explicit QueryEnvironment(NativeCampaignAiQueryFp &state):state_(state) {
    if((state.mxcsr&0x1f80u)!=0x1f80u || (state.mxcsr&0xffff0000u))
      throw std::invalid_argument("Native campaign query requires masked source SSE exceptions");
    saved_=_mm_getcsr();_mm_setcsr(state.mxcsr);
  }
  ~QueryEnvironment(){state_.mxcsr=_mm_getcsr();_mm_setcsr(saved_);}
};
__m128 weighted_add(__m128 population,__m128 value,__m128 sum) {
#if !defined(_MSC_VER)
  asm volatile("mulss %2,%0; addss %1,%0":"+&x"(population):"x"(sum),"x"(value):"memory");
  return population;
#else
  return _mm_add_ss(_mm_mul_ss(population,value),sum);
#endif
}
__m128 living_fraction(std::int32_t living,std::int32_t population) {
  auto numerator=_mm_setzero_ps(),denominator=_mm_setzero_ps();
#if !defined(_MSC_VER)
  //2AF744..74C converts the complete signed sums in this order. The
  // population gate belongs to the caller; this primitive never truncates
  // living to the SHORT returned by the sibling count evaluator.
  asm volatile("cvtsi2ss %2,%0; cvtsi2ss %3,%1; divss %1,%0"
    :"+&x"(numerator),"+&x"(denominator):"r"(living),"r"(population):"memory");
  return numerator;
#else
  numerator=_mm_cvtsi32_ss(numerator,living);
  denominator=_mm_cvtsi32_ss(denominator,population);
  return _mm_div_ss(numerator,denominator);
#endif
}
__m128 mean(__m128 sum,std::int32_t count) {
  const auto population=_mm_cvtsi32_ss(_mm_setzero_ps(),count);
#if !defined(_MSC_VER)
  asm volatile("divss %1,%0":"+x"(sum):"x"(population):"memory");return sum;
#else
  return _mm_div_ss(sum,population);
#endif
}
#endif
class AiQuery {
public:
  explicit AiQuery(const SourcePhysicsContactMemory &memory):memory_(memory) {}
  std::uint32_t actor(std::uint32_t reference) { return resolve_actor(reference); }
  std::uint32_t query(std::uint32_t reference,NativeCampaignAiQuery operation) {
    if((reference>>30)>=2u) {
      const auto actor=resolve_actor(reference);
      if(actor==none)return 0;
      const auto actors=read(0x5020c8u);
      const auto actor_rows=read(actors+0x44u);
      const auto objects=read(0x4ecbd0u);
      const auto unit=read(actor_rows+(actor&0xffffu)*0x888u+0x18u);
      const auto object_rows=read(objects+0x44u);
      const auto body=read(object_rows+(unit&0xffffu)*12u+8u);
      // MOVSS remains a required read when the caller selects a count or
      // living fraction instead of returning this vitality value.
      const auto vitality=read(body+0xecu);
      if(operation==NativeCampaignAiQuery::living_fraction) {
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
        return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(living_fraction(1,1)));
#else
        return 0x3f800000u;
#endif
      }
      return operation==NativeCampaignAiQuery::strength?vitality:1u;
    }
    const bool group=(reference>>30)==1u;
    if(group)start_group(reference&0xffffu);
    //27DC76 performs this read even when a group's subtree is empty.
    const auto squads=read(0x52be7cu);
    auto single=reference&0xffffu;
    std::uint32_t count=0,population_total=0;
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    auto weighted_vitality=_mm_setzero_ps();
#else
    float weighted_vitality=0.f;
#endif
    while(true) {
      std::uint32_t squad;
      if(group)squad=next_group_squad();
      else if(single!=none) {
        squad=read(squads+0x44u)+(single&0xffffu)*0x98u;
        single=none;
      } else break;
      if(!squad)break;
      std::int32_t contribution;
      if(operation==NativeCampaignAiQuery::swarm_count)
        contribution=std::int16_t(read(squad+0xcu,2));
      else if(operation==NativeCampaignAiQuery::nonswarm_count) {
        const auto swarm=std::int32_t(std::int16_t(read(squad+0xcu,2)));
        const auto living=std::int32_t(std::int16_t(read(squad+0xau,2)));
        contribution=living-swarm;
        if(contribution<0)contribution=0;
      } else contribution=std::int16_t(read(squad+0xau,2));
      count+=std::uint32_t(contribution);
      const auto members=std::int32_t(std::int16_t(read(squad+8u,2)));
      const auto vitality=read(squad+0x10u);
      // The original accumulates weighted vitality before checking the next
      // squad, including when both optional output pointers are null. Native
      // scalar SSE preserves denormal/NaN exception behavior and operation order.
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
      const auto population=_mm_cvtsi32_ss(_mm_setzero_ps(),members);
      const auto value=_mm_set_ss(std::bit_cast<float>(vitality));
      weighted_vitality=weighted_add(population,value,weighted_vitality);
      volatile float evaluated_each=_mm_cvtss_f32(weighted_vitality);
#else
      weighted_vitality=float(float(members)*std::bit_cast<float>(vitality))+weighted_vitality;
      volatile float evaluated_each=weighted_vitality;
#endif
      (void)evaluated_each;
      population_total+=std::uint32_t(members);
    }
    // Keep the required arithmetic observable even for the null-output form.
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    volatile float evaluated=_mm_cvtss_f32(weighted_vitality);
#else
    volatile float evaluated=weighted_vitality;
#endif
    (void)evaluated;
    if(operation==NativeCampaignAiQuery::living_fraction) {
      const auto population=std::bit_cast<std::int32_t>(population_total);
      if(population<=0)return 0;
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
      return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
        living_fraction(std::bit_cast<std::int32_t>(count),population)));
#else
      return std::bit_cast<std::uint32_t>(float(std::bit_cast<std::int32_t>(count))/float(population));
#endif
    }
    if(operation==NativeCampaignAiQuery::strength) {
      const auto population=std::bit_cast<std::int32_t>(population_total);
      if(population<=0)return 0;
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
      return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(mean(weighted_vitality,population)));
#else
      return std::bit_cast<std::uint32_t>(weighted_vitality/float(population));
#endif
    }
    return std::uint16_t(count);
  }
private:
  const SourcePhysicsContactMemory &memory_;
  std::uint32_t root_group_{},group_{},next_squad_=none;
  std::uint32_t read(std::uint32_t address,unsigned width=4) const {
    std::array<std::byte,4> bytes{};
    memory_.read(address,std::span(bytes).first(width));
    std::uint32_t value=0;
    for(unsigned i=0;i<width;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
  }
  void descend(std::uint32_t groups) {
    for(auto child=read(group_+4u);child!=none;child=read(group_+4u)) {
      group_=read(groups+0x44u)+(child&0xffffu)*0x38u;
    }
  }
  void start_group(std::uint32_t index) {
    const auto groups=read(0x52be80u);
    root_group_=group_=read(groups+0x44u)+index*0x38u;
    descend(groups);
    next_squad_=read(group_+8u);
  }
  std::uint32_t next_group_squad() {
    if(!group_)return 0;
    const auto groups=read(0x52be80u);
    while(next_squad_==none) {
      if(group_==root_group_) {group_=0;return 0;}
      const auto sibling=read(group_+0xcu);
      if(sibling==none) {
        const auto parent=read(group_+0x10u);
        group_=read(groups+0x44u)+(parent&0xffffu)*0x38u;
      } else {
        group_=read(groups+0x44u)+(sibling&0xffffu)*0x38u;
        descend(groups);
      }
      next_squad_=read(group_+8u);
    }
    const auto squads=read(0x52be7cu);
    const auto row=read(squads+0x44u)+(next_squad_&0xffffu)*0x98u;
    next_squad_=std::uint32_t(std::int32_t(std::int16_t(read(row+0x74u,2))));
    return row;
  }
  std::uint32_t resolve_actor(std::uint32_t reference) {
    if((reference>>30)==2u) {
      const auto index=reference&0xffffu;
      const auto actors=read(0x5020c8u);
      const auto stride=read(actors+0x24u);
      const auto rows=read(actors+0x44u);
      return (read(rows+index*stride,2)<<16)|index;
    }
    const auto squad=(reference>>16)&0x3fffu;
    const auto location=std::int32_t(std::int16_t(reference));
    const auto scenario=read(0x4ecc20u);
    const auto squads=std::bit_cast<std::int32_t>(read(scenario+0x160u));
    if(std::int32_t(squad)>=squads)return none;
    const auto definitions=read(scenario+0x164u);
    const auto definition=definitions+squad*0x74u;
    if(location<0)return none;
    const auto locations=std::bit_cast<std::int32_t>(read(definition+0x48u));
    if(location>=locations)return none;
    const auto starts=read(definition+0x4cu);
    const auto globals=read(0x5020a8u);
    std::uint32_t member=none;
    if(read(globals+1u,1)) {
      const auto pool=read(0x52be7cu);
      const auto rows=read(pool+0x44u);
      member=read(rows+squad*0x98u+0x68u);
    }
    std::optional<std::uint32_t> name;
    while(true) {
      const auto state=read(0x5020a8u);
      if(!read(state+1u,1) || member==none)return none;
      const auto actors=read(0x5020c8u);
      const auto rows=read(actors+0x44u);
      const auto actor=rows+(member&0xffffu)*0x888u;
      const auto current=member;
      member=read(actor+0x20u);
      if(!name)name=read(starts+std::uint32_t(location)*0x64u);
      if(read(actor+0x38u)==*name)return current;
    }
  }
};
}
std::optional<NativeCampaignAiQueryBuiltin> native_campaign_ai_query_builtin_tu15(std::string_view name) {
  if(name=="ai_living_count")return NativeCampaignAiQueryBuiltin{NativeCampaignAiQuery::living_count,7};
  if(name=="ai_swarm_count")return NativeCampaignAiQueryBuiltin{NativeCampaignAiQuery::swarm_count,7};
  if(name=="ai_nonswarm_count")return NativeCampaignAiQueryBuiltin{NativeCampaignAiQuery::nonswarm_count,7};
  if(name=="ai_strength")return NativeCampaignAiQueryBuiltin{NativeCampaignAiQuery::strength,6};
  if(name=="ai_living_fraction")return NativeCampaignAiQueryBuiltin{NativeCampaignAiQuery::living_fraction,6};
  if(name=="ai_fighting_count")return NativeCampaignAiQueryBuiltin{NativeCampaignAiQuery::fighting_count,7};
  return {};
}
std::uint16_t native_campaign_ai_living_count_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t reference,NativeCampaignAiQueryFp &fp) {
  return std::uint16_t(native_campaign_ai_query_tu15(memory,reference,NativeCampaignAiQuery::living_count,fp));
}
std::uint32_t native_campaign_ai_query_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t reference,NativeCampaignAiQuery operation,NativeCampaignAiQueryFp &fp) {
  if(operation==NativeCampaignAiQuery::fighting_count)
    return native_campaign_ai_fighting_count_tu15(memory,reference);
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
  QueryEnvironment environment(fp);
  return AiQuery(memory).query(reference,operation);
#else
  (void)memory;(void)reference;(void)operation;(void)fp;
  throw std::runtime_error("Native campaign query has no qualified source SSE policy on this architecture");
#endif
}
std::uint32_t native_campaign_ai_actor_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t reference) {
  if((reference>>30)<2u)throw std::invalid_argument("Expected an actor or starting-location AI reference");
  return AiQuery(memory).actor(reference);
}
}
