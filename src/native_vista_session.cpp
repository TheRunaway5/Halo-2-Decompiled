#include "halo2/native_vista_session.hpp"
#include "halo2/vista_network_browser_back.hpp"
#include "halo2/vista_lobby_input.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <system_error>
#include <vector>
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#include <bcrypt.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#if defined(__linux__)
#include <sys/random.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#endif

namespace halo2 {
VistaConnectionTaskQueries vista_connection_task_queries(std::uint32_t state) noexcept {
  const auto signed_state=std::bit_cast<std::int32_t>(state);
  return {state,signed_state>=2 && signed_state<=6,
      signed_state>=8 && signed_state<=11,state==7};
}
std::optional<NativeVistaAvailableMapReadiness> native_vista_available_map_readiness(
    const std::filesystem::path& selected,const std::filesystem::path& mainmenu) {
  const auto available=[](const std::filesystem::path& path) {
#if defined(_WIN32)
    return GetFileAttributesW(path.c_str())!=INVALID_FILE_ATTRIBUTES;
#else
    std::error_code error;
    return std::filesystem::exists(std::filesystem::symlink_status(path,error)) && !error;
#endif
  };
  if(!available(selected) || !available(mainmenu))return {};
  return NativeVistaAvailableMapReadiness{2,0x3f800000};
}
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t o) noexcept {
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[o+i])<<(8*i);return v;
}
void put(std::span<std::byte> b,std::size_t o,std::uint32_t v,unsigned n=4) noexcept {
  for(unsigned i=0;i<n;++i)b[o+i]=std::byte(v>>(8*i));
}
void clear(std::span<std::byte> b,std::size_t o,std::size_t n) noexcept {
  std::fill_n(b.begin()+o,n,std::byte{});
}
std::uint64_t performance_counter() noexcept {
#if defined(_WIN32)
  LARGE_INTEGER counter;QueryPerformanceCounter(&counter);return std::uint64_t(counter.QuadPart);
#else
  return std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}
std::uint64_t performance_milliseconds() noexcept {
#if defined(_WIN32)
  LARGE_INTEGER counter,frequency;QueryPerformanceCounter(&counter);QueryPerformanceFrequency(&frequency);
  const auto c=std::uint64_t(counter.QuadPart),f=std::uint64_t(frequency.QuadPart);
  return (c/f)*1000+((c%f)*1000)/f;
#else
  return std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}
