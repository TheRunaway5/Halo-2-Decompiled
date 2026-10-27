#include "halo2/source_gameplay_fade_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <limits>
namespace halo2 {
namespace {
void identity(const SourceGameplayFadeIdentityTu15 &value,unsigned slot) {
  if(value.player==0xffffffffu){if(value.activation)throw FormatError("Absent fade player has an activation");return;}
  if((value.player&0xffffu)!=slot || !(value.player>>16u) || (value.player>>16u)==0xffffu)
    throw FormatError("Gameplay fade lost its full salted player owner");
}
struct Writer {
 std::vector<std::byte> bytes;
 void word(std::uint64_t value,unsigned width){for(unsigned i=0;i<width;++i)bytes.push_back(std::byte(value>>(i*8u)));}
};
struct Reader {
 std::span<const std::byte> bytes;std::size_t offset{};
 std::uint64_t word(unsigned width){if(width>bytes.size()-offset)throw FormatError("Truncated gameplay fade observations");
  std::uint64_t value{};for(unsigned i=0;i<width;++i)value|=std::uint64_t(std::to_integer<unsigned char>(bytes[offset++]))<<(i*8u);return value;}
 bool flag(){const auto v=word(1);if(v>1)throw FormatError("Noncanonical gameplay fade flag");return v!=0;}
};
template<class W>void encode_identity(W &out,const SourceGameplayFadeIdentityTu15 &id){out.word(id.player,4);out.word(id.activation,8);}
SourceGameplayFadeIdentityTu15 decode_identity(Reader &in){return {std::uint32_t(in.word(4)),in.word(8)};}
}
void validate_source_gameplay_fade_journal_tu15(const SourceGameplayFadeJournalTu15 &journal){
 const auto count=std::min<std::uint64_t>(journal.newest,source_gameplay_fade_history_tu15);
 if(journal.slots.size()!=count)throw FormatError("Gameplay fade history extent differs from its source sequence");
 if(journal.context_available) {
  if(journal.rate<1 || (journal.x87_control_word&0x3f)!=0x3f || (journal.x87_control_word&0x300)==0x100 ||
      (journal.mxcsr&0x1f80)!=0x1f80 || (journal.mxcsr&0xffff0000))throw FormatError("Gameplay fade current source context is unsupported");
 } else if(journal.engine_present||journal.rate||journal.x87_control_word||journal.mxcsr||journal.newest ||
     std::any_of(journal.current.begin(),journal.current.end(),[](const auto &id){return id.activation!=0;}))
  throw FormatError("Gameplay fade inputs have no observed source FP/clock context");
 for(unsigned i=0;i<16;++i)identity(journal.current[i],i);
 for(unsigned index=0;index<count;++index){const auto &frame=journal.slots[index];
  const auto oldest=journal.newest-count+1;
  if(frame.sequence<oldest||frame.sequence>journal.newest || (frame.sequence-1)%source_gameplay_fade_history_tu15!=index)
   throw FormatError("Gameplay fade source history is missing or reordered");
  if(frame.rate<1)throw FormatError("Gameplay fade has no actual source tick rate");
  if((frame.x87_control_word&0x3f)!=0x3f || (frame.x87_control_word&0x300)==0x100 ||
      (frame.mxcsr&0x1f80)!=0x1f80 || (frame.mxcsr&0xffff0000))throw FormatError("Gameplay fade has an unsupported FP context");
  for(unsigned i=0;i<16;++i){identity(frame.identities[i],i);
   if(frame.players[i].present!=(frame.identities[i].player!=0xffffffffu))throw FormatError("Gameplay fade player input has no matching source identity");
  }
 }
}
std::vector<std::byte> encode_source_gameplay_fade_journal_tu15(const SourceGameplayFadeJournalTu15 &journal){
 validate_source_gameplay_fade_journal_tu15(journal);Writer out;out.word(journal.newest,8);
 out.word(journal.context_available,1);out.word(journal.engine_present,1);out.word(std::uint16_t(journal.rate),2);out.word(journal.x87_control_word,2);out.word(journal.mxcsr,4);
 for(const auto &id:journal.current)encode_identity(out,id);out.word(journal.slots.size(),2);
 for(const auto &f:journal.slots){out.word(f.sequence,8);out.word(f.tick,4);out.word(f.round_start_tick,4);out.word(f.engine_present,1);out.word(f.raw_mode,1);
  out.word(std::uint16_t(f.phase),2);out.word(std::uint16_t(f.rate),2);out.word(std::uint16_t(f.phase_counter_before),2);
  out.word(std::uint32_t(f.pending_phase),4);out.word(std::uint32_t(f.life_limit),4);out.word(f.x87_control_word,2);out.word(f.mxcsr,4);
  for(unsigned i=0;i<16;++i){encode_identity(out,f.identities[i]);const auto &p=f.players[i];out.word(p.present,1);out.word(p.current_unit,4);
   out.word(p.flags,2);out.word(std::uint8_t(p.membership),1);out.word(std::uint16_t(p.lives),2);out.word(std::uint32_t(p.respawn_seconds),4);}
 }
 return std::move(out.bytes);
}
SourceGameplayFadeJournalTu15 decode_source_gameplay_fade_journal_tu15(std::span<const std::byte> bytes){
 Reader in{bytes};SourceGameplayFadeJournalTu15 journal;journal.newest=in.word(8);journal.context_available=in.flag();journal.engine_present=in.flag();
 journal.rate=std::bit_cast<std::int16_t>(std::uint16_t(in.word(2)));journal.x87_control_word=in.word(2);journal.mxcsr=in.word(4);
 for(auto &id:journal.current)id=decode_identity(in);
 const auto count=in.word(2);if(count>source_gameplay_fade_history_tu15)throw FormatError("Gameplay fade history exceeds its bounded source window");journal.slots.resize(count);
 for(auto &f:journal.slots){f.sequence=in.word(8);f.tick=in.word(4);f.round_start_tick=in.word(4);f.engine_present=in.flag();f.raw_mode=in.word(1);
  f.phase=std::bit_cast<std::int16_t>(std::uint16_t(in.word(2)));f.rate=std::bit_cast<std::int16_t>(std::uint16_t(in.word(2)));f.phase_counter_before=std::bit_cast<std::int16_t>(std::uint16_t(in.word(2)));
  f.pending_phase=std::bit_cast<std::int32_t>(std::uint32_t(in.word(4)));f.life_limit=std::bit_cast<std::int32_t>(std::uint32_t(in.word(4)));f.x87_control_word=in.word(2);f.mxcsr=in.word(4);
  for(unsigned i=0;i<16;++i){f.identities[i]=decode_identity(in);auto &p=f.players[i];p.present=in.flag();p.current_unit=in.word(4);p.flags=in.word(2);
   p.membership=std::bit_cast<std::int8_t>(std::uint8_t(in.word(1)));p.lives=std::bit_cast<std::int16_t>(std::uint16_t(in.word(2)));p.respawn_seconds=std::bit_cast<std::int32_t>(std::uint32_t(in.word(4)));}
 }
 if(in.offset!=bytes.size())throw FormatError("Gameplay fade observation has trailing bytes");validate_source_gameplay_fade_journal_tu15(journal);return journal;
}
void SourceGameplayFadePresentationTu15::activate(const std::array<SourceGameplayFadeIdentityTu15,16> &ids,std::span<const std::uint32_t> local){
 for(unsigned view=0;view<local.size();++view){const auto player=local[view],slot=player&0xffffu;if(slot>=16)throw FormatError("Gameplay fade local player has no admitted source slot");
  const auto &id=ids[slot];if(id.player!=player){if(id.player!=0xffffffffu)throw FormatError("Gameplay fade local player identity changed");continue;}
  if(applied_[view].player==player&&id.activation<applied_[view].activation)throw FormatError("Gameplay fade activation regressed");
  if(id.activation && applied_[view]!=id){source_gameplay_fade_activate_tu15(state_,view);applied_[view]=id;}
 }
}
void SourceGameplayFadePresentationTu15::consume(const SourceGameplayFadeJournalTu15 &journal,std::span<const std::uint32_t> local){
 validate_source_gameplay_fade_journal_tu15(journal);if(local.empty()||local.size()>4)throw FormatError("Gameplay fade requires1..4 actual local players");
 for(unsigned i=0;i<local.size();++i) {
  if((local[i]&0xffffu)>=16 || !(local[i]>>16u) || (local[i]>>16u)==0xffffu)throw FormatError("Gameplay fade local view has no full salted source player");
  for(unsigned j=0;j<i;++j)if(local[i]==local[j])throw FormatError("Gameplay fade repeats a local view owner");
 }
 if(journal.newest<sequence_)return; // An older authenticated snapshot cannot rewind presentation.
 const auto oldest=journal.newest-journal.slots.size()+1;
 if(sequence_<journal.newest && sequence_<oldest-1) {
  // A genuinely new local player may join an older world. Its full salted
  // identity must be absent at the start of the retained window, and its
  // real init/bind activation must be present now. An existing view that
  // merely missed too many source inputs cannot reset itself this way.
  if(sequence_ || journal.slots.empty())throw FormatError("Gameplay fade missed its actual source history window");
  const auto &first=journal.slots[(oldest-1)%source_gameplay_fade_history_tu15];
  for(const auto player:local){const auto slot=player&0xffffu;
    if(first.identities[slot].player==player || journal.current[slot].player!=player || !journal.current[slot].activation)
      throw FormatError("Gameplay fade lacks the actual source history for an existing local view");}
  sequence_=oldest-1;
 }
 while(sequence_<journal.newest){const auto seq=sequence_+1;const auto &f=journal.slots[(seq-1)%source_gameplay_fade_history_tu15];activate(f.identities,local);
  SourceGameplayFadeTickTu15 tick;tick.engine_present=f.engine_present;tick.raw_mode=f.raw_mode;tick.phase=f.phase;tick.rate=f.rate;
  tick.pending_phase=f.pending_phase;tick.life_limit=f.life_limit;tick.tick=f.tick;tick.round_start_tick=f.round_start_tick;
  for(unsigned view=0;view<local.size();++view){const auto slot=local[view]&0xffffu;if(slot>=16)throw FormatError("Gameplay fade local slot outside original pool");
   if(f.identities[slot].player==local[view])tick.users[view]=f.players[slot];}
  state_.phase_counter=f.phase_counter_before;rate_=f.rate;fp_={f.x87_control_word,f.mxcsr};
  context_available_=true;engine_present_=f.engine_present;source_gameplay_fade_tick_tu15(state_,tick,fp_);sequence_=seq;
 }
 activate(journal.current,local);
 if(journal.context_available){context_available_=true;engine_present_=journal.engine_present;rate_=journal.rate;fp_={journal.x87_control_word,journal.mxcsr};}
}
std::optional<float> SourceGameplayFadePresentationTu15::render(unsigned view,std::int32_t pending){
 if(!context_available_)return {};
 return source_gameplay_fade_render_tu15(state_,{engine_present_,view,pending,rate_},fp_);
}
}
