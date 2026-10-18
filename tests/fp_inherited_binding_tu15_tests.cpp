#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/fp_replication.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/unit_animation_selection.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <bit>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char **argv)try {
 if(argc!=4)throw FormatError("Supply original TU1.5 XBE, original map, and output JSON");
 const auto original=OriginalData::load_xbe(argv[1]);
 if(original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("TU1.5 only");
 Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),original);
 const auto tag=[&](std::uint32_t id,const char *group) {
  const auto p=cache.query_tag({.handle=TagHandle{id},.group=TagGroup::from_string(group),.require_address=true});
  if(!p)throw FormatError("Actual diagnostic tag identity is unavailable");
  return p;
 };
 const auto identity=[](const Tag *p){return Json{{"datum",p->datum},{"group",p->group},{"name",p->name},{"address",p->address}};};
 const auto weapon=4059500484u,graph=4062187501u;
 const auto root=tag(graph,"jmad");
 SourceAnimationGraphMemory memory;
 memory.read=[&](std::uint32_t address,std::size_t size){return cache.read_tag_bytes(address,size);};
 Json report{{"passed",false},{"reference","English TU1.5 only"},{"map",argv[2]},
  {"weapon",identity(tag(weapon,"weap"))},{"root_graph",identity(root)},
  {"inherited_graphs",Json::array()},{"selected_actions",Json::array()},
  {"full_physics_parity",false},{"supplied_physical_output",false}};
 const Bytes header(cache.read_tag_bytes(root->address,0x54));
 const auto inheritance=read_tag_block_descriptor(header,0x4c);
 const Bytes inherited(cache.read_tag_bytes(inheritance.address,inheritance.extent(0x20,64)));
 for(unsigned i=0;i<unsigned(inheritance.count);++i) {
  const auto p=tag(inherited.u32(i*0x20+4),"jmad");
  report["inherited_graphs"].push_back({{"index",i},{"graph",identity(p)}});
 }
 for(bool dual:{false,true})for(auto activity:{0x0400000cu,0x0c000064u,0x0b000065u}) {
  const auto cls=source_animation_weapon_class(root->address,dual?0x0400054bu:0x030000d9u,0x030000d9u,memory);
  std::array<std::byte,28> iterator{};
  const auto put=[&](unsigned at,std::uint32_t v){for(unsigned i=0;i<4;++i)iterator[at+i]=std::byte(v>>(8*i));};
  put(0,root->address);put(4,0x030000d9u);put(8,cls);put(12,0x030000d9u);
  std::array<std::uint32_t,3> resolved{};
  while(const auto row=next_source_animation_weapon(iterator,resolved,memory)) {
   const auto action=find_source_animation_record(row+4,activity,8,memory);
   if(!action)continue;
   const Bytes entry(cache.read_tag_bytes(action,8));
   const auto packed=entry.u32(4);const auto index=std::bit_cast<std::int16_t>(std::uint16_t(packed>>16));
   if(index==-1)continue;
   const auto parent=std::bit_cast<std::int16_t>(std::uint16_t(packed));
   if(parent<-1 || parent>=inheritance.count)throw FormatError("Actual action inheritance index escapes graph table");
   const auto owner=parent==-1?graph:inherited.u32(unsigned(parent)*0x20+4);
   const auto selected=cache.load_unit_weapon_action(graph,0x030000d9u,dual?0x0400054bu:0x030000d9u,0x030000d9u,activity,false);
   if(selected.source_graph!=owner || selected.source_index!=unsigned(index))throw FormatError("Actual resolved clip differs from the retained action row");
   const auto source=tag(owner,"jmad");const Bytes source_root(cache.read_tag_bytes(source->address,0x34));
   const auto clips=read_tag_block_descriptor(source_root,0x2c);
   if(index<0 || index>=clips.count)throw FormatError("Actual action clip escapes resolved graph definitions");
   const Bytes definition(cache.read_tag_bytes(clips.address+unsigned(index)*0x6c,0x6c));
   report["selected_actions"].push_back({{"dual",dual},{"activity",activity},{"packed",packed},
    {"inheritance",parent},{"index",index},{"resolved_graph",identity(source)},
    {"frames",definition.u16(0x14)},{"loop_frame",definition.u16(0x2e)},
    {"timing_event_count",definition.u32(0x4c)},{"sound_event_count",definition.u32(0x54)},
    {"effect_event_count",definition.u32(0x5c)}});
   break;
  }
 }
 const auto binding=cache.load_first_person_binding(weapon,1);
 unsigned checked=0,events=0;
 for(const auto &row:report["selected_actions"]) {
  const auto packed=row.at("packed").get<std::uint32_t>();
  const auto activity=row.at("activity").get<std::uint32_t>();
  FirstPersonHandBinding hand;hand.flags=6;hand.graph=graph;hand.weapon=weapon;hand.dual_wield=row.at("dual").get<bool>();
  const auto clocks=first_person_clips(hand,binding);
  const auto found=std::find_if(clocks.begin(),clocks.end(),[&](const auto &c){return c.activity==activity;});
  if(found==clocks.end() || first_person_clip_identifier(*found)!=packed || found->frames!=row.at("frames").get<int>() ||
      found->loop_frame!=row.at("loop_frame").get<int>())throw FormatError("FP binding lost the actual packed graph/clip definition");
  const auto decoded=cache.load_animation_identifier(graph,packed);
  if(decoded.source_graph!=row.at("resolved_graph").at("datum").get<std::uint32_t>() ||
      decoded.source_identifier!=packed || decoded.source_index!=row.at("index").get<unsigned>())
    throw FormatError("Presentation decoder lost the original inherited graph selector");
  const auto definition=cache.load_source_unit_animation_definition(graph,packed);
  std::vector<FirstPersonAnimationEvent> expected;
  for(const auto &event:definition.timing)expected.push_back({0,event.frame,0xffffffffu,std::uint32_t(std::int32_t(event.type)),0,0x0600008au});
  for(const auto &event:definition.sounds) {
    if(event.reference==-1)continue;
    if(event.reference<0 || unsigned(event.reference)>=definition.sound_references.size())throw FormatError("Actual sound reference is invalid");
    const auto &reference=definition.sound_references[event.reference];
    if(reference.tag!=0xffffffffu)expected.push_back({1,event.frame,reference.tag,0xffffffffu,reference.flags,event.marker});
  }
  for(const auto &event:definition.effects) {
    if(event.reference==-1)continue;
    if(event.reference<0 || unsigned(event.reference)>=definition.effect_references.size())throw FormatError("Actual effect reference is invalid");
    const auto &reference=definition.effect_references[event.reference];
    if(reference.tag!=0xffffffffu)expected.push_back({2,event.frame,reference.tag,0xffffffffu,reference.flags,0x0600008au});
  }
  if(found->events.size()!=expected.size())throw FormatError("Inherited FP event count differs from the retained source definition");
  for(unsigned i=0;i<expected.size();++i) {
    const auto &a=found->events[i],&b=expected[i];
    if(a.type!=b.type || a.frame!=b.frame || a.tag!=b.tag || a.id!=b.id || a.reference_flags!=b.reference_flags || a.marker!=b.marker)
      throw FormatError("Inherited FP event packet differs from its actual graph owner");
    ++events;
  }
  first_person_activity(hand,binding,activity);
  if(hand.animation_identifier!=packed)throw FormatError("Native FP activity discarded the packed inheritance selector");
  FirstPersonViewBinding before;before.unit=1;before.representation=1;before.hands[0]=hand;
  auto after=before;first_person_animation_clock(after.hands[0],binding,1.f/30.f);
  FirstPersonReplicatedClock clock{true,first_person_channel_state(before.hands[0]),first_person_channel_state(after.hands[0])};
  auto adopted=before,origin=before;
  if(!apply_replicated_first_person_clock(origin,adopted,clock,binding) ||
     !same_first_person_channel(first_person_channel_state(adopted.hands[0]),clock.current))
    throw FormatError("Replicated FP clock lost its complete inherited selector");
  auto corrupt=hand;corrupt.animation_identifier^=1u;bool rejected=false;
  try {first_person_animation_clock(corrupt,binding,1.f/30.f);}catch(const FormatError &){rejected=true;}
  if(!rejected)throw FormatError("FP clock accepted a different inherited graph with the same clip index");
  ++checked;
 }
 if(checked!=6 || !events)throw FormatError("Actual inherited binding regression corpus is incomplete");
 report["passed"]=true;report["verified_packed_activities"]=checked;report["verified_original_event_packets"]=events;
 report["boundary"]="Read-only original action rows, exact packed selector, resolved graph definitions/resources/events, native clock and validated replicated clock transitions. Clock identities are declared fixture inputs; this does not supply or claim source player/body/pose/solver output, live multiplayer or Xbox rendering.";
 std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump(2)<<'\n';
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
