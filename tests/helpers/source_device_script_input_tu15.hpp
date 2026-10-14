#pragma once
#include "halo2/scripts.hpp"
#include <bit>
namespace halo2::test {
struct DeviceScriptProgram {
  ScriptProgram data;
  int device_name;
  std::uint32_t add(ScriptNode node){const auto index=std::uint32_t(data.nodes.size());
    node.salt=std::uint16_t(0xe000u+index);data.nodes.push_back(node);return (std::uint32_t(node.salt)<<16u)|index;}
  std::uint32_t literal(unsigned type,std::uint32_t bits){return add({0,std::uint16_t(type),std::uint16_t(type),9,null_datum,0,bits});}
  std::uint32_t real(float value){return literal(6,std::bit_cast<std::uint32_t>(value));}
  std::uint32_t device(){return add({0,60,54,9,null_datum,0,std::uint32_t(device_name)});}
  std::uint32_t group(){return literal(17,0);}
  std::uint32_t call(const std::string &name,std::vector<std::uint32_t> children,unsigned type=4){
    const auto offset=std::uint32_t(data.strings.size());data.strings+=name;data.strings.push_back('\0');
    for(unsigned n=0;n<children.size();++n)data.nodes[children[n]&65535u].next=n+1<children.size()?children[n+1]:null_datum;
    const auto function=add({0,0,2,9,children.empty()?null_datum:children[0],offset,0});
    return add({0,0,std::uint16_t(type),8,null_datum,offset,function});
  }
  std::uint32_t assign(unsigned index,std::uint32_t value){
    const auto type=data.globals.at(index).type;
    return call("set",{add({0,type,type,13,null_datum,0,index}),value},type);
  }
  explicit DeviceScriptProgram(int name,unsigned pause_ticks=1,bool dormant=false):device_name(name){
    for(unsigned n=0;n<9;++n){const unsigned type=n==0?8:(n==4 || n==5 || n==8)?5:6;
      data.globals.push_back({"device_result_"+std::to_string(n),std::uint16_t(type),literal(type,0)});}
    std::vector<std::uint32_t> steps;
    const auto emit=[&](auto node){steps.push_back(node);};
    const auto observe=[&](unsigned stage){
      emit(assign(1,call("device_get_position",{device()},6)));
      emit(assign(2,call("device_get_power",{device()},6)));
      emit(assign(3,call("device_group_get",{group()},6)));
      emit(assign(0,literal(8,stage)));emit(call("sleep",{literal(7,pause_ticks)}));
    };
    emit(call("device_set_position_immediate",{device(),real(.375f)}));observe(1);
    emit(call("device_set_power",{device(),real(.5f)}));observe(2);
    emit(assign(4,call("device_set_position",{device(),real(.75f)},5)));observe(3);
    emit(assign(5,call("device_set_position",{device(),real(.125f)},5)));observe(4);
    emit(call("device_group_set_immediate",{group(),real(.25f)}));observe(5);
    emit(assign(5,call("device_group_set",{device(),group(),real(1.f)},5)));observe(6);
    emit(call("device_set_position_immediate",{device(),real(1.25f)}));
    emit(call("device_set_power",{device(),real(-.25f)}));observe(7);
    emit(assign(6,call("device_get_position",{literal(54,null_datum)},6)));
    emit(assign(7,call("device_get_power",{literal(54,null_datum)},6)));
    emit(assign(8,call("device_set_position",{literal(54,null_datum),real(1.f)},5)));observe(8);
    data.scripts.push_back({"device_source_sequence",std::uint16_t(dormant?1:0),4,call("begin",std::move(steps))});
  }
};

}
