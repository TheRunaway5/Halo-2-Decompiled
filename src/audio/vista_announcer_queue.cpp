#include "halo2/vista_announcer_queue.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::int16_t low_signed16(std::uint32_t value) {
  return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
}
void vista_announcer_queue_initialize(VistaAnnouncerQueueState& state,
                                     std::int16_t tick_rate) {
  state={};
  state.count=1;
  state.slots[0].sound=vista_announcer_none;
  state.slots[0].delay=low_signed16(std::uint32_t(std::int32_t(tick_rate)*2));
}
void vista_announcer_queue_enqueue(VistaAnnouncerQueueState& state,
                                  std::uint32_t sound,std::int32_t delay_ticks) {
  if(state.count>4)return;
  auto& slot=state.slots[state.count];
  slot.sound=sound;
  slot.delay=low_signed16(std::bit_cast<std::uint32_t>(delay_ticks));
  ++state.count;
}
void vista_announcer_queue_completed(VistaAnnouncerQueueState& state,
                                    std::uint32_t voice) {
  if(state.slots[0].voice==voice)state.slots[0].active=0;
}
void vista_announcer_queue_update(VistaAnnouncerQueueState& state,
                                 const VistaAnnouncerPlayback& play) {
  if(state.count>state.slots.size())
    throw std::invalid_argument("Vista announcer queue update count exceeds source capacity");
  if(!state.count)return;
  auto& front=state.slots[0];
  if(front.delay>=0) {
    front.delay=low_signed16(std::uint32_t(std::int32_t(front.delay)-1));
    if(front.delay!=-1)return;
    const auto voice=front.sound==vista_announcer_none?vista_announcer_none:
        play(front.sound,1.f);
    front.voice=voice;
    if(voice!=vista_announcer_none)front.active=1;
    return;
  }
  if(front.active)return;
  for(std::uint32_t i=1;i<state.count;++i)state.slots[i-1]=state.slots[i];
  --state.count;
}
} // namespace halo2
