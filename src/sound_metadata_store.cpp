#include "halo2/sound_metadata_store.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <iterator>
#include <map>

namespace halo2 {
std::array<std::byte,20> Cache::load_source_sound_definition(std::uint32_t datum) const {
  const auto* tag = find_tag({datum}, tag_groups::sound);
  if(!tag || !tag->address)throw FormatError("Original resident sound definition unavailable");
  const auto payload=load_source_tag_metadata(tag->datum);
  std::span<const std::byte> input;
  try {input=payload.bytes_at(tag->address,20);}
  catch(const std::out_of_range &) {throw FormatError("Original sound definition escapes its owned payload");}
  std::array<std::byte,20> result;
  std::copy(input.begin(),input.end(),result.begin());
  return result;
}
std::vector<std::array<std::byte,92>> Cache::load_source_sound_classes() const {
  const auto tag=query_tag({.group=tag_groups::sound_classes, .require_address=true});
  if(!tag)throw FormatError("Original resident sound classes unavailable");
  const auto classes=select_tags({.group=tag_groups::sound_classes, .require_address=true});
  if(std::distance(classes.begin(),classes.end())!=1)
    throw FormatError("Original resident sound class selection is ambiguous");
  const auto payload=load_source_tag_metadata(tag->datum);
  std::span<const std::byte> header_bytes;
  try {header_bytes=payload.bytes_at(tag->address,8);}
  catch(const std::out_of_range &) {throw FormatError("Original sound class header escapes its owned payload");}
  const Bytes header(header_bytes);
  const auto count=header.u32(0);
  if(!count || count>0x100000)throw FormatError("Invalid original sound class extent");
  std::span<const std::byte> records;
  try {records=payload.bytes_at(header.u32(4),std::size_t(count)*92);}
  catch(const std::out_of_range &) {throw FormatError("Original sound classes escape their owned payload");}
  std::vector<std::array<std::byte,92>> result(count);
  for(std::size_t row=0;row<result.size();++row) {
    const auto input=records.subspan(row*92,92);
    std::copy(input.begin(),input.end(),result[row].begin());
  }
  return result;
}
struct SoundMetadataStore::State {
  explicit State(std::shared_ptr<const Cache> owned):cache(std::move(owned)) {
    if(!cache)throw FormatError("Original sound metadata requires an owned cache");
    classes=cache->load_source_sound_classes();
    for(const auto &tag:cache->select_tags()) {
      if(tag.group!="snd!" || !tag.address)continue;
      if(!sounds.emplace(tag.datum,cache->load_source_sound_definition(tag.datum)).second)
        throw FormatError("Duplicate original sound definition datum");
    }
  }
  std::shared_ptr<const Cache> cache;
  std::map<std::uint32_t,std::array<std::byte,20>> sounds;
  std::vector<std::array<std::byte,92>> classes;
};
SoundMetadataStore::SoundMetadataStore(std::shared_ptr<const Cache> cache)
    :state_(std::make_shared<State>(std::move(cache))) {}
SourceSoundMetadataProviders SoundMetadataStore::providers() const {
  const auto state=state_;
  return {
    [state](std::uint32_t datum) {
      const auto found=state->sounds.find(datum);
      if(found==state->sounds.end())throw FormatError("Original sound datum unavailable");
      return found->second;
    },
    [state](std::int8_t index) {
      if(index<0 || std::size_t(index)>=state->classes.size())
        throw FormatError("Original sound class index unavailable");
      return state->classes[std::size_t(index)];
    }
  };
}
std::size_t SoundMetadataStore::sound_count() const {return state_->sounds.size();}
std::size_t SoundMetadataStore::class_count() const {return state_->classes.size();}
} // namespace halo2
