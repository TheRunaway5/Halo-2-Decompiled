#pragma once
#include "halo2/vista_offline_identity.hpp"
#include "halo2/source_player_profile.hpp"
#include "halo2/vista_session_reconciliation.hpp"
#include "halo2/vista_session_storage.hpp"
#include "halo2/vista_session_disposal.hpp"
#include <chrono>
#include <string>
#include <filesystem>
#include <vector>

namespace halo2 {
struct NativeVistaAvailableMapReadiness {
  std::int32_t query; //4655A1 raw3→44B633 result2.
  std::uint32_t progress_bits; //4317D2 normal mode0, exact binary32.
};
enum class NativeVistaWorldKind {unloaded,mainmenu,multiplayer,campaign};
struct VistaConnectionTaskQueries {
  std::uint32_t state;
  bool blocks,gate_a,gate_b;
};
// Actual protected5AD704/70E/718/722 wrappers: their own decryptors expose
// the shared5F69BA/BE/D4/EA leaves on lifecycle6's context state word.
VistaConnectionTaskQueries vista_connection_task_queries(std::uint32_t state) noexcept;
// Actual6156B9→5B1643→5B169B, with the caller's real820FC0 predicate.
bool vista_session_pcr_details_available(std::span<const std::byte>,bool lifecycle_enabled);
// Native filesystem attribute boundary for already-resolved actual file paths.
// Original available-map branch requires selected and mainmenu attributes;
//430DFA then computes(80*1+100*1)/(80+100)==1. Missing/cache/download/task
// branches remain unresolved, represented by no value instead of success.
std::optional<NativeVistaAvailableMapReadiness> native_vista_available_map_readiness(
    const std::filesystem::path& selected_file,const std::filesystem::path& mainmenu_file);
// Literal fresh branch100C4EC0..100C5006 of the supplied xlive.dll. The caller
// owns the complete cleared machine context and actual QPC-derived ms sample.
// This is a fresh-context initializer; calling it on a running ring rejects.
void vista_offline_machine_context_initialize(std::span<std::byte>,std::uint64_t milliseconds);
// Literal608608, including its48 adjacent global bytes. Local users are a
// separate owner and are not cleared by this controller initializer.
void vista_controller_storage_initialize(std::span<std::byte> controllers_and_globals);

// Explicit native selected-profile provider conversion. Original PC60696F
// agrees with the first132 bytes for a fresh native cleared input/account
// record: same bounded UTF16/name,16 appearance bytes,team and metadata byte
// positions. Account/retained/sentinel bytes not produced by this fresh source
// branch reject instead of being silently imported from an Xbox account.
VistaLocalUserPayload native_vista_selected_profile_payload(const SourcePlayerConfiguration&);

struct NativeVistaSessionPlayer {
  std::uint32_t controller,user,peer,player;
  VistaLocalUserIdentity identity;
  VistaLocalUserPayload requested_payload,final_payload;
};

struct NativeVistaSessionConfiguration {
  std::uint32_t language{}; // Actual native language selection, not a locale guess.
  std::uint32_t lan_ipv4{}; // Network-order bytes in the source's little endian word.
  std::uint16_t base_port=2000; // Actual supplied-DLL initialized word107534D4.
  std::string interface_name; // Empty selects the native OS's first up IPv4 interface.
};
enum class NativeVistaContextPublication {unpublished_offline};
struct NativeVistaContextEvent {
  std::uint32_t controller,context,value;
  NativeVistaContextPublication publication;
};
enum class NativeVistaTeardownKind {key_unregistration,connection_mode};
struct NativeVistaTeardownEvent {
  NativeVistaTeardownKind kind;
  std::uint32_t slot,value;
  std::array<std::byte,8> key_id;
  NativeVistaContextPublication publication;
};

// Application-owned LOCAL OFFLINE platform/session owner. It binds actual OS
// clocks, entropy and interface configuration to the source routines. Native
// pointer identities never enter the source's32-bit owner words: retained
// source addresses identify the real detached banks exposed below.
//
// Explicit native port boundaries: getifaddrs replaces Windows adapter
// enumeration; OS entropy initializes bytes which were uninitialized source
// stack storage and seeds the source CRT state. The supplied DLL's RC4 XOR and
// CRT recurrence then execute literally. These choices do not claim identical
// identities/keys to a Windows launch. No online account/remote transport,
// network QoS listener or source input-layout/profile-storage implementation is
// represented. Source membership/host/state are produced only by birth and
// reconciliation, never assigned from a desired UI state.
class NativeVistaSession {
public:
  explicit NativeVistaSession(NativeVistaSessionConfiguration);
  NativeVistaSession(const NativeVistaSession&)=delete;
  NativeVistaSession& operator=(const NativeVistaSession&)=delete;

