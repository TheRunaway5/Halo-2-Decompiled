#pragma once
#include "halo2/native_grenade.hpp"
#include "halo2/authority_network_limits.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/unit_animation_selection.hpp"
#include <string_view>

namespace halo2::grenade_preflight {
// Borrow immutable admitted metadata. No resource read or clip decode occurs
// until every incoming key and the aggregate decoded-asset cost pass.
inline const Tag &tag(const Cache &cache,std::uint32_t id,std::string_view group) {
  const auto *value=cache.find_tag({id});
  if(!value||!value->address||value->group!=group)
    throw FormatError("Grenade resource preflight has an invalid typed tag identity");
  return *value;
}
inline Bytes bytes(const Cache &cache,const Tag &owner,std::uint64_t address,std::size_t size) {
  // Xbox reflexives can point beyond the referring tag's declared payload
  // into the admitted cache metadata allocation. Resolve that address through
  // the same cache-wide mapping as load_animation/load_initial_model_definition;
  // these addresses come from immutable local metadata, never from the cut.
  try {
    if(address>UINT32_MAX)throw FormatError("metadata address exceeds 32 bits");
    return Bytes(cache.read_tag_bytes(std::uint32_t(address),size));
  }catch(const FormatError &error) {
    throw FormatError("Grenade resource preflight metadata range: tag="+std::to_string(owner.datum)+
        " group="+owner.group+" root="+std::to_string(owner.address)+" declared_bytes="+std::to_string(owner.size)+
        " address="+std::to_string(address)+" requested_bytes="+std::to_string(size)+": "+error.what());
  }
}
inline void charge(std::uint64_t &remaining,std::uint64_t cost) {
  if(cost>remaining)throw FormatError("Grenade authority decoded asset budget exceeded");
  remaining-=cost;
}
struct Block {std::uint32_t count{};std::uint64_t address{};std::size_t stride{};};
template<class T> Block block(const Cache &cache,const Tag &owner,std::uint64_t descriptor,std::size_t stride,
    std::uint64_t &remaining,unsigned maximum=65536) {
  const auto row=bytes(cache,owner,descriptor,8);const auto count=row.u32(0);
  if(count>maximum)throw FormatError("Grenade resource preflight has an excessive authored block");
  // Two simultaneously live model-definition decodes, push-back capacity,
  // temporary offset arrays, and selector choice scratch at each leaf. Charge
  // each occurrence, including repeated/aliased immutable metadata pointers.
  charge(remaining,std::uint64_t(count)*(4*(sizeof(T)+sizeof(std::size_t))+32));
  if(count)bytes(cache,owner,row.u32(4),std::uint64_t(count)*stride);
  return {count,row.u32(4),stride};
}
inline void unit(const Cache &cache,std::uint32_t id,std::uint64_t &remaining) {
  const auto &owner=tag(cache,id,"bipd");const auto body=bytes(cache,owner,owner.address,0x3c);
  const auto &model=tag(cache,body.u32(0x38),"hlmt");const auto root=bytes(cache,model,model.address,0x78);
  const auto &render=tag(cache,root.u32(4),"mode");const auto mesh=bytes(cache,render,render.address,0x60);
  // Use the actual payload resolver for zero-sized Xbox aliases too: the
  // later SourceTagMetadata copy resolves its same-address owner this way.
  charge(remaining,std::uint64_t(cache.tag_payload({owner.datum}).size())+
      cache.tag_payload({render.datum}).size()+2*(render.name.size()+32)+4096);
  const auto variants=block<SourceInitialModelVariant>(cache,model,model.address+0x50,0x38,remaining);
  for(unsigned v=0;v<variants.count;++v) {
    const auto regions=block<SourceInitialVariantRegion>(cache,model,variants.address+v*0x38+0x14,0x14,remaining);
    for(unsigned r=0;r<regions.count;++r) {
      const auto permutations=block<SourceInitialVariantPermutation>(cache,model,regions.address+r*0x14+8,0x20,remaining);
      for(unsigned p=0;p<permutations.count;++p)
        block<SourceInitialModelState>(cache,model,permutations.address+p*0x20+0xc,0x18,remaining);
    }
  }
  const auto regions=block<SourceInitialModelRegion>(cache,model,model.address+0x70,0x10,remaining,16);
  for(unsigned r=0;r<regions.count;++r)block<std::uint8_t>(cache,model,regions.address+r*0x10+8,8,remaining);
  using Sections=std::vector<std::array<std::int16_t,6>>;
  const auto render_regions=block<Sections>(cache,render,render.address+0x1c,16,remaining,16);
  for(unsigned r=0;r<render_regions.count;++r)
    block<std::array<std::int16_t,6>>(cache,render,render_regions.address+r*16+8,16,remaining);
  block<ModelBone>(cache,render,render.address+0x48,0x60,remaining,255);
  const auto groups=block<SourceModelCompoundNode>(cache,render,render.address+0x34,12,remaining,256);
  for(unsigned lod=0;lod<6;++lod) {
    const auto selected=std::int8_t(std::to_integer<std::uint8_t>(mesh.slice(0x3c+lod,1)[0]));
    if(selected<0||unsigned(selected)>=groups.count)throw FormatError("Grenade resource preflight lost an authored LOD group");
    block<SourceModelCompoundNode>(cache,render,groups.address+unsigned(selected)*12+4,16,remaining,256);
  }
  const auto markers=block<ObjectAttachmentMarker>(cache,render,render.address+0x58,12,remaining);
  for(unsigned i=0;i<markers.count;++i) {
    const auto at=markers.address+i*12;
    if(bytes(cache,render,at,4).u32(0)==0x09000100) {
      block<ObjectAttachmentMarker>(cache,render,at+4,36,remaining);break;
    }
  }
}
inline void animation(const Cache &cache,std::uint32_t graph,std::uint32_t clip,std::uint64_t &remaining) {
  const auto &owner=tag(cache,graph,"jmad");const auto root=bytes(cache,owner,owner.address,cache.is_vista()?0xac:0xb4);
  const auto count=root.u32(0x2c),nodes=root.u32(0xc),stride=cache.is_vista()?0x60u:0x6cu,shift=cache.is_vista()?12u:0u;
  if(count>65536||clip>=count||nodes>255)throw FormatError("Grenade resource preflight has an invalid animation identity");
  const auto row=bytes(cache,owner,root.u32(0x30)+std::uint64_t(clip)*stride,stride);
  const auto frames=std::int16_t(row.u16(0x14));
  if(frames<=0||frames>10000||std::to_integer<unsigned>(row.slice(0x13,1)[0])!=nodes)
    throw FormatError("Grenade resource preflight has an invalid animation extent");
  std::uint64_t raw{};
  if(cache.is_vista())raw=row.u32(0x28);
  else {
    const auto groups=root.u32(0xac);const auto group=std::int32_t(row.u32(0x24));
    if(group<0||std::uint32_t(group)>=groups)throw FormatError("Grenade resource preflight has an invalid animation resource group");
    raw=bytes(cache,owner,root.u32(0xb0)+std::uint64_t(group)*20,20).u32(4);
  }
  const auto events=row.u32(0x4c-shift);
  // A vector grown by push_back retains less than twice its logical count.
  // Each channel has at most frames+1 distinct sparse times (or frames dense
  // keys), plus its one static key. Include transient sparse times/raw input,
  // dense poses, node/channel scaffolding, movement, events, and names.
  const std::uint64_t samples=std::uint64_t(nodes)*(unsigned(frames)+2);
  const auto cost=2*raw+2*samples*sizeof(BonePose)+6*samples*(sizeof(unsigned)+sizeof(std::array<float,4>))+
      std::uint64_t(nodes)*512+std::uint64_t(events)*2*sizeof(AnimationClip::FrameEvent)+
      std::uint64_t(frames)*48+2*(owner.name.size()+32)+4096;
  charge(remaining,cost);
}
inline std::pair<std::uint32_t,unsigned> action(const Cache &cache,std::uint32_t graph,
    std::uint32_t weapon_class,std::uint32_t weapon_type,std::uint32_t activity) {
  const auto &owner=tag(cache,graph,"jmad");const auto root=bytes(cache,owner,owner.address,0x54);
  SourceAnimationGraphMemory memory;
  if(cache.is_mcc())memory.dual_weapon_class=0x04000557u;
  memory.read=[&](auto address,auto size){return cache.read_tag_bytes(address,size);};
  weapon_class=source_animation_weapon_class(owner.address,weapon_class,weapon_type,memory);
  // Same authored overlay/ordinary lookup as Cache::load_unit_weapon_action;
  // the shared literal selectors resolve identities without decoding clips.
  for(const unsigned table:{12u,4u}) {
    std::array<std::byte,28> iterator{};
    const auto put=[&](unsigned at,std::uint32_t value){for(unsigned i=0;i<4;++i)iterator[at+i]=std::byte(value>>(8*i));};
    put(0,owner.address);put(4,0x06000086);put(8,weapon_class);put(12,weapon_type);
    std::array<std::uint32_t,3> resolved{};
    while(const auto weapon=next_source_animation_weapon(iterator,resolved,memory)) {
      const auto found=find_source_animation_record(weapon+table,activity,8,memory);if(!found)continue;
      const Bytes row(cache.read_tag_bytes(found,8));
      const auto inheritance=std::int16_t(row.u16(4)),clip=std::int16_t(row.u16(6));
      if(clip==-1)continue;
      if(clip<0)throw FormatError("Grenade resource preflight has an invalid action clip");
      auto selected=graph;
      if(inheritance!=-1) {
        if(inheritance<0||std::uint32_t(inheritance)>=root.u32(0x4c))throw FormatError("Grenade resource preflight has invalid inheritance");
        selected=bytes(cache,owner,root.u32(0x50)+std::uint64_t(inheritance)*32,32).u32(4);
      }
      return {selected,unsigned(clip)};
    }
  }
  throw FormatError("Grenade resource preflight cannot resolve the authored action");
}
inline std::uint64_t admit(const Cache &cache,const NativeGrenadeSandbox::AuthorityCheckpoint &saved) {
  if(saved.unit_bindings.size()>256||saved.weapon_clip_bindings.size()>4096||saved.selected_clip_bindings.size()>4096||
      !saved.unit_bindings.contains(saved.initial_unit))
    throw FormatError("Grenade resource preflight exceeds binding capacity");
  std::uint64_t remaining=authority_network_asset_bytes;
  for(const auto &[unit,digest]:saved.unit_bindings) {
    (void)digest;grenade_preflight::unit(cache,unit,remaining);
  }
  for(const auto &[key,digest]:saved.weapon_clip_bindings) {
    (void)digest;
    if(!saved.unit_bindings.contains(key.first))throw FormatError("Grenade resource preflight lost its action unit");
    tag(cache,key.second,"weap");const auto identifiers=cache.load_weapon_animation_identifiers(key.second);
    const auto graph=cache.load_model_animation_graph(key.first);
    const auto ordinary=action(cache,graph,identifiers[0],identifiers[1],0x0d000021);
    animation(cache,ordinary.first,ordinary.second,remaining);
    std::optional<std::pair<std::uint32_t,unsigned>> preferred;
    try{preferred=action(cache,graph,identifiers[0],identifiers[1],0x1000006c);}catch(const FormatError &){}
    if(preferred)animation(cache,preferred->first,preferred->second,remaining);
  }
  for(const auto &[key,digest]:saved.selected_clip_bindings){(void)digest;animation(cache,key.first,key.second,remaining);}
  return authority_network_asset_bytes-remaining;
}
}
