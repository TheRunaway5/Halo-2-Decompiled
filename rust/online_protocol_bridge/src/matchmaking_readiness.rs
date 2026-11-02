//! Typed native IO into the same readiness owner used by the Rust service.
use crate::ffi;
use halo2_online_protocol::matchmaking_readiness as core;

fn seats(rows: &[ffi::MatchmakingSeat]) -> Vec<core::Seat> {
    rows.iter()
        .map(|row| core::Seat {
            occupied: row.occupied,
            peer: row.peer,
            team: row.team,
        })
        .collect()
}

pub fn matchmaking_connected_teams(
    peers: &[u32],
    current: u32,
    rows: &[ffi::MatchmakingSeat],
) -> bool {
    core::connected_teams(peers, current as usize, &seats(rows))
}

pub fn matchmaking_eligibility(
    require_teams: bool,
    connected: bool,
    check_original_policy: bool,
    live_logon: bool,
    local_bit31: bool,
    disable_teams: bool,
    disable_local_policy: bool,
) -> ffi::MatchmakingEligibility {
    let ready = core::eligible(
        require_teams,
        connected,
        check_original_policy,
        live_logon,
        local_bit31,
        disable_teams,
        disable_local_policy,
    );
    ffi::MatchmakingEligibility {
        ready,
        defer: !ready,
    }
}

pub fn matchmaking_error_dialog(state: i32, local_cancel: bool) -> ffi::MatchmakingErrorDialog {
    let dialog = core::error_dialog(state, local_cancel);
    ffi::MatchmakingErrorDialog {
        present: dialog.is_some(),
        id: dialog.unwrap_or(0),
    }
}

pub fn matchmaking_error_clear(local_host: bool, state: i32, shown: u32, now: u32) -> bool {
    core::error_clear(local_host, state, shown, now)
}

pub fn matchmaking_native_launch_ready(
    observation: &ffi::MatchmakingLaunchObservation,
    rows: &[ffi::MatchmakingSeat],
) -> bool {
    core::native_launch_ready(
        core::NativeLaunchObservation {
            session_present: observation.session_present,
            transferred: observation.transferred,
            loading: observation.loading,
            start_supported: observation.start_supported,
            start_ms: observation.start_present.then_some(observation.start_ms),
            team_mode: observation
                .team_mode_present
                .then_some(observation.team_mode),
            current_peer: observation.current_peer,
        },
        &seats(rows),
    )
}
