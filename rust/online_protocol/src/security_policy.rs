//! Shared security policy; callers own SQL transactions, HTTP and game worlds.
use std::collections::HashSet;
pub const PLAYER_CAPACITY: u64 = 1000;
pub const ORIGIN_CAPACITY: u64 = 128;
pub const FOREIGN_CAPACITY: u64 = 512;
pub const OWNER_LISTING_CAPACITY: u64 = 16;
pub const LOCAL_LISTING_CAPACITY: u64 = 1000 - FOREIGN_CAPACITY;
pub fn listing_admission(origin: u64, foreign: u64, total: u64) -> u16 {
    if origin >= ORIGIN_CAPACITY { 429 } else if foreign >= FOREIGN_CAPACITY || total >= 1000 { 503 } else { 0 }
}
pub fn local_listing_admission(owner: u64, local: u64, total: u64) -> u16 {
    if owner >= OWNER_LISTING_CAPACITY { 429 }
    else if local >= LOCAL_LISTING_CAPACITY || total >= 1000 { 503 }
    else { 0 }
}
pub fn steam_revocation_accepted(status: u16, body: &str) -> bool {
    status == 200 && body.len() <= 65536 && serde_json::from_str::<serde_json::Value>(body)
        .is_ok_and(|v| v == serde_json::json!({"state": "signed_out"}))
}
// No raw write, ABI, entrypoint, lens-device, or render-pointer capability may
// cross the network. Player-camera calls also carry raw controller/observer
// banks that are installed before native execution. Unknown kinds fail closed.
// Kind 32 is a match step with numeric displayed views for the replay world's
// own lag-compensation owner.
// Local diagnostic tapes retain
// their separate trusted replay API.
pub fn network_replay_call(kind: u8) -> bool {
    matches!(kind, 1..=14 | 24 | 29 | 31 | 32)
}
#[derive(Default)]
pub struct EosAdmissions { principals: HashSet<[u64; 4]> }
impl EosAdmissions {
    pub fn admit(&mut self, principal: [u64; 4]) -> u16 {
        if self.principals.len() >= 32 || !self.principals.insert(principal) { 429 } else { 0 }
    }
    pub fn release(&mut self, principal: [u64; 4]) { self.principals.remove(&principal); }
    pub fn pending(&self) -> usize { self.principals.len() }
}
pub const PCR_SIZE: usize = 0xdc78;
pub fn persistable_result(raw: &[u8]) -> Option<Vec<u8>> {
    if raw.len() != PCR_SIZE { return None; }
    let mut result = vec![0; PCR_SIZE];
    for range in [0..2, 0x10..0x14, 0x20..0x150] { result[range.clone()].copy_from_slice(&raw[range]); }
    for slot in 0..16 {
        let at = 0x370 + slot * 0x94;
        for range in [at..at+1, at+2..at+10, at+12..at+76, at+0x88..at+0x89, at+0x90..at+0x94] {
            result[range.clone()].copy_from_slice(&raw[range]);
        }
        for stat in [7, 8, 9, 11] {
            let at = 0xe30 + 2 * (slot * 0x1b5 + stat);
            result[at..at+2].copy_from_slice(&raw[at..at+2]);
        }
    }
    for team in 0..16 {
        let at = 0xcb0 + team * 0x18;
        result[at..at+4].copy_from_slice(&raw[at..at+4]);
    }
    Some(result)
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn revocation_requires_exact_success_and_summary_excludes_detail_banks() {
        assert!(steam_revocation_accepted(200, r#"{"state":"signed_out"}"#));
        for (status, body) in [(500, r#"{"state":"signed_out"}"#), (200,"{}"), (200,"not json"),
            (200,r#"{"state":"signed_in"}"#)] { assert!(!steam_revocation_accepted(status, body)); }
        let raw = vec![255; PCR_SIZE]; let projected = persistable_result(&raw).unwrap();
        assert!(projected[0x44d0..0x48d0+8*45*2].iter().all(|b| *b == 0));
        for slot in 0..16 {
            assert_eq!(&projected[0x370+slot*0x94+0x4c..0x370+slot*0x94+0x5c], &[0; 16]);
            let base = 0xe30 + slot*437*2;
            for i in 0..437 { assert_eq!(projected[base+2*i], if [7,8,9,11].contains(&i) {255} else {0}); }
        }
        assert!(persistable_result(&raw[..PCR_SIZE-1]).is_none());
    }
    #[test]
    fn dangerous_replay_capabilities_are_not_network_operations() {
        for kind in 15..=23 { assert!(!network_replay_call(kind)); }
        for kind in [0, 25, 26, 27, 28, 30, 255] { assert!(!network_replay_call(kind)); }
        assert!(network_replay_call(1)); assert!(network_replay_call(24));
        assert!(network_replay_call(31)); // Numeric input to an independently installed native camera.
        assert!(network_replay_call(32)); // Step with numeric displayed views for the replay world's rewind owner.
        assert!(!network_replay_call(33));
    }
    #[test]
    fn one_principal_cannot_occupy_queue_or_foreign_bank() {
        let mut gate = EosAdmissions::default(); assert_eq!(gate.admit([1;4]),0);
        for _ in 0..32 { assert_eq!(gate.admit([1;4]),429); }
        assert_eq!(gate.admit([2;4]),0); gate.release([1;4]); assert_eq!(gate.pending(),1);
        assert_eq!(listing_admission(128,128,128),429);
        assert_eq!(listing_admission(0,128,128),0);
        assert_eq!(local_listing_admission(16,16,16),429);
        assert_eq!(local_listing_admission(0,488,488),503);
        assert_eq!(local_listing_admission(0,487,999),0);
        assert_eq!(local_listing_admission(0,0,1000),503);
    }
}
