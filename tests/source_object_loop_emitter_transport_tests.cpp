// The actual private native snapshot codec, followed by the production Rust
// acknowledged stream. Values below are declared transport stimuli; they do
// not claim a native object allocation, source/PAS admission or audible PCM.
#include "../src/network/multiplayer_network.cpp"
#include "halo2/source_execution_calls.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
void word(std::span<std::byte> bytes,std::size_t at,std::uint32_t value,unsigned width=4) {
  check(at+width<=bytes.size(),"Malformed object-loop test offset");
  for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(n*8));
}
SourceObjectLoopEmitterTu15 emitter(unsigned index,unsigned salt=1) {
  SourceObjectLoopEmitterTu15 out;
  out.loop=(salt<<16)|index;out.object=0x12340000u|index;out.definition=0x43210020u;
  out.function_active=true;out.cluster=7;out.maximum_distance=4.f;out.stop_duration=.25f;
  out.location[3]=std::byte{1};word(out.location,4,std::bit_cast<std::uint32_t>(1.f));
  word(out.location,0xcu,std::bit_cast<std::uint32_t>(float(index)+.125f));
  word(out.location,0x10u,0x80000000u);word(out.location,0x2cu,7,2);
  return out;
}
MultiplayerState fixture() {
  MultiplayerState state;state.source_game_type=2;state.source_engine=2;
  state.source_round_lifecycle_available=true;state.round_phase=1;state.round_index=2;state.round_countdown=90;
  return state;
}
Packet encoded(const MultiplayerState &state,unsigned version=native_multiplayer_protocol,SnapshotBodyLayout *layout=nullptr) {
  Packet bytes;encode_state(bytes,state,{},layout,version);return bytes;
}
MultiplayerState decoded(const Packet &bytes,unsigned version=native_multiplayer_protocol) {
  Reader reader{bytes};auto state=decode_state(reader,{},version);reader.end();return state;
}
}
int main(int argc,char **argv)try {
  check(argc==1 || argc==2,"Supply optional bounded transport report path");
  auto state=fixture();SnapshotBodyLayout unavailable_layout;
  const auto unavailable=encoded(state,native_multiplayer_protocol,&unavailable_layout);
  check(!decoded(unavailable).source_object_loop_emitters,"Unavailable object-loop bank became an available empty bank");
  // Later native extensions follow this bank. Locate its real serializer
  // boundary so malformed emitter inputs cannot accidentally mutate a valid
  // spatial float or the independent physics impulse journal instead.
  const auto frame_key=(std::uint64_t(Anchor::object_loop_emitters)<<40)|0xffffffffu;
  const auto frame=std::ranges::find(unavailable_layout.anchor_keys,frame_key);
  check(frame!=unavailable_layout.anchor_keys.end(),"Object-loop emitter frame anchor is absent");
  const auto bank_begin=unavailable_layout.anchor_offsets.at(std::size_t(frame-unavailable_layout.anchor_keys.begin()));
  check(bank_begin<unavailable.size() && unavailable[bank_begin]==std::byte{},
      "Object-loop frame anchor does not identify its unavailable presence byte");
  state.source_object_loop_emitters.emplace();const auto empty=encoded(state);
  check(empty.size()==unavailable.size()+2 && decoded(empty).source_object_loop_emitters &&
      decoded(empty).source_object_loop_emitters->empty(),"Available empty object-loop bank did not explicitly retire emitters");
  state.source_object_loop_emitters=std::vector{emitter(3),emitter(9,2)};
  state.source_object_loop_emitters->front().maximum_distance=-0.f;
  const auto complete=encoded(state);auto accepted=decoded(complete);
  check(complete.size()==unavailable.size()+2+2*source_object_loop_emitter_wire_bytes_tu15 && encoded(accepted)==complete,
      "Object-loop packet round trip changed identities, signed-zero fields or the owned spatial packet");
  const auto source_seal=source_execution_state_result(state);
  auto changed=state;changed.source_object_loop_emitters.reset();
  check(source_execution_state_result(changed)!=source_seal,"Full native state seal omitted emitter availability");
  const auto unavailable_seal=source_execution_state_result(changed);changed.source_object_loop_emitters.emplace();
  check(source_execution_state_result(changed)!=unavailable_seal,"Full native state seal conflated unavailable and available-empty emitter banks");
  changed=state;changed.source_object_loop_emitters->front().maximum_distance=0.f;
  check(source_execution_state_result(changed)!=source_seal,"Full native state seal lost signed-zero emitter fields");
  changed=state;changed.source_object_loop_emitters->front().function_active=false;
  check(source_execution_state_result(changed)!=source_seal,"Full native state seal omitted emitter function admission");
  changed=state;word(changed.source_object_loop_emitters->front().location,0xcu,std::bit_cast<std::uint32_t>(2.5f));
  check(source_execution_state_result(changed)!=source_seal,"Full native state seal omitted the original emitter spatial packet");
  unsigned rejected{};
  const auto reject=[&](const Packet &bytes) {
    bool failed{};try{accepted=decoded(bytes);}catch(const FormatError &){failed=true;}
    if(!failed)throw FormatError("Malformed pre-view object-loop bank was accepted at case "+std::to_string(rejected));
    check(encoded(accepted)==complete,"Rejected object-loop packet partially changed the previously accepted bank");++rejected;
  };
  for(std::size_t size=bank_begin;size<complete.size();++size)reject(Packet(complete.begin(),complete.begin()+size));
  const auto corrupt=[&](std::size_t offset,std::uint32_t value,unsigned width=4) {
    auto bytes=complete;word(bytes,offset,value,width);reject(bytes);
  };
  const auto first=bank_begin+3;
  corrupt(bank_begin,2,1);corrupt(bank_begin+1,1025,2);
  corrupt(first,0xffffffffu);corrupt(first+4,0xffffffffu);corrupt(first+8,0xffffffffu);
  corrupt(first,0x00010400u);corrupt(first+4,0x12340800u);
  corrupt(first+12,1,2);corrupt(first+14,2,1);
  corrupt(first+15,0xfffeu,2);corrupt(first+15,8,2);
  corrupt(first+17,0x7fc00000u);corrupt(first+21,0xbf800000u);
  corrupt(first+25+3,2,1);corrupt(first+25+4,0x7f800000u);
  corrupt(first+source_object_loop_emitter_wire_bytes_tu15,state.source_object_loop_emitters->front().loop);
  corrupt(first+source_object_loop_emitter_wire_bytes_tu15,0x00020003u);
  auto trailing=complete;trailing.push_back(std::byte{});reject(trailing);
  for(unsigned version:{110u,111u}) {
    const auto bytes=encoded(state,version);const auto legacy=decoded(bytes,version);
    check(!legacy.source_object_loop_emitters && encoded(legacy,version)==bytes,
        "Legacy native body acquired object-loop emitters or changed its historical byte layout");
    check(legacy.source_round_lifecycle_available==(version>=111),"Object-loop extension changed legacy round lifecycle availability");
  }
  auto maximum=fixture();maximum.source_object_loop_emitters.emplace();
  for(unsigned n=0;n<source_object_loop_emitter_capacity_tu15;++n)maximum.source_object_loop_emitters->push_back(emitter(n));
  const auto bounded=encoded(maximum);
  check(bounded.size()==unavailable.size()+2+source_object_loop_emitter_capacity_tu15*source_object_loop_emitter_wire_bytes_tu15 &&
      encoded(decoded(bounded))==bounded,"Original1024-slot object-loop capacity did not round trip");
  maximum.source_object_loop_emitters->push_back(emitter(0,2));
  bool over_capacity{};try{(void)encoded(maximum);}catch(const FormatError &){over_capacity=true;}
  check(over_capacity,"Encoder accepted more than the actual source object-loop pool capacity");

  // A generic native packet is exact even when the bandwidth profile chooses
  // to hold visual skeletons. These empty WORLD banks isolate that contract.
  SnapshotStreamSender sender(1292,80);SnapshotStreamReceiver receiver(1292);
  unsigned delivered{},datagrams{},dropped{},maximum_fragments{};std::uint64_t wire_bytes{};
  Packet last_accepted;std::vector<Packet> stale;
  for(unsigned sequence=1;sequence<=10;++sequence) {
    auto current=fixture();current.tick=sequence;current.devices.tick=sequence;
    if(sequence==2 || sequence==8)current.source_object_loop_emitters.emplace();
    else if(sequence>=3 && sequence<=7) {
      current.source_object_loop_emitters=std::vector{emitter(3),emitter(9,sequence>=6?3:2)};
      auto &first_emitter=current.source_object_loop_emitters->front();
      first_emitter.function_active=sequence!=5;first_emitter.maximum_distance=-0.f;
      word(first_emitter.location,0xcu,std::bit_cast<std::uint32_t>(float(sequence)+.125f));
      if(sequence==6)std::reverse(current.source_object_loop_emitters->begin(),current.source_object_loop_emitters->end());
      if(sequence==7)current.source_object_loop_emitters->erase(current.source_object_loop_emitters->begin());
    }
    if(sequence==3 || sequence==4) {
      current.source_object_loop_emitters.emplace();
      for(unsigned n=0;n<source_object_loop_emitter_capacity_tu15;++n)current.source_object_loop_emitters->push_back(emitter(n));
    }
    SnapshotBodyLayout layout;const auto bytes=encoded(current,native_multiplayer_protocol,&layout);
    const auto now=std::uint64_t(sequence)*1000;
    SnapshotStreamSender::Frame frame{bytes,layout.floats,layout.anchor_offsets,layout.anchor_keys,layout.poses,
        sequence,sequence,now,{},{},{}};
    const auto outgoing=sender.encode(frame);check(outgoing && outgoing->emitted,"Native Rust stream omitted an emitter lifecycle publication");
    maximum_fragments=std::max(maximum_fragments,unsigned(outgoing->packets.size()));
    if(sequence==4){stale=outgoing->packets;++dropped;continue;}
    bool completed{};
    for(auto at=outgoing->packets.rbegin();at!=outgoing->packets.rend();++at) {
      check(at->size()<=1300,"Object-loop delta packet exceeded native fragment bounds");
      ++datagrams;wire_bytes+=at->size()+80;bool failed{};
      const auto incoming=receiver.push(*at,now,failed);check(!failed,"Native Rust delta stream rejected the bounded emitter bank");
      if(incoming) {
        check(incoming->body==bytes && incoming->digest==outgoing->digest,
            "Rust acknowledged delta stream changed a pre-view emitter description");
        check(encoded(decoded(incoming->body))==bytes,"Reconstructed native packet lost emitter availability or source bytes");
        check(receiver.commit(incoming->sequence),"Native emitter delta receipt was not committed");
        last_accepted=incoming->body;completed=true;++delivered;
      }
    }
    check(completed,"Native emitter packet did not publish atomically after complete reassembly");
    for(const auto &packet:outgoing->packets) {
      bool failed{};check(!receiver.push(packet,now,failed) && !failed,"Duplicate emitter snapshot replayed an already committed bank");
    }
    if(sequence==5)for(const auto &packet:stale) {
      bool failed{};check(!receiver.push(packet,now,failed) && !failed,"Late emitter snapshot replaced a newer committed bank");
    }
    sender.ack(receiver.ack(),now+1);
  }
  check(delivered==9 && dropped==1 && maximum_fragments>1 && !decoded(last_accepted).source_object_loop_emitters,
      "Emitter delta lifecycle did not preserve creation, changes, retirement and unavailable state");
  nlohmann::json report={{"passed",true},{"protocol",native_multiplayer_protocol},{"record_bytes",source_object_loop_emitter_wire_bytes_tu15},
    {"emitter_bank_begin",bank_begin},{"subsequent_extension_bytes",unavailable.size()-bank_begin-1},
    {"source_capacity",source_object_loop_emitter_capacity_tu15},{"maximum_tail_bytes",3+source_object_loop_emitter_capacity_tu15*source_object_loop_emitter_wire_bytes_tu15},
    {"malformed_rejected",rejected},{"native_state_seal_covers_emitters",true},
    {"delta_deliveries",delivered},{"dropped_messages",dropped},{"datagrams",datagrams},{"maximum_fragments",maximum_fragments},{"wire_bytes",wire_bytes},
    {"scope","Production native body codec and Rust acknowledged stream with declared transport-only emitter stimuli. Exact raw spatial bytes/signed zeros, legacy110/111 absence, available-empty retirement, slot salts, bounds, truncation, reorder, duplicates and loss. No host PVS, source allocation, audible playback, live UDP/WAN or original-network-protocol claim."}};
  if(argc==2){std::ofstream file(argv[1]);file<<report.dump(2)<<'\n';check(bool(file),"Cannot write bounded emitter transport report");}
  std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
