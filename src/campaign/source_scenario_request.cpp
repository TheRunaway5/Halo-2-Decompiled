#include "halo2/source_scenario_request.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Request {
  const SourceScenarioRequestServices &p;
  const ObjectConstructionObserver &observe;
public:
  Request(const SourceScenarioRequestServices &services,const ObjectConstructionObserver &o)
      :p(services),observe(o) {}
  std::uint32_t read(std::uint32_t address,unsigned bytes=4) const {
    if(!p.read)throw std::runtime_error("Scenario request read owner unavailable");
    const auto data=p.read(address,bytes);
    if(data.size()!=bytes)throw std::runtime_error("Scenario request read extent unavailable");
    std::uint32_t value=0;
    for(unsigned i=0;i<bytes;++i)value|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned bytes=4) {
    if(!p.memory)throw std::runtime_error("Scenario request storage owner unavailable");
    auto data=p.memory(address,bytes);
    if(data.size()!=bytes)throw std::runtime_error("Scenario request storage extent unavailable");
    std::uint32_t before=0;
    for(unsigned i=0;i<bytes;++i)before|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
    if(bytes<4)value&=(1u<<(8*bytes))-1;
    for(unsigned i=0;i<bytes;++i)data[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,bytes,before,value});
  }
  std::uint32_t call(std::uint32_t pc,std::uint32_t entry,
      std::initializer_list<std::pair<ObjectConstructionRegister,std::uint32_t>> regs,
      std::initializer_list<std::uint32_t> args,ObjectConstructionFloatingPointContext &fp) {
    if(!p.invoke)throw std::runtime_error("Scenario request lower owner unavailable");
    ObjectConstructionCall c{pc,entry};
    for(const auto &[reg,value]:regs) {
      const auto index=unsigned(reg);c.register_mask|=1u<<index;c.registers[index]=value;
    }
    for(auto value:args)c.arguments[c.argument_count++]=value;
    return p.invoke(c,fp);
  }
};
}
std::uint32_t source_scenario_request_policy(std::uint32_t placement,
    const SourceScenarioRequestServices &p) {
  Request r(p,{});
  const auto policy=std::bit_cast<std::int8_t>(std::uint8_t(r.read(placement+0x30,1)));
  if(policy==1)return 0;
  if(policy==2)return 2;
  if(policy!=0)return 0xffffffffu;
  const auto source=std::bit_cast<std::int8_t>(std::uint8_t(r.read(placement+0x2f,1)));
  if(source==0)return 1;
  if(source!=1)return 0xffffffffu;
  if(r.read(placement+0x2c,2)==0xffffu)return 0;
  return ((1u<<(r.read(placement+0x2e,1)&31))&0x181fu)?0:1;
}
bool source_scenario_creation_request(const SourceScenarioRequestInput &in,
    const SourceScenarioRequestOperands &c,ObjectConstructionFloatingPointContext &fp,
    const SourceScenarioRequestServices &p,const ObjectConstructionObserver &o) {
  using R=ObjectConstructionRegister;
  Request r(p,o);
  bool allowed=(~(r.read(in.placement+4)>>6)&1)!=0;
  if(!p.type_address)throw std::runtime_error("Scenario request type table owner unavailable");
  const auto type_address=p.type_address(in.type);
  if(in.type==6) {
    if(!p.game_options)throw std::runtime_error("Scenario request current game-options owner unavailable");
    const auto game=p.game_options();
    if(r.read(game+8)==2) {
      const auto mask=r.read(in.placement+0x5a,2);
      if(mask && mask!=0x7f) {
        const auto engine=r.read(game+0x180)-1u;
        constexpr std::array<int,9> bit{0,1,2,3,-1,-1,4,5,6};
        if(engine<bit.size() && bit[engine]>=0)
          allowed=((mask>>unsigned(bit[engine]))&1)!=0;
      }
    }
  }
  if(!allowed || !type_address)return false;
  const auto index=std::bit_cast<std::int16_t>(std::uint16_t(r.read(in.placement,2)));
  if(index==-1)return false;
  std::int32_t clamped=0;
  if(index>=0) {
    clamped=std::bit_cast<std::int32_t>(r.read(in.palette)-1u);
    if(index<=clamped)clamped=index;
  }
  if(clamped!=index)return false;
  const auto definition=r.read(r.read(in.palette+4)+std::uint32_t(index)*40u+4);
  const bool admitted=definition!=0xffffffffu;
  if(admitted) {
    r.call(0xd673b,0xb8820,{{R::Esi,in.request}},
        {definition,0xffffffffu,0},fp);
    r.write(0xd6748,in.request+0x1c,r.read(in.placement+8));
    r.write(0xd674d,in.request+0x20,r.read(in.placement+0xc));
    r.write(0xd6753,in.request+0x24,r.read(in.placement+0x10));
    // The authenticated comparison operand is +0 in both selected images.
    if(c.zero_bits!=0)throw std::runtime_error("Scenario request COMISS operand unavailable");
    const auto scale=r.read(in.placement+0x20);
    r.write(0xd6777,in.request+0x58,
        source_object_construction_positive_scalar(scale,fp)?scale:c.one_bits);
    r.call(0xd677c,0x11f680,{{R::Eax,in.placement+0x14},
        {R::Ecx,in.request+0x34},{R::Edx,in.request+0x28}},{},fp);
    const auto policy=r.call(0xd6783,0xd7070,{{R::Edx,in.placement}},{},fp);
    r.write(0xd6791,in.request+0x14,policy,1);
    if((1u<<(std::uint32_t(in.type)&31))&0x847u) {
      if(!p.placement_extent)throw std::runtime_error("Scenario request type placement extent unavailable");
      if(p.placement_extent(in.type)>=0x4c) {
        r.write(0xd67af,in.request+0xc,r.read(in.placement+0x34));
        r.write(0xd67b5,in.request+0x74,r.read(in.placement+0x38));
        auto color=in.request+0x78;
        for(unsigned i=0;i<4;++i)color=r.call(0xd67c6,0x133320,
            {{R::Eax,color},{R::Ecx,r.read(in.placement+0x3c+4*i)}},{},fp)+12;
      }
    }
  }
  // Definition NONE bypasses initialization, yet these writes still occur.
  r.write(0xd67d7,in.request+4,r.read(in.placement+0x28));
  r.write(0xd67e3,in.request+8,r.read(in.placement+0x2c));
  const bool forced=in.force && ((1u<<(r.read(in.placement+0x2e,1)&31))&0x1883u);
  if(forced || (r.read(in.placement+4)&0x100u)) {
    r.write(0xd6804,in.request+0x18,r.read(in.request+0x18)|8u);
    r.write(0xd680b,in.request+0x10,std::uint32_t(in.ordinal));
  } else {
    r.write(0xd6829,in.request+0x18,r.read(in.request+0x18)&~8u);
    r.write(0xd682c,in.request+0x10,std::uint32_t(in.ordinal));
  }
  return admitted;
}
} // namespace halo2
