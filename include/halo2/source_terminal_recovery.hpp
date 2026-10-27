#pragma once
#include "halo2/source_authority_receiver.hpp"

namespace halo2 {
// A cold, disconnected source ending only. This cannot resume a network role,
// advertise Ready, write competitive results or supply a winner/score/clock.
class SourceTerminalRecovery {
public:
  static std::shared_ptr<MultiplayerPreparedWorld> prepare(const MultiplayerOptions &,
      const SourceAuthorityRetainedWorld &,const OnlineMatchSession::TerminalObservation &);
};
} // namespace halo2
