//! TU1.5 62DE0/721B0 connected-team decisions and post-host-leave eligibility
//! 72150..721A6. See the independent unchanged-XBE controls in
//! tools/oracle_matchmaking_readiness_tu15.py. Native and service callers share
//! this owner; the C++ header contains typed IO declarations only. Transport admission and
//! original platform policy transactions remain explicit caller-owned inputs.

#[derive(Clone, Copy, Debug)]
pub struct Seat {
    pub occupied: bool,
    pub peer: u32,
    pub team: i32,
}

pub fn connected_teams(peers: &[u32], current: usize, seats: &[Seat]) -> bool {
    if peers.is_empty() || peers.len() > 16 || current >= peers.len() {
        return false;
    }
    let complete = (1_u32 << peers.len()) - 1;
    let mut connected = 1_u32 << current;
    for (index, mask) in peers.iter().enumerate() {
        if *mask == complete {
            connected |= 1_u32 << index;
        }
    }
    let mut represented = [0_u16; 16];
    for seat in seats {
        if seat.occupied
            && seat.peer < 16
            && connected & (1_u32 << seat.peer) != 0
            && (0..16).contains(&seat.team)
        {
            represented[seat.team as usize] |= 1_u16 << seat.peer;
        }
    }
    represented.iter().filter(|mask| **mask != 0).count() > 1
}

pub fn eligible(
    require_teams: bool,
    connected_teams: bool,
    check_original_policy: bool,
    original_live_logon_succeeded: bool,
    original_local_bit31: bool,
    disable_teams: bool,
    disable_local_policy: bool,
) -> bool {
    !((require_teams && !disable_teams && !connected_teams)
        || (check_original_policy
            && !disable_local_policy
            && original_live_logon_succeeded
            && original_local_bit31))
}

/// TU1.5 0x256AF7; the widget constructor remains native presentation.
pub fn error_dialog(state: i32, local_cancel: bool) -> Option<u32> {
    match state {
        0 | 2 | 3 => None,
        4 if local_cancel => None,
        5 => Some(0xac),
        4 => Some(0xad),
        6 | 14 | 19 => Some(0xae),
        _ => Some(0xab),
    }
}

/// Original unsigned UI-clock subtraction; no platform clock originates here.
pub fn error_clear(local_host: bool, state: i32, shown: u32, now: u32) -> bool {
    shown != 0 && local_host && state != 0 && now.wrapping_sub(shown) >= 3000
}

/// Native transport observations, separate from original Live policy inputs.
#[derive(Clone, Copy, Debug, Default)]
pub struct NativeLaunchObservation {
    pub session_present: bool,
    pub transferred: bool,
    pub loading: bool,
    pub start_supported: bool,
    pub start_ms: Option<u32>,
    pub team_mode: Option<bool>,
    pub current_peer: u32,
}

/// Preserve the native loading handoff policy in one Rust owner. Loading is the
/// native transport's completed reciprocal-link admission, not an original
/// XNet sensor result. Original platform logon/policy bits are never invented.
pub fn native_launch_ready(observation: NativeLaunchObservation, seats: &[Seat]) -> bool {
    if !observation.session_present
        || observation.transferred
        || !observation.loading
        || !observation.start_supported
        || observation.start_ms != Some(0)
    {
        return false;
    }
    if observation.team_mode != Some(true) {
        return true;
    }
    if seats.len() > 16 || seats.iter().any(|seat| seat.peer >= 16) {
        return false;
    }
    let count = seats
        .iter()
        .map(|seat| seat.peer as usize + 1)
        .max()
        .unwrap_or(0);
    let peers = vec![(1u32 << count) - 1; count];
    eligible(
        true,
        connected_teams(&peers, observation.current_peer as usize, seats),
        false,
        false,
        false,
        false,
        false,
    )
}
