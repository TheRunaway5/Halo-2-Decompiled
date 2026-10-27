#include "halo2/source_decal_frame_wire_tu15.hpp"
#include "helpers/source_decal_frame_checks.hpp"
#include "halo2/snapshot_stream.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstdlib>
using namespace halo2;
namespace {
void check(bool ok,const char* text){if(!ok)throw FormatError(text);}
struct Writer {
 std::vector<std::byte> bytes;std::vector<std::uint32_t> floats,offsets;std::vector<std::uint64_t> keys;
 void word(std::uint64_t value,unsigned width){for(unsigned n=0;n<width;++n)bytes.push_back(std::byte(value>>(n*8)));}
 void real(float value){floats.push_back(bytes.size());word(std::bit_cast<std::uint32_t>(value),4);}
 void anchor(SourceDecalWireAnchorTu15 kind,std::uint32_t id){offsets.push_back(bytes.size());keys.push_back((std::uint64_t(kind)<<40)|id);}
};
struct Reader {
 std::span<const std::byte> bytes;std::size_t at{};
 std::uint64_t word(unsigned width){check(width<=8 && width<=remaining(),"Truncated source decal test packet");std::uint64_t value{};for(unsigned n=0;n<width;++n)value|=std::uint64_t(std::to_integer<unsigned>(bytes[at++]))<<(n*8);return value;}
 float real(){const auto value=std::bit_cast<float>(std::uint32_t(word(4)));check(std::isfinite(value),"Nonfinite source decal test packet");return value;}
 std::size_t remaining()const{return bytes.size()-at;}
};
Writer encoded(const std::optional<SourceDecalFrameTu15>& frame){Writer out;encode_source_decal_frame_wire_tu15(out,frame);return out;}
std::optional<SourceDecalFrameTu15> decoded(std::span<const std::byte> bytes){Reader in{bytes};auto frame=decode_source_decal_frame_wire_tu15(in);check(!in.remaining(),"Trailing source decal test packet");return frame;}
void change(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value,unsigned width){check(at+width<=bytes.size(),"Malformed test offset");for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(n*8));}
}
int main(int argc,char**argv)try {
 check(argc==3,"Supply actual completed cache fixture and bounded report path");
 std::ifstream input(argv[1]);if(!input)return 77;nlohmann::json fixture;input>>fixture;
 SourceRuntimeMemory memory;
 for(const auto& region:fixture.at("regions")) {
  std::vector<std::byte> bytes;
  if(region.contains("file")) {
   std::ifstream f(std::filesystem::path(argv[1]).parent_path()/region.at("file").get<std::string>(),std::ios::binary);if(!f)return 77;
   for(char value;f.get(value);)bytes.push_back(std::byte(static_cast<unsigned char>(value)));
  } else {const auto value=region.at("value").get<std::uint32_t>();for(unsigned n=0;n<4;++n)bytes.push_back(std::byte(value>>(n*8)));}
  memory.mount(region.at("address"),bytes,SourceRuntimeMemory::Access::ReadWrite,"Actual completed Zanzibar decal fixture");
 }
 // The old retained fixture omits its unrelated game clock. Declare that
 // transport-only scalar; all records, pages, salts and heads are original.
 const std::array clock_pointer{std::byte{0},std::byte{0},std::byte{0},std::byte{0x70}};
 std::array<std::byte,0x24> clock{};clock[8]=std::byte{123};
 memory.mount(0x51df54u,clock_pointer,SourceRuntimeMemory::Access::ReadWrite,"Declared fixture clock pointer");
 memory.mount(0x70000000u,clock,SourceRuntimeMemory::Access::ReadWrite,"Declared fixture transport tick123");
 const auto frame=source_decal_frame_tu15(memory);const auto wire=encoded(frame);
 check(!frame.records.empty() && !frame.allocations.empty() && !frame.heads.empty(),"Actual retained cache fixture empty");
 check(test::same_source_decal_frame(frame,decoded(wire.bytes)),"Source decal wire changed actual records, metadata, heads or packed geometry");
 check(encoded(decoded(wire.bytes)).bytes==wire.bytes,"Source decal wire byte round-trip changed");
 check(encoded({}).bytes.size()==1 && !decoded(encoded({}).bytes),"Absent cache was not preserved");
 SourceDecalFrameTu15 empty;const auto blank=decoded(encoded(empty).bytes);
 check(encoded(empty).bytes.size()==11 && blank && blank->records.empty() && blank->heads.empty(),"Present empty cache did not clear independently of absence");
 std::size_t quads{},record_quads{};for(const auto& a:frame.allocations)quads+=a.quads.size();for(const auto& r:frame.records)record_quads+=r.quad_count;
 check(wire.bytes.size()==11+frame.records.size()*43+frame.allocations.size()*6+quads*64+frame.heads.size()*7,
       "Source decal wire transmitted duplicated decoded projections or unexpected bytes");
 unsigned rejected{};
 const auto reject=[&](const std::vector<std::byte>& bytes){bool failed{};try{(void)decoded(bytes);}catch(const FormatError&){failed=true;}check(failed,"Malformed source decal wire accepted");++rejected;};
 for(std::size_t length=0;length<wire.bytes.size();++length)reject({wire.bytes.begin(),wire.bytes.begin()+length});
 const auto corrupt=[&](std::size_t at,std::uint32_t value,unsigned width){auto bytes=wire.bytes;change(bytes,at,value,width);reject(bytes);};
 corrupt(0,2,1);corrupt(5,1025,2);corrupt(7,1025,2);corrupt(9,3585,2);
 corrupt(11+43,frame.records.front().datum,4);
 const auto allocation_at=11+frame.records.size()*43;
 corrupt(allocation_at,0,4);corrupt(allocation_at+4,1281,2);corrupt(allocation_at+6,0x7fc00000u,4);
 const auto head_record=std::find_if(frame.records.begin(),frame.records.end(),[&](const auto& r){return r.datum==frame.heads.front().datum;});
 check(head_record!=frame.records.end(),"Actual head record missing");
 const auto record_at=11+std::size_t(head_record-frame.records.begin())*43;
 corrupt(record_at+36,head_record->datum,4);
 corrupt(record_at+32,0xffffffffu,4);
 const auto head_at=allocation_at+frame.allocations.size()*6+quads*64;
 corrupt(head_at+2,7,1);
 auto trailing=wire.bytes;trailing.push_back(std::byte{});reject(trailing);
 // Use the actual original-profile stream in this isolated test process.
 // The shared clock word and stream envelope are present in both controls;
 // their packet overhead is charged once, not once again for a decal bank.
 const auto profile=[](const char* name) {
#if defined(_WIN32)
  check(_putenv_s("H2_NET_PROFILE",name)==0,"Cannot declare native network profile");
#else
  check(setenv("H2_NET_PROFILE",name,1)==0,"Cannot declare native network profile");
#endif
 };
 constexpr std::size_t chunk=1292,overhead=80;
 struct StreamCost {unsigned delivered{},packets{},initial_budget{};std::uint64_t first{},settled{};SnapshotStreamSender::Stats stats;};
 const auto measure=[&](bool with_cache,const char* selected_profile) {
  profile(selected_profile);SnapshotStreamSender sender(chunk,overhead);SnapshotStreamReceiver receiver(chunk);StreamCost cost;
  cost.initial_budget=sender.stats().budget_bps;check(cost.initial_budget==30720,"Actual native stream did not start at30720bits/s");
  for(unsigned sequence=1;sequence<=120;++sequence) {
   auto current_frame=frame;current_frame.tick=frame.tick+sequence-1;Writer current_wire;
   current_wire.word(current_frame.tick,4); // Existing native snapshot tick prefix.
   encode_source_decal_frame_wire_tu15(current_wire,with_cache?std::optional(current_frame):std::nullopt);
   const auto now=std::uint64_t(sequence)*1000/30;
   SnapshotStreamSender::Frame outgoing{current_wire.bytes,current_wire.floats,current_wire.offsets,current_wire.keys,{},current_frame.tick,sequence,now,{},{},{}};
   const auto emitted=sender.encode(outgoing);check(emitted && emitted->emitted,"Actual stream omitted generic source cache");
   std::uint64_t bytes{};
   for(const auto& packet:emitted->packets) {
    ++cost.packets;bytes+=packet.size()+overhead;bool failed{};const auto incoming=receiver.push(packet,now,failed);
    check(!failed,"Actual stream rejected source decal message");
    if(incoming){check(incoming->body==current_wire.bytes,"Actual delta stream changed source cache bytes");check(receiver.commit(incoming->sequence),"Actual source cache delta commit failed");++cost.delivered;}
   }
   if(sequence==1)cost.first=bytes;else cost.settled+=bytes;
   sender.ack(receiver.ack(),now+1);
  }
  cost.stats=sender.stats();check(cost.delivered==120,"Actual source cache stream did not converge");
  check(cost.stats.budget_bps>=4096 && cost.stats.budget_bps<=71680,"Actual stream did not use original bandwidth profile");
  return cost;
 };
 const auto baseline=measure(false,"original"),present=measure(true,"original");
 const auto fixed_baseline=measure(false,"30720,30720,30720"),fixed_present=measure(true,"30720,30720,30720");
 check(fixed_baseline.stats.budget_bps==30720 && fixed_present.stats.budget_bps==30720,"Fixed30720native control changed channel budget");
 const double seconds=(120000/30-1000/30)/1000.;
 const auto cost_json=[&](const StreamCost& cost) {return nlohmann::json{
  {"delivered",cost.delivered},{"datagrams",cost.packets},{"first_frame_wire_bytes",cost.first},
  {"settled119_frames_wire_bytes",cost.settled},{"settled_bits_per_second",cost.settled*8/seconds},
  {"reported_wire_bytes",cost.stats.wire_bytes},{"initial_budget_bits_per_second",cost.initial_budget},{"ending_budget_bits_per_second",cost.stats.budget_bps},
  {"over_budget",cost.stats.over_budget}};};
 nlohmann::json report={{"passed",true},{"records",frame.records.size()},{"aliased_record_quads",record_quads},
  {"unique_cache_allocations",frame.allocations.size()},{"unique_cache_quads",quads},{"heads",frame.heads.size()},
  {"raw_wire_bytes",wire.bytes.size()},{"malformed_cases_rejected",rejected},
  {"declared_profile","original"},{"starting_budget_bits_per_second",present.initial_budget},
  {"baseline_existing_clock_packet",cost_json(baseline)},{"same_packet_with_actual_cache",cost_json(present)},
  {"marginal_first_frame_wire_bytes",std::int64_t(present.first)-std::int64_t(baseline.first)},
  {"marginal_settled_wire_bytes",std::int64_t(present.settled)-std::int64_t(baseline.settled)},
  {"marginal_settled_bits_per_second",(double(present.settled)-double(baseline.settled))*8/seconds},
  {"fixed30720_baseline_existing_clock_packet",cost_json(fixed_baseline)},
  {"fixed30720_same_packet_with_actual_cache",cost_json(fixed_present)},
  {"fixed30720_marginal_settled_bits_per_second",(double(fixed_present.settled)-double(fixed_baseline.settled))*8/seconds},
  {"scope","Same production typed codec: actual retained original Zanzibar records/pages/heads with declared transport tick. Bounds, malformed rejection, empty clear and acknowledged original-profile delta size against the same shared clock/envelope baseline. This isolates marginal decal cost; it is not live blood, a complete gameplay packet/budget, ONLINE end-to-end or Xbox network protocol parity."}};
 std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
