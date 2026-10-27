#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/math.hpp"
#include "halo2/source_decal_vertex_cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/nonhost_movement_client.hpp"
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
#include "sword_online_socket_fixture.hpp"
#endif
#include <algorithm>
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <set>
#include <thread>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
// The same logical-endpoint transport interface used by OnlineMatchSession.
// Delivery is deterministic here; this does not claim DTLS/NAT/WAN coverage.
struct CombatBus {
  struct Packet {unsigned from;std::vector<std::byte> bytes;};
  std::array<std::deque<Packet>,2> incoming;
  std::array<std::uint64_t,2> received_packets{},received_snapshot_starts{};
  std::array<std::optional<std::chrono::steady_clock::time_point>,2> first_snapshot;
};
struct CombatEndpoint final:MultiplayerDatagrams {
  std::shared_ptr<CombatBus> bus;unsigned self;
  CombatEndpoint(std::shared_ptr<CombatBus> b,unsigned s):bus(std::move(b)),self(s){}
  void send(std::span<const std::byte> bytes,unsigned peer)override {
    check(peer<bus->incoming.size(),"Sword peer endpoint exceeds its admitted topology");
    bus->incoming[peer].push_back({self,{bytes.begin(),bytes.end()}});
  }
  bool receive(std::vector<std::byte> &bytes,unsigned &peer)override {
    auto &queue=bus->incoming[self];if(queue.empty())return false;
    peer=queue.front().from;bytes=std::move(queue.front().bytes);queue.pop_front();
    ++bus->received_packets[self];
    // Passive diagnostics of the real complete-snapshot wire header. Logical
    // endpoints carry plaintext native packets, with the first fragment at27.
    if(bytes.size()>=35 && Bytes(bytes).u32(0)==0x324e3248u &&
        bytes[6]==std::byte{3} && Bytes(bytes).u16(27)==0) {
      ++bus->received_snapshot_starts[self];
      if(!bus->first_snapshot[self])bus->first_snapshot[self]=std::chrono::steady_clock::now();
    }
    return true;
  }
};
// Optional bounded read-only observations of the genuine source stores and
// CALL return pushes. Cache body addresses before stepping: callbacks must not
// reenter the public world API or supply contact/action/damage results.
struct SwordLungeTrace {
  MultiplayerObjectRuntime *owner{};
  const SourceRuntimeMemory *memory{};
  std::array<std::uint32_t,2> bodies{};
  unsigned tick{},lines{},casts{},hits{},selections{},damage_calls{},dispatches{};
  std::uint32_t selected=null_datum;float fraction{};
  ~SwordLungeTrace(){if(owner)owner->observe_physics_stores({});}
  std::uint32_t word(std::uint32_t address)const{return Bytes(memory->view(address,4)).u32(0);}
  bool line(){if(lines>=160)return false;++lines;std::cout<<"[sword-lunge] tick="<<tick;return true;}
  void observe(const SourcePhysicsContactRecordStore &store) {
    const auto pc=store.instruction,sp=store.address;
    switch(pc) {
    case 0xec684u:
      if(line())std::cout<<" impact attacker="<<std::hex<<word(sp+4)<<" activity="<<word(sp+8)
          <<std::dec<<" modifier="<<word(sp+12)<<" scale="<<std::bit_cast<float>(word(sp+16))<<'\n';
      break;
    case 0xec722u:
      if(line())std::cout<<" strike_transition attacker="<<std::hex<<word(sp+4)<<std::dec
          <<" kind="<<word(sp+8)<<'\n';
      break;
    case 0xd1b84u:++casts;break;
    case 0xd1ba6u:++hits;break; // Executed only after a successful grid cast.
    case 0xd1c75u:++selections;selected=Bytes(store.written).u32(0);break;
    case 0xd1c8bu:fraction=Bytes(store.written).f32(0);break;
    case 0xd1f5bu: {
      ++damage_calls;const auto hit=word(sp+12);
      if(line())std::cout<<" D2180 attacker="<<std::hex<<word(sp+4)<<" damage="<<word(sp+8)
          <<" target="<<word(hit)<<std::dec<<" scale="<<std::bit_cast<float>(word(hit+0x14))<<'\n';
      break;
    }
    case 0xd22f9u: {
      ++dispatches;const auto request=word(sp+4);
      if(line())std::cout<<" DA0D0 target="<<std::hex<<word(sp+8)<<" damage="<<word(request)
          <<" owner_player="<<word(request+8)<<" owner_unit="<<word(request+12)
          <<std::dec<<" scale="<<std::bit_cast<float>(word(request+0x54))
          <<" material="<<std::bit_cast<std::int16_t>(Bytes(memory->view(request+0x7c,2)).u16(0))<<'\n';
      break;
    }
    default:break;
    }
    for(unsigned slot=0;slot<bodies.size();++slot)if(bodies[slot]) {
      for(const auto offset:{0x3dcu,0x3ecu}) {
        const unsigned bytes=offset==0x3dc?1u:4u;const auto address=bodies[slot]+offset;
        if(store.before==store.written || std::uint64_t(store.address)+store.written.size()<=address || store.address>=address+bytes)continue;
        if(line())std::cout<<" physical_store slot="<<slot<<" pc="<<std::hex<<pc<<" offset="<<offset
            <<std::dec<<" mode="<<unsigned(std::to_integer<std::uint8_t>(memory->view(bodies[slot]+0x3dc,1).front()))
            <<" clock="<<std::bit_cast<std::int32_t>(word(bodies[slot]+0x3ec))<<'\n';
      }
    }
    if(pc==0xec704u || pc==0xec3a7u || pc==0xec2aau || pc==0xec2b3u || pc==0xec363u || pc==0xec38du)
      if(line())std::cout<<" action_store pc="<<std::hex<<pc<<" address="<<store.address
          <<std::dec<<" value="<<(store.written.size()==4?Bytes(store.written).u32(0):
              unsigned(std::to_integer<std::uint8_t>(store.written.front())))<<'\n';
  }
  void finish_tick() {
    if(casts || damage_calls || dispatches)if(line())std::cout<<" grid_casts="<<casts<<" hits="<<hits
        <<" selections="<<selections<<" selected="<<std::hex<<selected<<std::dec
        <<" fraction="<<fraction<<" D2180_calls="<<damage_calls<<" DA0D0_calls="<<dispatches<<'\n';
    casts=hits=selections=damage_calls=dispatches=0;selected=null_datum;fraction=0;
  }
};
// Read-only witnesses around an actual delivered-input interruption. Mode6
// need not survive the following physical update: those owners decide it.
struct SwordCancelWitness {
  const SourceRuntimeMemory *memory{};
  std::uint32_t unit=null_datum,body{},component{};
  unsigned tick{},requested_tick{},cancel_stores{},active_clears{},old_impacts{},physical_stores{};
  unsigned reload_interrupts{},switch_interrupts{};std::uint32_t reload_return{},switch_return{};
  std::vector<std::byte> interrupt_body;bool interrupt_captured{};
  unsigned activity_byte_contracts{},active_byte_contracts{};
  bool requested{},fresh_attack{};
  std::uint32_t word(std::uint32_t address)const{return Bytes(memory->view(address,4)).u32(0);}
  void observe(const SourcePhysicsContactRecordStore &store) {
    if(!memory || !body)return;
    const auto pc=store.instruction;
    if(requested && pc==0xe9741u)reload_return=store.address;
    if(requested && pc==0xeb00cu)switch_return=store.address;
    // E8F10 saves ESI before E8F41 pushes action, unit and callback return.
    // Its caller return is therefore at callback-SP+16, not among its args.
    if(requested && pc==0xe8f41u && word(store.address+4u)==unit && word(store.address+8u)==27u) {
      if(store.address+16u==reload_return && word(reload_return)==0xe9746u)++reload_interrupts;
      if(store.address+16u==switch_return && word(switch_return)==0xeb011u)++switch_interrupts;
      check(component>=body && std::uint64_t(component)+0x38u<=std::uint64_t(body)+0x8000u,
          "Original Sword action component exceeds the bounded live-body witness");
      // Factory unit storage can append the action component beyond the
      // ordinary physical body; the source's signed+346 offset owns it.
      interrupt_body.resize(std::max<std::size_t>(0x700u,component-body+0x38u));
      const auto current=memory->view(body,interrupt_body.size());
      std::copy(current.begin(),current.end(),interrupt_body.begin());interrupt_captured=true;
    }
    if(requested && !fresh_attack && pc==0xec684u && word(store.address+4u)==unit)++old_impacts;
    if(requested && pc==0xec770u && store.address==component+0x2cu) {
      check(Bytes(store.written).u32(0)==null_datum,"Original melee interruption did not clear its retained activity");
      check(interrupt_captured && component>=body && component+0x30u<=body+interrupt_body.size(),
          "Live Sword interruption lost its original callback/body witness");
      std::fill_n(interrupt_body.begin()+std::ptrdiff_t(component+0x2cu-body),4,std::byte{0xff});
      const auto current=memory->view(body,interrupt_body.size());
      check(std::equal(current.begin(),current.end(),interrupt_body.begin()),
          "Live EC750 changed retained body bytes outside its original activity-clear contract");
      ++activity_byte_contracts;
      ++cancel_stores;
      std::cout<<"Source sword cancel store: tick="<<tick<<" pc=ec770 unit="<<std::hex<<unit
        <<" activity_before="<<Bytes(store.before).u32(0)<<std::dec
        <<" snapshot_bytes="<<interrupt_body.size()
        <<" mode="<<unsigned(std::to_integer<std::uint8_t>(memory->view(body+0x3dcu,1)[0]))
        <<" clock="<<std::bit_cast<std::int32_t>(word(body+0x3ecu))<<'\n';
    }
    if(requested && pc==0xe8f60u && store.address==component+4u &&
        (Bytes(store.before).u32(0)&(1u<<27)) && !(Bytes(store.written).u32(0)&(1u<<27))) {
      ++active_clears;check(interrupt_captured,"Generic Sword interruption lost its body witness");
      const auto offset=component+4u-body;const auto expected=Bytes(interrupt_body).u32(offset)&~(1u<<27);
      for(unsigned n=0;n<4;++n)interrupt_body[offset+n]=std::byte(expected>>(n*8));
      const auto current=memory->view(body,interrupt_body.size());
      check(std::equal(current.begin(),current.end(),interrupt_body.begin()),
          "Live E8F10 changed retained body bytes outside its original activity/active-bit contract");
      ++active_byte_contracts;interrupt_captured=false;
    }
    if(cancel_stores && store.address>=body && store.address<body+0x700u &&
        ((pc>=0x1ed880u && pc<0x1edb00u) || (pc>=0x1f39d0u && pc<0x1f5000u) ||
         (pc>=0xe4a40u && pc<0xe4e00u)))++physical_stores;
  }
};
struct SwordRapidWitness {
  const SourceRuntimeMemory *memory{};std::uint32_t body{};
  unsigned tick{},lines{},selector_commits{},admission_samples{};std::array<unsigned,5> target_clears{};
  std::uint32_t word(unsigned offset)const{return Bytes(memory->view(body+offset,4)).u32(0);}
  void observe(const SourcePhysicsContactRecordStore &store) {
    if(!memory || !body)return;const auto pc=store.instruction;
    if(pc==0x152335u && store.written.size()==4 && Bytes(store.written).u32(0)==body) {
      ++admission_samples;
      if(tick>=76 && tick<=100 && lines++<100)std::cout<<"Source rapid admission phase: tick="<<tick
        <<" current="<<unsigned(std::to_integer<std::uint8_t>(memory->view(body+0x212u,1)[0]))
        <<" requested="<<unsigned(std::to_integer<std::uint8_t>(memory->view(body+0x216u,1)[0]))
        <<" retained_flags="<<std::hex<<word(0x148u)<<" target="<<word(0x1c8u)<<std::dec
        <<" confidence="<<std::bit_cast<float>(word(0x1e4u))<<'\n';
    }
    if(pc==0xeade1u && store.address==body+0x212u) {
      ++selector_commits;
      std::cout<<"Source rapid selector commit: tick="<<tick<<" pc=eade1 before="
        <<unsigned(std::to_integer<std::uint8_t>(store.before[0]))<<" current="
        <<unsigned(std::to_integer<std::uint8_t>(store.written[0]))<<'\n';
    }
    constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,5> clears{{
      {0xeae4au,0x1c8u},{0xeae50u,0x1ccu},{0xeae56u,0x1d0u},{0xeae62u,0x1e4u},{0xeae6au,0x1e8u}}};
    for(unsigned n=0;n<clears.size();++n)if(pc==clears[n].first && store.address==body+clears[n].second) {
      check(Bytes(store.written).u32(0)==(n<3?null_datum:0u),"Original weapon switch did not preserve its exact target/confidence-clear store");
      ++target_clears[n];
      if(lines++<100)std::cout<<"Source rapid target clear: tick="<<tick<<" pc="<<std::hex<<pc
        <<" before="<<Bytes(store.before).u32(0)<<" after="<<Bytes(store.written).u32(0)<<std::dec<<'\n';
    }
  }
};
}
// Declared native placement beside a real map wall. The source world owns
// action admission, posed collision, damage, contact and animation frames.
// No fabricated geometry, source state, hit result or lower return is supplied.
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
  if(const auto result=test::sword_online_socket_worker_main(argc,argv);result>=0)return result;
