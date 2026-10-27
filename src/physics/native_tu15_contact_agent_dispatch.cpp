#include "halo2/native_tu15_contact_agent_dispatch.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
}
NativeTu15ContactAgent native_tu15_dispatch_contact_agent(SourcePhysicsContactMemory&memory,
  NativeTu15ContactPayload first,NativeTu15ContactPayload second,NativeTu15ContactContext context,
  std::uint32_t user_data,NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactAgentDispatchServices&services){
  const NativeTu15ContactChild first_shape{word(memory,first.identity)};
  const auto first_table=word(memory,first_shape.identity);const auto dispatch=word(memory,context.identity);
  const NativeTu15ContactVirtualOwner first_owner{word(memory,first_table+20u)};
  if(!services.classify_shape)throw std::runtime_error("Native TU1.5 contact dispatch requires actual shape classification owner");
  const auto first_class=services.classify_shape(first_shape,first_owner,fp);
  const NativeTu15ContactChild second_shape{word(memory,second.identity)};const auto second_table=word(memory,second_shape.identity);
  const NativeTu15ContactVirtualOwner second_owner{word(memory,second_table+20u)};
  const auto second_class=services.classify_shape(second_shape,second_owner,fp);
  const auto index=(first_class<<5u)+second_class;
  const NativeTu15ContactVirtualOwner factory{word(memory,dispatch+index*4u+0x18cu)};
  if(!services.create_agent)throw std::runtime_error("Native TU1.5 contact dispatch requires its actual selected agent factory");
  return services.create_agent(factory,first,second,context,user_data,fp);
}
} // namespace halo2