  // Native offline login event, not merely selection of a display name.
  // The supplied DLL's real zero-rand failure remains a returned failure.
  bool sign_in_offline(std::uint32_t controller=0);
  bool signed_in(std::uint32_t controller=0) const noexcept;
  std::uint32_t master_state() const noexcept {return master_state_;}
  const VistaOfflineIdentityState& identity() const noexcept {return identity_;}
  VistaPlatformXuidResult xuser_get_xuid(std::uint32_t controller) const;

  // The native profile provider supplies the complete132-byte PC payload.
  // Each explicitly signed-in native local user has a separate identity.
  // This provider owns a complete1208-byte native profile record (cleared
  // fields plus selected name/appearance), copied before the actual source
  // flag8/profile-index publication event and local-user payload leaves.
  // It does not claim Windows profile persistence or source input layouts.
  void publish_profile(std::uint32_t controller,VistaLocalUserPayload actual_payload);
  void remove_profile(std::uint32_t controller);
  // Complete608690 uses this actual native provider's retained catalog. Its
  // published stable indices map back to owned selected_profiles_ entries;
  // Windows on-disk catalog/cache persistence is a distinct port boundary.
  void refresh_selected_profile_catalog();
  VistaSessionBirthResult birth_local();
  // Actual complete5CC258 writes against these genuine owned session/key/
  // registration banks, bounded to the sole local host. Native XNet has no
  // remote key publication; its ignored unregister request is retained as
  // unpublished_offline, never fabricated platform success.
  bool dispose_local();
  // Explicit offline sink for actual null-handler5B54CF(slot,value) requests.
  // Retains their arguments without claiming Windows transport mode effects.
  void request_connection_mode(std::uint32_t slot,std::uint32_t value);
  std::span<const NativeVistaTeardownEvent> teardown_events() const noexcept {return teardown_events_;}
  // Actual source preworld local offline publication/reconciliation. Map
  // status/progress must come from the actual file/loader readiness owner.
  // Names are the selected controller display name, as61716B→615024 supplies
  // both source name/party owners. Unsupported loaded-world/team services,
  // authored-name fallback, optional admission and remote IO reject if reached.
  void reconcile_offline(std::uint32_t map_status,std::uint32_t map_progress,
      NativeVistaWorldKind world=NativeVistaWorldKind::unloaded);
  // Compatibility for an actual unloaded predicate; true cannot identify the
  // loaded game's team owner and therefore rejects.
  void reconcile_offline(std::uint32_t map_status,std::uint32_t map_progress,bool world_ready);
  // Actual protected61A995→XUserCheckPrivilege(0,FE) supplied-DLL result.
  std::uint8_t join_context_byte() const noexcept {return 1;}
  // Actual5E0CE1 initialized local network/bandwidth owner; source5E0D4F.
  std::uint8_t network_busy() const noexcept {return network_busy_;}
  // Explicit local offline presence sink for actual XUserSetContext call
  // events. Original callers ignore HRESULT. The supplied DLL's game-module
  // presence formatting and online publication are not represented: each
  // retained event explicitly reports unpublished_offline, never success.
  NativeVistaContextPublication set_user_context(std::uint32_t context,std::uint32_t value);
  std::span<const NativeVistaContextEvent> context_events() const noexcept {return context_events_;}
  //5D7810 registers source lifecycle6 while leaving its actual BSS context
  // state zero. This local-only owner never starts a remote connection task.
  VistaConnectionTaskQueries connection_task_queries() const noexcept {
    return vista_connection_task_queries(connection_task_state_);
  }
  bool pcr_details_available(bool lifecycle_enabled) const {
    return vista_session_pcr_details_available(bank(),lifecycle_enabled);
  }
  // Reads actual reconciled membership and verifies the controller/user/XUID
  // join. Returns no row before source reconciliation has inserted it.
  std::optional<NativeVistaSessionPlayer> player_for_controller(std::uint32_t) const;
  // Literal5B3C3A: always samples raw437E51, including an unfreeze event.
  void freeze_network_clock(bool) noexcept;
  std::uint32_t raw_network_clock() const noexcept; //437E51.
  std::uint32_t network_clock() const noexcept;
  std::uint32_t elapsed_network_clock(std::uint32_t previous) const noexcept {
    return network_clock()-previous; //5B3CB4 unsigned subtraction, source wrap.
  }
  std::uint32_t process_clock() const noexcept;
  // Literal45906F: actual network clock XOR CRT time XOR retained CRT rand.
  std::uint32_t random_session_seed();
  // Original5D9746 passes indeterminate stack bytes to XOR XNetRandom.
  // OS entropy explicitly owns those bytes at this native port boundary.
  std::array<std::uint32_t,2> random_session_identity();
  std::uint32_t language() const noexcept {return configuration_.language;}
  std::uint32_t interface_ipv4() const noexcept {return interface_ipv4_;}

