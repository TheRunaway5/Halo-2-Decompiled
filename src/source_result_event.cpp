#include "halo2/source_result_event.hpp"
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
void word(SourceResultEvent& event,unsigned offset,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)event[offset+i]=std::uint8_t(value>>(i*8));
}
}
void source_result_event_append(SourceResultEventState& state,const SourceResultRecording& recording,
    const SourceResultEvent& event) {
  if(!recording.enabled || recording.frozen)return;
  if(state.index>=state.events.size())
    throw std::out_of_range("Original event index escapes its provided ring backing");
  // Original REP MOVSD copies forward, including aliased ring input.
  auto* destination=state.events[state.index].data();
  for(unsigned i=0;i<9;++i) {
    std::uint32_t value;std::memcpy(&value,event.data()+i*4,4);
    std::memcpy(destination+i*4,&value,4);
  }
  state.index=(state.index+1u)%1000u;
}
void source_result_event_record(SourceResultEventState& state,const SourceResultRecording& recording,
    std::uint32_t player,std::uint32_t first,std::uint32_t second,const SourceResultEventServices& services) {
  if(!recording.enabled || recording.frozen)return;
  if(!services.time)throw std::invalid_argument("Result event requires its actual wall-clock owner");
  const auto now=services.time();
  if(std::uint32_t(state.last-now)<=3u)return;
  SourceResultEvent event{};event[0]=2;event[1]=std::uint8_t(player);event[2]=0xff;
  word(event,0x20,services.time()-state.start);
  word(event,0x10,second);word(event,0x14,first);
  if(!services.position)throw std::invalid_argument("Result event requires its actual unit-position owner");
  if(const auto position=services.position(player)) {
    for(unsigned i=0;i<3;++i)word(event,4+i*4,(*position)[i]);
    source_result_event_append(state,recording,event);
  }
  state.last=now;
}
} // namespace halo2
