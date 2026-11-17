// Native owners of TU1.5 object_new (B9B70): the object memory pool and the
// datum/body allocators built on it. Stores follow the original order and
// widths; reads have no side effects. See docs/NATIVE_OBJECT_FACTORY.md.
#include "halo2/native_object_factory_tu15.hpp"
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include "halo2/native_tu15_lens_query_slots.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include <array>
#include <span>

namespace halo2::native_object_factory_tu15 {
namespace {
using namespace detail;
constexpr std::uint32_t kernel_tick=0x41b774u;
constexpr std::uint32_t head_tag=0x68656164u; // 'head'

bool debug_headers(const M& m,std::uint32_t pool){return r8(m,pool+0x3fu)!=0;}
// 1912E0/1913F0/191350's debug record before a block (skipped at address 0).
void debug_header(M& m,std::uint32_t header,std::uint32_t tag_b,std::uint32_t tag_a) {
  if(!header)return;
  w32(m,header,head_tag);w32(m,header+4u,tag_b);w32(m,header+8u,tag_a);
  w32(m,header+0xcu,r32(m,r32(m,kernel_tick)));   // 33B590
}
// 1912E0: link a new block at `start` between `previous` and `next`.
std::uint32_t link_between(M& m,std::uint32_t pool,std::uint32_t start,std::uint32_t total,std::uint32_t previous,
    std::uint32_t next,std::uint32_t tag_b,std::uint32_t tag_a) {
  auto block=start;
  if(debug_headers(m,pool)){block+=0x10u;debug_header(m,block-0x10u,tag_b,tag_a);}
  w32(m,block,total);w32(m,block+8u,next);w32(m,block+0xcu,previous);
  if(!previous)w32(m,pool+0x34u,block);else w32(m,previous+8u,block);
  if(!next)w32(m,pool+0x38u,block);else w32(m,next+0xcu,block);
  return block;
}
std::uint32_t block_start(const M& m,std::uint32_t pool,std::uint32_t block) {
  return debug_headers(m,pool)?block-0x10u:block;
}
// 191480: the first gap between consecutive blocks that holds `total`.
std::uint32_t allocate_in_gap(M& m,std::uint32_t pool,std::uint32_t total,std::uint32_t tag_b,std::uint32_t tag_a) {
  for(auto current=r32(m,pool+0x34u);current;) {
    const auto next=r32(m,current+8u);
    if(!next)return 0;
    const auto end=r32(m,current)+block_start(m,pool,current);
    if(end+total<=block_start(m,pool,next))
      return link_between(m,pool,end,total,current,next,tag_b,tag_a);
    current=next;
  }
  return 0;
}
// 1913F0: a block at the pool base, before the first block.
std::uint32_t allocate_front(M& m,std::uint32_t pool,std::uint32_t total,std::uint32_t tag_b,std::uint32_t tag_a) {
  const auto first=r32(m,pool+0x34u);
  const auto limit=first?block_start(m,pool,first):r32(m,pool+0x2cu)+r32(m,pool+0x28u);
  auto block=r32(m,pool+0x28u);
  if(total+block>limit)return 0;
  if(debug_headers(m,pool)){block+=0x10u;debug_header(m,block-0x10u,tag_b,tag_a);}
  w32(m,block,total);w32(m,block+8u,first);w32(m,block+0xcu,0);
  w32(m,pool+0x34u,block);
  if(!first)w32(m,pool+0x38u,block);else w32(m,first+0xcu,block);
  return block;
}
// 191350: a block after the last block.
std::uint32_t allocate_back(M& m,std::uint32_t pool,std::uint32_t total,std::uint32_t tag_b,std::uint32_t tag_a) {
  const auto last=r32(m,pool+0x38u);
  auto block=last?r32(m,last)+block_start(m,pool,last):r32(m,pool+0x28u);
  if(total+block>r32(m,pool+0x2cu)+r32(m,pool+0x28u))return 0;
  if(debug_headers(m,pool)){block+=0x10u;debug_header(m,block-0x10u,tag_b,tag_a);}
  w32(m,block,total);w32(m,block+8u,0);w32(m,block+0xcu,last);
  if(!last)w32(m,pool+0x34u,block);else w32(m,last+8u,block);
  w32(m,pool+0x38u,block);
  return block;
}
std::uint32_t rounded_total(const M& m,std::uint32_t pool,std::uint32_t size) {
  auto total=(debug_headers(m,pool)?0x20u:0x10u)+size;
  if(total&0xfu)total=(total|0xfu)+1u;
  return total;
}
// 1910A0 without the payload-slot store: returns the block (0 on refusal)
// after publishing its owner and the pool's free bytes.
std::uint32_t allocate_block(M& m,std::uint32_t pool,std::uint32_t owner_slot,std::uint32_t size,
    std::uint32_t tag_a,std::uint32_t tag_b) {
  const auto total=rounded_total(m,pool,size);
  std::uint32_t block=0;
  if(r8(m,pool+0x3du)) {
    block=allocate_in_gap(m,pool,total,tag_b,tag_a);
    if(!block)block=allocate_front(m,pool,total,tag_b,tag_a);
  }
  if(!block)block=allocate_back(m,pool,total,tag_b,tag_a);
  if(!block)return 0;
  w32(m,block+4u,r8(m,pool+0x3cu)?0u:owner_slot);
  w32(m,pool+0x30u,r32(m,pool+0x30u)-r32(m,block));
  return block;
}
// 191220: unlink the block whose payload `slot` holds and return its bytes.
void unlink(M& m,std::uint32_t pool,std::uint32_t slot) {
  const auto block=r32(m,slot)-0x10u;
  w32(m,pool+0x30u,r32(m,pool+0x30u)+r32(m,block));
  const auto previous=r32(m,block+0xcu);
  if(previous)w32(m,previous+8u,r32(m,block+8u));else w32(m,pool+0x34u,r32(m,block+8u));
  const auto next=r32(m,block+8u);
  if(next)w32(m,next+0xcu,r32(m,block+0xcu));else w32(m,pool+0x38u,r32(m,block+0xcu));
}
// One ordered store of a Havok record initializer.
struct Store {std::uint8_t offset,width;std::uint32_t value;};
void apply(M& m,std::uint32_t record,std::span<const Store> stores) {
  for(const auto& s:stores) {
    const auto at=record+s.offset;
    if(s.width==4)w32(m,at,s.value);else if(s.width==2)w16(m,at,std::uint16_t(s.value));else w8(m,at,std::uint8_t(s.value));
  }
}
constexpr std::uint32_t F=0xffffffffu;
// 1D3180's own stores (each link's +4/+6 halves are written before and
// after +0), followed by the +60..+67 flag bytes; 1D3270 then runs.
constexpr Store initialize_stores[]={
  {4,2,F},{6,2,F},{0,4,F},{4,2,F},{6,2,F},{0x1c,4,0},{0x18,4,one},{8,4,F},{0xe,2,F},{0xc,1,F},{0xd,1,F},
  {0x10,1,0},{0x11,1,0},{0x12,2,0},{0x14,2,0},{0x16,2,0},
  {0x24,2,F},{0x26,2,F},{0x20,4,F},{0x24,2,F},{0x26,2,F},{0x30,1,0},{0x31,1,0},{0x32,2,0},{0x34,2,0},{0x36,2,0},
  {0x3c,4,0},{0x38,4,one},{0x28,4,F},{0x2e,2,F},{0x2c,1,F},{0x2d,1,F},
  {0x44,2,F},{0x46,2,F},{0x40,4,F},{0x44,2,F},{0x46,2,F},{0x50,1,0},{0x51,1,0},{0x52,2,0},{0x54,2,0},{0x56,2,0},
  {0x5c,4,0},{0x58,4,one},{0x48,4,F},{0x4e,2,F},{0x4c,1,F},{0x4d,1,F},
  {0x60,1,0},{0x61,1,0},{0x62,1,0},{0x63,1,0},{0x64,1,0},{0x65,1,0},{0x66,1,0},{0x67,1,0},
};
// 1D3270's ordered stores.
constexpr Store reset_stores[]={
  {0,4,F},{4,2,F},{6,2,F},{0x1c,4,0},{0x10,1,0},{0x11,1,0},{0x12,2,0},{0x18,4,one},{0x14,2,0},{0x16,2,0},
  {8,4,F},{0xe,2,F},{0xc,1,F},{0xd,1,F},
  {0x20,4,F},{0x24,2,F},{0x26,2,F},{0x3c,4,0},{0x30,1,0},{0x31,1,0},{0x32,2,0},{0x38,4,one},{0x34,2,0},{0x36,2,0},
  {0x28,4,F},{0x2e,2,F},{0x2c,1,F},{0x2d,1,F},
  {0x40,4,F},{0x44,2,F},{0x46,2,F},{0x5c,4,0},{0x50,1,0},{0x51,1,0},{0x52,2,0},{0x58,4,one},{0x54,2,0},{0x56,2,0},
  {0x48,4,F},{0x4e,2,F},{0x4c,1,F},{0x4d,1,F},
  {0x6c,2,0},{0x6e,2,0},{0x68,4,F},{0x65,1,0},{0x64,1,0},{0x67,1,0},{0x61,1,0},{0x60,1,0},{0x63,1,0},
  {0x70,4,F},{0x7c,4,F},{0x74,4,F},{0x78,4,F},{0x80,4,0},
};
}

bool memory_pool_allocate(M& m,std::uint32_t pool,std::uint32_t slot,std::uint32_t size,std::uint32_t tag_a,std::uint32_t tag_b) {
  const auto block=allocate_block(m,pool,slot,size,tag_a,tag_b);
  if(!block)return false;
  w32(m,slot,block+0x10u);
  return true;
}

bool memory_pool_resize(M& m,std::uint32_t pool,std::uint32_t slot,std::uint32_t size,std::uint32_t tag_a,
    std::uint32_t tag_b,std::uint32_t scratch_slot) {
  const auto payload=r32(m,slot);
  const auto block=payload-0x10u;
  const auto total=rounded_total(m,pool,size);
  auto limit=r32(m,block+8u);
  if(!limit)limit=r32(m,pool+0x2cu)+r32(m,pool+0x28u);
  if(total+block<=limit) {
    w32(m,pool+0x30u,r32(m,pool+0x30u)+(r32(m,block)-total));
    w32(m,block,total);
    if(debug_headers(m,pool)) {
      const auto header=block-0x10u;
      if(header){w32(m,header+4u,tag_b);w32(m,header+8u,tag_a);w32(m,header+0xcu,r32(m,r32(m,kernel_tick)));}
    }
    return true;
  }
  // Relocate: the nested 1910A0 owns the caller's temporary slot until the
  // copy completes (its +4 store), then this routine retargets `slot`.
  const auto moved=allocate_block(m,pool,scratch_slot,size,tag_a,tag_b);
  if(!moved)return false;
  const auto destination=moved+0x10u;
  const auto bytes=r32(m,block)-(debug_headers(m,pool)?0x20u:0x10u);
  for(std::uint32_t n=0;n<bytes>>2;++n)w32(m,destination+n*4u,r32(m,payload+n*4u));
  for(std::uint32_t n=0;n<(bytes&3u);++n)w8(m,destination+(bytes&~3u)+n,r8(m,payload+(bytes&~3u)+n));
  unlink(m,pool,slot);
  w32(m,destination-0xcu,r8(m,pool+0x3cu)?0u:slot);
  w32(m,slot,destination);
  return true;
}

std::uint32_t object_allocate(M& m,std::int16_t size) {
  const auto pool=r32(m,object_pool);
  const auto datum=native_tu15_data_array_allocate(m,pool);
  if(datum==none)return none;
  const auto row=r32(m,pool+0x44u)+(datum&0xffffu)*12u;
  const auto bytes=std::uint32_t(std::int32_t(size));
  if(!memory_pool_allocate(m,r32(m,object_memory),row+8u,bytes,0,0)) {
    native_tu15_data_array_delete(m,r32(m,object_pool),datum);
    return none;
  }
  w16(m,row+6u,std::uint16_t(size));
  zero_fill(m,r32(m,row+8u),bytes);
  return datum;
}

bool object_block_allocate(M& m,std::uint32_t object,std::int16_t header,std::int16_t bytes,std::uint8_t alignment,
    std::uint32_t scratch_slot) {
  const auto offset=(object&0xffffu)*12u;
  const auto row=r32(m,r32(m,object_pool)+0x44u)+offset;
  if(!bytes) {
    const auto at=r32(m,row+8u)+std::uint32_t(std::int32_t(header));
    w16(m,at+2u,0xffffu);w16(m,at,0);
    return true;
  }
  const auto mask=(1u<<(alignment&31u))-1u;
  const auto grow=std::uint32_t(std::int32_t(bytes))+mask;
  const auto size=std::uint32_t(std::int32_t(std::int16_t(r16(m,row+6u))))+grow;
  if(!memory_pool_resize(m,r32(m,object_memory),row+8u,size,0,0,scratch_slot))return false;
  const auto old=r16(m,row+6u);
  w16(m,row+6u,std::uint16_t(old+grow));
  const auto body=r32(m,r32(m,r32(m,object_pool)+0x44u)+offset+8u);
  const auto at=body+std::uint32_t(std::int32_t(header));
  w16(m,at+2u,std::uint16_t((mask+old)&~mask));
  w16(m,at,std::uint16_t(bytes));
  zero_fill(m,r32(m,row+8u)+std::uint32_t(std::int32_t(std::int16_t(old))),grow);
  return true;
}

bool object_model_tag_pair(M& m,std::uint32_t object,std::uint32_t out_first,std::uint32_t out_second) {
  const auto definition=tag_data(m,r32(m,object_body(m,object)));
  const auto model_tag=r32(m,definition+0x38u);
  if(model_tag==none)return false;
  const auto model=tag_data(m,model_tag);
  const auto first=r32(m,model+4u);
  if(first==none || r32(m,model+0xcu)==none)return false;
  const auto data=tag_data(m,first);
  const bool populated=s32(r32(m,data+0x74u))>0 && s32(r32(m,data+0x7cu))>0;
  w32(m,out_first,first);
  w32(m,out_second,r32(m,model+0xcu));
  return populated;
}

std::uint32_t object_damage_info(const M& m,std::uint32_t object) {
  if(object==none)return 0;
  const auto model_tag=r32(m,tag_data(m,r32(m,object_body(m,object)))+0x38u);
  if(model_tag==none)return 0;
  const auto model=tag_data(m,model_tag);
  return s32(r32(m,model+0x60u))>0?r32(m,model+0x64u):0u;
}

std::uint32_t object_root_header_bit6(const M& m,std::uint32_t object) {
  auto root=none;
  for(auto current=object;current!=none;current=r32(m,object_body(m,current)+0x14u))root=current;
  return (r8(m,r32(m,r32(m,object_pool)+0x44u)+(root&0xffffu)*12u+2u)>>6)&1u;
}

void object_clear_block_12a(M& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  const auto block=body+std::uint32_t(std::int32_t(std::int16_t(r16(m,body+0x12au))))+0x64u;
  w8(m,block+1u,0);w8(m,block,0);w8(m,block+3u,0);
  NativeCampaignScriptBank bank(m);
  native_campaign_script_tu15::object_mark_for_update(bank,object);
}

void object_queue_definition_flagged(M& m,std::uint32_t object) {
  const auto body=object_body(m,object);
  if(!((r8(m,tag_data(m,r32(m,body))+2u)>>6)&1u))return;
  const auto queue=r32(m,0x51daf0u);
  if(s32(r32(m,queue+0x1d80u))>=0x20)return;
  w8(m,body+0xc0u,std::uint8_t(r8(m,body+0xc0u)|8u));
  const auto count=r32(m,queue+0x1d80u);
  w32(m,queue+count*4u+0x1d00u,object);
  w32(m,queue+0x1d80u,r32(m,queue+0x1d80u)+1u);
}

bool object_definition_physics_capacity(const M& m,std::uint32_t definition) {
  const auto type=r8(m,tag_data(m,definition));
  if(!((1u<<(type&31u))&0x1883u))return true;
  if(!r8(m,0x48b870u))return s32(r32(m,r32(m,0x52be44u)))<0x200;
  const auto physics=r32(m,0x52be5cu);
  return s32(r32(m,physics+0x3cu))<s32(r32(m,physics+0x20u));
}

std::uint32_t object_memory_locked(const M& m) {
  const auto lock=r32(m,0x4eabc4u);
  return lock && r8(m,lock)?1u:0u;
}

void havok_record_initialize(M& m,std::uint32_t record) {
  apply(m,record,initialize_stores);
  havok_record_reset(m,record);
}
void havok_record_reset(M& m,std::uint32_t record){apply(m,record,reset_stores);}
void havok_record_clear_links(M& m,std::uint32_t record) {
  for(std::uint32_t link=0x40u;;link-=0x20u) {
    w32(m,record+link,none);w16(m,record+link+4u,0xffffu);w16(m,record+link+6u,0xffffu);
    w32(m,record+link+8u,none);w8(m,record+link+0x10u,0);
    if(!link)break;
  }
}

void object_free(M& m,std::uint32_t object) {
  const auto row=r32(m,r32(m,object_pool)+0x44u)+(object&0xffffu)*12u;
  const auto body=r32(m,row+8u);
  w8(m,row+2u,0);
  if(body) {
    const auto pool=r32(m,object_memory);
    w32(m,pool+0x30u,r32(m,pool+0x30u)+r32(m,body-0x10u));
    const auto previous=r32(m,body-4u),next=r32(m,body-8u);
    if(previous)w32(m,previous+8u,next);else w32(m,pool+0x34u,next);
    const auto following=r32(m,body-8u);
    if(following)w32(m,following+0xcu,r32(m,body-4u));else w32(m,pool+0x38u,r32(m,body-4u));
    w32(m,row+8u,0);
  }
  native_tu15_data_array_delete(m,r32(m,object_pool),object);
}

// 0x474B90 type definitions' group tags (+4), TU1.5 .data constants.
constexpr std::array<std::uint32_t,13> type_groups{0x62697064u,0x76656869u,0x77656170u,0x65716970u,0x67617262u,
  0x70726f6au,0x7363656eu,0x6d616368u,0x6374726cu,0x6c696669u,0x73736365u,0x626c6f63u,0x63726561u};

std::uint32_t definition_object_type(const M& m,std::uint32_t tag) {
  const auto index=std::uint32_t(std::int32_t(std::int16_t(tag)));
  const auto group=r32(m,index*16u+r32(m,tag_instances));
  for(std::uint32_t type=0;type<type_groups.size();++type)if(type_groups[type]==group)return type;
  return none;
}

std::uint32_t model_variant_index(const M& m,std::uint32_t model_tag,std::uint32_t name) {
  if(model_tag==none)return none;
  const auto model=tag_data(m,model_tag);
  const auto count=s32(r32(m,model+0x50u));
  if(!name)return count>0?0u:none;
  const auto rows=r32(m,model+0x54u);
  for(std::int32_t n=0;n<count;++n)if(r32(m,rows+std::uint32_t(n)*0x38u)==name)return std::uint32_t(n);
  return none;
}

void object_set_variant(M& m,std::uint32_t object,std::uint32_t name) {
  const auto body=object_body(m,object);
  const auto definition=tag_data(m,r32(m,body));
  const auto selected=name?name:r32(m,definition+0x30u);
  w8(m,body+0xb1u,std::uint8_t(model_variant_index(m,r32(m,definition+0x38u),selected)));
}

std::uint32_t object_node_matrices(const M& m,std::uint32_t object,std::uint32_t& count) {
  const auto body=object_body(m,object);
  const auto bytes=std::uint32_t(std::int32_t(std::int16_t(r16(m,body+0x114u))));
  count=bytes/0x34u;   // the original's MUL 4EC4EC4F/SHR 36: exact unsigned division
  return body+std::uint32_t(std::int32_t(std::int16_t(r16(m,body+0x116u))));
}

void object_retire_lens_queries(M& m,std::uint32_t object) {
  if(object==none)return;
  const auto owner=object&0xffffu;
  for(std::uint32_t slot=0;s32(slot)<s32(r32(m,0x531fbcu));++slot) {
    const auto flags=r32(m,0x52dfdcu+slot*0x20u);
    if((flags&0x10u) && ((flags>>6)&0xfffu)==owner)native_tu15_retire_lens_query_slot(m,slot);
  }
}
}
