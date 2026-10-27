#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>

namespace halo2 {
namespace {
void require(bool yes,const char *why){if(!yes)throw FormatError(why);}
template<std::size_t N>void copy(std::array<std::byte,N> &out,std::span<const std::byte> in) {
  require(in.size()==N,"Immutable sound metadata row has a different stride");
  std::ranges::copy(in,out.begin());
}
std::int16_t short_at(const Bytes &bytes,unsigned offset){return std::bit_cast<std::int16_t>(bytes.u16(offset));}
}
struct MccSoundMetadataPublication::Impl {
  std::shared_ptr<const Cache> cache;
  MccSoundRuntimePlan plan;
  std::array<std::byte,8> classes{};
  std::map<std::uint32_t,std::array<std::byte,20>> sounds;
  std::span<const std::byte> read(std::uint32_t at,std::size_t size)const {
    require(at>=plan.address && std::uint64_t(at)-plan.address<=plan.bytes.size() &&
        size<=plan.bytes.size()-(std::uint64_t(at)-plan.address),
        "Immutable sound metadata read escapes its actual converted allocation");
    return std::span<const std::byte>(plan.bytes).subspan(at-plan.address,size);
  }
  std::span<const std::byte> row(TagBlock block,unsigned stride,unsigned index)const {
    require(block.count>=0 && std::uint32_t(block.count)<=0x100000u && index<std::uint32_t(block.count),
        "Immutable sound metadata index escapes its converted table");
    const auto extent=std::uint64_t(std::uint32_t(block.count))*stride;
    require(extent<=UINT32_MAX && std::uint64_t(block.address)+extent<=0x100000000ull,
        "Immutable sound metadata table wraps its logical namespace");
    (void)read(block.address,std::size_t(extent));
    return read(block.address+index*stride,stride);
  }
};
MccSoundMetadataPublication::MccSoundMetadataPublication(std::shared_ptr<const Impl> impl):impl_(std::move(impl)){}
MccSoundMetadataPublication MccSoundMetadataPublication::create(std::shared_ptr<const Cache> cache) {
  require(cache && cache->is_mcc(),"Immutable converted sound publication requires its strong MCC Cache owner");
  auto out=std::make_shared<Impl>();out->cache=std::move(cache);
  bool captured{};
  // These are serialized logical tokens, as in PhysicsLiveWorldTu15. No
  // runtime memory, fake globals, device state or World epoch is constructed.
  const auto delivery=MccRuntimeCacheDelivery::plan(*out->cache,0x90000000u,0x60000000u,
      [&](MccSoundRuntimePlan &&plan){require(!captured,"Duplicate immutable sound conversion");out->plan=std::move(plan);captured=true;});
  require(captured,"Complete immutable sound conversion did not publish its sound plan");
  const auto patch=[&](TagHandle owner,unsigned size)->const MccSoundRuntimePatch& {
    const auto found=std::ranges::find_if(out->plan.patches,[&](const auto &p){return p.owner==owner && p.after.size()==size && p.before.size()==size;});
    require(found!=out->plan.patches.end(),"Immutable sound conversion lost its actual projected root");
    return *found;
  };
  const auto *globals=out->cache->find_tag(out->cache->globals_tag(),tag_groups::globals);
  require(globals,"Immutable sound conversion lost its globals owner");
  const Bytes root(out->cache->read_tag_bytes(globals->address+0xc0,8));
  require(root.u32(0)==1,"Immutable sound conversion has no singular SoundGlobals owner");
  const auto projected=project_mcc_sound_globals(out->cache->read_tag_bytes(root.u32(4),0x2c));
  const auto legacy=TagHandle{Bytes(projected).u32(4)};
  require(out->cache->find_tag(legacy,TagGroup{"sncl"}),"Immutable sound conversion lost its Legacy class owner");
  copy(out->classes,patch(legacy,8).after);
  const auto &index=delivery.index();const Bytes directory(index.region.bytes);
  const auto table=index.table-index.region.address;
  for(const auto &sound:out->plan.sounds) {
    const auto *raw=out->cache->find_tag(sound.sound,TagGroup{"snd!"});
    const auto &p=patch(sound.sound,20);
    require(raw && raw->address==sound.source_root && p.source_address==sound.source_root &&
        Bytes(p.after).u16(0)==sound.native_flags && sound.sound.index()<index.count,
        "Immutable sound conversion has a foreign source root or flags");
    const auto at=table+sound.sound.index()*16u;
    require(directory.u32(at)==TagGroup{"snd!"}.value && directory.u32(at+4)==sound.sound.value &&
        directory.u32(at+8)==sound.native_root && directory.u32(at+12)>=20,
        "Immutable sound conversion lost its canonical full-salt directory identity");
    std::array<std::byte,20> bytes;copy(bytes,p.after);
    require(out->sounds.emplace(sound.sound.value,bytes).second,"Duplicate immutable sound identity");
  }
  require(!out->sounds.empty(),"Immutable sound conversion contains no native sound roots");
  return MccSoundMetadataPublication(std::move(out));
}
void MccSoundMetadataPublication::require_owner(const Cache &cache)const {
  require(impl_ && impl_->cache.get()==&cache,"Immutable sound metadata belongs to another Cache");
}
void MccSoundMetadataPublication::require_owner(const std::shared_ptr<const Cache> &cache)const {
  require(bool(cache),"Immutable sound metadata requires a strong Cache owner");require_owner(*cache);
  require(!cache.owner_before(impl_->cache) && !impl_->cache.owner_before(cache),
      "Immutable sound metadata belongs to another strong Cache ownership group");
}
bool MccSoundMetadataPublication::same_owner(const MccSoundMetadataPublication &other)const noexcept{return impl_==other.impl_;}
SoundVariationDefinition MccSoundMetadataPublication::load(const Cache &cache,TagHandle sound,unsigned pitch)const {
  require_owner(cache);const auto &owner=*impl_;
  const auto found=owner.sounds.find(sound.value);
  require(found!=owner.sounds.end(),"Immutable sound metadata has no exact full-salt sound owner");
  SoundVariationDefinition out;out.datum=sound.value;out.sound=found->second;
  const Bytes snd(out.sound);const auto cls=std::bit_cast<std::int8_t>(out.sound[2]);
  const auto ranges=std::to_integer<unsigned>(out.sound[10]);
  require(cls>=0 && pitch<ranges,"Immutable sound class or pitch range is invalid");
  out.sample_rate_index=std::to_integer<unsigned>(out.sound[3]);
  const Bytes classes(owner.classes);copy(out.sound_class,owner.row({std::int32_t(classes.u32(0)),classes.u32(4)},92,unsigned(cls)));
  const auto playback=short_at(snd,6),first_range=short_at(snd,8);
  const auto scale=std::bit_cast<std::int8_t>(out.sound[11]);
  require(playback>=0 && first_range>=0 && scale>=0,"Immutable sound metadata has a negative authored index");
  copy(out.playback,owner.row(owner.plan.blocks[0],56,unsigned(playback)));
  copy(out.scale,owner.row(owner.plan.blocks[1],20,unsigned(scale)));
  require(std::uint64_t(unsigned(first_range))+ranges<=std::uint32_t(owner.plan.blocks[4].count),
      "Immutable sound pitch ranges escape the converted table");
  for(unsigned n=0;n<ranges;++n) {
    const Bytes range(owner.row(owner.plan.blocks[4],12,unsigned(first_range)+n));
    const auto interval=short_at(range,2);require(interval>=0,"Immutable sound interval index is negative");
    auto &target=out.pitch_ranges.emplace_back();copy(target.interval,owner.row(owner.plan.blocks[3],10,unsigned(interval)));
    target.permutations=short_at(range,10);
  }
  copy(out.range,owner.row(owner.plan.blocks[4],12,unsigned(first_range)+pitch));
  const Bytes range(out.range);const auto first=short_at(range,8),count=short_at(range,10);
  require(first>=0 && count>=1 && count<=32,"Unrecovered immutable sound permutation count");
  require(std::uint64_t(unsigned(first))+unsigned(count)<=std::uint32_t(owner.plan.blocks[5].count),
      "Immutable sound permutations escape the converted table");
  out.permutations.resize(unsigned(count));
  for(unsigned n=0;n<unsigned(count);++n)copy(out.permutations[n],owner.row(owner.plan.blocks[5],16,unsigned(first)+n));
  if(count>1 && !(Bytes(out.sound_class).u16(10)&0x20u)) {
    const auto size=unsigned(count+7)/8,offset=unsigned(range.u16(6))>>3;
    require(owner.plan.blocks[7].count>=0 && std::uint64_t(offset)+size<=std::uint32_t(owner.plan.blocks[7].count),
        "Immutable sound used flags escape the converted byte bank");
    const auto used=owner.read(owner.plan.blocks[7].address+offset,size);out.used.assign(used.begin(),used.end());
  }
  return out;
}
}
