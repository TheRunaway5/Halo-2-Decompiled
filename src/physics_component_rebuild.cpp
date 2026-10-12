#include "halo2/physics_component_rebuild.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
struct Data {
  const SourcePhysicsComponentRebuildServices &services;
  const ObjectPhysicsConstructionObserver &observer;
  std::span<std::byte> memory(std::uint32_t address,unsigned n)const {
    if(!services.memory)throw std::runtime_error("Required original component rebuild storage unavailable");
    auto bytes=services.memory(address,n);
    if(bytes.size()!=n)throw std::runtime_error("Required original component rebuild extent unavailable");
    return bytes;
  }
  std::uint32_t read(std::uint32_t address,unsigned n=4)const {
    std::array<std::byte,4> raw{};
    if(services.read)services.read(address,std::span(raw).first(n));
    else {const auto source=memory(address,n);std::copy(source.begin(),source.end(),raw.begin());}
    const auto bytes=std::span(raw).first(n);std::uint32_t value=0;
    for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
  }
  void write(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned n=4)const {
    const auto before=read(address,n);std::array<std::byte,4> raw{};auto bytes=std::span(raw).first(n);
    if(n<4)value&=(1u<<(n*8))-1u;
    for(unsigned i=0;i<n;++i)bytes[i]=std::byte(value>>(i*8));
    if(services.write)services.write(address,bytes);
    else {auto target=memory(address,n);std::copy(bytes.begin(),bytes.end(),target.begin());}
    if(observer)observer({site,address,before,value,n});
  }
  std::uint32_t call(const PhysicsComponentRebuildCall &request)const {
    if(!services.invoke)throw std::runtime_error("Required original component rebuild lower owner unavailable");
    return services.invoke(request);
  }
};
std::uint32_t header(const Data &data,const SourcePhysicsComponentRebuildState &state,std::uint32_t datum) {
  return data.read(data.read(state.object_pool_slot)+0x44)+(datum&0xffffu)*12u;
}
PhysicsComponentRebuildCall grouping(std::uint32_t site,std::uint32_t component,unsigned override_mode6) {
  return {site,0x1d9330,component,0,0,0,{component,0,override_mode6,override_mode6?0u:1u},4};
}
}
void source_physics_component_rebuild_ordinary(const SourcePhysicsComponentRebuildState &state,
    std::uint32_t component,const SourcePhysicsComponentRebuildServices &services,
    const ObjectPhysicsConstructionObserver &observer) {
  const Data data{services,observer};
  const auto object=data.read(header(data,state,data.read(component+8))+8);
  const auto result=data.call(grouping(0x1d90b0,component,0))&0xffu;
  data.write(0x1d90b5,component+0x1c,result,1);
  const auto flags=data.read(object+0x12c);
  if(std::bit_cast<std::int8_t>(std::uint8_t(result))>1)
    data.write(0x1d90d0,object+0x12c,flags&0xfffffffeu);
  else data.write(0x1d90c5,object+0x12c,flags|1u);
}
void source_physics_component_rebuild(const SourcePhysicsComponentRebuildState &state,
    std::uint32_t datum,const PhysicsComponentRebuildBranches &branches,
    const SourcePhysicsComponentRebuildServices &services,
    const ObjectPhysicsConstructionObserver &observer) {
  const Data data{services,observer};
  // Preserve the original object-pool read before the component-pool read.
  const auto object_pool=data.read(state.object_pool_slot);
  const auto component=data.read(data.read(state.component_pool_slot)+0x44)+(datum&0xffffu)*160u;
  const auto object_datum=data.read(component+8);
  const auto record=data.read(object_pool+0x44)+(object_datum&0xffffu)*12u;
  const auto object=data.read(record+8);
  const auto active=data.read(record+2,1)&1u;
  const auto type=std::bit_cast<std::int8_t>(std::uint8_t(data.read(object+0xaa,1)));
  const auto index=data.read(state.type_remap+std::uint32_t(std::int32_t(type)),1);
  const auto target=data.read(state.branch_table+index*4u);
  if(target==branches.ordinary)source_physics_component_rebuild_ordinary(state,component,services,observer);
  else if(target==branches.unit)data.call({0x1d2b6c,0x1da570,component,0,0,0,{},0});
  else if(target==branches.item)data.call({0x1d2b98,0x1d90e0,component,0,0,0,{},0});
  else if(target==branches.creature)data.call({0x1d2b73,0x1da690,component,0,0,0,{},0});
  else if(target==branches.direct_grouping) {
    data.write(0x1d2b89,component+4,data.read(component+4)|0x80u);
    const auto result=data.call(grouping(0x1d2b8c,component,1));
    data.write(0x1d2b91,component+0x1c,result,1);
  }else throw std::runtime_error("Required original component indirect branch owner unavailable");
  data.call({0x1d2baf,0x1c8430,component,data.read(component+8),object+0x88,object+0x94,{object+0x94,0,0,0},2});
  const auto flags=data.read(component+4)|1u;
  data.write(0x1d2bc0,component+4,flags);
  data.write(active?0x1d2bcd:0x1d2bda,component+4,active?(flags|0x20000u):(flags&0xfffdffffu));
}
} // namespace halo2
