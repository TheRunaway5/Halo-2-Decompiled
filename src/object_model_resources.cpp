#include "halo2/object_model_resources.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
bool source_object_model_resources(const SourceObjectModelResourceState &state,
    std::uint32_t object,std::uint32_t mode_output,std::uint32_t collision_output,
    const SourceObjectModelResourceServices &services,const SourceObjectModelResourceObserver &observe){
  const auto memory=[&](std::uint32_t address){
    if(!services.memory)throw std::runtime_error("Original object/model resource backing unavailable");
    auto data=services.memory(address,4);
    if(data.size()!=4)throw std::runtime_error("Original object/model resource extent unavailable");
    return data;
  };
  const auto read=[&](std::uint32_t address){
    const auto data=memory(address);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
    return value;
  };
  const auto write=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value){
    const auto before=read(address);auto data=memory(address);
    for(unsigned i=0;i<4;++i)data[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,before,value});
  };
  const auto tag=[&](std::uint32_t datum){return read(state.tag_table+(datum&0xffffu)*16+8);};
  const auto heads=read(state.object_pool+0x44);
  const auto body=read(heads+(object&0xffffu)*12+8);
  const auto definition=tag(read(body));
  const auto model_datum=read(definition+0x38);
  if(model_datum==0xffffffffu)return false;
  const auto model=tag(model_datum);
  const auto mode=read(model+4);
  if(mode==0xffffffffu||read(model+0xc)==0xffffffffu)return false;
  const auto mode_body=tag(mode);
  const bool valid=std::bit_cast<std::int32_t>(read(mode_body+0x74))>0&&
                   std::bit_cast<std::int32_t>(read(mode_body+0x7c))>0;
  write(valid?0xc0901:0xc0916,mode_output,mode);
  // The original rereads this after the first output store. Retain aliasing.
  const auto collision=read(model+0xc);
  write(valid?0xc090c:0xc0921,collision_output,collision);
  return valid;
}
} // namespace halo2