void entropy(std::span<std::byte> output) {
#if defined(_WIN32)
  if(BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(output.data()),ULONG(output.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)
    throw std::runtime_error("Native Vista OS entropy service failed");
#elif defined(__linux__)
  while(!output.empty()) {
    const auto n=::getrandom(output.data(),output.size(),0);
    if(n<0) {if(errno==EINTR)continue;throw std::system_error(errno,std::generic_category(),"Native Vista OS entropy");}
    if(n==0)throw std::runtime_error("Native Vista OS entropy returned no bytes");
    output=output.subspan(std::size_t(n));
  }
#else
  const int descriptor=::open("/dev/urandom",O_RDONLY);
  if(descriptor<0)throw std::system_error(errno,std::generic_category(),"Native Vista OS entropy open");
  struct Owner {int value;~Owner(){::close(value);}} owner{descriptor};
  while(!output.empty()) {
    const auto n=::read(descriptor,output.data(),output.size());
    if(n<0) {if(errno==EINTR)continue;throw std::system_error(errno,std::generic_category(),"Native Vista OS entropy read");}
    if(n==0)throw std::runtime_error("Native Vista OS entropy returned no bytes");
    output=output.subspan(std::size_t(n));
  }
#endif
}
std::uint32_t interface_address(std::string_view selected) {
#if defined(_WIN32)
  ULONG length=0;const auto initial=GetAdaptersAddresses(AF_INET,0,nullptr,nullptr,&length);
  if(initial!=ERROR_BUFFER_OVERFLOW)throw std::runtime_error("Native Vista adapter size query failed");
  std::vector<std::byte> storage(length);
  auto* adapters=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.data());
  if(GetAdaptersAddresses(AF_INET,0,nullptr,adapters,&length)!=NO_ERROR)
    throw std::runtime_error("Native Vista adapter enumeration failed");
  std::uint32_t loopback{};
  for(auto* a=adapters;a;a=a->Next) {
    if(a->OperStatus!=IfOperStatusUp || (!selected.empty() && selected!=a->AdapterName))continue;
    for(auto* u=a->FirstUnicastAddress;u;u=u->Next)if(u->Address.lpSockaddr->sa_family==AF_INET) {
      const auto value=reinterpret_cast<const sockaddr_in*>(u->Address.lpSockaddr)->sin_addr.s_addr;
      if(a->IfType!=IF_TYPE_SOFTWARE_LOOPBACK || !selected.empty())return value;
      loopback=value;
    }
  }
#else
  ifaddrs* interfaces=nullptr;
  if(::getifaddrs(&interfaces)!=0)throw std::system_error(errno,std::generic_category(),"Native Vista interface enumeration");
  struct Owner {ifaddrs* value;~Owner(){::freeifaddrs(value);}} owner{interfaces};
  std::uint32_t loopback{};
  for(auto* i=interfaces;i;i=i->ifa_next) {
    if(!i->ifa_addr || i->ifa_addr->sa_family!=AF_INET || !(i->ifa_flags&IFF_UP) ||
        (!selected.empty() && selected!=i->ifa_name))continue;
    const auto value=reinterpret_cast<const sockaddr_in*>(i->ifa_addr)->sin_addr.s_addr;
    if(!(i->ifa_flags&IFF_LOOPBACK) || !selected.empty())return value;
    loopback=value;
  }
#endif
  if(!selected.empty())throw FormatError("Selected native Vista IPv4 interface is unavailable");
  if(loopback)return loopback;
  throw FormatError("No up native Vista IPv4 interface is available");
}
}
bool vista_session_pcr_details_available(std::span<const std::byte> bank,bool enabled) {
  if(bank.size()<vista_session_source_size)throw FormatError("Vista PCR details query requires the complete actual session bank");
  const auto state=get(bank,0x73a0);
  return enabled && state>=3 && state<=8 && get(bank,0x1c)==2;
}
VistaLocalUserPayload native_vista_selected_profile_payload(const SourcePlayerConfiguration& configuration) {
  for(unsigned i=0x50;i<0x7c;++i)if(configuration[i])
    throw FormatError("Native Vista fresh selected profile has unproved account bytes");
  if(configuration[0x7e] || configuration[0x7f] || configuration[0x83] || configuration[62] || configuration[63])
    throw FormatError("Native Vista fresh selected profile has unproved retained bytes or unbounded name");
  if(configuration[0x7c]!=0xff && configuration[0x7c]>16)
    throw FormatError("Native Vista selected profile team was not source-clamped");
  VistaLocalUserPayload output;
  std::transform(configuration.begin(),configuration.begin()+0x84,output.begin(),
      [](std::uint8_t value){return std::byte(value);});return output;
}
void vista_offline_machine_context_initialize(std::span<std::byte> context,std::uint64_t milliseconds) {
  if(context.size()<0x218 || context[0]!=std::byte{})
    throw FormatError("Vista fresh machine context is truncated or already running");
  context[0]=std::byte{1};
  clear(context,4,0x18);clear(context,0x1c,0x78);clear(context,0x10c,0x78);
  put(context,0x210,0);put(context,0x214,0);put(context,0x1fc,0xffffffffu);put(context,0x200,0xffffffffu);
  put(context,0x208,std::uint32_t(milliseconds));put(context,0x20c,std::uint32_t(milliseconds>>32));
}
void vista_controller_storage_initialize(std::span<std::byte> bank) {
  if(bank.size()<0x4a38)throw FormatError("Vista controller initialization requires its adjacent global bank");
  clear(bank,0,0x4a38);
  for(unsigned i=0;i<4;++i) {
    auto c=bank.subspan(i*0x127c,0x127c);
    //608257 after the initializer erased mappings and flags: no user/profile
    // removal or reference callback is reached in this actual fresh branch.
    put(c,4,0xffffffffu);put(c,0x10,0xffffffffu);
  }
  bank[0x4a34]=std::byte{}; //D7128C; redundant after the complete source clear.
}
class NativeVistaSession::OfflineServices final:public VistaOfflineIdentityServices {
public:
  NativeVistaSession& owner;
  std::uint32_t controller;
  explicit OfflineServices(NativeVistaSession& o,std::uint32_t c):owner(o),controller(c){}
  void random_bytes(std::span<std::byte> b) override {owner.random_bytes(b);}
  std::int32_t crt_rand() override {return vista_crt_rand(owner.crt_state_);}
  std::uint32_t interface_ipv4() override {return owner.interface_ipv4_;}
  std::uint32_t inet_addr(std::string_view text) override {
    const std::string terminated(text);return ::inet_addr(terminated.c_str());
  }
  void initialize_machine_context(std::span<std::byte> b) override {
    vista_offline_machine_context_initialize(b,performance_milliseconds());
  }
  void master_status_changed(std::uint32_t state) override {
    //10535F30 reads the retained user state, mapping2 to A and all others to2.
    if(state!=get(owner.local_identity(controller).user_info,0xc))throw FormatError("Vista master status observed a different retained sign-in state");
    if(controller==0)owner.master_state_=state==2?0xau:2u;
  }
};
NativeVistaSession::NativeVistaSession(NativeVistaSessionConfiguration configuration)
  :configuration_(std::move(configuration)),interface_ipv4_(interface_address(configuration_.interface_name)) {
  std::array<std::byte,4> seed;entropy(seed);crt_state_=get(seed,0);
  vista_controller_storage_initialize(controllers_);
  // Source addresses are stable binding IDs for these actual owned banks,
  // not manufactured native addresses or key/machine/player identities.
  for(unsigned slot=0;slot<2;++slot)
    vista_session_storage_initialize(sessions_[slot],slot,1,slot,
        {{0x91c468,0x91c470,0x91c478,0x91c47c},0x91c474+slot*0x7b88,registrations_,registry_});
  // Explicit native browser ownership: no OS handle has been created.
  put(browser_,0x428,0xffffffffu);
}
bool NativeVistaSession::dispose_local() {
  VistaSessionDisposalServices services;
  services.unregister_key=[this](std::span<const std::byte> id) {
    NativeVistaTeardownEvent event{NativeVistaTeardownKind::key_unregistration,0,0,{},NativeVistaContextPublication::unpublished_offline};
    std::copy(id.begin(),id.end(),event.key_id.begin());teardown_events_.push_back(event);
  };
  // Native key ownership is real; Windows XNet publication and QoS listener
  // ownership were never implemented by this local-only provider. Preserve
  // the ignored unregister request explicitly, while source clears execute.
  return vista_session_dispose_single_offline(session(),{keys_,registrations_,0x91c470},true,false,services);
}
void NativeVistaSession::reset_local_browser() {
  vista_browser_reset(browser_,6,{});
}
void NativeVistaSession::request_connection_mode(std::uint32_t slot,std::uint32_t value) {
  if(slot>2 || value>1)throw FormatError("Native offline connection-mode sink received an unbound request");
  teardown_events_.push_back({NativeVistaTeardownKind::connection_mode,slot,value,{},NativeVistaContextPublication::unpublished_offline});
}
void NativeVistaSession::random_bytes(std::span<std::byte> b) {
  vista_xnet_random(random_,b,[]{return performance_counter();});
}
std::uint32_t NativeVistaSession::random_session_seed() {
  const auto clock=raw_network_clock();const auto time=process_clock();
  return clock^time^std::uint32_t(vista_crt_rand(crt_state_));
}
std::array<std::uint32_t,2> NativeVistaSession::random_session_identity() {
  std::array<std::byte,8> bytes;entropy(bytes);random_bytes(bytes);
  return {get(bytes,0),get(bytes,4)};
}
VistaOfflineIdentityState& NativeVistaSession::local_identity(std::uint32_t controller) {
  if(controller>=4)throw FormatError("Native offline controller is outside0..3");
  return controller==0?identity_:local_identities_[controller-1];
}
const VistaOfflineIdentityState& NativeVistaSession::local_identity(std::uint32_t controller) const {
  if(controller>=4)throw FormatError("Native offline controller is outside0..3");
  return controller==0?identity_:local_identities_[controller-1];
}
bool NativeVistaSession::sign_in_offline(std::uint32_t controller) {
  if(signed_in(controller))return true;
  VistaOfflineIdentityScratch scratch;
  // The source caller's stack bytes are indeterminate. Native OS entropy is
  // their explicit replacement, never silently cleared scratch storage.
  entropy(scratch.enet);entropy(scratch.online);
  OfflineServices services(*this,controller);
  return vista_offline_identity_configure(local_identity(controller),scratch,configuration_.lan_ipv4,configuration_.base_port,services);
}
bool NativeVistaSession::signed_in(std::uint32_t controller) const noexcept {
  if(controller>=4)return false;
  const auto state=get(local_identity(controller).user_info,0xc);return state==1 || state==2;
}
void NativeVistaSession::reconcile_offline(std::uint32_t map_status,std::uint32_t map_progress,
    bool world_ready) {
  if(world_ready)throw FormatError("Native Vista loaded-world reconciliation requires its actual game/map team owner");
  reconcile_offline(map_status,map_progress,NativeVistaWorldKind::unloaded);
}
void NativeVistaSession::reconcile_offline(std::uint32_t map_status,std::uint32_t map_progress,
    NativeVistaWorldKind world) {
  std::optional<std::uint16_t> allowed;
  switch(world) {
    case NativeVistaWorldKind::unloaded:allowed=0;break;
    case NativeVistaWorldKind::mainmenu:allowed=0xff;break;
    case NativeVistaWorldKind::multiplayer:case NativeVistaWorldKind::campaign:break;
    default:throw FormatError("Native Vista reconciliation world kind is invalid");
  }
  class Services final:public VistaSessionReconciliationServices {
    NativeVistaSession& owner;
    std::optional<std::uint16_t> mask;
    std::uint16_t team_mask() const {
      if(!mask)throw FormatError("Native Vista multiplayer/campaign reconciliation consumed an unbound game/map team owner");
      return *mask;
    }
  public:
    Services(NativeVistaSession& value,std::optional<std::uint16_t> allowed):owner(value),mask(allowed){}
    std::uint32_t network_clock() override {return owner.network_clock();}
    std::uint32_t language() override {return owner.language();}
    //5B087B: !4497EA/default822428==-1→0; real mainmenu/nonMP/noncampaign→FF.
    std::uint16_t allowed_team_mask(std::span<const std::byte>) override {return team_mask();}
    std::uint16_t primary_allowed_team_mask() override {return team_mask();}
    std::uint8_t join_context_byte() override {return owner.join_context_byte();}
    bool authored_random_name(std::span<std::byte>) override {throw FormatError("Native Vista authored random-name owner is not bound");}
    bool identity_admitted(std::uint32_t,VistaLocalUserIdentity) override {throw FormatError("Native Vista optional identity-admission owner is not bound");}
    void send_session_message(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::span<const std::byte>) override {throw FormatError("Native Vista remote session transport is not bound");}
  } services(*this,allowed);
  VistaSessionLocalPeerValues peer{};peer.name=peer_name_;peer.party=peer_party_;
  //5B07B2 clears complete638h91A590 owner: optional metadata byte remains0.
  //5E0CE1 initializes926D08 toNONE; there is no native bandwidth measurement.
  peer.bandwidth=bandwidth_;peer.map_status=map_status;peer.map_progress=map_progress;
  //5ABB1F EBP=7D0;5ABD62/68/6E/74 publish these four source defaults.
  vista_session_reconcile_local_users(reconciliation_banks(),{2000,2000,2000,2000},peer,services);
}
VistaPlatformXuidResult NativeVistaSession::xuser_get_xuid(std::uint32_t controller) const {
  if(controller>=4)throw FormatError("Native Vista XUser controller is outside0..3");
  VistaPlatformXuidResult result;
  result.hresult_word=vista_offline_identity_xuid(local_identity(controller).user_info,result.output);return result;
}
void NativeVistaSession::publish_profile(std::uint32_t controller,VistaLocalUserPayload p) {
  if(controller>=4)throw FormatError("Native Vista offline profile controller is outside0..3");
  const auto xuid=xuser_get_xuid(controller);
  if(xuid.hresult_word!=0)throw FormatError("Native Vista profile publication requires the offline sign-in event");
  auto c=std::span(controllers_).subspan(controller*0x127c,0x127c);
  auto slot=vista_controller_local_user(controllers_,controller);
  if(slot==0xffffffffu) {slot=vista_local_user_allocate(local_users_,xuid.output);put(c,4,slot);}
  std::copy(xuid.output.begin(),xuid.output.end(),c.begin()+8);
  vista_local_user_write_identity(local_users_,slot,xuid.output);
  // Real native profile-provider selection event. Its complete owned record is
  // the same explicit cleared provider input used by the original6087BF oracle.
  //6087BF copies1208 bytes and publishes flag8/index before606A97 writes payload.
  auto& profile=selected_profiles_[controller];profile.fill(std::byte{});
  std::copy_n(p.begin(),64,profile.begin()+8);
  std::copy_n(p.begin()+0x40,16,profile.begin()+0x1148);
  std::copy(profile.begin(),profile.end(),c.begin()+0x14);
  put(c,0,get(c,0)|8);put(c,0x121c,controller);
  vista_local_user_write_payload(local_users_,slot,controller,p,0,0);
  // The actual provider payload also supplies these independent controller
  // metadata/team fields. They are not session readiness or host assignments.
  std::copy_n(p.begin(),64,c.begin()+0x123c);
  // Native selected-name event mirrors606B67→61716B→615024→5B100B.
  // The source supplies the same controller display name to both bounded
  // retained owners. Profile removal does not clear these independent names.
  std::u16string display;
  for(unsigned i=0;i<32;++i) {
    const auto value=std::to_integer<unsigned>(c[0x123c+i*2])|(std::to_integer<unsigned>(c[0x123d+i*2])<<8);
    if(!value)break;
    display+=char16_t(value);
  }
  if(controller==0){peer_name_=display.substr(0,15);peer_party_=display.substr(0,31);}
  std::copy_n(p.begin()+0x40,16,c.begin()+0x115c);
  put(c,0x1220,p[0x7c]==std::byte{0xff}?0xffffffffu:std::to_integer<unsigned>(p[0x7c]));
  c[0x1224]=p[0x7d];c[0x122c]=p[0x80];c[0x1228]=p[0x81];c[0x1230]=p[0x82];
}
void NativeVistaSession::refresh_selected_profile_catalog() {
  VistaBrowserProfileRefreshServices services;
  // The native selected-profile provider retains its actual catalog in this
  // owner. It has no separate Windows profile-cache/persistence work queue.
  services.refresh_catalog=[] {};
  services.remap_index=[this](std::uint32_t index) {
    if(index>=selected_profiles_.size())return 0xffffffffu;
    const auto c=std::span<const std::byte>(controllers_).subspan(index*0x127c,0x127c);
    if(!(get(c,0)&8u) || get(c,0x121c)!=index ||
        !std::equal(selected_profiles_[index].begin(),selected_profiles_[index].end(),c.begin()+0x14))
      return 0xffffffffu;
    // publish_profile assigned this index to this genuine provider record;
    // refreshing its unchanged catalog retains that same allocation.
    return index;
  };
  services.controller_mask=[this]{return vista_lobby_controller_mask(controllers_);};
  services.missing_profile_dialog=[](const auto&) {
    throw FormatError("Native selected profile disappeared from its actual provider catalog");
  };
  vista_network_browser_refresh_profiles(controllers_,profile_refresh_pending_,services);
}
void NativeVistaSession::remove_profile(std::uint32_t controller) {
  if(controller>=4)throw FormatError("Native Vista offline profile controller is outside0..3");
  auto c=std::span(controllers_).subspan(controller*0x127c,0x127c);const auto slot=vista_controller_local_user(controllers_,controller);
  if(slot!=0xffffffffu)vista_local_user_remove(local_users_,slot);
  // Fresh native provider boundary clears only its projections/ownership.
  put(c,4,0xffffffffu);clear(c,8,8);clear(c,0x123c,64);
  clear(c,0x14,0x1208);put(c,0,get(c,0)&~8u);put(c,0x121c,0xffffffffu);
  selected_profiles_[controller].fill(std::byte{});
}
std::optional<NativeVistaSessionPlayer> NativeVistaSession::player_for_controller(std::uint32_t controller) const {
  const auto user=vista_controller_local_user(controllers_,controller);
  if(user==0xffffffffu)return {};
  if(user>=4)throw FormatError("Native Vista retained controller user is outside its source bank");
  auto b=std::span(sessions_[0]);const auto state=get(b,0x73a0);
  if(state<3 || state>8)return {};
  const auto peer=get(b,0x71c0);
  if(peer>=17)throw FormatError("Native Vista reconciled session has no actual local peer");
  const auto player=get(b,0x184+peer*0x10c+user*4);
  if(player==0xffffffffu)return {};
  if(player>=16 || !(get(b,0x1258)&(1u<<player)))
    throw FormatError("Native Vista reconciled player is outside actual membership");
  const auto p=0x125c+player*0x128;VistaLocalUserIdentity id;
  if(!vista_local_user_read_identity(local_users_,user,id) || get(b,p+8)!=peer || get(b,p+0xc)!=user ||
      !std::equal(id.begin(),id.end(),b.begin()+p))
    throw FormatError("Native Vista player identity/controller/user join differs");
  NativeVistaSessionPlayer result{controller,user,peer,player,id,{},{}};
  std::copy_n(b.begin()+p+0x18,0x84,result.requested_payload.begin());
  std::copy_n(b.begin()+p+0x9c,0x84,result.final_payload.begin());return result;
}
std::uint32_t NativeVistaSession::network_clock() const noexcept {
  return network_clock_frozen_?frozen_network_clock_:raw_network_clock();
}
void NativeVistaSession::freeze_network_clock(bool value) noexcept {
  network_clock_frozen_=value;frozen_network_clock_=raw_network_clock();
}
std::uint32_t NativeVistaSession::raw_network_clock() const noexcept {
  //437E51 is timeGetTime plus8818F4. Its supplied initialized bias is0;
  // no source invocation of437E39 is established, so none is manufactured.
#if defined(_WIN32)
  return std::uint32_t(GetTickCount());
#else
  return std::uint32_t(performance_milliseconds());
#endif
}
std::uint32_t NativeVistaSession::process_clock() const noexcept {return std::uint32_t(std::time(nullptr));}
NativeVistaContextPublication NativeVistaSession::set_user_context(std::uint32_t context,std::uint32_t value) {
  if(context!=2 && context!=5 && context!=0x8001)
    throw FormatError("Native Vista offline presence sink received an unbound source context");
  const auto publication=NativeVistaContextPublication::unpublished_offline;
  context_events_.push_back({0,context,value,publication});return publication;
}
VistaSessionBirthResult NativeVistaSession::birth_local() {
  auto& b=sessions_[0];
  if(get(b,0x73a0)!=0)return vista_session_local_birth(b,{});
  VistaSessionBirthQueries q;
  VistaSessionBirthValues values;
  q.local_machine_available=vista_offline_identity_machine(identity_.machine,values.local_machine)!=0;
  if(!q.local_machine_available)return vista_session_local_birth(b,q);
  // Source5B63C7's normal XNetCreateKey output, using the retained supplied-DLL
  // XOR stream. Native entropy replaces its uninitialized stack destinations.
  entropy(values.session_key_id);entropy(values.session_key);
  random_bytes(values.session_key_id);random_bytes(values.session_key);
  values.session_key_id[0]=(values.session_key_id[0]&std::byte{0x0f})|std::byte{0x40};
  // Actual32-byte key registry5B5E30; this local owner genuinely retains both
  // keys. Remote XNet registration/QoS transmission is outside this local port.
  keys_[0]=std::byte{1};keys_[1]=std::byte{1};keys_[2]=std::byte{};put(keys_,4,1);
  std::copy(values.session_key_id.begin(),values.session_key_id.end(),keys_.begin()+8);
  std::copy(values.session_key.begin(),values.session_key.end(),keys_.begin()+0x10);
  q.network_created=true;q.session_key_available=keys_[0]!=std::byte{};
  values.network_clocks[0]=network_clock();values.language=language();
  //5CAF2E clears the parameter bank before5B5A84 writes session+4DC8.
  // Unlike the offline/key stack destinations, these original bytes are0.
  values.random.fill(std::byte{});random_bytes(values.random);
  values.network_clocks[1]=network_clock();values.network_clocks[2]=network_clock();values.process_clock=process_clock();
  q.values=values;auto result=vista_session_local_birth(b,q);
  if(result.accepted) {
    // Exact5BD629 registration, separate from constructor's returned intent.
    put(registrations_,0x18,get(b,0x5c));put(registrations_,0x1c,1);
    std::copy(values.session_key_id.begin(),values.session_key_id.end(),registrations_.begin()+0x20);
    std::copy(values.session_key.begin(),values.session_key.end(),registrations_.begin()+0x28);
  }
  return result;
}
} // namespace halo2
