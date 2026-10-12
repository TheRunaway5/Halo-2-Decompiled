#include "halo2/native_vista_session.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string& text) {
  std::vector<std::byte> out;for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return out;
}
std::uint32_t get(std::span<const std::byte> b,std::size_t o) {
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[o+i])<<(8*i);return v;
}
void put(std::span<std::byte> b,std::size_t o,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[o+i]=std::byte(v>>(8*i));
}
template<class F>void rejects(F f) {try{f();}catch(const std::exception&){return;}throw std::runtime_error("Missing native admission rejection");}
void native_owner(NativeVistaWorldKind world) {
#if defined(__linux__)
  NativeVistaSession owner({.language=0,.interface_name="lo"});
#else
  NativeVistaSession owner({.language=0,.interface_name={}});
#endif
  check(!owner.signed_in() && get(owner.bank(),0x73a0)==0,"Native owner fabricated initial sign-in/session state");
  check(owner.network_busy()==0,"Native owner differs from original initial network mode");
  const auto connection=owner.connection_task_queries();
  check(connection.state==0 && !connection.blocks && !connection.gate_a && !connection.gate_b,"Native owner fabricated a connection task");
  owner.freeze_network_clock(true);const auto frozen=owner.network_clock();
  for(unsigned i=0;i<8;++i) {
    (void)owner.raw_network_clock();
    check(owner.network_clock()==frozen && owner.elapsed_network_clock(frozen+1)==0xffffffffu,"Native sampled network clock or wrap differs");
  }
  owner.freeze_network_clock(false);
  const auto before_clock=owner.raw_network_clock();const auto live_clock=owner.network_clock();const auto after_clock=owner.raw_network_clock();
  check(live_clock-before_clock<=after_clock-before_clock,"Native network clock did not return to actual OS time");
  const auto unchanged_bank=std::vector(owner.bank().begin(),owner.bank().end());
  for(const auto event:std::array<std::array<std::uint32_t,2>,4>{{{2,0},{5,0},{0x8001,4},{0x8001,5}}})
    check(owner.set_user_context(event[0],event[1])==NativeVistaContextPublication::unpublished_offline,"Native offline context event claimed external publication");
  check(owner.context_events().size()==4 && owner.context_events()[2].controller==0 &&
    owner.context_events()[2].context==0x8001 && owner.context_events()[2].value==4,"Native offline context sink changed the source event");
  check(std::equal(unchanged_bank.begin(),unchanged_bank.end(),owner.bank().begin()),"Native rich-presence event changed selected session configuration");
  rejects([&]{owner.set_user_context(0,0);});
  const auto failed=owner.birth_local();check(!failed.accepted,"Session birth succeeded without an actual offline identity");
#if defined(__linux__)
  check(owner.interface_ipv4()==0x0100007f,"Native loopback interface binding differs");
#else
  check(owner.interface_ipv4()!=0,"Native selected interface binding missing");
#endif
  bool accepted=false;for(unsigned i=0;i<8 && !accepted;++i)accepted=owner.sign_in_offline();
  check(accepted && owner.master_state()==2,"Offline source sign-in/master-state mapping differs");
  check(get(owner.identity().machine,0x90)==1 && owner.identity().machine[0x30]==std::byte{1},"Machine context owner/validity missing");
  const auto xuid=owner.xuser_get_xuid(0);check(xuid.hresult_word==0 && get(xuid.output,0)!=0,"Actual offline XUID was not published");
  check(owner.xuser_get_xuid(1).hresult_word==0x4dd,"Unselected controller fabricated an offline XUser");
  SourcePlayerConfiguration selected{};selected[0]='P';selected[2]='l';selected[4]='a';selected[6]='y';selected[8]='e';selected[10]='r';selected[0x40]=10;selected[0x43]=10;selected[0x7c]=0xff;
  const auto payload=native_vista_selected_profile_payload(selected);owner.publish_profile(0,payload);
  check((get(owner.controllers(),0)&8)!=0 && owner.controllers()[0x1c]==std::byte{'P'},"Real native profile provider did not publish its source profile event");
  const auto user=vista_controller_local_user(owner.controllers(),0);
  check(user<4,"Actual local-user allocator mapping missing");
  VistaLocalUserIdentity actual;check(vista_local_user_read_identity(owner.local_users(),user,actual) && actual==xuid.output,"Profile display replaced actual XUID");
  check(!owner.player_for_controller(0),"Profile selection fabricated source session membership");
  const auto birth=owner.birth_local();check(birth.accepted && get(owner.bank(),0x73a0)==5,"Actual constructor did not produce local session state");
  check(get(owner.bank(),0x84)==1 && get(owner.bank(),0x71c0)==0,"Actual birth local peer missing");
  check(!owner.pcr_details_available(true),"Local source birth fabricated PCR details capability");
  check(get(owner.bank(),0x1254)==0 && !owner.player_for_controller(0),"Birth fabricated player reconciliation");
  check(std::equal(owner.key_registry().begin()+8,owner.key_registry().begin()+16,owner.bank().begin()+0x20),"Birth copied a different registry XNKID");
  check(std::equal(owner.key_registry().begin()+16,owner.key_registry().begin()+32,owner.bank().begin()+0x49),"Birth copied a different registry XNKEY");
  // These are explicit map-service controls for this local source-chain test.
  // No runtime map readiness or remote transport result is inferred from them.
  owner.reconcile_offline(3,0,world);
  const auto player=owner.player_for_controller(0);
  check(player && player->identity==actual && player->user==user && player->controller==0,"Actual reconciled player/user/XUID join missing");
  check(player->requested_payload==payload,"Reconciled selected132-byte payload differs");
  check(get(owner.bank(),0xb0+0x6c)==0xffffffffu,"Native offline bandwidth differs from source NONE initialization");
  check((std::to_integer<unsigned>(owner.bank()[0x125c+0x10])&1)!=0,"Supplied DLL multiplayer privilege did not reach actual player flags");
  rejects([&]{owner.reconcile_offline(3,0,true);});
  // The source FFA path does not consume a loaded-world team owner.
  owner.reconcile_offline(4,100,NativeVistaWorldKind::multiplayer);
  owner.reconcile_offline(4,100,NativeVistaWorldKind::campaign);
  check(!owner.birth_local().accepted,"A second birth recreated an active session");
  const auto controllers=std::vector(owner.controllers().begin(),owner.controllers().end());
  const auto users=std::vector(owner.local_users().begin(),owner.local_users().end());
  const auto old_key=std::vector(owner.key_registry().begin()+8,owner.key_registry().begin()+16);
  owner.reset_local_browser();
  check(owner.dispose_local(),"Actual local disposal was suppressed");
  check(get(owner.bank(),0x73a0)==0 && get(owner.bank(),0x70)==0xffffffffu &&
      get(owner.bank(),0x4c60)==0xffffffffu && owner.key_registry()[0]==std::byte{} &&
      get(owner.transport_registrations(),0x18)==0xffffffffu,"Actual local disposal did not consume owned banks");
  check(!owner.player_for_controller(0),"Disposed session retained a published player");
  check(std::equal(controllers.begin(),controllers.end(),owner.controllers().begin()) &&
      std::equal(users.begin(),users.end(),owner.local_users().begin()),"Session disposal changed selected profiles/local users");
  check(owner.teardown_events().size()==1 && owner.teardown_events()[0].kind==NativeVistaTeardownKind::key_unregistration &&
      std::equal(old_key.begin(),old_key.end(),owner.teardown_events()[0].key_id.begin()) &&
      owner.teardown_events()[0].publication==NativeVistaContextPublication::unpublished_offline,
      "Native key disposal fabricated platform completion or changed the request");
  check(!owner.dispose_local(),"State0 disposal fabricated a second operation");
  owner.refresh_selected_profile_catalog();
  check(std::equal(controllers.begin(),controllers.end(),owner.controllers().begin()) &&
      std::equal(users.begin(),users.end(),owner.local_users().begin()),
      "Actual native profile catalog refresh changed its retained identity/provider allocation");
  put(owner.controllers(),0x121c,7);
  rejects([&]{owner.refresh_selected_profile_catalog();});
  put(owner.controllers(),0x121c,0);
  owner.controllers()[0x14]^=std::byte{1};
  rejects([&]{owner.refresh_selected_profile_catalog();});
  owner.controllers()[0x14]^=std::byte{1};
  owner.refresh_selected_profile_catalog();
  owner.request_connection_mode(1,0);owner.request_connection_mode(2,0);
  check(owner.teardown_events().size()==3 && owner.teardown_events()[1].slot==1 && owner.teardown_events()[2].slot==2,
      "Null lifecycle connection requests lost their actual order");
  check(owner.birth_local().accepted,"Disposed source owner could not birth a new real session");
  owner.reconcile_offline(3,0,world);
  check(owner.player_for_controller(0) && owner.player_for_controller(0)->identity==actual,
      "New source birth lost the retained actual selected identity");
  owner.remove_profile(0);owner.reconcile_offline(3,0);
  check(get(owner.bank(),0x1254)==0 && !owner.player_for_controller(0),"Profile removal did not consume actual source membership removal");
  // A separate explicit team-bearing source control must reach and reject
  // the unbound loaded game/map owner, instead of importing a guessed mask.
  auto team_payload=payload;team_payload[0x7c]=std::byte{};owner.publish_profile(0,team_payload);
  owner.bank()[0x4e84]|=std::byte{1};
  rejects([&]{owner.reconcile_offline(4,100,NativeVistaWorldKind::multiplayer);});
  rejects([&]{owner.reconcile_offline(4,100,NativeVistaWorldKind::campaign);});
  owner.remove_profile(0);
  selected[0x50]=1;rejects([&]{native_vista_selected_profile_payload(selected);});
  rejects([&]{owner.publish_profile(1,payload);});
}
}
int main(int argc,char** argv) {
  try {
    check(argc==2 || argc==3,"Expected native original-owner fixture and optional menu-services fixture");std::ifstream file(argv[1]);check(bool(file),"Cannot open original owner fixture");
    std::string line;unsigned count{};
    while(std::getline(file,line)) {
      const auto row=nlohmann::json::parse(line);const auto kind=row.at("kind").get<std::string>();
      if(kind=="context" || kind=="controllers") {
        auto actual=bytes(row.at("before"));const auto expected=bytes(row.at("after"));actual.resize(actual.size()+32,std::byte{0x6d});
        if(kind=="context")vista_offline_machine_context_initialize(actual,row.at("clock").get<std::uint64_t>());
        else vista_controller_storage_initialize(actual);
        check(std::equal(expected.begin(),expected.end(),actual.begin()),"Native owner differs from unchanged source bytes");
        check(std::all_of(actual.begin()+expected.size(),actual.end(),[](auto b){return b==std::byte{0x6d};}),"Native owner modified its source suffix");
      } else if(kind=="profile") {
        const auto b=bytes(row.at("configuration"));SourcePlayerConfiguration c;
        std::transform(b.begin(),b.end(),c.begin(),[](auto v){return std::to_integer<std::uint8_t>(v);});
        const auto actual=native_vista_selected_profile_payload(c);const auto expected=bytes(row.at("payload"));
        check(std::equal(actual.begin(),actual.end(),expected.begin()),"Selected native profile differs from original PC payload");
      } else if(kind=="privilege") {
#if defined(__linux__)
        NativeVistaSession owner({.interface_name="lo"});
#else
        NativeVistaSession owner({});
#endif
        check(owner.join_context_byte()==row.at("context").get<std::uint8_t>(),"Native join context differs from original protected code and supplied DLL");
      } else if(kind=="map_progress") {
        const auto ready=native_vista_available_map_readiness(argv[1],argv[1]);
        check(ready && ready->query==2 && ready->progress_bits==row.at("progress_bits").get<std::uint32_t>(),"Native available-file branch differs from original progress math");
        check(!native_vista_available_map_readiness({},argv[1]),"Missing selected map fabricated availability");
        check(!native_vista_available_map_readiness(argv[1],{}),"Missing mainmenu map fabricated availability");
      } else if(kind=="local_owners") {
#if defined(__linux__)
        NativeVistaSession owner({.interface_name="lo"});
#else
        NativeVistaSession owner({});
#endif
        check(owner.network_busy()==row.at("network_busy").get<std::uint8_t>(),"Native network mode differs from original initializer");
        bool accepted=false;for(unsigned i=0;i<8 && !accepted;++i)accepted=owner.sign_in_offline();
        check(accepted,"Native offline identity service repeatedly rejected sign-in");
        VistaLocalUserPayload payload{};const auto name=bytes(row.at("selected_name"));
        std::copy(name.begin(),name.end(),payload.begin());payload[0x7c]=std::byte{0xff};
        owner.publish_profile(0,payload);check(owner.birth_local().accepted,"Native named owner failed original birth");
        owner.reconcile_offline(3,0,NativeVistaWorldKind::mainmenu);
        const auto expected=bytes(row.at("names"));
        check(std::equal(expected.begin(),expected.end(),owner.bank().begin()+0xb0),"Native selected peer/party names differ from original bounded publication");
        check(get(owner.bank(),0xb0+0x6c)==row.at("bandwidth").get<std::uint32_t>(),"Native peer bandwidth differs from original owner");
        check(row.at("optional_metadata").get<unsigned>()==0 && get(owner.bank(),0xb0+0x68)==0 && get(owner.bank(),0xb0+0x70)==0,"Native peer optional metadata differs from original cleared owner");
      } else throw std::runtime_error("Unknown owner fixture");
      ++count;
    }
    native_owner(NativeVistaWorldKind::unloaded);native_owner(NativeVistaWorldKind::mainmenu);
    unsigned menu_count{};
    if(argc==3) {
      std::ifstream menus(argv[2]);check(bool(menus),"Cannot open original menu-services fixture");nlohmann::json rows;menus>>rows;
      for(const auto& row:rows) {
        const auto kind=row.at("kind").get<std::string>();
        if(kind=="connection_task") {
          const auto q=vista_connection_task_queries(row.at("state").get<std::uint32_t>());
          check(q.state==row.at("returned_state").get<std::uint32_t>() &&
            q.blocks==bool(row.at("blocks").get<unsigned>()) && q.gate_a==bool(row.at("gate_a").get<unsigned>()) &&
            q.gate_b==bool(row.at("gate_b").get<unsigned>()),"Native connection queries differ from original protected code");
        } else if(kind=="pcr_details") {
          std::vector<std::byte> bank(vista_session_source_size+32,std::byte{0x6d});
          put(bank,0x73a0,row.at("state").get<std::uint32_t>());put(bank,0x1c,row.at("session_kind").get<std::uint32_t>());
          const auto before=bank;
          check(vista_session_pcr_details_available(bank,row.at("enabled").get<unsigned>()!=0)==bool(row.at("available").get<unsigned>()),"Native PCR details query differs from original session predicate");
          check(bank==before,"Native PCR details query modified its source owner");
        } else throw std::runtime_error("Unknown menu-services fixture");
        ++menu_count;
      }
      rejects([&]{vista_session_pcr_details_available({},true);});
    }
    std::cout<<count<<" original owner/profile/platform cases, "<<menu_count<<" original menu-service cases and actual native OS offline birth/reconciliation passed\n";
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
