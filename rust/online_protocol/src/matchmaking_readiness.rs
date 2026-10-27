//! TU1.5 62DE0/721B0 connected-team decisions and post-host-leave eligibility
//! 72150..721A6. See the independent unchanged-XBE controls in
//! tools/oracle_matchmaking_readiness_tu15.py and the C++ source projection in
//! include/halo2/original_matchmaking_readiness.hpp. Transport admission and
//! original platform policy transactions remain explicit caller-owned inputs.

pub struct Seat { pub occupied: bool, pub peer: u32, pub team: i32 }

pub fn connected_teams(peers: &[u32], current: usize, seats: &[Seat]) -> bool {
    if peers.is_empty() || peers.len() > 16 || current >= peers.len() { return false; }
    let complete=(1_u32<<peers.len())-1;
    let mut connected=1_u32<<current;
    for (index,mask) in peers.iter().enumerate() {
        if *mask==complete {connected|=1_u32<<index;}
    }
    let mut represented=[0_u16;16];
    for seat in seats {
        if seat.occupied && seat.peer<16 && connected&(1_u32<<seat.peer)!=0 && (0..16).contains(&seat.team) {
            represented[seat.team as usize]|=1_u16<<seat.peer;
        }
    }
    represented.iter().filter(|mask|**mask!=0).count()>1
}

pub fn eligible(require_teams: bool, connected_teams: bool, check_original_policy: bool,
    original_live_logon_succeeded: bool, original_local_bit31: bool,
    disable_teams: bool, disable_local_policy: bool) -> bool {
    !((require_teams && !disable_teams && !connected_teams) ||
      (check_original_policy && !disable_local_policy && original_live_logon_succeeded && original_local_bit31))
}
