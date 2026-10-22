#include "halo2/source_scenario_request.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_euler.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86_FP)
#include <xmmintrin.h>
#define HALO2_REQUEST_SSE 1
#endif

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> data) {
  std::uint32_t value=0;
  for(unsigned i=0;i<data.size();++i)value|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
  return value;
}
std::uint32_t read(const SourceScenarioRequestServices &p,std::uint32_t at,unsigned n=4) {
  if(!p.read)throw std::runtime_error("Scenario request byte read owner unavailable");
  const auto data=p.read(at,n);
  if(data.size()!=n)throw std::runtime_error("Scenario request byte read extent unavailable");
  return word(data);
}
void write(const SourceScenarioRequestServices &p,const ObjectConstructionObserver &o,
    std::uint32_t pc,std::uint32_t at,std::uint32_t value) {
  if(!p.memory)throw std::runtime_error("Scenario request byte storage owner unavailable");
  auto data=p.memory(at,4);
  if(data.size()!=4)throw std::runtime_error("Scenario request byte storage extent unavailable");
  const auto before=word(data);
  for(unsigned i=0;i<4;++i)data[i]=std::byte(value>>(8*i));
  if(o)o({pc,at,4,before,value});
}
class Nearest {
  std::fenv_t saved;
#if HALO2_REQUEST_SSE
  unsigned saved_sse;
#endif
public:
  Nearest(std::uint16_t x87,const ObjectConstructionFloatingPointContext &fp) {
    if(x87!=0x027f || (fp.mxcsr&~0x3fu)!=0x1f80)
      throw std::runtime_error("Scenario request nearest math configuration unavailable");
#if HALO2_REQUEST_SSE
    saved_sse=_mm_getcsr();
#endif
    if(std::feholdexcept(&saved)!=0)
      throw std::runtime_error("Scenario request host floating environment unavailable");
    if(std::fesetround(FE_TONEAREST)!=0) {
      std::fesetenv(&saved);
      throw std::runtime_error("Scenario request host nearest rounding unavailable");
    }
#if HALO2_REQUEST_SSE
    _mm_setcsr(0x1f80); // Clear host DAZ/FTZ as well as rounding/status.
#endif
  }
  ~Nearest(){
    std::fesetenv(&saved);
#if HALO2_REQUEST_SSE
    _mm_setcsr(saved_sse);
#endif
  }
  Nearest(const Nearest &)=delete;
  Nearest &operator=(const Nearest &)=delete;
};
}
SourceScenarioRequestServices source_scenario_request_byte_owners(
    SourceScenarioRequestServices services,const ObjectCreationRequestOperands &initializer,
    const SourceScenarioRequestOperands &operands,std::uint16_t x87,
    const ObjectConstructionObserver &observe) {
  // Copy the underlying retained providers before installing invoke. No
  // callback depends on the returned facade's lifetime or its own invoke.
  auto backing=services;
  services.invoke=[backing,initializer,operands,x87,observe](const ObjectConstructionCall &c,
      ObjectConstructionFloatingPointContext &fp)->std::uint32_t {
    using R=ObjectConstructionRegister;
    const auto reg=[&](R r){return c.registers[unsigned(r)];};
    const auto mask=[](std::initializer_list<R> regs){unsigned v=0;for(auto r:regs)v|=1u<<unsigned(r);return v;};
    if(c.entry==0xb8820 && c.instruction==0xd673b && c.register_mask==mask({R::Esi}) &&
        c.argument_count==3 && c.arguments[1]==0xffffffffu && c.arguments[2]==0) {
      source_object_creation_request_initialize(reg(R::Esi),c.arguments[0],c.arguments[1],
          c.arguments[2],0,initializer,{backing.memory},[&](const auto &event){
            if(observe)observe({event.instruction,event.address,event.bytes,event.before,event.after});
          });
      // D6660 does not consume B8820's undefined EAX return.
      return 0;
    }
    if(c.argument_count)throw std::runtime_error("Scenario request lower stack ABI differs");
    if(c.entry==0xd7070 && c.instruction==0xd6783 && c.register_mask==mask({R::Edx}))
      return source_scenario_request_policy(reg(R::Edx),backing);
    if(c.entry==0x11f680 && c.instruction==0xd677c &&
        c.register_mask==mask({R::Eax,R::Ecx,R::Edx})) {
      Nearest nearest(x87,fp);
      const Vec3 rotation{std::bit_cast<float>(read(backing,reg(R::Eax))),
          std::bit_cast<float>(read(backing,reg(R::Eax)+4)),
          std::bit_cast<float>(read(backing,reg(R::Eax)+8))};
      for(float angle:{rotation.x,rotation.y,rotation.z})
        if(!std::isfinite(angle) || std::abs(angle)>100000.f)
          throw std::runtime_error("Scenario request Euler input outside verified range");
      const auto basis=source_euler_basis(rotation);
      const std::array values{basis.forward.x,basis.forward.y,basis.forward.z,
                              basis.up.x,basis.up.y,basis.up.z};
      constexpr std::array<std::uint32_t,6> pcs{
          0x11f69c,0x11f6a2,0x11f6a9,0x11f6b4,0x11f6ba,0x11f6bd};
      for(unsigned i=0;i<6;++i)write(backing,observe,pcs[i],
          (i<3?reg(R::Edx):reg(R::Ecx))+4*(i%3),std::bit_cast<std::uint32_t>(values[i]));
      return 0; // EAX is not consumed by D6660 here.
    }
    if(c.entry==0x133320 && c.instruction==0xd67c6 && c.register_mask==mask({R::Eax,R::Ecx})) {
      Nearest nearest(x87,fp);
      // Each byte-times-binary32 product is exact in binary64 before its
      // single binary32 rounding, matching the original FILD/FMUL/FSTP bytes
      // in this configuration. Alpha is never read by the original leaf.
      const double scale=std::bit_cast<float>(operands.color_scale_bits);
      constexpr std::array<std::uint32_t,3> pcs{0x13334f,0x133370,0x133384};
      for(unsigned i=0;i<3;++i) {
        const float value=float(double((reg(R::Ecx)>>(16-8*i))&0xffu)*scale);
        write(backing,observe,pcs[i],reg(R::Eax)+4*i,std::bit_cast<std::uint32_t>(value));
      }
      return reg(R::Eax);
    }
    throw std::runtime_error("Scenario request reached an unbound lower owner");
  };
  return services;
}
SourceScenarioRequestResult source_cache_scenario_creation_request(
    const SourceTagMetadataProviders &tags,const OriginalData &original,std::uint32_t scenario,
    std::int32_t type,std::int32_t ordinal,std::uint8_t force,
    const std::array<std::byte,0xc4> &prior,std::optional<SourceScenarioGameOptions> game,
    std::uint16_t x87,ObjectConstructionFloatingPointContext fp,
    const ObjectConstructionObserver &observe) {
  if(!tags.definition || !tags.read || !original.object_construction_tables ||
      !original.object_creation_request_operands || !original.scenario_request_operands)
    throw std::runtime_error("Scenario request retained source providers unavailable");
  const auto &scnr=tags.definition(scenario);
  if(scnr.group()!="scnr")throw std::invalid_argument("Scenario request requires selected scnr datum");
  const auto &tables=*original.object_construction_tables;
  const auto offsets=tables.scenario_blocks(type);
  const auto stride=std::bit_cast<std::int16_t>(tables.placement_extent(type));
  if(offsets[0]<0 || offsets[1]<0 || stride<=0 || ordinal<0 || ordinal>32767)
    throw std::out_of_range("Scenario request placement selection unavailable");
  const auto block=scnr.source_address()+std::uint32_t(offsets[0]);
  const auto count=std::bit_cast<std::int32_t>(word(tags.read(block,4)));
  if(ordinal>=count)throw std::out_of_range("Scenario request ordinal escapes selected block");
  const auto placement=word(tags.read(block+4,4))+std::uint32_t(ordinal)*std::uint32_t(stride);
  (void)tags.read(placement,std::size_t(stride));
  constexpr std::uint32_t request=0x02000000,game_address=0x02001000;
  auto bytes=prior;
  std::array<std::byte,0x184> game_bytes{};
  if(game)for(const auto &[at,value]:std::array<std::pair<unsigned,std::uint32_t>,2>{
      {{8,game->game_type},{0x180,game->engine}}})
    for(unsigned i=0;i<4;++i)game_bytes[at+i]=std::byte(value>>(8*i));
  SourceScenarioRequestServices services;
  services.memory=[&](std::uint32_t at,std::size_t n)->std::span<std::byte> {
    if(at>=request && std::uint64_t(at)+n<=std::uint64_t(request)+bytes.size())
      return std::span(bytes).subspan(at-request,n);
    throw std::out_of_range("Scenario request write escapes owned C4 backing");
  };
  services.read=[&](std::uint32_t at,std::size_t n)->std::span<const std::byte> {
    if(at>=request && std::uint64_t(at)+n<=std::uint64_t(request)+bytes.size())
      return std::span<const std::byte>(bytes).subspan(at-request,n);
    if(game && ((at==game_address+8 && n==4) || (at==game_address+0x180 && n==4)))
      return std::span<const std::byte>(game_bytes).subspan(at-game_address,n);
    return tags.read(at,n);
  };
  services.type_address=[&](std::int32_t t){
    if(t<0 || std::size_t(t)>=tables.type_addresses.size())
      throw std::out_of_range("Scenario request selected type unavailable");
    return tables.type_addresses[std::size_t(t)];
  };
  services.placement_extent=[&](std::int32_t t){return tables.placement_extent(t);};
  services.game_options=[&]{
    if(!game)throw std::runtime_error("Scenario request current game-options fields unavailable");
    return game_address;
  };
  services=source_scenario_request_byte_owners(services,*original.object_creation_request_operands,
      *original.scenario_request_operands,x87,observe);
  const bool admitted=source_scenario_creation_request({request,placement,
      scnr.source_address()+std::uint32_t(offsets[1]),type,ordinal,force},
      *original.scenario_request_operands,fp,services,observe);
  return {admitted,bytes,fp};
}
} // namespace halo2
