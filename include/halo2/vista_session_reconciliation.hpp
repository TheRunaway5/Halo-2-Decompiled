#pragma once
#include "halo2/vista_controller_profile.hpp"
#include "halo2/vista_session_local_peer.hpp"

namespace halo2 {
// Original91A908..91AB80: two generations, two peer publication clocks,
// two membership revisions, and two complete130h cached variants.
inline constexpr std::size_t vista_session_reconciliation_bank_size=0x278;
struct VistaSessionReconciliationBanks {
  std::span<std::byte> session,local_users,controllers,retained;
};
struct VistaSessionReconciliationThresholds {
  std::int32_t peer,add,remove,update; // Actual8F8DCC/DD0/DD4/DD8 words.
};
class VistaSessionReconciliationServices : public VistaSessionLocalPeerServices {
 public:
  virtual std::uint32_t language()=0; // Actual4381FD calls, including unused call.
  // Actual map/game-data-backed5B087B and5B0A25 source service results.
  virtual std::uint16_t allowed_team_mask(std::span<const std::byte> session)=0;
  virtual std::uint16_t primary_allowed_team_mask()=0;
  virtual std::uint8_t join_context_byte()=0; // Actual609674->61A995 AL.
  // Actual5B4C3B authored-name lookup/RNG. Preserve destination tail bytes.
  virtual bool authored_random_name(std::span<std::byte> destination64)=0;
  // Actual optional session+7B78 vtable+C identity admission owner.
  virtual bool identity_admitted(std::uint32_t bound_owner,VistaLocalUserIdentity)=0;
  // Actual5BED40 source operands; real transport binding owns delivery.
  virtual void send_session_message(std::uint32_t transport_owner,
      std::uint32_t session_slot,std::uint32_t channel,std::uint32_t message_id,
      std::span<const std::byte> payload)=0;
  void send_peer_message(const VistaSessionLocalPeerMessage&) final;
};
// Complete5B333D normal retained-owner control chain, including generation
// invalidation, full peer publication, language, four real local-user selectors,
// insert/update/remove actions and5B2F21 final payload/variant cache publication.
// Source local state5 and remote state3 are admitted;7/8 obey their source flag
// gate and parameter guard. Other states retain their original early returns.
// All bank bindings are genuine caller-owned source owners. Platform/map/name,
// clock, optional identity-admission and transport services must provide real
// results and must not reenter these banks. No default successful services are
// provided. Exceptions after a service or mutation are not rolled back.
void vista_session_reconcile_local_users(VistaSessionReconciliationBanks,
    VistaSessionReconciliationThresholds,const VistaSessionLocalPeerValues&,
    VistaSessionReconciliationServices&);
// Exact5B2F21 standalone entry, for the same retained owner and service binding.
void vista_session_publish_final_players(VistaSessionReconciliationBanks,
    VistaSessionReconciliationServices&);
} // namespace halo2
