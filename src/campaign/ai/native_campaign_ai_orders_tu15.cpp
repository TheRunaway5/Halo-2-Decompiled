#include "halo2/native_campaign_ai_orders_tu15.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
class OrderScriptAssignments {
  SourcePhysicsContactMemory &memory_;
  std::uint32_t read(std::uint32_t address)const {
    std::array<std::byte,4> bytes{};memory_.read(address,bytes);
    std::uint32_t result=0;
    for(unsigned i=0;i<4;++i)result|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8u);
    return result;
  }
  void write(std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8u));
    memory_.write(address,bytes);
  }
  std::uint32_t hash(std::uint32_t table,std::uint32_t key)const {
    if(read(table+0x2cu)!=0x267a30u)
      throw std::runtime_error("Campaign order-script table has no native hash owner");
    return (key&0xffu)*4u;
  }
  bool equal(std::uint32_t table,std::uint32_t a,std::uint32_t b)const {
    if(read(table+0x30u)!=0x267a40u)
      throw std::runtime_error("Campaign order-script table has no native equality owner");
    return a==b;
  }
  std::uint32_t bucket(std::uint32_t table,std::uint32_t hash_value)const {
    const auto count=read(table+0x20u);
    if(!count)throw std::runtime_error("Campaign order-script table has zero buckets");
    return table+0x3cu+(hash_value%count)*4u;
  }
  void payload(std::uint32_t table)const {
    if(read(table+0x28u)!=4u)
      throw std::runtime_error("Campaign order-script table lost its actor-datum payload owner");
  }
  void erase(std::uint32_t table,std::uint32_t key) {
    const auto slot=bucket(table,hash(table,key));
    auto previous=0u,node=read(slot);
    while(node) {
      const auto stored_key=read(node);
      if(equal(table,key,stored_key)) {
        const auto next=read(node+8u);
        if(previous)write(previous+8u,next);else write(slot,next);
        const auto free=read(table+0x38u);
        write(node+8u,free);write(table+0x38u,node);
        return;
      }
      previous=node;node=read(node+8u);
    }
  }
  std::uint32_t find(std::uint32_t table,std::uint32_t key)const {
    const auto hashed=hash(table,key);
    auto node=read(bucket(table,hashed));
    while(node) {
      if(read(node+4u)==hashed) {
        const auto stored_key=read(node);
        if(equal(table,key,stored_key))return node;
      }
      node=read(node+8u);
    }
    return 0;
  }
  void insert(std::uint32_t table,std::uint32_t key,std::uint32_t actor) {
    if(!read(table+0x38u))return; // Original allocation refusal is not success.
    const auto hashed=hash(table,key);
    const auto slot=bucket(table,hashed);
    const auto node=read(table+0x38u);
    const auto next_free=read(node+8u);
    write(table+0x38u,next_free);
    write(node+4u,hashed);write(node,key);
    payload(table);write(node+0xcu,actor);
    const auto head=read(slot);
    write(node+8u,head);write(slot,node);
  }
public:
  explicit OrderScriptAssignments(SourcePhysicsContactMemory &memory):memory_(memory) {}
  void assign(std::uint32_t actor,std::uint32_t target) {
    const auto actors=read(0x5020c8u);
    const auto rows=read(actors+0x44u);
    const auto none=read(0x47d6c0u);
    const auto current_actor=rows+(actor&0xffffu)*0x888u;
    const auto old=read(current_actor+0x418u);
    if(old!=none)erase(read(0x56aeb0u),old);
    if(target!=read(0x47d6c0u)) {
      const auto table=read(0x56aeb0u);
      const auto node=find(table,target);
      std::uint32_t previous=0xffffffffu;
      if(node) {payload(table);previous=read(node+0xcu);}
      if(previous!=0xffffffffu) {
        const auto old_actors=read(0x5020c8u);
        const auto old_rows=read(old_actors+0x44u);
        const auto no_assignment=read(0x47d6c0u);
        write(old_rows+(previous&0xffffu)*0x888u+0x418u,no_assignment);
        const auto current_table=read(0x56aeb0u);
        if(const auto current=find(current_table,target)) {
          payload(current_table);write(current+0xcu,actor);
        }
      } else insert(read(0x56aeb0u),target,actor);
    }
    //26C600 keeps this actor-side publication even after allocation refusal.
    write(current_actor+0x418u,target);
  }
};
}
void native_campaign_ai_order_script_assignment_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t actor,std::uint32_t script_key) {
  OrderScriptAssignments(memory).assign(actor,script_key);
}
}
