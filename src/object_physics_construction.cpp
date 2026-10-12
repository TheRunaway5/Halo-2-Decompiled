#include "halo2/object_physics_construction.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
class Backing {
  const SourceObjectPhysicsConstructionServices &services;
  ObjectPhysicsConstructionObserver observer;
public:
  Backing(const SourceObjectPhysicsConstructionServices &s,const ObjectPhysicsConstructionObserver &o):services(s),observer(o){}
  std::span<std::byte> memory(std::uint32_t address,unsigned count)const{
    if(!services.memory)throw std::runtime_error("Original object physics storage unavailable");
    const auto data=services.memory(address,count);
    if(data.size()!=count)throw std::runtime_error("Original object physics storage extent unavailable");
    return data;
  }
  std::uint32_t read(std::uint32_t address,unsigned count=4)const{
    const auto data=memory(address,count);std::uint32_t value=0;
    for(unsigned i=0;i<count;++i)value|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned count=4)const{
    const auto before=read(address,count);auto data=memory(address,count);
    const auto mask=count==4?0xffffffffu:(1u<<(8*count))-1;value&=mask;
    for(unsigned i=0;i<count;++i)data[i]=std::byte(value>>(8*i));
    if(observer)observer({pc,address,before,value,count});
  }
  std::uint32_t heads(const SourceObjectPhysicsConstructionState &state)const{
    return read(read(state.object_pool_slot)+0x44);
  }
  std::uint32_t invoke(std::uint32_t pc,std::uint32_t entry,std::uint32_t object=0,std::uint32_t component=0)const{
    if(!services.invoke)throw std::runtime_error("Complete original object physics lower owner unavailable");
    return services.invoke({pc,entry,object,component});
  }
};
std::uint32_t header(std::uint32_t heads,std::uint32_t object){return heads+(object&0xffffu)*12;}
}
std::uint32_t source_object_physics_root(const SourceObjectPhysicsConstructionState &state,
    std::uint32_t object,const SourceObjectPhysicsConstructionServices &services){
  if(object==0xffffffffu)return object;
  const Backing data(services,{});const auto heads=data.heads(state);
  for(;;){
    const auto current=object;object=data.read(data.read(header(heads,object)+8)+0x14);
    if(object==0xffffffffu)return current;
  }
}
void source_object_physics_word_bit(std::uint32_t address,std::uint32_t bit,
    std::uint32_t request,const SourceObjectPhysicsConstructionServices &services,
    const ObjectPhysicsConstructionObserver &observer){
  const Backing data(services,observer);const auto mask=1u<<(bit&31);const auto before=data.read(address,2);
  if(request&0xffu)data.write(0xc0d1f,address,before|mask,2);
  else data.write(0xc0d29,address,before&~mask,2);
}
void source_object_physics_mark(const SourceObjectPhysicsConstructionState &state,
    std::uint32_t object,const SourceObjectPhysicsConstructionServices &services,
    const ObjectPhysicsConstructionObserver &observer){
  const Backing data(services,observer);const auto body=data.read(header(data.heads(state),object)+8);
  if(!(data.read(body+0xc0,1)&1)){
    data.write(0x1c7304,body+0xc0,data.read(body+0xc0,2)|1,2);
    const auto counter=data.read(state.counter_slot);
    data.write(0x1c7310,counter,data.read(counter)+1);
  }
}
void source_object_physics_construct(const SourceObjectPhysicsConstructionState &state,
    std::uint32_t object,const SourceObjectPhysicsConstructionServices &services,
    const ObjectPhysicsConstructionObserver &observer){
  const Backing data(services,observer);const auto heads=data.heads(state),record=header(heads,object);
  if(!((1u<<(data.read(record+3,1)&31))&0x1883u)||(data.read(record+2,1)&0x10))return;
  const auto ancestor=source_object_physics_root(state,object,services);
  const auto ancestor_record=header(heads,ancestor);
  const auto flags=data.read(record+8)+0xc0;
  data.write(0x1c6fa1,flags+1,data.read(flags+1,1)&0xef,1);
  if(data.read(ancestor_record+4,2)==0xffffu)return;
  const auto context=data.read(state.context_flag,1);
  const auto body=data.read(record+8);
  if(context){
    const auto component=data.invoke(0x1c6fc7,0x1d2aa0,object);
    data.write(0x1c6fcc,body+0xb4,component);
    data.invoke(0x1c6fd2,0x1d2b10,0,component);
    const auto retained=data.read(body+0xb4);
    const auto component_body=data.read(data.read(state.component_pool_slot)+0x44)+(retained&0xffffu)*160;
    if(data.read(body+0xc0,1)&0x40)data.invoke(0x1c7003,0x1d4f30,0,component_body);
    const auto component_flags=data.read(component_body+4);auto request=component_flags>>14;
    if((request&1)||((component_flags>>20)&1))request=1;
    else request&=0xffffff00u;
    source_object_physics_word_bit(data.read(record+8)+0xc0,12,request,services,observer);
  }
  source_object_physics_mark(state,object,services,observer);
}
void source_object_physics_construct_tree(const SourceObjectPhysicsConstructionState &state,
    std::uint32_t root,std::uint32_t object,const SourceObjectPhysicsConstructionServices &services,
    const ObjectPhysicsConstructionObserver &observer){
  const Backing data(services,observer);const auto body=data.read(header(data.heads(state),object)+8);
  const auto component=data.read(body+0xb4);auto child=data.read(body+0x10);
  if(component==0xffffffffu){
    data.invoke(0x1c7079,0x148360);
    source_object_physics_construct(state,object,services,observer);
    data.invoke(0x1c7084,0x27f190);
    data.invoke(0x1c7089,0x148360);
  }
  while(child!=0xffffffffu){
    const auto child_body=data.read(header(data.heads(state),child)+8);
    if(child==root)break;
    source_object_physics_construct_tree(state,root,child,services,observer);
    child=data.read(child_body+0xc);
  }
}
} // namespace halo2
