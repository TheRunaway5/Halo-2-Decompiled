//! Typed ABI for one synchronized Rust authority owner. No native world proof.
use crate::ffi;
use halo2_online_protocol::{
    migration_preparation::PreparationCut, peer_authority_transfer as core,
};
pub struct PeerAuthorityTransfer(core::Transfer);
fn protect<T>(call: impl FnOnce() -> core::Result<T>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(call))
        .map_err(|_| "Rust authority transfer failed".to_owned())?
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
fn cut(c: &ffi::MigrationPreparationCut) -> core::Result<PreparationCut> {
    crate::preparation_cut(c).ok_or(core::Error)
}
fn to_cut(c: PreparationCut) -> ffi::MigrationPreparationCut {
    crate::matchmaking_native_publication::cut(Some(c))
}
pub fn new_peer_authority_transfer(
    self_peer: u32,
    protocol: u16,
    c: &ffi::SourceAdmissionsContext,
    l: &ffi::AuthorityTransferLimits,
) -> Result<Box<PeerAuthorityTransfer>, String> {
    protect(|| {
        core::Transfer::new(
            self_peer,
            protocol,
            context(c),
            core::Limits {
                maximum_bytes: l.maximum_bytes,
                bytes_per_second: l.bytes_per_second,
                burst_bytes: l.burst_bytes,
                retry_ms: l.retry_ms,
                offer_ms: l.offer_ms,
                idle_ms: l.idle_ms,
                lifetime_ms: l.lifetime_ms,
                window_chunks: l.window_chunks,
                features: l.features,
            },
        )
        .map(|s| Box::new(PeerAuthorityTransfer(s)))
    })
}
pub fn peer_authority_transfer_context(
    s: &PeerAuthorityTransfer,
) -> Result<ffi::SourceAdmissionsContext, String> {
    protect(|| s.0.context().map(to_context))
}
pub fn peer_authority_transfer_replace(
    s: &PeerAuthorityTransfer,
    expected: &ffi::SourceAdmissionsContext,
    next: &ffi::SourceAdmissionsContext,
) -> Result<bool, String> {
    protect(|| s.0.replace_context(context(expected), context(next)))
}
pub fn peer_authority_transfer_supported(
    s: &PeerAuthorityTransfer,
    p: u32,
) -> Result<bool, String> {
    protect(|| s.0.supported(p))
}
pub fn peer_authority_transfer_feature_supported(
    s: &PeerAuthorityTransfer,
    p: u32,
    f: u32,
) -> Result<bool, String> {
    protect(|| s.0.feature_supported(p, f))
}
pub fn peer_authority_transfer_features(
    s: &PeerAuthorityTransfer,
) -> Result<ffi::AuthorityTransferFeatures, String> {
    protect(|| {
        s.0.feature_snapshot()
            .map(|(context, features)| ffi::AuthorityTransferFeatures {
                context: to_context(context),
                features,
            })
    })
}
pub fn peer_authority_transfer_publish(
    s: &PeerAuthorityTransfer,
    c: &ffi::SourceAdmissionsContext,
    tick: u64,
    bytes: &[u8],
    targets: &[u32],
    now: u64,
) -> Result<ffi::MigrationPreparationCut, String> {
    protect(|| {
        s.0.publish(context(c), tick, bytes.to_vec(), targets, now)
            .map(to_cut)
    })
}
pub fn peer_authority_transfer_ahead(
    s: &PeerAuthorityTransfer,
    c: &ffi::SourceAdmissionsContext,
    previous: &ffi::MigrationPreparationCut,
    tick: u64,
    bytes: &[u8],
    peer: u32,
    now: u64,
) -> Result<ffi::MigrationPreparationCut, String> {
    protect(|| {
        s.0.publish_ahead(context(c), cut(previous)?, tick, bytes.to_vec(), peer, now)
            .map(to_cut)
    })
}
pub fn peer_authority_transfer_delivered(
    s: &PeerAuthorityTransfer,
    c: &ffi::SourceAdmissionsContext,
    seal: &ffi::MigrationPreparationCut,
    p: u32,
) -> Result<bool, String> {
    protect(|| s.0.delivered_to(context(c), cut(seal)?, p))
}
pub fn peer_authority_transfer_failed(
    s: &PeerAuthorityTransfer,
    c: &ffi::SourceAdmissionsContext,
    seal: &ffi::MigrationPreparationCut,
    p: u32,
    now: u64,
) -> Result<bool, String> {
    protect(|| s.0.failed_to(context(c), cut(seal)?, p, now))
}
pub fn peer_authority_transfer_preparing(
    s: &PeerAuthorityTransfer,
    c: &ffi::SourceAdmissionsContext,
    seal: &ffi::MigrationPreparationCut,
    p: u32,
    now: u64,
) -> Result<bool, String> {
    protect(|| s.0.note_native_preparation(context(c), cut(seal)?, p, now))
}
pub fn peer_authority_transfer_poll(
    s: &PeerAuthorityTransfer,
    now: u64,
    budget: u32,
) -> Result<Vec<ffi::SourceAdmissionsPacket>, String> {
    protect(|| {
        s.0.poll(now, budget).map(|v| {
            v.into_iter()
                .map(|p| ffi::SourceAdmissionsPacket {
                    peer: p.peer,
                    bytes: p.bytes,
                })
                .collect()
        })
    })
}
pub fn peer_authority_transfer_receive(
    s: &PeerAuthorityTransfer,
    p: u32,
    bytes: &[u8],
    now: u64,
) -> Result<bool, String> {
    protect(|| Ok(s.0.receive(p, bytes, now)))
}
pub fn peer_authority_transfer_completed(
    s: &PeerAuthorityTransfer,
) -> Result<ffi::AuthorityTransferReceived, String> {
    protect(|| {
        Ok(match s.0.take_completed()? {
            Some(v) => ffi::AuthorityTransferReceived {
                present: true,
                peer: v.peer,
                context: to_context(v.context),
                seal: to_cut(v.seal),
                bytes: v.bytes,
            },
            None => ffi::AuthorityTransferReceived {
                present: false,
                peer: 16,
                context: to_context(s.0.context()?),
                seal: crate::matchmaking_native_publication::cut(None),
                bytes: Vec::new(),
            },
        })
    })
}
pub fn peer_authority_transfer_acknowledge(
    s: &PeerAuthorityTransfer,
    c: &ffi::SourceAdmissionsContext,
    seal: &ffi::MigrationPreparationCut,
) -> Result<bool, String> {
    protect(|| s.0.acknowledge_validated(context(c), cut(seal)?))
}
pub fn peer_authority_transfer_reject(
    s: &PeerAuthorityTransfer,
    c: &ffi::SourceAdmissionsContext,
    seal: &ffi::MigrationPreparationCut,
) -> Result<bool, String> {
    protect(|| s.0.reject_delivered(context(c), cut(seal)?))
}
pub fn peer_authority_transfer_validated(
    s: &PeerAuthorityTransfer,
) -> Result<Vec<ffi::AuthorityTransferReceipt>, String> {
    protect(|| {
        s.0.validated_cuts().map(|v| {
            v.into_iter()
                .map(|p| ffi::AuthorityTransferReceipt {
                    peer: p.peer,
                    context: to_context(p.context),
                    seal: to_cut(p.seal),
                })
                .collect()
        })
    })
}
pub fn peer_authority_transfer_is_packet(bytes: &[u8]) -> bool {
    std::panic::catch_unwind(|| core::is_packet(bytes)).unwrap_or(false)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn raw_native_indices_and_limits_do_not_alias() {
        let mut c = ffi::SourceAdmissionsContext {
            session: [1; 32],
            epoch: 0,
            membership_revision: 0,
            peers: 3,
            host: 0,
        };
        let l = ffi::AuthorityTransferLimits {
            maximum_bytes: 4096,
            bytes_per_second: 65536,
            burst_bytes: 2048,
            retry_ms: 10,
            offer_ms: 20,
            idle_ms: 500,
            lifetime_ms: 1000,
            window_chunks: 64,
            features: 128,
        };
        for bad in [16, 256, 257, u32::MAX] {
            assert!(new_peer_authority_transfer(bad, 124, &c, &l).is_err());
        }
        c.host = 256;
        assert!(new_peer_authority_transfer(0, 124, &c, &l).is_err());
        c.host = 0;
        let s = new_peer_authority_transfer(0, 124, &c, &l).unwrap();
        assert!(!peer_authority_transfer_supported(&s, 256).unwrap());
        let mut bad = l;
        bad.window_chunks = 0x10001;
        assert!(new_peer_authority_transfer(0, 124, &c, &bad).is_err());
    }
}