  std::span<std::byte> session() noexcept {return sessions_[0];}
  std::span<std::byte> bank() noexcept {return session();}
  std::span<const std::byte> bank() const noexcept {return sessions_[0];}
  std::span<std::byte> secondary_session() noexcept {return sessions_[1];}
  std::span<std::byte> controllers() noexcept {return controllers_;}
  std::span<std::byte> local_users() noexcept {return local_users_;}
  std::span<std::byte> input_controls() noexcept {return input_controls_;}
  std::span<std::byte> reconciliation() noexcept {return reconciliation_;}
  std::span<const std::byte> key_registry() const noexcept {return keys_;}
  std::span<const std::byte> transport_registrations() const noexcept {return registrations_;}
  std::span<const std::byte> session_registry() const noexcept {return registry_;}
  VistaSessionReconciliationBanks reconciliation_banks() noexcept {
    return {session(),local_users(),controllers(),reconciliation()};
  }
private:
  class OfflineServices;
  NativeVistaSessionConfiguration configuration_;
  VistaOfflineIdentityState identity_{};
  // Native split-screen provider: each explicitly selected local controller
  // owns an independent offline user bank initialized by the recovered DLL
  // constructor. The host alone owns the session's machine/platform context.
  std::array<VistaOfflineIdentityState,3> local_identities_{};
  VistaOfflineIdentityState& local_identity(std::uint32_t);
  const VistaOfflineIdentityState& local_identity(std::uint32_t) const;
  VistaXnetRandomState random_{};
  std::uint32_t crt_state_{},master_state_{},interface_ipv4_{};
  std::uint8_t network_busy_{};
  std::uint32_t connection_task_state_{};
  bool network_clock_frozen_{};
  std::uint32_t frozen_network_clock_{};
  std::vector<NativeVistaContextEvent> context_events_;
  std::vector<NativeVistaTeardownEvent> teardown_events_;
  std::uint32_t bandwidth_=0xffffffffu; //5E0CE1→926D08=NONE.
  //5B07B2 clears these owners;5B100B publishes them on selected-name events.
  std::u16string peer_name_,peer_party_;
  std::array<std::array<std::byte,vista_session_source_size>,2> sessions_{};
  std::array<std::byte,vista_controller_user_bank_size+0x48> controllers_{};
  std::array<std::byte,vista_local_user_bank_size> local_users_{};
  std::array<std::byte,vista_input_control_bank_size> input_controls_{};
  std::array<VistaPcProfile,vista_local_user_count> selected_profiles_{};
  std::uint8_t profile_refresh_pending_{}; // Native owner of source882427.
  std::array<std::byte,vista_session_reconciliation_bank_size> reconciliation_{};
  // Original5BD60C/5BD629 rows start at owner+14h with36-byte stride.
  std::array<std::byte,0x14+2*36> registrations_{};
  std::array<std::byte,8> registry_{};
  std::array<std::byte,2*32> keys_{};
  void random_bytes(std::span<std::byte>);
};
} // namespace halo2
