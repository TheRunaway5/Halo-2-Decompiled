#pragma once
namespace halo2 {
// A bounded original19A2CE branch: established pregame session, ordinary
// membership ready, no countdown, local host. Online/membership-state paths
// are not inferred from these counts.
struct FrontendLobbyReadyInput {
  int session_enum = -1;
  int present_count{}, ready_count{};
  bool team_mode{};
  int distinct_team_count{}, maximum_team_count = 1;
};
inline int frontend_lobby_ready_status(const FrontendLobbyReadyInput &input) {
  if (input.session_enum == 0 || input.session_enum == 2 || input.session_enum == 4)
    return input.ready_count == 2 && input.present_count == input.ready_count ? 9 : 7;
  if (input.team_mode && (input.distinct_team_count < 1 ||
                         input.distinct_team_count > input.maximum_team_count))
    return 7;
  return 9;
}
} // namespace halo2
