// Declared transport controls for the actual production body codec and Rust
// acknowledged delta stream; actual native-source/GPU evidence is separate.
#include "../src/network/multiplayer_network.cpp"
#include "halo2/source_execution_calls.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void need(bool value,const char *why){if(!value)throw FormatError(why);}
Packet encode(const MultiplayerState &s,unsigned version=native_multiplayer_protocol,SnapshotBodyLayout *layout=nullptr){Packet p;encode_state(p,s,{},layout,version);return p;}
MultiplayerState decode(const Packet &p,unsigned version=native_multiplayer_protocol){Reader r{p};auto s=decode_state(r,{},version);r.end();return s;}
SourceGameplayFadeFrameTu15 frame(std::uint64_t sequence){
 SourceGameplayFadeFrameTu15 f;f.sequence=sequence;f.tick=std::uint32_t(sequence);f.engine_present=true;f.raw_mode=1;
 f.phase=f.pending_phase=1;f.rate=30;f.x87_control_word=0x27f;f.mxcsr=0x1f80;
 for(unsigned i=0;i<16;++i){f.identities[i]={0x10000u+i,1};f.players[i]={true,0x20000u+i,1,0,1,0};}return f;
}
void append(SourceGameplayFadeJournalTu15 &j){const auto sequence=++j.newest;const auto slot=(sequence-1)%source_gameplay_fade_history_tu15;
 if(slot==j.slots.size())j.slots.push_back(frame(sequence));else j.slots[slot]=frame(sequence);j.current=frame(sequence).identities;
 j.context_available=j.engine_present=true;j.rate=30;j.x87_control_word=0x27f;j.mxcsr=0x1f80;}
}
int main()try {
 need(native_multiplayer_protocol>=114,"Fade transport requires its explicit appended observation protocol");
 // Declared original active-round transport state, matching the existing
 // source emitter transport fixture;114 must never bypass the111 lifecycle
 // admission just because this case isolates an appended presentation bank.
 MultiplayerState state;state.source_game_type=2;state.source_engine=2;
 state.source_round_lifecycle_available=true;state.round_phase=1;state.round_index=0;state.round_countdown=0;
 const auto unavailable=encode(state);need(!decode(unavailable).source_gameplay_fade,"Unavailable fade source became a fabricated input owner");
 state.source_gameplay_fade.emplace();
 const auto empty=encode(state);need(decode(empty).source_gameplay_fade&&decode(empty).source_gameplay_fade->newest==0,"Real pre-tick fade owner was omitted");
 for(unsigned version=110;version<=113;++version){auto no_fade=state;no_fade.source_gameplay_fade.reset();
  need(encode(state,version)==encode(no_fade,version),"Fade extension changed a legacy snapshot extent");
  need(!decode(encode(state,version),version).source_gameplay_fade,"Legacy snapshot fabricated original fade observations");}
 for(unsigned i=0;i<512;++i)append(*state.source_gameplay_fade);
 const auto complete=encode(state);need(encode(decode(complete))==complete,"Complete source fade history did not roundtrip exactly");
 const auto seal=source_execution_state_result(state);auto changed=state;changed.source_gameplay_fade->slots[0].players[0].respawn_seconds=1;
 need(source_execution_state_result(changed)!=seal,"Native complete state seal omitted an original fade operand");
 const auto reject=[](auto call){bool failed{};try{call();}catch(const std::exception&){failed=true;}need(failed,"Malformed fade body was accepted");};
 for(const auto amount:{1u,2u,200u,454u,1000u})reject([&]{decode(Packet(complete.begin(),complete.end()-amount));});
 changed=state;changed.source_gameplay_fade->slots[0].identities[0].player=0x10001u;reject([&]{encode(changed);});
 changed=state;changed.source_gameplay_fade->newest++;reject([&]{encode(changed);});
 SnapshotStreamSender sender(1292,80);SnapshotStreamReceiver receiver(1292);unsigned fragments{},last_fragments{},delivered{};std::uint64_t wire{};
 std::vector<Packet> dropped;
 for(unsigned sequence=1;sequence<=6;++sequence){if(sequence>1)append(*state.source_gameplay_fade);
  state.tick=sequence;state.devices.tick=sequence;SnapshotBodyLayout layout;const auto bytes=encode(state,native_multiplayer_protocol,&layout);
  SnapshotStreamSender::Frame input{bytes,layout.floats,layout.anchor_offsets,layout.anchor_keys,layout.poses,sequence,sequence,std::uint64_t(sequence)*1000,{},{},{}};
  const auto out=sender.encode(input);need(out&&out->emitted,"Actual acknowledged source-fade stream omitted a publication");
  if(sequence==3){dropped=out->packets;continue;}
  last_fragments=out->packets.size();fragments+=last_fragments;bool completed{};
  for(auto it=out->packets.rbegin();it!=out->packets.rend();++it){bool failed{};wire+=it->size()+80;
   auto got=receiver.push(*it,sequence*1000,failed);need(!failed,"Fade delta stream rejected a valid source observation");
   if(got){need(got->body==bytes&&decode(got->body).source_gameplay_fade==state.source_gameplay_fade,"Fade delta changed the actual source input history");
    need(receiver.commit(got->sequence),"Fade source snapshot was not committed atomically");completed=true;++delivered;}}
  need(completed,"Fade snapshot never completed its atomic native publication");
  if(sequence==4)for(const auto &packet:dropped){bool failed{};need(!receiver.push(packet,sequence*1000,failed)&&!failed,"Older fade snapshot replaced current source input history");}
  sender.ack(receiver.ack(),sequence*1000+1);
 }
 need(delivered==5&&last_fragments<=4,"One new original fade frame retransmitted the full retained history");
 nlohmann::json report={{"passed",true},{"protocol",native_multiplayer_protocol},{"history_frames",512},
  {"history_wire_bytes",encode_source_gameplay_fade_journal_tu15(*state.source_gameplay_fade).size()},
  {"legacy_bodies_unchanged",{110,111,112,113}},{"delta_deliveries",delivered},{"delta_datagrams",fragments},{"last_frame_datagrams",last_fragments},{"wire_bytes",wire},
  {"scope","Production body codec and actual Rust acknowledged delta stream with declared transport-only source observations. Exact pointer-free payload, legacy extents, full state seal, bounded512 frame history, wrapped slots, loss/reorder and one-frame delta cost. Native source producer/real UDP/rendered fade remain separate acceptance."}};
 const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Fade transport test requires owned scratch");
 const auto output=std::getenv("H2_GAMEPLAY_FADE_TRANSPORT_REPORT");std::ofstream file(output?std::filesystem::path(output):std::filesystem::path(scratch)/"gameplay-fade-transport.json");file<<report.dump(2)<<'\n';need(bool(file),"Cannot write bounded source fade transport report");
 std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
