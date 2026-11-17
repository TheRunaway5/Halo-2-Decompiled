// Native TU1.5 object_new (B9B70), track "birth". Stores follow the
// original order and widths; see the header for injected owners.
#include "halo2/native_object_factory_birth_tu15.hpp"
#include "halo2/native_object_factory_core_tu15.hpp"
#include "halo2/native_object_factory_tu15.hpp"
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace halo2::native_object_factory_tu15 {
using namespace detail;
namespace {
// The original's stack frame: a private extent at `base`; every other
// access goes to the world in order.
// Nested frames (C0790 -> object_new) get distinct private bases.
class Locals final : public SourcePhysicsContactMemory {
public:
  Locals(SourcePhysicsContactMemory& world,std::size_t bytes):world_(world),bytes_(bytes) {
    const auto* outer=dynamic_cast<const Locals*>(&world);
    base_=outer?outer->base_+0x1000u:0xfff00000u;
    if(base_<0xfff00000u)refuse("B9B70","native frame nesting depth");
  }
  std::uint32_t base()const{return base_;}
  void read(std::uint32_t address,std::span<std::byte> output)const override {
    if(address>=base_ && address-base_+output.size()<=bytes_.size())
      std::copy_n(bytes_.begin()+(address-base_),output.size(),output.begin());
    else world_.read(address,output);
  }
  void write(std::uint32_t address,std::span<const std::byte> input)override {
    if(address>=base_ && address-base_+input.size()<=bytes_.size())
      std::copy(input.begin(),input.end(),bytes_.begin()+(address-base_));
    else world_.write(address,input);
  }
private:
  SourcePhysicsContactMemory& world_;
  std::vector<std::byte> bytes_;
  std::uint32_t base_=0xfff00000u;
};

template<class F,class... A> decltype(auto) need(const F& service,const char* owner,A&&... args) {
  if(!service)refuse(owner,"service");
  return service(std::forward<A>(args)...);
}
template<class Map> const typename Map::mapped_type& callback(const Map& map,std::uint32_t address,const char* slot) {
  const auto found=map.find(address);
  if(found==map.end()) {
    char name[16];std::snprintf(name,sizeof name,"%X",address);
    refuse(name,std::string("object type ")+slot+" callback");
  }
  return found->second;
}
std::uint32_t chain_type(const M& m,std::uint32_t object){return sx8(r8(m,object_body(m,object)+0xaau));}
}

std::uint32_t object_new(M& world,std::uint32_t request,const ObjectFactoryServices& services) {
  namespace fp=native_campaign_script_fp;
  Locals m(world,0x100);
  const auto failed_byte=m.base(),tag_outputs=m.base()+4u,probe_record=m.base()+0x10u;
  const auto request_flags=[&]{return r32(m,request+0x18u);};
  if(!(request_flags()&0x10u)) {
    if(r32(m,request)==none)return none;
    // 10DAF0: the definition type's validate slots.
    const auto type=definition_object_type(m,r32(m,request));
    if(type==none)refuse("10DAF0","definition group outside the type table");
    for(const auto address:object_type_callbacks(type,type_slot_validate))
      if(address)callback(services.validate,address,"validate")(m,request);
  }
  const auto tag=r32(m,request);
  if(tag==none)return none;
  const auto definition=tag_data(m,tag);
  const auto type=sx16(r16(m,definition));
  if(type>=13u)refuse("B9B70","object type "+std::to_string(std::int32_t(type)));
  std::uint32_t model=0;
  if(r32(m,definition+0x38u)!=none)model=tag_data(m,r32(m,definition+0x38u));
  if((1u<<(type&31u))&0x1883u)physics_memory_reserve(m,services.mxcsr);
  // Body size: the type definition's +08 word (0x474B90 literal level 3 or 2).
  static constexpr std::uint16_t body_sizes[13]={0x464,0x428,0x25c,0x184,0x170,0x1ac,0x13c,0x1e0,0x1d4,0x1e4,0x148,0x130,0x208};
  const auto object=object_allocate(m,std::int16_t(body_sizes[type]));
  if(object==none)return none;
  const auto offset=(object&0xffffu)*12u;
  const auto row=r32(m,r32(m,object_pool)+0x44u)+offset;
  const auto body=r32(m,row+8u);
  w8(m,row+2u,std::uint8_t(r8(m,row+2u)|8u));
  w8(m,row+3u,r8(m,definition));
  w32(m,body,r32(m,request));
  if(r8(m,request+0xbu)==0xffu) {
    const auto counter=r32(m,0x4eabc4u);
    w32(m,counter+0x18u,r32(m,counter+0x18u)+1u);
    w8(m,body+0xaau,std::uint8_t(type));
    w8(m,body+0xabu,2);
    w16(m,body+0xa8u,0xffffu);
    w32(m,body+0xa4u,r32(m,counter+0x18u));
    w16(m,body+0x1au,0xffffu);
    w8(m,body+0xaeu,r8(m,0x474c24u));
  } else {
    w32(m,body+0xa4u,r32(m,request+4u));
    const auto team=r32(m,request+8u);
    w32(m,body+0xa8u,team);
    w16(m,body+0x1au,r16(m,request+0x10u));
    w8(m,body+0xaeu,std::uint8_t(team));
  }
  for(const auto [from,to]:{std::pair{0x1cu,0x64u},{0x28u,0x70u},{0x34u,0x7cu},{0x40u,0x88u},{0x4cu,0x94u}})
    for(std::uint32_t n=0;n<3;++n)w32(m,body+to+n*4u,r32(m,request+from+n*4u));
  w32(m,body+0xa0u,r32(m,request+0x58u));
  auto flags=r32(m,body+4u);
  w32(m,body+4u,request_flags()&1u?flags|0x400u:flags&~0x400u);
  const bool collision=model && r32(m,model+0xcu)!=none;
  flags=r32(m,body+4u);
  w32(m,body+4u,collision?flags|0x200u:flags&~0x200u);
  const bool populated=object_model_tag_pair(m,object,tag_outputs,tag_outputs+4u);
  flags=r32(m,body+4u);
  const auto default_team=r16(m,0x474c24u);
  w32(m,body+4u,populated?flags|0x80000000u:flags&0x7fffffffu);
  w16(m,row+4u,0xffffu);
  w32(m,body+0x28u,none);
  w16(m,body+0x2cu,0xffffu);
  w16(m,body+0x2eu,default_team);
  w32(m,body+0x60u,none);
  w8(m,body+0xb0u,r8(m,request+0x14u));
  w32(m,body+0x14u,none);
  w32(m,body+0xcu,none);
  w32(m,body+0x10u,none);
  w16(m,body+0xacu,0xffffu);
  w8(m,body+0xafu,0xffu);
  w8(m,body+0x108u,0xffu);
  w8(m,body+0x109u,0xffu);
  if(r8(m,definition+2u)&1u)w32(m,body+4u,r32(m,body+4u)|0x10000u);
  {
    const auto live=r32(m,r32(m,object_pool)+0x44u)+offset;
    const auto current=r32(m,live+8u);
    if(r8(m,current+4u)&1u) {
      w32(m,current+4u,r32(m,current+4u)&~1u);
      if(object_root_header_bit6(m,object))need(services.disconnect,"C0F60",m,object,0,1,0,0);
      object_refresh_spatial(m,object);
    }
  }
  w16(m,body+0xc2u,r16(m,request+0x70u));
  w32(m,body+0xc4u,r32(m,request+0x68u));
  w32(m,body+0xc8u,r32(m,request+0x6cu));
  w8(m,body+0xb1u,0xffu);
  w32(m,body+0xccu,none);
  w16(m,body+0xd0u,0xffffu);
  w16(m,body+0xc0u,0);
  w8(m,body+0xc1u,(request_flags()>>3)&1u?std::uint8_t(r8(m,body+0xc1u)|1u):std::uint8_t(r8(m,body+0xc1u)&0xfeu));
  w32(m,body+0xb4u,none);
  w32(m,body+0xd4u,none);
  w8(m,body+0xd8u,0);
  w16(m,body+0xe0u,r16(m,request+0xb0u));
  w16(m,body+0xe2u,r16(m,request+0xb2u));
  if(std::int8_t(std::uint8_t(1u<<(r8(m,row+3u)&31u)))<0 || s32(r32(m,r32(m,0x51df54u)+8u))<6)w8(m,body+0xb3u,0);
  else w8(m,body+0xb3u,6);
  std::uint32_t nodes=1,regions=1,attachments=0;
  bool animated=false,havok_blocks=false;
  if(model) {
    if(s32(r32(m,model+0x78u))>=1)nodes=r32(m,model+0x78u);
    if(s32(r32(m,model+0x70u))>=1)regions=r32(m,model+0x70u);
    if(s32(r32(m,model+0x60u))>0)attachments=r32(m,r32(m,model+0x64u)+0xbcu);
    if(r32(m,model+0x14u)!=none) {
      havok_record_initialize(m,probe_record);
      if(need(services.animation_setup,"1D3380",m,probe_record,r32(m,model+0x14u),r32(m,definition+0x38u))) {
        const auto bit=1u<<(r8(m,definition)&31u);
        animated=true;
        havok_blocks=!(bit&0x7e0u);
        if((bit&0x380u) && (r8(m,tag_data(m,r32(m,request))+0xbcu)&4u))havok_blocks=true;
      }
      havok_record_clear_links(m,probe_record);
    }
  }
  const auto block=[&](std::uint32_t header,std::uint32_t bytes,std::uint8_t alignment) {
    return object_block_allocate(m,object,std::int16_t(header),std::int16_t(bytes),alignment,
        services.entry_stack?services.entry_stack-0xf8u:0u);
  };
  bool built=block(0x11c,std::uint16_t(r16(m,definition+0x94u)<<3),0) && block(0x120,attachments*8u,0) &&
    block(0x124,std::uint16_t(r16(m,definition+0xacu)*0x18u),0) && block(0x114,nodes*0x34u,0) &&
    block(0x118,regions*10u,0) && block(0x110,havok_blocks?nodes<<5:0u,4) && block(0x10c,havok_blocks?nodes<<5:0u,4) &&
    block(0x128,animated?0x90u:0u,0) && object_definition_physics_capacity(m,r32(m,request));
  if(!built)w8(m,failed_byte,1);
  auto current=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  if(!built){object_free(m,object);return none;}
  if(animated) {
    const auto record=sx16(r16(m,current+0x12au))+current;
    havok_record_reset(m,record);
    const bool installed=need(services.animation_setup,"1D3380",m,record,r32(m,model+0x14u),r32(m,definition+0x38u));
    flags=r32(m,current+4u);
    w32(m,current+4u,installed?flags|0x800u:flags&~0x800u);
  } else w8(m,current+0xb3u,0);
  if(s32(r32(m,definition+0x94u))>0) {
    const auto bytes=(sx16(r16(m,current+0x11cu))>>3)<<3;
    const auto at=sx16(r16(m,current+0x11eu))+current;
    for(std::uint32_t n=0;n<bytes>>2;++n)w32(m,at+n*4u,none);
  }
  // 10DB40: the chain's new slots; after a failure the rest are skipped.
  bool constructed=true;
  for(const auto address:object_type_callbacks(chain_type(m,object),type_slot_new))
    if(address && constructed)constructed=callback(services.construct,address,"new")(m,object,request,failed_byte);
  const auto saved=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  if(!constructed) {
    for(const auto address:object_type_callbacks(chain_type(m,object),type_slot_new_failed))
      if(address)callback(services.construct_failed,address,"new failed")(m,object);
    object_free(m,object);
    return none;
  }
  const bool kept=(r32(m,saved+4u)>>17)&1u;
  if(request_flags()&6u)w32(m,saved+4u,r32(m,saved+4u)&0xfffdffffu);
  object_set_variant(m,object,r32(m,request+0xcu));
  need(services.regions,"BC5C0",m,object,r32(m,request+0xacu));
  need(services.change_colors,"C0270",m,object,r32(m,request+0x74u),request+0x78u);
  const auto vitality_body=object_body(m,object);
  const auto info=object_damage_info(m,object);
  const auto health=info?r32(m,info+0x28u):0u,shield=info?r32(m,info+0x8cu):0u;
  w32(m,vitality_body+0xe4u,health);
  w32(m,vitality_body+0xe8u,shield);
  w32(m,vitality_body+0xecu,fp::sse_compared(health,services.mxcsr)>0.0f?one:0u);
  w32(m,vitality_body+0xf0u,fp::sse_compared(shield,services.mxcsr)>0.0f?one:0u);
  need(services.function_colors,"C0910",m,object);
  w32(m,saved+0x24u,r32(m,request+0xa8u));
  if(r16(m,object_body(m,object)+0x12au)!=0xffffu)object_clear_block_12a(m,object);
  need(services.node_rebuild,"BF050",m,object);
  if(object_memory_locked(m)) {
    std::uint32_t location=request+0xbcu;
    if(!r8(m,request+0xb8u)) {
      const bool found=need(services.cluster_location,"C1540",m,object,request+0xbcu);
      w8(m,request+0xb8u,found);
      if(!found)location=0;
    }
    need(services.connect,"BA630",m,object,location);
  }
  object_notify_nodes(m,object,services.unit_nodes);
  {
    NativeCampaignScriptBank bank(m);
    native_campaign_script_tu15::object_mark_for_update(bank,object);
  }
  w8(m,saved+0xc0u,(request_flags()>>5)&1u?std::uint8_t(r8(m,saved+0xc0u)|2u):std::uint8_t(r8(m,saved+0xc0u)&0xfdu));
  need(services.activate,"BA8C0",m,object);
  need(services.type_finish,"C06C0",m,object);
  for(const auto address:object_type_callbacks(chain_type(m,object),type_slot_place)) {
    if(!address)continue;
    // C0790 (object level, every type) is this track's own native body.
    if(address==0xc0790u)object_place_variant_children(m,object,services);
    else callback(services.place,address,"place")(m,object);
  }
  if(r32(m,definition+0x50u)!=none)refuse("179610","creation effect");
  object_retire_lens_queries(m,object);
  flags=r32(m,saved+4u);
  w32(m,saved+4u,kept?flags|0x20000u:flags&0xfffdffffu);
  object_queue_definition_flagged(m,object);
  if(((r32(m,saved+4u)>>17)&1u) && !(r8(m,row+2u)&1u) && object_memory_locked(m) && !(request_flags()&2u) &&
     !((request_flags()&4u) && r16(m,saved+0x2cu)==0xffffu))
    need(services.deferred_delete,"BA570",m,object);
  return object;
}

void object_place_variant_children(M& world,std::uint32_t object,const ObjectFactoryServices& services) {
  Locals m(world,0x100);
  const auto offset=(object&0xffffu)*12u;
  const auto body=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  const auto variant_index=r8(m,body+0xb1u);
  if(variant_index==0xffu)return;
  const auto model=tag_data(m,r32(m,tag_data(m,r32(m,body))+0x38u));
  const auto variant=sx8(variant_index)*0x38u+r32(m,model+0x54u);
  if(s32(r32(m,variant+0x1cu))<=0)return;
  const auto location=m.base(),request=m.base()+0x20u;
  for(std::uint32_t n=0;s32(n)<s32(r32(m,variant+0x1cu));++n) {
    const auto entry=r32(m,variant+0x20u)+n*0x10u;
    const auto tag=r32(m,entry+0xcu);
    if(tag==none)continue;
    const auto current=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
    w32(m,location+4u,r32(m,current+0xc8u));
    w32(m,location,r32(m,current+0xc4u));
    w16(m,location+8u,r16(m,current+0xc2u));
    placement_data_new(m,request,tag,object,location);
    object_world_position(m,object,request+0x1cu,services.mxcsr);
    const auto child=object_new(m,request,services);
    if(child==none)continue;
    const auto child_body=r32(m,r32(m,r32(m,object_pool)+0x44u)+(child&0xffffu)*12u+8u);
    need(services.attach,"BAF10",m,object,r32(m,entry),child,r32(m,entry+4u));
    w32(m,child_body+4u,r32(m,child_body+4u)|0x4000000u);
  }
}
}