#endif
  check(argc==5 || argc==6,"Supply MAP SHARED TU15|native-mcc OUTPUT_DIRECTORY [sword|primary|grenade|plasma|target|sword-target|primary-target|lunge|lunge-host|lunge-client|sword-close-host|sword-close-client|fire-N]");
  const bool native_mcc=std::string_view(argv[3])=="native-mcc";
  if(native_mcc && !std::filesystem::is_regular_file(argv[1]))return 77;
  if(native_mcc)check(std::getenv("HALO2_TEST_SCRATCH"),"MCC melee acceptance requires managed test scratch");
  const auto mode=argc==6?std::string_view(argv[5]):std::string_view("melee");
  const bool firing=mode.starts_with("fire-");
  const bool rapid_near=mode=="sword-rapid-near-host" || mode=="sword-rapid-near-client";
  const bool rapid_far=mode=="sword-rapid-far-host" || mode=="sword-rapid-far-client";
  const bool rapid=rapid_near || rapid_far;
  const std::string_view rapid_sequence=std::getenv("HALO2_SWORD_RAPID_SEQUENCE")?
      std::getenv("HALO2_SWORD_RAPID_SEQUENCE"):"y-rt";
  if(rapid)check(native_mcc && (rapid_sequence=="y-rt" || rapid_sequence=="y-then-rt" ||
      rapid_sequence=="held-rt-y" || rapid_sequence=="x-y-b-rt" || rapid_sequence=="x-y-b-then-rt" ||
      rapid_sequence=="x-y-b-rt-same" || rapid_sequence=="y-b" ||
      rapid_sequence=="y-release-rt"),
      "Rapid Sword requires a declared canonical input sequence");
  const bool flight_swap=mode=="sword-flight-swap-host" || mode=="sword-flight-swap-client";
  const bool cancel=mode.starts_with("sword-cancel-") || flight_swap;
  const bool cancel_jump=mode=="sword-cancel-jump-host" || mode=="sword-cancel-jump-client";
  const bool cancel_reattack=mode=="sword-cancel-reattack-host" || mode=="sword-cancel-reattack-client";
  const bool cancel_switch=mode=="sword-cancel-switch-host" || mode=="sword-cancel-switch-client";
  const bool cancel_neutral=mode=="sword-cancel-neutral-host" || mode=="sword-cancel-neutral-client";
  if(cancel)check(native_mcc && (flight_swap || cancel_jump || cancel_reattack || cancel_switch || cancel_neutral),
      "Sword cancellation acceptance requires a declared canonical prepared MCC sequence");
  const bool network=mode=="lunge-host" || mode=="lunge-client" ||
      mode=="sword-close-host" || mode=="sword-close-client" || cancel || rapid;
  unsigned host_slot=0,remote_slot=1;
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
  std::unique_ptr<test::SwordOnlineSocketPair> sockets;
  if(test::SwordOnlineSocketPair::requested()) {
    check(native_mcc && network,"Actual ONLINE socket acceptance requires a canonical network Sword case");
    sockets=std::make_unique<test::SwordOnlineSocketPair>();
    host_slot=sockets->host_slot();remote_slot=sockets->other_slot();
  }
#endif
  const unsigned attacker_slot=network && mode.ends_with("-client")?remote_slot:host_slot,victim_slot=1u-attacker_slot;
  const bool close_sword=mode=="sword-close-host" || mode=="sword-close-client";
  const bool lunge=mode=="lunge" || mode=="lunge-host" || mode=="lunge-client" || cancel;
  const bool target=mode=="target" || mode=="sword-target" || mode=="primary-target" || lunge || close_sword || rapid;
  const bool canonical_target=native_mcc && target;
  const bool primary_intent_probe=rapid || std::getenv("HALO2_SWORD_PRIMARY_INTENT_PROBE")!=nullptr;
  if(primary_intent_probe)check(canonical_target,
      "Primary input comparison requires canonical prepared MCC source commands");
  const bool jump_before_attack=std::getenv("HALO2_SWORD_JUMP_BEFORE_ATTACK")!=nullptr;
  if(jump_before_attack)check(canonical_target,"Jump-before-Sword acceptance requires a canonical prepared MCC target");
  const bool strict_decal_cache=mode=="plasma-cache";
  const bool plasma=mode=="plasma" || strict_decal_cache;
  const bool grenade=mode=="grenade" || plasma,special=mode=="primary" || mode=="special" || mode=="primary-target" || lunge || close_sword;
  const bool sword=mode=="sword" || mode=="sword-target" || special || plasma || rapid;
  unsigned sword_trigger_ticks=1;
  if(const auto raw=std::getenv("HALO2_SWORD_TRIGGER_TICKS")) {
    const std::string_view value(raw);
    const auto parsed=std::from_chars(value.data(),value.data()+value.size(),sword_trigger_ticks);
    check(parsed.ec==std::errc{} && parsed.ptr==value.data()+value.size() &&
        sword_trigger_ticks>=1 && sword_trigger_ticks<=30,
        "HALO2_SWORD_TRIGGER_TICKS requires an integer from1 through30");
  }
  if(sword && special)std::cout<<"Source sword primary trigger held_ticks="<<sword_trigger_ticks<<'\n';
  check(grenade || sword || target || mode=="melee" || firing,"Invalid source combat acceptance mode");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  if(!native_mcc)options.gameplay_xbe=argv[3];
  options.source_object_savegame_directory=std::filesystem::absolute(argv[4])/"source-savegames";
  Cache cache(options.map,options.shared_maps,native_mcc?std::shared_ptr<const OriginalData>{}:
      OriginalData::load_xbe(options.gameplay_xbe));
  check(!native_mcc || (cache.is_mcc() && !cache.is_vista() &&
      cache.original_data()==OriginalData::native_tu15()),
      "MCC melee acceptance requires prepared MCC assets and compiled native TU1.5 operands");
  const auto content=cache.load_content(0,true);CollisionWorld geometry(content.collision);
  unsigned rapid_firearm_option=5;std::uint32_t rapid_firearm_definition=null_datum,rapid_sword_definition=null_datum;
  float rapid_distance=2.5f,rapid_sword_range{},rapid_firearm_range{};
  if(rapid) {
    const std::string_view firearm=std::getenv("HALO2_SWORD_RAPID_FIREARM")?std::getenv("HALO2_SWORD_RAPID_FIREARM"):
        rapid_far?"battle-rifle":"smg";
    check(firearm=="smg" || firearm=="rocket" || firearm=="battle-rifle",
        "Rapid firearm requires an actual authored smg, rocket or battle-rifle choice");
    if(firearm!="smg") {
      std::set<std::uint32_t> matches;
      std::string choices;
      const auto suffix=firearm=="rocket"?"\\rocket_launcher":"\\battle_rifle";
      for(unsigned option=3;option<=20;++option) {
        std::uint32_t definition;
        try {definition=cache.slayer_starting_weapon(option);}
        catch(const FormatError &error) {
          if(std::string_view(error.what())!="Authored starting-weapon choice has no resident weapon")throw;
          continue;
        }
        const auto tag=cache.query_tag({.handle=TagHandle{definition}});
        choices+=" option="+std::to_string(option)+":"+(tag?tag->name:"<missing tag>");
        // The authenticated TU1.5 multiplayer table repeats the same rocket
        // in two slots. Require one distinct resident weapon, retaining its
        // first authored option, rather than rejecting that original alias.
        if(tag && tag->name.ends_with(suffix) && matches.insert(definition).second && matches.size()==1)
          rapid_firearm_option=option;
      }
      if(matches.size()!=1)throw FormatError("Actual authored starting choices did not identify one requested firearm: "+
          std::string(firearm)+" distinct_matches="+std::to_string(matches.size())+choices);
    }
    rapid_firearm_definition=cache.slayer_starting_weapon(rapid_firearm_option);rapid_sword_definition=cache.slayer_starting_weapon(17);
    const auto bounds=[&](std::uint32_t definition) {
      const auto metadata=cache.load_source_tag_metadata(definition);
      const Bytes values(metadata.bytes_at(metadata.source_address()+0x208u,0x14));
      check(std::isfinite(values.f32(4)) && std::isfinite(values.f32(12)) && values.f32(4)>0 && values.f32(12)>0,
          "Rapid range fixture requires actual positive authored target ranges");
      return std::pair{values.f32(4),values.f32(12)};
    };
    const auto sword_bounds=bounds(rapid_sword_definition),firearm_bounds=bounds(rapid_firearm_definition);
    rapid_sword_range=std::max(sword_bounds.first,sword_bounds.second);
    // Unchanged1AD460 fades distance confidence beyond half the primary
    // target range (original46A0CC=0.5). The SMG's maximum12 therefore
    // cannot supply full primary confidence at10. Use an actual longer-range
    // firearm for the far control, retaining the >=1 live-query assertion.
    rapid_firearm_range=firearm_bounds.first*.5f;
    if(rapid_far) {
      check(rapid_firearm_range>rapid_sword_range,"Authored firearm has no independently longer full-confidence range than Sword");
      rapid_distance=(rapid_sword_range+rapid_firearm_range)*.5f;
    }
    std::cout<<"Source rapid authored ranges: firearm_option="<<rapid_firearm_option<<" firearm="<<std::hex
      <<rapid_firearm_definition<<" sword="<<rapid_sword_definition<<std::dec<<" sword20C="<<sword_bounds.first
      <<" sword214="<<sword_bounds.second<<" firearm20C="<<firearm_bounds.first<<" firearm214="<<firearm_bounds.second
      <<" firearm_full_distance="<<rapid_firearm_range
      <<" declared_distance="<<rapid_distance<<" sequence="<<rapid_sequence<<'\n';
  }
  auto spawn=cache.spawns().front();const auto original=spawn.position;float nearest=1e9f;bool found=grenade || target || firing;
  if(rapid_far) {
    found=false;
    for(const auto &candidate:cache.spawns()) {
      for(unsigned direction_=0;direction_<64 && !found;++direction_) {
        const float yaw=float(direction_)*6.28318530717958647692f/64.f;const Vec3 d{std::cos(yaw),std::sin(yaw),0};
        const auto floor=geometry.raycast(candidate.position+d*rapid_distance+Vec3{0,0,.5f},{0,0,-1},1.f);
        if(!floor || floor->normal.z<=.9f)continue;
        const auto feet=floor->position+Vec3{0,0,.002f};
        if(geometry.overlaps(feet+Vec3{0,0,.03f},content.player.height,content.player.radius))continue;
        const auto eye=candidate.position+Vec3{0,0,content.player.camera_height},to=feet+Vec3{0,0,content.player.camera_height};
        if(geometry.raycast(eye,normalized(to-eye),length(to-eye)))continue;
        spawn=candidate;spawn.facing=yaw;found=true;
      }
      if(found)break;
    }
    check(found,"Far rapid fixture has no authored start with actual floor/LOS inside the declared range interval");
  }else if(lunge || close_sword || rapid) {
    found=false;
    for(const auto &candidate:cache.spawns()) {
      for(unsigned direction_=0;direction_<4 && !found;++direction_) {
        const float yaw=float(direction_)*1.5707963267948966f;
        const Vec3 d{std::cos(yaw),std::sin(yaw),0};
        const auto eye=candidate.position+Vec3{0,0,content.player.camera_height};
        if(geometry.raycast(eye,d,3.f) || geometry.raycast(eye,d*-1.f,3.f))continue;
        bool floor=true;
        for(const auto distance:{-2.5f,-1.25f,0.f,1.25f,2.5f}) {
          const auto hit=geometry.raycast(candidate.position+d*distance+Vec3{0,0,.5f},{0,0,-1},1.f);
          floor&=hit && hit->normal.z>.9f;
        }
        if(floor){spawn=candidate;spawn.facing=yaw;found=true;}
      }
      if(found)break;
    }
    check(found,"Lunge fixture has no clear authored start and real floor corridor");
  }
  if(!grenade && !target && !firing)for(const auto &triangle:content.collision) {
    const auto n=normalized(triangle.source_normal);if(std::abs(n.z)>.1f)continue;
    const auto midpoint=(triangle.a+triangle.b+triangle.c)/3.f;
    const auto side=normalized(Vec3{n.x,n.y,0});const auto outside=midpoint+side*.3f;
    const auto floor=geometry.raycast(outside+Vec3{0,0,1},{0,0,-1},3.f);
    if(!floor || floor->normal.z<.9f)continue;
    const auto feet=floor->position+Vec3{0,0,.002f},eye=feet+Vec3{0,0,content.player.camera_height};
    const auto hit=geometry.raycast(eye,side*-1.f,.35f);if(!hit || hit->distance<.2f)continue;
    const float distance=length(feet-original);if(distance>=nearest)continue;
    nearest=distance;spawn.position=feet;spawn.facing=std::atan2(-side.y,-side.x);found=true;
  }
  check(found,"No admitted original wall/floor placement for melee acceptance");
  std::array<Spawn,2> target_placements{spawn,spawn};
  if(!canonical_target) {
    options.native_spawn_inputs={spawn};options.defer_initial_player_spawn=true;
  }
  options.starting_weapon=sword?17:5;
  if(flight_swap)options.starting_weapon=5;
  if(rapid)options.starting_weapon=rapid_firearm_option;
  if(firing)options.starting_weapon=std::stoul(std::string(mode.substr(5)));
  if(target) {
    auto victim=spawn;victim.position=victim.position+Vec3{std::cos(spawn.facing),std::sin(spawn.facing),0}*(rapid?rapid_distance:lunge?2.5f:.65f);
    const auto floor=geometry.raycast(victim.position+Vec3{0,0,.5f},{0,0,-1},1.f);
    check(bool(floor) && floor->normal.z>.9f,"Target placement has no real map floor");
    victim.position=floor->position+Vec3{0,0,.002f};victim.facing+=3.14159265358979323846f;
    target_placements[1]=victim;
    if(!canonical_target)options.native_spawn_inputs.push_back(victim);
    options.local_players=network?1u:2u;
  }
  if(plasma)options.variant.values.set(83,17); // Authored Sword weapon-set rule owns plasma conversion.
  options.session_random_seed=0x12345678;
  std::vector<std::uint32_t> players;
  if(canonical_target)options.variant.values.set(0x59,int(options.starting_weapon));
  else {
    options.player_birth_owner=[&](MultiplayerPlayer &p,unsigned){players.push_back(p.source_datum);p.source_flags=8;};
    options.pre_player_update_owner=[&](MultiplayerMatch &match) {
      for(const auto player:players) {
        const auto *p=match.player_from_datum(player);check(p,"Source melee player disappeared");
        if(!(p->source_flags&1u))match.apply_source_player_activation(player,p->source_flags,p->source_flags|1u);
      }
    };
  }
  if(cancel_switch || flight_swap || rapid)options.variant.values.set(94,(flight_swap || rapid)?17:5);
  unsigned representation=2;
  if(const auto raw=std::getenv("HALO2_SWORD_REPRESENTATION")) {
    const std::string_view value(raw);const auto parsed=std::from_chars(value.data(),value.data()+value.size(),representation);
    check(parsed.ec==std::errc{} && parsed.ptr==value.data()+value.size() && (representation==2 || representation==3),
        "HALO2_SWORD_REPRESENTATION requires original Spartan2 or Elite3");
  }
  auto profile=source_default_player_configuration();profile[0x44]=representation==3;
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
  if(sockets){options.local_players=0;options.dedicated=true;}
