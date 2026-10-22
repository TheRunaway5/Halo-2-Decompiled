#include "halo2/source_multiplayer_engine_entity_tu15.hpp"
#include "halo2/source_multiplayer_engines_tu15.hpp"
#include "halo2/source_ctf_activation_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
struct Schema {std::int32_t type;std::size_t size;std::uint32_t initialize,difference;};
// Exact complete-store extents and registered callbacks in the pinned image.
// Original instruction receipts:engine-round-runtime-tu15-20261021/
// original-engine-entity-owners.json. CTF/Assault share their actual class.
constexpr std::array<Schema,7> schemas{{
  {1,0x84,0x24c680,0x24c6f0},{2,0x24,0x2c5f80,0x2c5fb0},
  {3,0x24,0x2c5f80,0x2c5fb0},{4,0x28,0x2c7900,0x2c7940},
  {7,0x26,0x198880,0x1988c0},{8,0x64,0x2ca290,0x2ca2d0},
  {9,0x84,0x24c680,0x24c6f0}}};
const Schema& schema(std::int32_t type) {
  const auto found=std::find_if(schemas.begin(),schemas.end(),[&](const auto& s){return s.type==type;});
  if(found==schemas.end())throw std::invalid_argument("Unknown original multiplayer engine entity type");
  return *found;
}
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t address,unsigned size=4) {
  std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(size));std::uint32_t result{};
  for(unsigned n=0;n<size;++n)result|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
  return result;
}
void put(SourcePhysicsContactMemory& memory,std::uint32_t address,std::uint32_t value) {
  std::array<std::byte,4> bytes{};
  for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));memory.write(address,bytes);
}
class CallerMemory final:public SourcePhysicsContactMemory {
public:
  CallerMemory(const SourceRuntimeMemory& source,std::vector<SourceMultiplayerEngineEntityReadTu15>* evidence)
      :source_(source),evidence_(evidence) {
    const auto gap=source.find_unowned_range(0x70000000u,0x7f000000u,caller_.size(),0x10000u);
    if(!gap)throw std::runtime_error("Original engine query has no disjoint caller frame");
    base_=*gap;
  }
  std::uint32_t base()const{return base_;}
  void read(std::uint32_t address,std::span<std::byte> bytes)const override {
    if(local(address,bytes.size())) {
      std::copy_n(caller_.begin()+(address-base_),bytes.size(),bytes.begin());return;
    }
    if(overlap(address,bytes.size()))throw std::runtime_error("Original engine query straddled its caller frame");
    source_.read(address,bytes);
    if(evidence_)evidence_->push_back({address,{bytes.begin(),bytes.end()}});
  }
  void write(std::uint32_t address,std::span<const std::byte> bytes)override {
    if(!local(address,bytes.size()))throw std::runtime_error("Original engine query attempted a world store");
    std::copy(bytes.begin(),bytes.end(),caller_.begin()+(address-base_));
  }
private:
  bool local(std::uint32_t address,std::size_t size)const {
    return address>=base_ && std::uint64_t(address)+size<=std::uint64_t(base_)+caller_.size();
  }
  bool overlap(std::uint32_t address,std::size_t size)const {
    return std::uint64_t(address)+size>base_ && address<std::uint64_t(base_)+caller_.size();
  }
  const SourceRuntimeMemory& source_;
  std::vector<SourceMultiplayerEngineEntityReadTu15>* evidence_;
  std::uint32_t base_{};
  std::array<std::byte,0x10000> caller_{};
};
}
std::size_t source_multiplayer_engine_entity_size_tu15(std::int32_t type){return schema(type).size;}
bool source_multiplayer_engine_entity_enemies_tu15(
    const SourceMultiplayerEngineEntityTu15& entity,unsigned left,unsigned right) {
  if(entity.bytes.size()!=schema(entity.type).size || (entity.type!=7 && entity.relationship_mask))
    throw std::invalid_argument("Original engine relationship has an invalid entity schema");
  if(left>=16 || right>=16)throw std::out_of_range("Original engine membership is outside the native roster");
  if(left==right)return false;
  if(entity.type!=7 || !entity.relationship_mask)return true;
  return (entity.relationship_mask & ((1u<<left)|(1u<<right)))!=0;
}
SourceMultiplayerEngineEntityTu15 source_multiplayer_engine_entity_tu15(
    const SourceRuntimeMemory& source,SourcePhysicsFactoryFloatingPoint fp,
    std::vector<SourceMultiplayerEngineEntityReadTu15>* evidence) {
  CallerMemory memory(source,evidence);
  const auto caller_esp=memory.base()+0x8000u,output=caller_esp+0x100u,mask=caller_esp+0x200u;
  const auto options=word(memory,0x4f3218u);
  if(!options || word(memory,options+8u)!=2u || !word(memory,0x4f63b8u))
    throw std::invalid_argument("Original engine entity requires an actual multiplayer world");
  const auto type=std::int32_t(word(memory,options+0x180u));const auto& selected=schema(type);
  const auto object=word(memory,0x56dfa0u+std::uint32_t(type)*4u);
  if(!object)throw std::runtime_error("Original engine entity has no registered class");
  const auto vtable=word(memory,object);
  if(word(memory,vtable+44u*4u)!=selected.initialize || word(memory,vtable+45u*4u)!=selected.difference ||
      word(memory,vtable+41u*4u)!=0x25c250u || word(memory,vtable+42u*4u)!=0x25c290u)
    throw std::runtime_error("Original engine entity callback registration differs");
  if(word(memory,vtable+27u*4u)!=(type==7?0x198680u:0x25bc70u))
    throw std::runtime_error("Original engine relationship callback registration differs");
  SourcePhysicsWorldRuntimeServices services;
  services.required_owner=[&](auto& m,const auto& call,auto& abi,const auto& observer) {
    if(call.target==0x25c250u || call.target==0x25c290u)
      source_run_multiplayer_engines_tu15(m,call.target,abi,fp,services,observer);
    else if(call.target==0x164930u)
      source_run_ctf_activation_tu15(m,call.target,abi,fp,services,observer);
    else throw std::runtime_error("Unresolved original engine entity query callee");
  };
  const auto call=[&](std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp[1]=object;abi.gp[4]=caller_esp;abi.eflags=2;abi.floating_point=fp;
    put(memory,caller_esp,memory.base()+0xf000u);unsigned n{};
    for(const auto value:args)put(memory,caller_esp+4u+(n++)*4u,value);
    source_run_multiplayer_engines_tu15(memory,entry,abi,fp,services);
    if(abi.gp[4]!=caller_esp+4u+args.size()*4u)
      throw std::runtime_error("Original engine entity query lost its complete return ABI");
  };
  call(selected.initialize,{0u,output});put(memory,mask,std::numeric_limits<std::uint32_t>::max());
  call(selected.difference,{mask,0u,output});
  SourceMultiplayerEngineEntityTu15 result{type,word(memory,mask),std::vector<std::byte>(selected.size)};
  if(type==7)result.relationship_mask=std::uint16_t(word(memory,word(memory,0x51df9cu)+2u,2));
  memory.read(output,result.bytes);return result;
}
} // namespace halo2
