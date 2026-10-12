#include "halo2/fp_sound_submit.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> data,std::size_t p,unsigned n) {
  std::uint32_t out=0;
  for(unsigned i=0;i<n;++i)out|=std::to_integer<std::uint32_t>(data[p+i])<<(8*i);
  return out;
}
void write(std::array<std::byte,160> &raw,std::size_t p,std::uint32_t value,
           unsigned n,std::uint32_t pc,const FirstPersonRawSoundObserver &observer) {
  const auto before=word(raw,p,n);
  for(unsigned i=0;i<n;++i)raw[p+i]=std::byte(value>>(8*i));
  if(observer)observer({pc,p,n,before,value});
}
}
bool source_first_person_sound_raw_event(FirstPersonSoundState &state,
    bool storage,unsigned view,const FirstPersonHandBinding &hand,
    const FirstPersonAnimationEvent &event,std::uint16_t bsp,
    const FirstPersonSoundConstants &constants,FirstPersonRawSoundBacking &backing,
    const FirstPersonRawSoundServices &services,const FirstPersonRawSoundObserver &observer) {
  auto owners=services.owners;
  // Observe the actual producer's class read; don't perform an additional
  // metadata call or infer whether the original OR branch ran from final bits.
  const auto class_provider=owners.sound_class;
  const auto definition_provider=owners.sound_definition;
  std::array<std::byte,20> definition;
  std::array<std::byte,92> sound_class;
  bool have_definition=false,have_class=false;
  if(definition_provider)owners.sound_definition=[&](auto sound) {
    definition=definition_provider(sound);have_definition=true;return definition;
  };
  if(class_provider)owners.sound_class=[&](auto index) {
    sound_class=class_provider(index);have_class=true;return sound_class;
  };
  owners.successful_attention=[&](const SourceSoundPlayRequest &request,
                                 std::span<const std::byte,20> current_definition) {
    if(!services.successful_attention)
      throw FormatError("Original post-submit raw sound attention owner unavailable");
    services.successful_attention(request.sound,backing.request,current_definition);
  };
  owners.submit=[&](const SourceSoundPlayRequest &request) {
    if(!have_definition || !have_class)
      throw FormatError("Original FP raw sound metadata phase unavailable");
    if(request.platform_argument!=0xffffffffu || request.data_size!=40 ||
       request.flags!=((request.callbacks?0x25u:5u)|(request.owner!=0xffffffffu?8u:0u)))
      throw FormatError("Unsupported non-FP original raw sound request branch");
    auto &raw=backing.request;
    write(raw,4,request.priority,4,0x18ba81,observer);
    write(raw,0,request.flags,4,0x18ba85,observer);
    for(unsigned n=0;n<17;++n) {
      const auto value=n<12?word(backing.event.location,n*4,4):
                            word(backing.location_tail,(n-12)*4,4);
      write(raw,0xc+n*4,value,4,0x18ba97,observer);
    }
    write(raw,0x50,request.owner,4,0x18ba99,observer);
    write(raw,0x58,request.callbacks,4,0x18baa1,observer);
    write(raw,0x98,request.platform_argument,4,0x18baaa,observer);
    if((std::to_integer<unsigned>(sound_class[10])&0x80) &&
       !(std::to_integer<unsigned>(definition[1])&1))
      write(raw,0xd,word(raw,0xd,1)|8,1,0x18bac7,observer);
    for(unsigned n=0;n<10;++n)
      write(raw,0x5c+n*4,word(request.marker,n*4,4),4,0x18badc,observer);
    write(raw,0x8c,40,2,0x18bade,observer);
    if(!services.submit)throw FormatError("Original whole raw sound submit unavailable");
    return services.submit(request.sound,raw);
  };
  return source_first_person_sound_event(state,storage,view,hand,event,bsp,constants,
                                        backing.event,owners);
}
} // namespace halo2