#endif
  if(canonical_target)options.selected_profiles.assign(options.local_players,profile);
  MultiplayerPreparedWorld world(options);auto &match=world.match;
  if(canonical_target)check(match.source_engine_owned(),"MCC target acceptance requires the complete original source engine");
  SwordLungeTrace sword_trace;
  SwordCancelWitness cancel_witness;
  SwordRapidWitness rapid_witness;
  const bool trace_sword=sword && std::getenv("HALO2_SWORD_LUNGE_TRACE");
  std::unique_ptr<LanHost> combat_host;std::unique_ptr<LanClient> combat_peer;
  std::shared_ptr<CombatBus> combat_bus;
  std::unique_ptr<NonHostMovementClient> joined_movement;
  unsigned joined_predictions{},joined_warm_predictions{},joined_receipts{},joined_presentations{};
  unsigned joined_nonwalking_samples{},joined_mode6_samples{},joined_cancel_none_mode6_samples{};
  unsigned joined_buffered_mode6_presentations{},native_physical_byte_samples{},joined_log_lines{};
  std::optional<MultiplayerState> joined_native_frame;
  const auto same_vector_bits=[](Vec3 a,Vec3 b) {
    return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
        std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
        std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);
  };
  if(network) {
    combat_bus=std::make_shared<CombatBus>();
    std::vector<OnlineAdmission> roster{{0,"Sword host",-1,{},0,false},{1,"Sword remote",-1,{},1,false}};
    for(unsigned i=0;i<roster.size();++i)roster[i].identity.front()=std::byte(i+1);
    std::uint64_t fingerprint=0x5455313553574f52ull;
    std::shared_ptr<MultiplayerDatagrams> host_endpoint=std::make_shared<CombatEndpoint>(combat_bus,host_slot),
        remote_endpoint=std::make_shared<CombatEndpoint>(combat_bus,remote_slot);
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
    if(sockets) {
      roster=sockets->roster();host_endpoint=sockets->endpoint(host_slot);remote_endpoint=sockets->endpoint(remote_slot);
      fingerprint=multiplayer_fingerprint(options.map,options.shared_maps,options.bsp,world.network_starting_weapon_identity());
      sockets->loaded(fingerprint,options.variant);
      check(match.join(roster.at(host_slot).name,-1,profile,host_slot)==host_slot,
          "Actual elected ONLINE host changed its admitted native slot");
      const std::array<unsigned,1> local{host_slot};world.bind_source_local_views(local);
    }else
#endif
      world.bind_source_local_views(world.local_slots);
    combat_host=std::make_unique<LanHost>(match,host_endpoint,fingerprint,roster,options.variant);
    const std::array<SourcePlayerConfiguration,1> remote_profile{profile};
    combat_peer=std::make_unique<LanClient>(remote_endpoint,host_slot,fingerprint,
        std::span<const std::int8_t>{},canonical_target?std::span<const SourcePlayerConfiguration>(remote_profile):
            std::span<const SourcePlayerConfiguration>{});
    std::array<Controls,multiplayer_capacity> controls{};const std::array<Controls,1> remote{};
    const auto join=[&] {
      combat_peer->send(remote);combat_host->receive(controls);combat_host->broadcast();combat_peer->receive();
      return combat_peer->ready();
    };
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
    if(sockets)sockets->wait_for_io(join,"native remote player admission");else
#endif
      for(unsigned n=0;n<32 && !combat_peer->ready();++n)join();
    check(combat_peer->ready() && combat_peer->error().empty() && combat_host->peers()==1 &&
        combat_peer->slots().size()==1 && combat_peer->slots().front()==remote_slot,
        "Sword logical-endpoint peer did not join its genuine remote source player");
    std::cout<<"Source sword topology: host_slot="<<host_slot<<" remote_slot="<<remote_slot
      <<" attacker_slot="<<attacker_slot<<" victim_slot="<<victim_slot<<'\n';
    joined_movement=std::make_unique<NonHostMovementClient>(combat_peer->state().netcode_mode,content.movement,
        cache.original_data()?cache.original_data()->unit_locomotion_deadzone_bits:std::nullopt);
  }
  const auto predict_joined_movement=[&](const Controls &controls,unsigned tick) {
    if(!joined_movement)return;
    const auto &snapshot=combat_peer->state();const auto &latest=snapshot.players[remote_slot];
    const auto &biped=multiplayer_biped(content,source_session_player_representation(latest.source_configuration,
        snapshot.source_game_type));
    if(latest.source_unit!=null_datum && latest.source_unit==match.player(remote_slot).source_current_object)
      check(biped.datum==match.unit(remote_slot).definition.datum,
          "Joined movement preview did not select its actual remote source biped definition");
    float yaw=controls.yaw,pitch=controls.pitch;
    if(const auto consumed=combat_peer->consumed_look(0)) {
      yaw=std::remainder(latest.yaw+std::remainder(controls.yaw-(*consumed)[0],6.283185307f),6.283185307f);
      pitch=std::clamp(latest.pitch+(controls.pitch-(*consumed)[1]),-1.54f,1.54f);
    }
    // Mirror the shipped joined consumer's predict-before-send call, while
    // leaving the source authority and the delivered motion-report slot alone.
    const auto report=joined_movement->predict(0,combat_peer->next_input_sequence(),controls,yaw,pitch,
        latest,biped,match.simulation().collision());
    ++joined_predictions;
    if(tick<75 && report)++joined_warm_predictions;
    if(latest.native_physical_state!=1)check(!report,
        "Joined Sword preview emitted ordinary predicted motion for a native nonwalking state");
  };
  const auto reconcile_joined_movement=[&] {
    if(!joined_movement)return;
    const auto states=combat_peer->received_states();const auto receipts=combat_peer->received_inputs();
    check(states.size()==receipts.size(),"Joined Sword preview lost an actual received-state input receipt");
    const std::array<unsigned,1> slots{remote_slot};
    const auto present_native=[&](const MultiplayerState &snapshot,bool buffered) {
      const auto &native=snapshot.players[remote_slot];
      for(const float fraction:{0.f,.5f,1.f}) {
        auto shown=snapshot;joined_movement->present(shown,slots,fraction);
        const auto &presented=shown.players[remote_slot];
        if(native.native_physical_state!=1)check(presented.native_physical_state==native.native_physical_state &&
            same_vector_bits(presented.position,native.position) && same_vector_bits(presented.velocity,native.velocity),
            "Joined movement presentation replaced an actual received native nonwalking trajectory");
        ++joined_presentations;
        if(buffered)++joined_buffered_mode6_presentations;
      }
    };
    for(std::size_t n=0;n<states.size();++n) {
      const auto &snapshot=states[n];const auto &native=snapshot.players[remote_slot];
      joined_movement->reconcile(0,native,receipts[n],match.simulation().collision());
      ++joined_receipts;present_native(snapshot,false);
      if(native.connected && native.source_unit!=null_datum && native.native_physical_state!=1)
        ++joined_nonwalking_samples;
      if(native.connected && !native.dead && native.source_unit!=null_datum && native.native_physical_state==6) {
        ++joined_mode6_samples;
        const bool cancelled_none=cancel_witness.cancel_stores && native.source_unit==cancel_witness.unit &&
            native.melee_action.activity==null_datum;
        joined_cancel_none_mode6_samples+=cancelled_none;
        if(!joined_native_frame || cancelled_none)joined_native_frame=snapshot;
        if(joined_log_lines++<24)std::cout<<"Source joined native motion: tick="<<snapshot.tick
          <<" receipt_known="<<receipts[n].known<<" consumed="<<receipts[n].consumed
          <<" physical_state="<<unsigned(native.native_physical_state)<<" activity="<<std::hex
          <<native.melee_action.activity<<std::dec<<" cancelled_none="<<cancelled_none<<'\n';
      }
      // Exercise a genuinely received older native endpoint after ordinary
      // prediction has resumed, as the runtime's buffered playout can do.
      if(joined_native_frame && native.native_physical_state==1 &&
          native.source_unit==joined_native_frame->players[remote_slot].source_unit &&
          joined_movement->predictor(0) && joined_movement->predictor(0)->active())
        present_native(*joined_native_frame,true);
    }
  };
  const auto receive_combat_snapshot=[&] {
    if(!combat_host)return;
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
    if(sockets) {
      const auto expected=match.state().tick;
      sockets->wait_for_io([&] {
        combat_host->broadcast();combat_peer->receive();reconcile_joined_movement();
        return combat_peer->state().tick==expected;
      },"completed native snapshot receipt");
    }else
#endif
      {
        // The native decoder deliberately limits new full-snapshot assemblies
        // per real second. An unpaced logical world can reach that bound even
        // though its source clock advances normally. Pump IO at the unchanged
        // tick, as the socket fixture/runtime does; never resend input or step
        // the source world to obtain a receipt.
        const auto expected=match.state().tick;
        const auto source_tick=match.simulation().ticks();
        const auto started=std::chrono::steady_clock::now();
        const auto deadline=started+std::chrono::seconds(10);
        unsigned polls{};
        const auto diagnostics=[&](const char *stage) {
          const auto now=std::chrono::steady_clock::now();
          std::cout<<"Source logical snapshot IO: stage="<<stage<<" expected_tick="<<expected
            <<" received_tick="<<combat_peer->state().tick<<" source_tick="<<match.simulation().ticks()
            <<" polls="<<polls<<" elapsed_ms="<<std::chrono::duration_cast<std::chrono::milliseconds>(now-started).count()
            <<" rejected="<<combat_peer->rejected_packet_count()<<" buffered_bytes="<<combat_peer->snapshot_buffered_bytes()
            <<" queued_host="<<combat_bus->incoming[host_slot].size()<<" queued_remote="<<combat_bus->incoming[remote_slot].size()
            <<" received_packets="<<combat_bus->received_packets[remote_slot]
            <<" full_snapshot_starts="<<combat_bus->received_snapshot_starts[remote_slot]
            <<" first_snapshot_age_ms="<<(combat_bus->first_snapshot[remote_slot]?
                std::chrono::duration_cast<std::chrono::milliseconds>(now-*combat_bus->first_snapshot[remote_slot]).count():-1)
            <<" last_rejection="<<combat_peer->last_packet_rejection()<<'\n';
        };
        do {
          combat_host->broadcast();combat_peer->receive();reconcile_joined_movement();++polls;
          check(match.state().tick==expected && match.simulation().ticks()==source_tick,
              "Sword snapshot IO advanced the authoritative source tick");
          check(combat_peer->error().empty(),"Sword peer rejected the authoritative snapshot");
          if(combat_peer->state().tick==expected)break;
          if(polls==1)diagnostics("waiting");
          if(std::chrono::steady_clock::now()>=deadline) {
            diagnostics("timeout");
            throw FormatError("Sword peer did not receive the unchanged authoritative source tick before its IO deadline");
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }while(true);
        if(polls>1)diagnostics("received");
      }
    check(combat_peer->error().empty(),"Sword peer rejected the authoritative snapshot");
  };
  const auto source_engine_live=[&](const SourceMultiplayerEngineStateTu15 &state) {
    return state.round.phase==1 && state.round.status==1 && (state.statistics.header&255u)==1u &&
        state.recording.enabled && !state.recording.frozen && !state.game_over.active && !state.finalized;
  };
  const auto record_engine_state=[&](const char *stage) {
    if(!canonical_target)return;
    const auto source=world.source_objects->tu15_world().multiplayer_engine_state();
    const auto &published=match.source_match_statistics();
    std::cout<<"Source engine evidence: stage="<<stage<<" source_engine_owned="<<match.source_engine_owned()
      <<" tick="<<match.state().tick<<" phase="<<source.round.phase<<" status="<<source.round.status
      <<" raw_header="<<source.statistics.header<<" published_header="<<published.header
      <<" recording_enabled="<<unsigned(source.recording.enabled)<<" recording_frozen="<<unsigned(source.recording.frozen)
      <<" game_over="<<source.game_over.active<<" finalized="<<source.finalized
      <<" match_attacker_kills="<<match.player(attacker_slot).kills
      <<" match_victim_deaths="<<match.player(victim_slot).deaths<<'\n';
    for(const auto slot:{attacker_slot,victim_slot}) {
      const auto &raw=source.statistics.players[slot].fields,&copied=published.players[slot].fields;
      std::cout<<"Source engine statistics: stage="<<stage<<" slot="<<slot<<" raw=";
      for(const auto field:raw)std::cout<<field<<',';
      std::cout<<" published=";for(const auto field:copied)std::cout<<field<<',';
      std::cout<<'\n';
      check(raw==copied,"Canonical source statistics differ from their published match bank");
    }
    check(source.statistics.header==published.header,"Canonical source statistics validity differs from the published match bank");
    if(std::string_view(stage)=="preattack")check(source_engine_live(source),"Canonical target strike requires a live original score/stat phase");
  };
  if(canonical_target) {
    bool born=false;unsigned setup_ticks{};
    for(;setup_ticks<240;++setup_ticks) {
      const auto source=world.source_objects->tu15_world().multiplayer_engine_state();
      born=match.player(attacker_slot).source_current_object!=null_datum &&
          match.player(victim_slot).source_current_object!=null_datum;
      if(born && source_engine_live(source))break;
      std::array<Controls,multiplayer_capacity> neutral{};
      if(combat_peer) {
        const std::array<Controls,1> remote{};
        predict_joined_movement(remote.front(),setup_ticks);combat_peer->send(remote);
        neutral[remote_slot].fire=true;
        const auto received=[&]{combat_host->receive(neutral);return !neutral[remote_slot].fire;};
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
        if(sockets)sockets->wait_for_io(received,"canonical neutral source command");else
#endif
          received();
        check(!neutral[remote_slot].fire,"Canonical remote setup did not receive its neutral input");
      }
      match.step(neutral);
      if(combat_host) {
        receive_combat_snapshot();
      }
    }
    check(born && setup_ticks<240,"Canonical source factory/stat phase did not become live within240 ticks");
    check(!match.unit(attacker_slot).vitality.dead && !match.unit(victim_slot).vitality.dead,
        "Canonical source setup lost a genuinely born participant");
    for(const auto slot:{attacker_slot,victim_slot})check(match.unit(slot).definition.datum==
        content.player_representations.at(representation).biped.datum,
        "Canonical source factory did not retain the requested original player species");
    if(cancel_switch || flight_swap || rapid) {
      const auto equipment=world.source_objects->player_equipment(match.player(attacker_slot).source_current_object);
      const auto sword_definition=cache.slayer_starting_weapon(17),
          firearm_definition=rapid?rapid_firearm_definition:cache.slayer_starting_weapon(5);
      bool actual_sword{},actual_firearm{};
      for(unsigned i=0;i<equipment.weapons.size();++i)if(equipment.weapons[i]!=null_datum) {
        actual_sword|=equipment.definitions[i]==sword_definition;
        actual_firearm|=equipment.definitions[i]==firearm_definition;
        std::cout<<"Source retained flight inventory: index="<<i<<" object="<<std::hex<<equipment.weapons[i]
          <<" definition="<<equipment.definitions[i]<<std::dec<<'\n';
      }
      check(actual_sword && actual_firearm,"Source variant factory did not create the actual Sword/firearm inventory pair");
    }
    // Declared existing-floor positions enter only through the original
    // B95D0 placement owner, after the unmodified source factory and loadout.
    world.source_objects->tu15_world().set_object_position(
        match.player(attacker_slot).source_current_object,target_placements[0].position);
    world.source_objects->tu15_world().set_object_position(
        match.player(victim_slot).source_current_object,target_placements[1].position);
    std::cout<<"Source canonical target setup: source_engine_owned="<<match.source_engine_owned()
      <<" representation="<<representation<<" setup_ticks="<<setup_ticks<<" attacker="<<std::hex<<match.player(attacker_slot).source_current_object
      <<" victim="<<match.player(victim_slot).source_current_object<<std::dec
      <<" authored_start_distance="<<length(target_placements[1].position-target_placements[0].position)<<'\n';
  }
  unsigned firing_requests{},allocated_effects{},source_colors{};
  if(firing)match.configure_weapon_effect_owner([&](const Unit&,const WeaponEffectInput &input) {
    ++firing_requests;
    const auto effect=world.source_objects->request_weapon_effect(input);
    if(effect.instance!=null_datum)++allocated_effects;
    source_colors+=effect.colors_observed;
    std::cout<<"Source weapon effect="<<std::hex<<input.effect<<" instance="<<effect.instance
      <<" colors="<<effect.colors.a<<','<<effect.colors.b<<std::dec<<'\n';
    return effect.instance;
  });
  std::array<std::uint8_t,2> initial_grenades{};
  unsigned admitted{},contacts{},detonations{};std::uint64_t serial{},detonation_serial{};std::set<std::uint64_t> poses;
  float victim_vitality=2.f;
  float victim_body=1.f,victim_shield=1.f,victim_initial_vitality=2.f;
  bool victim_killed{};
  bool received_victim_killed{},received_victim_damaged{},received_attacker_credited{};
  unsigned received_vitality_samples{},received_contacts{};
  std::uint32_t attacked_unit=null_datum,attacked_player=null_datum;
  std::uint64_t received_fatal_tick{},received_damage_tick{};
  int victim_deaths_before{},attacker_kills_before{};
  unsigned shield_effects{};std::uint64_t effect_serial{};
  bool dash_activity{},lunge_activity{},airborne_activity{};float attack_distance{};
  bool rapid_baseline{},rapid_firearm_lock{},rapid_sword_selected{},rapid_sword_rejected{},rapid_far_fresh_control{};
  unsigned rapid_firearm_samples{},rapid_sword_samples{},rapid_far_rejections{},rapid_input_samples{},rapid_received_samples{};
  unsigned rapid_rt_ticks{},rapid_y_ticks{},rapid_x_ticks{},rapid_b_ticks{};
  int rapid_kills_before{},rapid_deaths_before{},rapid_attacker_deaths_before{};
  bool cancel_sent{},cancelled{},fresh_attack{},switched_weapon{},jump_delivered{},airborne_after_cancel{};
  unsigned cancel_tick{},reattack_tick{},post_cancel_samples{},cancelled_contacts{},post_cancel_received{};
  std::uint32_t cancelled_weapon=null_datum;
  Vec3 cancel_position{},cancel_velocity{};float post_cancel_travel{},post_cancel_height{};
  Vec3 attack_origin{},idle_previous{};
  float idle_motion{},lunge_motion{},closest_distance=1e9f;
  std::array<Controls,16> input{};input[attacker_slot].yaw=target?0.f:spawn.facing;
  std::array<std::uint32_t,2> looked_units{null_datum,null_datum};
  std::set<std::uint32_t> source_glows;
  std::set<std::uint32_t> observed_decal_definitions;
  bool source_glow_faded=false;unsigned peak_source_decals{};
  for(unsigned tick=0;tick<(strict_decal_cache?240u:180u);++tick) {
    // Cover admission, movement, strike transition and recovery, without
    // observing map construction or retaining an unbounded store journal.
    if((trace_sword || cancel || rapid) && tick==75) {
      sword_trace.owner=world.source_objects.get();sword_trace.memory=&world.source_objects->memory();
      world.source_objects->observe_physics_stores([&](std::uint32_t,const auto &store){
        if(trace_sword)sword_trace.observe(store);
        if(cancel)cancel_witness.observe(store);
        if(rapid)rapid_witness.observe(store);
      });
    }
    if(sword_trace.owner) {
      sword_trace.tick=tick;
      for(unsigned slot=0;slot<sword_trace.bodies.size();++slot)
        sword_trace.bodies[slot]=match.player(slot).source_current_object==null_datum?0u:
            sword_trace.memory->source_address(
                world.source_objects->object_body(match.player(slot).source_current_object),4);
      if(cancel) {
        cancel_witness.tick=tick;cancel_witness.memory=sword_trace.memory;
        cancel_witness.unit=match.player(attacker_slot).source_current_object;
        cancel_witness.body=sword_trace.bodies[attacker_slot];
        if(cancel_witness.body) {
          const auto offset=std::bit_cast<std::int16_t>(Bytes(cancel_witness.memory->view(cancel_witness.body+0x346u,2)).u16(0));
          check(offset>=0,"Sword cancellation witness lost its genuine retained action component");
          cancel_witness.component=cancel_witness.body+std::uint32_t(offset);
        }
      }
      if(rapid) {
        rapid_witness.tick=tick;rapid_witness.memory=sword_trace.memory;
        rapid_witness.body=sword_trace.bodies[attacker_slot];
      }
      if(!cancel && tick==130){world.source_objects->observe_physics_stores({});sword_trace.owner=nullptr;}
    }
    if(strict_decal_cache && tick==79) {
      const std::array<unsigned,1> views{0};
      match.configure_local_views(views,world.cache->load_first_person_binding(match.unit(attacker_slot).weapon.definition,2));
    }
    if(strict_decal_cache && tick>=79)
      world.source_objects->publish_native_observer_position(0,match.unit(attacker_slot).body.position+Vec3{0,0,content.player.camera_height});
    if(target && tick>1 && match.player(attacker_slot).source_current_object!=null_datum &&
        match.player(victim_slot).source_current_object!=null_datum &&
        !match.unit(attacker_slot).vitality.dead && !match.unit(victim_slot).vitality.dead) {
      const auto delta=match.unit(victim_slot).body.position-match.unit(attacker_slot).body.position;
      const float desired=std::atan2(delta.y,delta.x);
      for(const auto slot:{attacker_slot,victim_slot}) {
        const auto &player=match.player(slot);
        const auto look=world.source_objects->player_look(player.source_datum,player.source_current_object);
        check(look.has_value(),"Sword aim requires the genuine source-created look owner");
        if(looked_units[slot]!=player.source_current_object) {
          looked_units[slot]=player.source_current_object;
          std::cout<<"Source look baseline slot="<<slot<<" unit="<<std::hex<<looked_units[slot]
            <<std::dec<<" raw_yaw="<<input[slot].yaw<<" source_yaw="<<look->x<<'\n';
        }
        // First delivery constructs the source heading and binds the current
        // raw accumulator. Later input is a delta from that retained raw
        // baseline; an absolute target angle would preserve a spawn offset.
        // Correct only delivered input against the completed genuine look.
        const auto heading=desired+(slot==victim_slot?3.14159265358979323846f:0.f);
        input[slot].yaw+=std::remainder(heading-look->x,6.28318530717958647692f);
        float pitch=(cancel_jump || jump_before_attack)?
            std::atan2(slot==victim_slot?-delta.z:delta.z,std::hypot(delta.x,delta.y)):0.f;
        if(rapid) {
          const auto &shooter=match.unit(slot),&aim_target=match.unit(slot==attacker_slot?victim_slot:attacker_slot);
          const auto eye=ordinary_unit_eye(shooter.body.position,shooter.definition.camera_height,
              shooter.definition.crouch_camera_height,shooter.crouch);
          const auto aim_delta=aim_target.body.position+Vec3{0,0,aim_target.body.height*.5f}-eye;
          pitch=std::atan2(aim_delta.z,std::hypot(aim_delta.x,aim_delta.y));
        }
        input[slot].pitch+=pitch-look->y;
      }
    }
    // The original spawn chooser can choose the same declared start twice.
    // Drive the actual victim through delivered movement to establish a
    // distant target; never overwrite a source body or action selection.
    input[victim_slot].forward=!canonical_target && lunge && tick>=25 && tick<65?-1.f:0.f;
    input[attacker_slot].melee=tick==80 && !special && !grenade && !firing;
    input[attacker_slot].fire=tick>=80 && tick<80+sword_trigger_ticks && special;
    input[attacker_slot].fire|=firing && tick>=80 && tick<92;
    input[attacker_slot].grenade=tick==80 && grenade;
    input[attacker_slot].reload=false;input[attacker_slot].jump=false;input[attacker_slot].switch_weapon=false;
    if(jump_before_attack && tick==76)input[attacker_slot].jump=true;
    if(cancel) {
      if(flight_swap) {
        // Actual firearm -> Sword sequence: fire once, request the retained
        // other item, settle its original equip owner, then request the dash.
        if(tick==40)input[attacker_slot].fire=true;
        if(tick==41)input[attacker_slot].switch_weapon=true;
      }
      if(tick>=80 && !cancel_sent)input[attacker_slot].fire=true;
      if(!cancel_sent && tick>80) {
        const auto &actor=match.unit(attacker_slot);const auto &action=actor.melee_action;
        const auto physical_mode=std::to_integer<unsigned>(world.source_objects->memory().view(
            world.source_objects->object_body(actor.source_datum)+0x3dcu,1)[0]);
        if((action.activity==0x0a0005bbu || action.activity==0x130005bcu) && physical_mode==6) {
          check(action.impact==-1,"Sword cancellation did not observe a pre-impact admitted dash");
          cancel_sent=true;cancel_tick=tick;cancelled_contacts=contacts;cancelled_weapon=actor.weapon.source_datum;
          cancel_position=actor.body.position;cancel_velocity=actor.body.velocity;
          reattack_tick=tick+unsigned(std::max(2,int(action.duration)-int(action.progress)+2));
          cancel_witness.requested=true;cancel_witness.requested_tick=tick;
          input[attacker_slot].fire=false;
          input[attacker_slot].reload=!cancel_switch;input[attacker_slot].switch_weapon=cancel_switch;
          std::cout<<"Source sword cancellation input: mode="<<mode<<" tick="<<tick
            <<" source_tick="<<match.state().tick<<" activity="<<std::hex<<action.activity<<std::dec
            <<" physical_mode="<<physical_mode<<" progress="<<int(action.progress)<<" impact="<<int(action.impact)
            <<" recovery="<<int(action.recovery)<<" duration="<<int(action.duration)
            <<" reload="<<input[attacker_slot].reload<<" switch="<<input[attacker_slot].switch_weapon<<'\n';
        }
      }
      if(cancel_sent && tick>cancel_tick) {
        input[attacker_slot].fire=cancel_reattack && tick>=reattack_tick && !fresh_attack;
        input[attacker_slot].jump=cancel_jump && tick==cancel_tick+1;
        jump_delivered|=input[attacker_slot].jump;
      }
      if(tick>=110)check(cancel_sent,"Delivered Sword primary never admitted the required live mode6 dash");
      if((tick>=75 && tick<=85) || (cancel_sent && tick<=cancel_tick+4) || input[attacker_slot].fire)
        std::cout<<"Source cancellation controls: tick="<<tick<<" source_tick="<<match.state().tick
          <<" fire="<<input[attacker_slot].fire<<" reload="<<input[attacker_slot].reload
          <<" jump="<<input[attacker_slot].jump<<" switch="<<input[attacker_slot].switch_weapon<<'\n';
    }
    if(rapid) {
      input[attacker_slot].fire=input[attacker_slot].melee=input[attacker_slot].reload=input[attacker_slot].switch_weapon=false;
      if(rapid_sequence=="y-rt") {
        input[attacker_slot].switch_weapon=tick==80;input[attacker_slot].fire=tick==80;
      }else if(rapid_sequence=="y-then-rt") {
        input[attacker_slot].switch_weapon=tick==80;input[attacker_slot].fire=tick==81;
      }else if(rapid_sequence=="held-rt-y") {
        input[attacker_slot].switch_weapon=tick==80;input[attacker_slot].fire=tick>=79 && tick<=87;
      }else if(rapid_sequence=="x-y-b-rt") {
        input[attacker_slot].reload=tick==78;input[attacker_slot].switch_weapon=tick==79;
        input[attacker_slot].melee=tick==80;input[attacker_slot].fire=tick==80;
      }else if(rapid_sequence=="x-y-b-then-rt") {
        input[attacker_slot].reload=tick==78;input[attacker_slot].switch_weapon=tick==79;
        input[attacker_slot].melee=tick==80;input[attacker_slot].fire=tick==81;
      }else if(rapid_sequence=="x-y-b-rt-same") {
        input[attacker_slot].reload=input[attacker_slot].switch_weapon=input[attacker_slot].melee=input[attacker_slot].fire=tick==80;
      }else if(rapid_sequence=="y-release-rt") {
        input[attacker_slot].switch_weapon=tick==80;input[attacker_slot].fire=tick==110;
      }else {
        input[attacker_slot].switch_weapon=tick==80;input[attacker_slot].melee=tick==81;
      }
      rapid_rt_ticks+=input[attacker_slot].fire;rapid_y_ticks+=input[attacker_slot].switch_weapon;
      rapid_x_ticks+=input[attacker_slot].reload;rapid_b_ticks+=input[attacker_slot].melee;
      if(tick>=76 && tick<=100)std::cout<<"Source rapid controls: sequence="<<rapid_sequence<<" tick="<<tick
        <<" source_tick="<<match.state().tick<<" RT="<<input[attacker_slot].fire<<" Y="<<input[attacker_slot].switch_weapon
        <<" X="<<input[attacker_slot].reload<<" B="<<input[attacker_slot].melee<<'\n';
    }
    if(!target)input[attacker_slot].pitch=plasma?-1.2f:grenade?.3f:0.f;
    if(combat_peer) {
      const std::array<Controls,1> remote{input[remote_slot]};
      predict_joined_movement(remote.front(),tick);combat_peer->send(remote);
      const auto requested=remote.front();
      // Only the receive owner may populate the remote source command.
      // Poison fire so a missing assignment also fails on neutral ticks.
      input[remote_slot]={};input[remote_slot].fire=!requested.fire;
      const auto received=[&] {
        combat_host->receive(input);const auto &actual=input[remote_slot];
        return actual.fire==requested.fire && actual.forward==requested.forward &&
            actual.yaw==requested.yaw && actual.pitch==requested.pitch && actual.reload==requested.reload &&
            actual.jump==requested.jump && actual.switch_weapon==requested.switch_weapon && actual.melee==requested.melee;
      };
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
      if(sockets)sockets->wait_for_io(received,"delivered remote source command");else
#endif
        received();
      check(input[remote_slot].fire==requested.fire && input[remote_slot].forward==requested.forward &&
          input[remote_slot].yaw==requested.yaw && input[remote_slot].pitch==requested.pitch && input[remote_slot].reload==requested.reload &&
          input[remote_slot].jump==requested.jump && input[remote_slot].switch_weapon==requested.switch_weapon &&
          input[remote_slot].melee==requested.melee,
          "Sword remote control changed before its authoritative source command");
    }
    if(rapid_far && rapid_sequence=="y-release-rt" && tick==110) {
      // Observe the completed source state immediately before the fresh RT
      // delivery. Ordinary melee remains a valid range-rejection result;
      // a dash/lunge must not borrow the old firearm target or confidence.
      const auto &attacker=match.unit(attacker_slot),&victim=match.unit(victim_slot);
      const auto &player=match.player(attacker_slot);const auto &memory=world.source_objects->memory();
      check(player.source_current_object==attacker.source_datum && victim.source_datum==attacked_unit &&
          match.player(victim_slot).source_datum==attacked_player && !attacker.vitality.dead && !victim.vitality.dead,
          "Far fresh-trigger control lost its actual living source participants");
      const auto body=world.source_objects->object_body(attacker.source_datum);
      const Bytes retained(memory.view(body,0x218u));
      const auto current=std::to_integer<unsigned>(retained.slice(0x212u,1)[0]),
          requested=std::to_integer<unsigned>(retained.slice(0x216u,1)[0]);
      const Bytes command(memory.view(0x6002800cu+(player.source_datum&0xffffu)*0x5cu,0x5c));
      const auto player_record=source_raw_datum_lookup(world.source_objects->source_player_pool(),player.source_datum,
          {.read=[&](auto address,auto size){return memory.view(address,size);}});
      check(player_record!=0,"Far fresh-trigger control lost its full source player allocation");
      const Bytes player_state(memory.view(player_record,0x44u));
      const auto edge_flags=player_state.u32(0x38u),retained_flags=retained.u32(0x148u),command_flags=command.u32(0);
      const auto query_target=command.u32(0x34u);const auto query_confidence=command.f32(0x50u);
      const float distance=length(victim.body.position-attacker.body.position);
      std::cout<<"Source far fresh-trigger precondition: tick="<<tick<<" source_tick="<<match.state().tick
        <<" player="<<std::hex<<player.source_datum<<" victim="<<victim.source_datum<<" weapon="<<attacker.weapon.definition
        <<" player38="<<edge_flags<<" retained_flags="<<retained_flags<<" command_flags="<<command_flags
        <<" retained_target="<<retained.u32(0x1c8u)<<" query_target="<<query_target<<std::dec
        <<" player3F="<<std::to_integer<unsigned>(player_state.slice(0x3fu,1)[0])
        <<" player40="<<std::to_integer<unsigned>(player_state.slice(0x40u,1)[0])
        <<" current="<<unsigned(current)<<" requested="<<unsigned(requested)
        <<" retained_confidence="<<retained.f32(0x1e4u)<<" query_confidence="<<query_confidence
        <<" distance="<<distance<<" RT="<<input[attacker_slot].fire<<'\n';
      check(attacker.weapon.definition==rapid_sword_definition && current==requested &&
          player_state.u32(0x2cu)==attacker.source_datum &&
          !(edge_flags&0x40u) && !(retained_flags&0x10040u) && !(command_flags&0x10040u) &&
          input[attacker_slot].fire && !input[attacker_slot].melee && !input[attacker_slot].reload &&
          !input[attacker_slot].switch_weapon && distance>rapid_sword_range &&
          retained.u32(0x1c8u)!=victim.source_datum && retained.f32(0x1e4u)<1.f &&
          query_target!=victim.source_datum && query_confidence<1.f,
          "Far fresh-trigger control lacks settled Sword, released source flags and an immediate living-target query rejection");
      rapid_far_fresh_control=true;
    }
    const auto before_tick=match.simulation().ticks();match.step(input);
    if(primary_intent_probe && (!rapid || (match.player(attacker_slot).source_current_object!=null_datum && !match.unit(attacker_slot).vitality.dead))) {
      // This is the actual retained14FEC0 input frame originally supplied by
      // deliver_command to the source unit/action owners.
      //1889D0 emits bits6/16 from the same held trigger byte for every item;
      //1522E0 owns weapon-specific admission after this input boundary.
      const auto slot=match.player(attacker_slot).source_datum&0xffffu;
      const auto flags=Bytes(world.source_objects->memory().view(0x6002800cu+slot*0x5cu,4)).u32(0);
      const auto expected=input[attacker_slot].fire?0x10040u:0u;
      if(input[attacker_slot].fire || (flags&0x10040u)!=expected)
        std::cout<<"Source primary intent: tick="<<tick<<" selected_weapon="<<std::hex
          <<match.unit(attacker_slot).weapon.definition<<" flags="<<flags<<" expected="<<expected<<std::dec<<'\n';
      check((flags&0x10040u)==expected,
          "Native primary trigger differs from original1889D0 held bits6/16");
      if(rapid) {
        const auto expected_buttons=(input[attacker_slot].melee?0x20u:0u)|(input[attacker_slot].reload?0x10000000u:0u);
        check((flags&0x10000020u)==expected_buttons,"Rapid native melee/reload command differs from its delivered original button intents");
        ++rapid_input_samples;
      }
    }
    if(sword_trace.owner)sword_trace.finish_tick();
    if(combat_host) {
      receive_combat_snapshot();
      const auto authoritative=match.state();const auto &received=combat_peer->state();
      check(received.tick==authoritative.tick,"Sword peer did not receive the completed authoritative source tick");
      if(received.tick==authoritative.tick) {
        for(const auto slot:{attacker_slot,victim_slot}) {
          const auto &native=authoritative.players[slot],&delivered=received.players[slot];
          check(delivered.source_unit==native.source_unit && delivered.native_physical_state==native.native_physical_state,
              "Sword peer changed the actual native biped physical-state byte");
          if(native.source_unit!=null_datum) {
            const auto motion=world.source_objects->biped_motion(native.source_unit);
            const auto body=world.source_objects->object_body(native.source_unit);
            const auto raw=std::to_integer<std::uint8_t>(world.source_objects->memory().view(body+0x3dcu,1)[0]);
            check(native.native_physical_state==raw && motion.physical_state==raw,
                "Sword authoritative/received physical state differs from its actual native byte/getter");
            ++native_physical_byte_samples;
          }
        }
        const auto &actual=received.players[victim_slot],&expected=authoritative.players[victim_slot];
        const auto &actual_attacker=received.players[attacker_slot],
            &expected_attacker=authoritative.players[attacker_slot];
        check(actual.source_unit==expected.source_unit && actual.dead==expected.dead &&
            actual.health==expected.health && actual.shield==expected.shield,
            "Sword victim identity/vitality/death changed across the ONLINE-shaped transport");
        check(actual_attacker.source_player==expected_attacker.source_player &&
            actual_attacker.kills==expected_attacker.kills,
            "Sword attacker identity/kill credit changed across the ONLINE-shaped transport");
        if(cancel || rapid) {
          const auto same_vector=[](Vec3 a,Vec3 b){return a.x==b.x && a.y==b.y && a.z==b.z;};
          check(actual.deaths==expected.deaths && actual_attacker.deaths==expected_attacker.deaths &&
              actual_attacker.source_unit==expected_attacker.source_unit && actual_attacker.source_weapon==expected_attacker.source_weapon &&
              same_vector(actual_attacker.position,expected_attacker.position) && same_vector(actual_attacker.velocity,expected_attacker.velocity) &&
              actual_attacker.melee_action.activity==expected_attacker.melee_action.activity &&
              actual_attacker.melee_action.progress==expected_attacker.melee_action.progress &&
              actual_attacker.melee_action.impact==expected_attacker.melee_action.impact &&
              actual_attacker.melee_action.recovery==expected_attacker.melee_action.recovery &&
              actual_attacker.melee_action.duration==expected_attacker.melee_action.duration,
              "Sword cancellation movement/action/weapon/stat state changed across the admitted transport");
          if(cancel_sent && tick>=cancel_tick)++post_cancel_received;
          if(rapid)++rapid_received_samples;
        }
        ++received_vitality_samples;
        const auto &body=match.unit(victim_slot);
        if(tick>80 && body.source_datum==attacked_unit && actual.source_player==attacked_player &&
            actual.health+actual.shield<victim_initial_vitality) {
          if(!received_victim_damaged) {
            received_damage_tick=received.tick;
            std::cout<<"Received sword damage tick="<<received_damage_tick<<" target="<<std::hex
              <<attacked_unit<<std::dec<<" body="<<actual.health<<" shield="<<actual.shield
              <<" dead="<<actual.dead<<'\n';
          }
          received_victim_damaged=true;
        }
        if(tick>80 && body.source_datum==attacked_unit && body.vitality.dead && actual.dead &&
            actual.source_player==attacked_player && actual.deaths==victim_deaths_before+1) {
          received_victim_killed=true;received_fatal_tick=received.tick;
        }
        if(tick>80 && actual_attacker.source_player==match.player(attacker_slot).source_datum &&
            actual_attacker.kills==attacker_kills_before+1)received_attacker_credited=true;
        check(received.melee_impacts.size()==authoritative.melee_impacts.size(),
            "Sword peer lost an authoritative contact");
        for(unsigned n=0;n<received.melee_impacts.size();++n) {
          const auto &a=received.melee_impacts[n],&e=authoritative.melee_impacts[n];
          check(a.serial==e.serial && a.attacker==e.attacker && a.weapon==e.weapon &&
              a.damage==e.damage && a.material==e.material,
              "Sword peer changed the source contact identity/damage/material");
        }
        received_contacts=std::max(received_contacts,unsigned(received.melee_impacts.size()));
      }
    }
    if(strict_decal_cache)world.source_objects->advance_effect_frame(
        match.simulation().ticks()==before_tick+1?tick_seconds:0.f);
    const auto state=match.state();const auto &unit=match.unit(attacker_slot);
    if(rapid && tick>=75 && match.player(attacker_slot).source_current_object!=null_datum && !unit.vitality.dead) {
      const auto body=world.source_objects->object_body(unit.source_datum);const auto &memory=world.source_objects->memory();
      const auto current=std::to_integer<unsigned>(memory.view(body+0x212u,1)[0]),
          requested=std::to_integer<unsigned>(memory.view(body+0x216u,1)[0]);
      const auto target_datum=Bytes(memory.view(body+0x1c8u,4)).u32(0);
      const auto confidence=Bytes(memory.view(body+0x1e4u,4)).f32(0);
      const auto slot=match.player(attacker_slot).source_datum&0xffffu;
      const Bytes command(memory.view(0x6002800cu+slot*0x5cu,0x5c));
      const auto query_target=command.u32(0x34u);const auto query_confidence=command.f32(0x50u);
      const float distance=length(match.unit(victim_slot).body.position-unit.body.position);
      if(unit.weapon.definition==rapid_firearm_definition && tick<78) {
        ++rapid_firearm_samples;
        rapid_firearm_lock|=current==requested && target_datum==match.unit(victim_slot).source_datum && confidence>=1.f &&
            query_target==target_datum && query_confidence>=1.f;
      }
      if(tick==77) {
        rapid_baseline=true;rapid_kills_before=match.player(attacker_slot).kills;rapid_deaths_before=match.player(victim_slot).deaths;
        rapid_attacker_deaths_before=match.player(attacker_slot).deaths;
        check(unit.weapon.definition==rapid_firearm_definition && rapid_firearm_lock && !match.unit(victim_slot).vitality.dead,
            "Rapid sequence lacks a genuine live full-confidence firearm target before its first button input");
        record_engine_state("rapid-firearm-baseline");
      }
      if(unit.weapon.definition==rapid_sword_definition && current==requested) {
        rapid_sword_selected=true;++rapid_sword_samples;
        if(rapid_far && distance>rapid_sword_range && !match.unit(victim_slot).vitality.dead &&
            query_target!=match.unit(victim_slot).source_datum && query_confidence<1.f) {
          rapid_sword_rejected=true;++rapid_far_rejections;
        }
      }
      if(tick<=105 || tick%15==0)std::cout<<"Source rapid target state: tick="<<tick<<" source_tick="<<state.tick
        <<" weapon="<<std::hex<<unit.weapon.definition<<" target="<<target_datum<<" command_target="<<query_target<<std::dec
        <<" confidence="<<confidence<<" command_confidence="<<query_confidence<<" current="<<current<<" requested="<<requested
        <<" distance="<<distance<<" activity="<<std::hex<<unit.melee_action.activity<<std::dec
        <<" physical_mode="<<unsigned(std::to_integer<std::uint8_t>(memory.view(body+0x3dcu,1)[0]))<<'\n';
    }
    if(cancel && cancel_sent && tick>=cancel_tick) {
      if(!cancelled && cancel_witness.cancel_stores && cancel_witness.active_clears) {
        check(!unit.melee_action.active(),"Delivered cancellation retained a live old melee activity");
        cancelled=true;record_engine_state("cancelled");
      }
      if(cancel_reattack && tick>=reattack_tick && unit.melee_action.active()) {
        fresh_attack=true;cancel_witness.fresh_attack=true;
      }
      if(cancel_switch && unit.weapon.source_datum!=cancelled_weapon)switched_weapon=true;
      if(tick>cancel_tick) {
        ++post_cancel_samples;airborne_after_cancel|=!unit.body.grounded;
        post_cancel_travel=std::max(post_cancel_travel,length(unit.body.position-cancel_position));
        post_cancel_height=std::max(post_cancel_height,unit.body.position.z-cancel_position.z);
        if(tick<=cancel_tick+8 || tick%15==0) {
          const auto body=world.source_objects->object_body(unit.source_datum);
          const auto &memory=world.source_objects->memory();
          std::cout<<"Source sword physical continuation: tick="<<tick<<" source_tick="<<state.tick
            <<" mode="<<unsigned(std::to_integer<std::uint8_t>(memory.view(body+0x3dcu,1)[0]))
            <<" clock="<<std::bit_cast<std::int32_t>(Bytes(memory.view(body+0x3ecu,4)).u32(0))
            <<" activity="<<std::hex<<unit.melee_action.activity<<std::dec
            <<" position="<<unit.body.position.x<<','<<unit.body.position.y<<','<<unit.body.position.z
            <<" velocity="<<unit.body.velocity.x<<','<<unit.body.velocity.y<<','<<unit.body.velocity.z
            <<" grounded="<<unit.body.grounded<<'\n';
        }
      }
      check(cancel_witness.old_impacts==0,"Cancelled Sword action later dispatched its old original impact");
      if(!fresh_attack)check(contacts==cancelled_contacts && !match.unit(victim_slot).vitality.dead &&
          match.unit(victim_slot).vitality.body+match.unit(victim_slot).vitality.shield>=victim_initial_vitality &&
          match.player(attacker_slot).kills==attacker_kills_before && match.player(victim_slot).deaths==victim_deaths_before,
          "Cancelled Sword action manufactured contact, damage, death or kill credit before a fresh attack");
    }
    if(plasma) {
      const auto decals=source_decal_vertex_cache_tu15(world.source_objects->memory());
      peak_source_decals=std::max(peak_source_decals,unsigned(decals.size()));
      for(const auto &decal:decals)if(observed_decal_definitions.insert(decal.definition).second)
        std::cout<<"Source cached decal definition="<<std::hex<<decal.definition<<std::dec
                 <<" name="<<world.cache->query_tag({.handle=TagHandle{decal.definition}})->name
                 <<" layer="<<unsigned(decal.layer)<<" lifetime="<<decal.lifetime
                 <<" decay="<<decal.decay<<" birth="<<decal.tick<<" observed="<<state.tick
                 <<" alpha="<<(decal.tint>>24)<<" quads="<<decal.quads.size()<<'\n';
      for(const auto &decal:decals)if(decal.layer==5 && decal.lifetime>0) {
        source_glows.insert(decal.datum);
        const auto alpha=decal.tint>>24;
        source_glow_faded|=alpha>0 && alpha<255;
      }
    }
    if(lunge && tick>=65 && tick<=79) {
      if(tick>65)idle_motion=std::max(idle_motion,length(unit.body.position-idle_previous));
      idle_previous=unit.body.position;
    }
    if(tick==1) {
      initial_grenades=unit.grenades;
      if(plasma)check(initial_grenades[1]>0,"Original Sword starting equipment did not supply plasma grenades");
    }
    if(lunge && tick%20==0) {
      const auto p=match.unit(victim_slot).body.position;
      std::cout<<"Source moving target tick="<<tick<<" position="<<p.x<<','<<p.y<<','<<p.z<<'\n';
    }
    if(target && tick==79) {
      record_engine_state("preattack");
      attack_distance=length(match.unit(victim_slot).body.position-unit.body.position);
      attack_origin=unit.body.position;
      std::cout<<"Source preattack target distance="<<attack_distance<<'\n';
      const auto body=world.source_objects->object_body(unit.source_datum);
      const Bytes targeting(world.source_objects->memory().view(body+0x1c8u,0x24));
      std::cout<<"Source aim target="<<std::hex<<targeting.u32(0)<<std::dec
        <<" confidence="<<targeting.f32(0x1c)<<'\n';
      const auto &victim=match.unit(victim_slot);
      victim_body=victim.vitality.body;victim_shield=victim.vitality.shield;
      victim_initial_vitality=victim_body+victim_shield;
      attacked_unit=victim.source_datum;attacked_player=match.player(victim_slot).source_datum;
      victim_deaths_before=match.player(victim_slot).deaths;
      attacker_kills_before=match.player(attacker_slot).kills;
      std::cout<<"Source preattack target="<<std::hex<<victim.source_datum<<std::dec
        <<" body="<<victim_body<<" shield="<<victim_shield
        <<" dead="<<victim.vitality.dead<<'\n';
      if(lunge || close_sword)check(!victim.vitality.dead && victim_body>0 && victim_shield>0,
          "Lunge acceptance requires an alive shielded target before the strike");
    }
    if(lunge && tick>80) {
      lunge_motion=std::max(lunge_motion,length(unit.body.position-attack_origin));
      closest_distance=std::min(closest_distance,length(match.unit(victim_slot).body.position-unit.body.position));
    }
    for(const auto &event:state.unit_damage_effects)if(event.serial>effect_serial) {
      effect_serial=event.serial;
      if(target && event.target==match.unit(victim_slot).source_datum) {
        ++shield_effects;
        std::cout<<"Source shield effect tick="<<event.tick<<" definition="<<std::hex<<event.effect.datum<<std::dec<<'\n';
      }
    }
    if(target && tick>80) {
      const auto &victim=match.unit(victim_slot);
      victim_vitality=std::min(victim_vitality,victim.vitality.body+victim.vitality.shield);
      if(victim.vitality.body!=victim_body || victim.vitality.shield!=victim_shield ||
          (victim.vitality.dead && !victim_killed)) {
        std::cout<<"Source target vitality tick="<<state.tick<<" target="<<std::hex
          <<victim.source_datum<<std::dec<<" body="<<victim.vitality.body
          <<" shield="<<victim.vitality.shield<<" dead="<<victim.vitality.dead<<'\n';
        victim_body=victim.vitality.body;victim_shield=victim.vitality.shield;
      }
      if(!victim_killed && victim.source_datum==attacked_unit && victim.vitality.dead)record_engine_state("fatal");
      victim_killed|=victim.source_datum==attacked_unit && victim.vitality.dead;
    }
    if(tick==80 && !rapid)check((firing || bool(content.weapons.at(unit.weapon.definition).inventory_flags&(1u<<24))==sword) &&
        world.source_objects->object_definition(unit.weapon.source_datum)==unit.weapon.definition,
        "Original initial equipment did not select the requested weapon kind");
    if(unit.melee_action.active()) {
      const auto activity=unit.melee_action.activity;
      dash_activity|=activity==0x0a0005bbu || activity==0x130005bcu;
      lunge_activity|=activity==0x0b0005b2u || activity==0x140005b3u;
      airborne_activity|=activity==0x130005bcu || activity==0x140005b3u;
      ++admitted;const auto nodes=world.source_objects->world_nodes(unit.source_datum);
      check(!nodes.empty(),"Actual melee body lost its WORLD nodes");std::uint64_t hash=1469598103934665603ull;
      const auto root=inverse_transform(nodes.front());
      for(const auto &node:nodes) {
        const auto relative=compose_transform(root,node);
        for(const auto &axis:relative.basis)for(float value:{axis.x,axis.y,axis.z}) {
          check(std::isfinite(value),"Source melee pose contains invalid matrices");
          hash=(hash^std::bit_cast<std::uint32_t>(value))*1099511628211ull;
        }
      }
      poses.insert(hash);
      std::cout<<"Source melee tick="<<state.tick<<" activity="<<std::hex<<unit.melee_action.activity<<std::dec
          <<" progress="<<int(unit.melee_action.progress)<<" impact="<<int(unit.melee_action.impact)<<'\n';
    }
    for(const auto &impact:state.melee_impacts)if(impact.serial>serial) {
      serial=impact.serial;++contacts;check(impact.material>=0,"Source melee lost actual material");
      std::cout<<"Source contact tick="<<impact.tick<<" material="<<impact.material
        <<" damage="<<std::hex<<impact.damage<<" weapon="<<impact.weapon
        <<" attacker="<<impact.attacker<<std::dec<<'\n';
    }
    for(const auto &event:state.grenade_detonations)if(event.serial>detonation_serial) {
      detonation_serial=event.serial;++detonations;
      check(!event.source_locations.empty(),"Source explosion lost its completed named locations");
      std::cout<<"Source detonation tick="<<event.tick<<" locations="<<event.source_locations.size()<<'\n';
      std::cout<<"Source explosion controls="<<event.controls.a<<','<<event.controls.b<<'\n';
    }
  }
  if(joined_movement) {
    const auto &stats=joined_movement->stats(0);
    std::cout<<"Source joined movement result: remote_slot="<<remote_slot<<" attacker_slot="<<attacker_slot
      <<" predict_calls="<<joined_predictions<<" warm_predictions="<<joined_warm_predictions
      <<" predicted="<<stats.predicted<<" reconciled="<<stats.reconciled<<" receipts="<<joined_receipts
      <<" presentations="<<joined_presentations<<" nonwalking_samples="<<joined_nonwalking_samples
      <<" mode6_samples="<<joined_mode6_samples<<" cancel_none_mode6_samples="<<joined_cancel_none_mode6_samples
      <<" buffered_mode6_presentations="<<joined_buffered_mode6_presentations
      <<" native_byte_samples="<<native_physical_byte_samples<<'\n';
    check(joined_predictions>0 && joined_warm_predictions>0 && stats.predicted>0 && joined_receipts>0 &&
        joined_presentations>=3 && native_physical_byte_samples>0,
        "Sword joined-consumer preview lacks warm ordinary prediction and real native receipt/presentation coverage");
    if(attacker_slot==remote_slot && lunge)check(joined_mode6_samples>0,
        "Remote Sword lunge/cancellation lacked an actual received native mode6 presentation witness");
  }
#if defined(HALO2_SWORD_SOCKET_FIXTURE)
  if(sockets) {
    sockets->health();const auto report=std::getenv("HALO2_SWORD_ONLINE_REPORT");
    check(report && *report,"Actual ONLINE socket qualification requires its bounded owned report path");
    sockets->write_report(report);
  }
#endif
  if(rapid) {
    record_engine_state("rapid-final");
    const bool target_cleared=std::all_of(rapid_witness.target_clears.begin(),rapid_witness.target_clears.end(),
        [](unsigned count){return count>0;});
    std::cout<<"Source rapid result: mode="<<mode<<" sequence="<<rapid_sequence<<" representation="<<representation
      <<" firearm_option="<<rapid_firearm_option<<" firearm_lock="<<rapid_firearm_lock<<" firearm_samples="<<rapid_firearm_samples
      <<" sword_selected="<<rapid_sword_selected<<" sword_samples="<<rapid_sword_samples
      <<" far_sword_query_rejected="<<rapid_sword_rejected<<" far_rejections="<<rapid_far_rejections
      <<" far_fresh_control="<<rapid_far_fresh_control
      <<" selector_commits="<<rapid_witness.selector_commits<<" target_clear_stores=";
    for(const auto count:rapid_witness.target_clears)std::cout<<count<<',';
    std::cout<<" admission_phase_samples="<<rapid_witness.admission_samples<<" input_samples="<<rapid_input_samples
      <<" received_samples="<<rapid_received_samples<<" RT_ticks="<<rapid_rt_ticks<<" Y_ticks="<<rapid_y_ticks
      <<" X_ticks="<<rapid_x_ticks<<" B_ticks="<<rapid_b_ticks<<" dash="<<dash_activity<<" lunge="<<lunge_activity
      <<" active_frames="<<admitted<<" contacts="<<contacts<<" minimum_vitality="<<victim_vitality
      <<" kills_added="<<match.player(attacker_slot).kills-rapid_kills_before
      <<" deaths_added="<<match.player(victim_slot).deaths-rapid_deaths_before
      <<" outcome="<<(lunge_activity?"source-lunge":admitted?"source-other-melee":"source-no-melee")<<'\n';
    check(rapid_baseline && rapid_firearm_lock && rapid_firearm_samples>0 && rapid_sword_selected && rapid_sword_samples>0 &&
        rapid_witness.selector_commits>0 && target_cleared && rapid_witness.admission_samples>0 &&
        rapid_input_samples>0 && rapid_received_samples>0 && rapid_y_ticks==1,
        "Rapid source factory/target/selector/input/received-state qualification is incomplete");
    const unsigned expected_rt=rapid_sequence=="y-b"?0u:rapid_sequence=="held-rt-y"?9u:1u;
    const unsigned expected_x=(rapid_sequence=="x-y-b-rt" || rapid_sequence=="x-y-b-then-rt" || rapid_sequence=="x-y-b-rt-same")?1u:0u;
    const unsigned expected_b=(expected_x || rapid_sequence=="y-b")?1u:0u;
    check(rapid_rt_ticks==expected_rt && rapid_x_ticks==expected_x && rapid_b_ticks==expected_b,
        "Rapid fixture did not deliver exactly its declared finite original button sequence");
    if(rapid_far)check(rapid_sword_rejected && rapid_far_rejections>0,
        "Far rapid case lacks a genuine living firearm target and later beyond-range Sword query rejection");
    if(rapid_far && rapid_sequence=="y-release-rt")check(rapid_far_fresh_control && !dash_activity && !lunge_activity &&
        contacts==0 && received_contacts==0 && !victim_killed && !received_victim_killed && !received_attacker_credited &&
        match.player(attacker_slot).kills==rapid_kills_before && match.player(victim_slot).deaths==rapid_deaths_before &&
        match.player(attacker_slot).deaths==rapid_attacker_deaths_before,
        "Far fresh-trigger range rejection admitted a dash/lunge, melee contact, death or kill credit");
    if(rapid_sequence=="y-release-rt" && !rapid_far)check(dash_activity && lunge_activity && contacts>0 && victim_killed &&
        received_victim_killed && received_attacker_credited && match.player(attacker_slot).kills==rapid_kills_before+1 &&
        match.player(victim_slot).deaths==rapid_deaths_before+1,
        "Fresh primary after settled equip did not produce and deliver its canonical lethal Sword lunge");
    std::cout<<"PASS canonical TU1.5 rapid source admission/rejection and exact received state\n";return 0;
  }
  if(cancel) {
    record_engine_state("cancel-final");
    std::cout<<"Source sword cancellation result: mode="<<mode<<" representation="<<representation
      <<" dash_admitted="<<dash_activity<<" cancelled="<<cancelled<<" cancel_stores="<<cancel_witness.cancel_stores
      <<" active_bit_clears="<<cancel_witness.active_clears<<" old_impacts="<<cancel_witness.old_impacts
      <<" reload_interrupts="<<cancel_witness.reload_interrupts<<" switch_interrupts="<<cancel_witness.switch_interrupts
      <<" activity_byte_contracts="<<cancel_witness.activity_byte_contracts<<" active_byte_contracts="<<cancel_witness.active_byte_contracts
      <<" physical_stores="<<cancel_witness.physical_stores<<" continuation_samples="<<post_cancel_samples
      <<" received_samples="<<post_cancel_received<<" jump_delivered="<<jump_delivered
      <<" airborne_after_cancel="<<airborne_after_cancel<<" airborne_activity="<<airborne_activity<<" fresh_attack="<<fresh_attack
      <<" switched_weapon="<<switched_weapon<<" travel="<<post_cancel_travel<<" height_gain="<<post_cancel_height
      <<" incoming_velocity="<<cancel_velocity.x<<','<<cancel_velocity.y<<','<<cancel_velocity.z<<'\n';
    check(attack_distance>1.5f && dash_activity && cancel_sent && cancelled && cancel_witness.cancel_stores &&
        cancel_witness.active_clears && !cancel_witness.old_impacts && cancel_witness.physical_stores &&
        post_cancel_samples>=8 && post_cancel_received>=8,
        "Canonical Sword cancellation/action/physical/received-state witnesses are incomplete");
    check(cancel_switch?cancel_witness.switch_interrupts>0:cancel_witness.reload_interrupts>0,
        "Sword interruption did not pass through the requested original reload/switch caller for its actual attacker");
    check(cancel_witness.activity_byte_contracts && cancel_witness.active_byte_contracts,
        "Live Sword cancellation did not reproduce the complete original two-store retained-body contract");
    if(cancel_jump)check(jump_delivered && airborne_after_cancel && post_cancel_height>0.f,
        "Post-cancel original jump did not produce actual airborne physical continuation");
    if(jump_before_attack)check(airborne_activity,"Delivered jump did not admit an actual original airborne Sword activity");
    if(cancel_switch)check(switched_weapon,"Source switch request did not select its genuine retained firearm");
    if(cancel_reattack)check(fresh_attack,"Delivered primary did not admit a fresh Sword action after original recovery");
    if(victim_killed)check(received_victim_killed && received_attacker_credited &&
        match.player(attacker_slot).kills==attacker_kills_before+1 && match.player(victim_slot).deaths==victim_deaths_before+1,
        "Fresh post-cancel attack lost its exact source and received fatal statistics");
    else check(match.player(attacker_slot).kills==attacker_kills_before &&
        match.player(victim_slot).deaths==victim_deaths_before,"Nonfatal cancellation sequence manufactured source statistics");
    std::cout<<"PASS canonical TU1.5 delivered Sword cancellation and observed physical continuation\n";return 0;
  }
  if(firing) {
    check(firing_requests>0 && source_colors>0,"Prepared shot never reached its actual TU1.5 color owner");
    std::cout<<"PASS actual TU1.5 prepared-shot creation/colors; retained allocations="<<allocated_effects<<'\n';return 0;
  }
  if(grenade) {
    if(strict_decal_cache) {
      const auto remaining=source_decal_vertex_cache_tu15(world.source_objects->memory());
      check(!source_glows.empty() && source_glow_faded && peak_source_decals>0,
          "Actual plasma source-cache glow never created/faded");
      check(std::none_of(remaining.begin(),remaining.end(),[&](const auto &decal){return source_glows.contains(decal.datum);}),
          "Actual plasma source-cache glow outlived its original lifetime");
      std::cout<<"Source cached decal peak="<<peak_source_decals<<" glow identities="<<source_glows.size()
               <<"; actual packed alpha faded and original cache allocations retired\n";
    }
    const auto counts=match.unit(attacker_slot).grenades;
    check(detonations==1 && int(counts[0])+counts[1]==int(initial_grenades[0])+initial_grenades[1]-1,
      "Live TU1.5 grenade detonation/count acceptance incomplete");
    LanHost host(match,0,0x545531354558504cull);
    LanClient peer(host.join_address(),0x545531354558504cull,1);
    for(unsigned n=0;n<64 && !peer.ready();++n) {peer.send(std::span(input).first(1));host.receive(input);host.broadcast();peer.receive();}
    check(peer.ready() && peer.error().empty(),"Source explosion LAN handshake incomplete");
    host.broadcast();peer.receive();
    const auto &actual=peer.state().grenade_detonations;const auto expected=match.state().grenade_detonations;
    check(actual.size()==expected.size() && !actual.empty(),"Source explosion LAN journal differs");
    const auto &a=actual.back(),&e=expected.back();
    check(a.colors.a==e.colors.a && a.colors.b==e.colors.b && a.controls.a==e.controls.a && a.controls.b==e.controls.b && a.source_locations.size()==e.source_locations.size(),
        "Source explosion LAN lost colors/locations");
    for(unsigned n=0;n<a.source_locations.size();++n) {
      const auto &x=a.source_locations[n],&y=e.source_locations[n];
      check(x.name==y.name && x.frame.scale==y.frame.scale && x.frame.position.x==y.frame.position.x &&
          x.frame.position.y==y.frame.position.y && x.frame.position.z==y.frame.position.z,"Source explosion LAN location differs");
      for(unsigned k=0;k<3;++k)check(x.frame.basis[k].x==y.frame.basis[k].x && x.frame.basis[k].y==y.frame.basis[k].y &&
          x.frame.basis[k].z==y.frame.basis[k].z,"Source explosion LAN basis differs");
    }
    const auto expected_state=match.state();
    for(const auto &player:expected_state.players)if(player.connected && player.source_weapon!=null_datum) {
      const auto pose=std::find_if(expected_state.source_body_poses.begin(),expected_state.source_body_poses.end(),
          [&](const auto &p){return p.object==player.source_weapon;});
      check(pose!=expected_state.source_body_poses.end(),"Source snapshot omitted the actual held weapon WORLD bank");
      const auto received=std::find_if(peer.state().source_body_poses.begin(),peer.state().source_body_poses.end(),
          [&](const auto &p){return p.object==player.source_weapon;});
      check(received!=peer.state().source_body_poses.end() && received->nodes.size()==pose->nodes.size(),
          "Source held weapon WORLD bank was lost across LAN");
      for(unsigned n=0;n<pose->nodes.size();++n) {
        const auto &x=pose->nodes[n],&y=received->nodes[n];
        check(x.scale==y.scale && x.position.x==y.position.x && x.position.y==y.position.y && x.position.z==y.position.z,
            "Source held weapon WORLD position changed across LAN");
        for(unsigned k=0;k<3;++k)check(x.basis[k].x==y.basis[k].x && x.basis[k].y==y.basis[k].y && x.basis[k].z==y.basis[k].z,
            "Source held weapon WORLD orientation changed across LAN");
      }
    }
    std::cout<<"PASS actual TU1.5 grenade detonation and source count debit\n";return 0;
  }
  if(target) {
    if(lunge)std::cout<<"Source lunge result: minimum_vitality="<<victim_vitality
      <<" victim_killed="<<victim_killed<<" contacts="<<contacts
      <<" shield_effects="<<shield_effects<<'\n';
    if(network)std::cout<<"Source sword network result: mode="<<mode
      <<" vitality_samples="<<received_vitality_samples<<" received_contacts="<<received_contacts
      <<" received_victim_damaged="<<received_victim_damaged<<" received_damage_tick="<<received_damage_tick
      <<" received_victim_killed="<<received_victim_killed<<" received_fatal_tick="<<received_fatal_tick
      <<" received_attacker_credited="<<received_attacker_credited
      <<" deaths_added="<<match.player(victim_slot).deaths-victim_deaths_before
      <<" attacker_kills_added="<<match.player(attacker_slot).kills-attacker_kills_before<<'\n';
    check(admitted>2 && poses.size()>2 && victim_vitality<2.f,"Live TU1.5 unit melee damage/pose acceptance incomplete");
    if(sword)check(shield_effects>0,"Live TU1.5 sword lost its source shield effect request");
    if(lunge) {
      std::cout<<"Source lunge movement="<<lunge_motion<<" idle_step="<<idle_motion
        <<" closest_target_distance="<<closest_distance<<'\n';
      check(attack_distance>1.5f && dash_activity && lunge_activity,
          "Live TU1.5 target admission did not select the original dash and lunge activities");
      if(jump_before_attack)check(airborne_activity,"Delivered jump did not select an actual original airborne Sword activity");
      check(lunge_motion>idle_motion && closest_distance<attack_distance,
          "Source lunge activity did not produce real closing movement");
      check(victim_killed,"TU1.5 sword lunge did not kill its ordinary shielded player target");
    }
    if(close_sword)check(attack_distance<=1.5f && contacts>0 && victim_vitality<victim_initial_vitality,
        "Close sword control did not deliver authored contact and damage at close range");
    if(network) {
      check(received_vitality_samples>0 && received_victim_damaged && received_contacts>0,
          "Sword damage/contact never reached the remote participant's received state");
      if(lunge || victim_killed) {
        check(received_victim_killed && received_attacker_credited &&
            match.player(victim_slot).deaths==victim_deaths_before+1 &&
            match.player(attacker_slot).kills==attacker_kills_before+1,
            "Fatal sword strike did not preserve its received death and single victim/attacker credit");
      } else check(match.player(victim_slot).deaths==victim_deaths_before &&
          match.player(attacker_slot).kills==attacker_kills_before,
          "Nonfatal sword contact manufactured death or kill credit");
    }
    std::cout<<"PASS actual TU1.5 unit melee: target minimum vitality="<<victim_vitality<<'\n';return 0;
  }
  check(admitted>2 && poses.size()>2 && contacts==1,"Live TU1.5 melee action/pose/contact acceptance incomplete");
  std::cout<<"PASS actual TU1.5 melee: "<<admitted<<" active frames, "<<poses.size()<<" relative poses, "<<contacts<<" real map contact\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
