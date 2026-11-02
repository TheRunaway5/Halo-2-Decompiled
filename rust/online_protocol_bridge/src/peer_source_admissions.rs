//! Typed ABI only. Immutable admission state, codec and retries live in Rust.
use crate::ffi;
use halo2_online_protocol::peer_source_admissions as core;
pub struct PeerSourceAdmissions(core::Exchange);
fn protect<T>(call: impl FnOnce() -> core::Result<T>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(call))
        .map_err(|_| "Rust source admission exchange failed".to_owned())?
        .map_err(|e| e.to_string())
}
fn context(c: &ffi::SourceAdmissionsContext) -> core::Context {
    core::Context {
        session: c.session,
        epoch: c.epoch,
        membership_revision: c.membership_revision,
        peers: c.peers,
        host: c.host,
    }
}
fn to_context(c: core::Context) -> ffi::SourceAdmissionsContext {
    ffi::SourceAdmissionsContext {
        session: c.session,
        epoch: c.epoch,
        membership_revision: c.membership_revision,
        peers: c.peers,
        host: c.host,
    }
}
fn roster(rows: &[ffi::SourceAdmissionsSeat]) -> Vec<core::Seat> {
    rows.iter()
        .map(|s| core::Seat {
            slot: s.slot,
            peer: s.peer,
            guest: s.guest,
            account: s.account,
        })
        .collect()
}
fn players(rows: &[ffi::SourceAdmissionsPlayer]) -> Vec<core::Player> {
    rows.iter()
        .map(|s| core::Player {
            slot: s.slot,
            player: s.player,
            identity: s.identity,
        })
        .collect()
}
pub fn new_peer_source_admissions(
    self_peer: u32,
    protocol: u16,
    c: &ffi::SourceAdmissionsContext,
    rows: &[ffi::SourceAdmissionsSeat],
) -> Result<Box<PeerSourceAdmissions>, String> {
    protect(|| {
        core::Exchange::new(self_peer, protocol, context(c), roster(rows))
            .map(|s| Box::new(PeerSourceAdmissions(s)))
    })
}
pub fn peer_source_admissions_resume(
    self_peer: u32,
    protocol: u16,
    next: &ffi::SourceAdmissionsContext,
    rows: &[ffi::SourceAdmissionsSeat],
    previous: &ffi::SourceAdmissionsAccepted,
) -> Result<Box<PeerSourceAdmissions>, String> {
    protect(|| {
        if !previous.present {
            return Err(core::Error);
        }
        core::Exchange::resume(
            self_peer,
            protocol,
            context(next),
            roster(rows),
            core::Accepted {
                context: context(&previous.context),
                digest: previous.digest,
                players: players(&previous.players),
            },
        )
        .map(|s| Box::new(PeerSourceAdmissions(s)))
    })
}
pub fn peer_source_admissions_context(
    state: &PeerSourceAdmissions,
) -> Result<ffi::SourceAdmissionsContext, String> {
    protect(|| Ok(to_context(state.0.context())))
}
pub fn peer_source_admissions_replace(
    state: &mut PeerSourceAdmissions,
    expected: &ffi::SourceAdmissionsContext,
    next: &ffi::SourceAdmissionsContext,
) -> Result<bool, String> {
    protect(|| state.0.replace_context(context(expected), context(next)))
}
pub fn peer_source_admissions_publish(
    state: &mut PeerSourceAdmissions,
    c: &ffi::SourceAdmissionsContext,
    rows: &[ffi::SourceAdmissionsPlayer],
) -> Result<bool, String> {
    protect(|| state.0.publish(context(c), players(rows)))
}
pub fn peer_source_admissions_accepted(
    state: &PeerSourceAdmissions,
) -> Result<ffi::SourceAdmissionsAccepted, String> {
    protect(|| {
        let value = state.0.accepted();
        Ok(ffi::SourceAdmissionsAccepted {
            present: value.is_some(),
            context: to_context(value.map(|v| v.context).unwrap_or(state.0.context())),
            digest: value.map(|v| v.digest).unwrap_or([0; 32]),
            players: value
                .map(|v| {
                    v.players
                        .iter()
                        .map(|p| ffi::SourceAdmissionsPlayer {
                            slot: p.slot,
                            player: p.player,
                            identity: p.identity,
                        })
                        .collect()
                })
                .unwrap_or_default(),
        })
    })
}
pub fn peer_source_admissions_confirmed(
    state: &PeerSourceAdmissions,
    peer: u32,
) -> Result<bool, String> {
    protect(|| Ok(state.0.confirmed(peer)))
}
pub fn peer_source_admissions_poll(
    state: &mut PeerSourceAdmissions,
    now: u64,
    budget: u32,
) -> Result<Vec<ffi::SourceAdmissionsPacket>, String> {
    protect(|| {
        Ok(state
            .0
            .poll(now, budget)
            .into_iter()
            .map(|p| ffi::SourceAdmissionsPacket {
                peer: p.peer,
                bytes: p.bytes,
            })
            .collect())
    })
}
pub fn peer_source_admissions_receive(
    state: &mut PeerSourceAdmissions,
    peer: u32,
    bytes: &[u8],
) -> Result<bool, String> {
    protect(|| Ok(state.0.receive(peer, bytes)))
}
pub fn peer_source_admissions_is_packet(bytes: &[u8]) -> bool {
    std::panic::catch_unwind(|| core::is_packet(bytes)).unwrap_or(false)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn native_unsigned_indices_and_legacy_owner_sentinel_are_preserved() {
        let c = ffi::SourceAdmissionsContext {
            session: [1; 32],
            epoch: 0,
            membership_revision: 0,
            peers: 3,
            host: 0,
        };
        let rows = (0..2)
            .map(|slot| ffi::SourceAdmissionsSeat {
                slot,
                peer: u32::MAX,
                guest: false,
                account: [slot as u8; 8],
            })
            .collect::<Vec<_>>();
        for bad in [16, 255, 256, u32::MAX] {
            assert!(new_peer_source_admissions(bad, 123, &c, &rows).is_err());
        }
        let mut wrong = c;
        wrong.host = 256;
        assert!(new_peer_source_admissions(0, 123, &wrong, &rows).is_err());
        wrong.host = 0;
        let s = new_peer_source_admissions(0, 123, &wrong, &rows).unwrap();
        assert_eq!(peer_source_admissions_context(&s).unwrap().host, 0);
        assert!(!peer_source_admissions_confirmed(&s, 256).unwrap());
    }
}
