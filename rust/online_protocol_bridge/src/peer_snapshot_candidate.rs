//! Snapshot ABI: genuine native byte banks and private delivery witness IO.
use crate::ffi;
use halo2_online_protocol::{peer_snapshot_candidate as core, peer_source_admissions::Context};

// SAFETY: The bank is exclusively owned by UniquePtr. Its vector has no thread affinity;
// reads borrow immutably and writes require a pinned exclusive reference. The
// protocol owner serializes all shared state with its Rust mutex.
#[allow(unsafe_code)]
unsafe impl Send for ffi::NativeSnapshotBuffer {}
struct NativeBuffer(cxx::UniquePtr<ffi::NativeSnapshotBuffer>);
impl core::Buffer for NativeBuffer {
    fn bytes(&self) -> &[u8] {
        self.0
            .as_ref()
            .expect("native byte bank")
            .snapshot_buffer_bytes()
    }
    fn write(&mut self, offset: usize, data: &[u8]) -> core::Result<()> {
        self.0
            .pin_mut()
            .snapshot_buffer_write(offset as u64, data)
            .map_err(|_| core::Error)
    }
}
fn allocate(n: usize) -> core::Result<NativeBuffer> {
    let bytes = ffi::new_snapshot_buffer(n as u64).map_err(|_| core::Error)?;
    if bytes.is_null() {
        Err(core::Error)
    } else {
        Ok(NativeBuffer(bytes))
    }
}
pub struct PeerSnapshotCandidate(core::Candidate<NativeBuffer>);
fn protect<T>(call: impl FnOnce() -> core::Result<T>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(call))
        .map_err(|_| "Rust snapshot transfer failed".to_owned())?
        .map_err(|e| e.to_string())
}
fn context(c: &ffi::SourceAdmissionsContext) -> Context {
    Context {
        session: c.session,
        epoch: c.epoch,
        membership_revision: c.membership_revision,
        peers: c.peers,
        host: c.host,
    }
}
fn to_context(c: Context) -> ffi::SourceAdmissionsContext {
    ffi::SourceAdmissionsContext {
        session: c.session,
        epoch: c.epoch,
        membership_revision: c.membership_revision,
        peers: c.peers,
        host: c.host,
    }
}
fn token(t: &ffi::NativeSnapshotToken) -> Option<core::Token> {
    t.present.then(|| core::Token {
        context: context(&t.context),
        recipients: t.recipients,
        nonce: t.nonce,
    })
}
fn to_token(t: Option<core::Token>) -> ffi::NativeSnapshotToken {
    ffi::NativeSnapshotToken {
        present: t.is_some(),
        context: to_context(t.map_or(
            Context {
                session: [0; 32],
                epoch: 0,
                membership_revision: 0,
                peers: 0,
                host: 16,
            },
            |t| t.context,
        )),
        recipients: t.map_or(0, |t| t.recipients),
        nonce: t.map_or(0, |t| t.nonce),
    }
}
fn close(mode: u32) -> Option<core::Close> {
    match mode {
        0 => Some(core::Close::Release),
        1 => Some(core::Close::Abort),
        _ => None,
    }
}
struct Lease<'a>(&'a ffi::NativeSnapshotLeaseIo);
impl core::LeaseIo<NativeBuffer> for Lease<'_> {
    fn witness(&self) -> core::Witness {
        let w = self.0.snapshot_witness();
        core::Witness {
            present: w.present,
            owner_matches: w.owner_matches,
            recipient: w.recipient,
            token: token(&w.token),
            bytes_present: w.bytes_present,
        }
    }
    fn issue(&self, t: core::Token, b: NativeBuffer) {
        self.0.snapshot_issue(&to_token(Some(t)), b.0);
    }
    fn destroy(&self) {
        self.0.snapshot_destroy();
    }
}
pub fn new_peer_snapshot_candidate(
    self_peer: u32,
    protocol: u16,
    c: &ffi::SourceAdmissionsContext,
    l: &ffi::NativeSnapshotLimits,
) -> Result<Box<PeerSnapshotCandidate>, String> {
    protect(|| {
        core::Candidate::new(
            self_peer,
            protocol,
            context(c),
            core::Limits {
                maximum_archive_bytes: l.maximum_archive_bytes,
                retry_ms: l.retry_ms,
                idle_ms: l.idle_ms,
                window_chunks: l.window_chunks,
            },
            allocate,
        )
        .map(|s| Box::new(PeerSnapshotCandidate(s)))
    })
}
pub fn peer_snapshot_self(s: &PeerSnapshotCandidate) -> u32 {
    s.0.self_peer()
}
pub fn peer_snapshot_begin(
    s: &PeerSnapshotCandidate,
    now: u64,
) -> Result<ffi::NativeSnapshotToken, String> {
    protect(|| s.0.begin(now).map(to_token))
}
pub fn peer_snapshot_arm(
    s: &PeerSnapshotCandidate,
    t: &ffi::NativeSnapshotToken,
) -> Result<bool, String> {
    protect(|| token(t).map_or(Ok(false), |t| s.0.acknowledge_arm(t)))
}
pub fn peer_snapshot_publish(
    s: &PeerSnapshotCandidate,
    t: &ffi::NativeSnapshotToken,
    archives: Vec<ffi::NativeSnapshotArchive>,
    now: u64,
) -> Result<bool, String> {
    protect(|| {
        let Some(t) = token(t) else {
            return Ok(false);
        };
        let mut rows = Vec::with_capacity(archives.len());
        for a in archives {
            if a.bytes.is_null() {
                return Ok(false);
            }
            rows.push(core::Archive {
                peer: a.peer,
                bytes: NativeBuffer(a.bytes),
            });
        }
        s.0.publish(t, rows, now)
    })
}
pub fn peer_snapshot_take(
    s: &PeerSnapshotCandidate,
    io: &ffi::NativeSnapshotLeaseIo,
) -> Result<bool, String> {
    protect(|| s.0.take_delivery(&Lease(io)))
}
pub fn peer_snapshot_capture(
    s: &PeerSnapshotCandidate,
    io: &ffi::NativeSnapshotLeaseIo,
) -> Result<bool, String> {
    protect(|| s.0.acknowledge_captured(&Lease(io)))
}
pub fn peer_snapshot_close(
    s: &PeerSnapshotCandidate,
    t: &ffi::NativeSnapshotToken,
    mode: u32,
) -> Result<bool, String> {
    protect(|| match (token(t), close(mode)) {
        (Some(t), Some(mode)) => s.0.close(t, mode),
        _ => Ok(false),
    })
}
pub fn peer_snapshot_reject(
    s: &PeerSnapshotCandidate,
    t: &ffi::NativeSnapshotToken,
) -> Result<bool, String> {
    protect(|| token(t).map_or(Ok(false), |t| s.0.reject(t)))
}
pub fn peer_snapshot_can_retire(
    s: &PeerSnapshotCandidate,
    t: &ffi::NativeSnapshotToken,
    io: &ffi::NativeSnapshotLeaseIo,
) -> Result<bool, String> {
    protect(|| token(t).map_or(Ok(false), |t| s.0.can_acknowledge_retired(t, &Lease(io))))
}
pub fn peer_snapshot_retire(
    s: &PeerSnapshotCandidate,
    t: &ffi::NativeSnapshotToken,
    io: &ffi::NativeSnapshotLeaseIo,
) -> Result<bool, String> {
    protect(|| token(t).map_or(Ok(false), |t| s.0.acknowledge_retired(t, &Lease(io))))
}
pub fn peer_snapshot_state(s: &PeerSnapshotCandidate) -> Result<ffi::NativeSnapshotState, String> {
    protect(|| {
        s.0.state().map(|v| ffi::NativeSnapshotState {
            context: to_context(v.context),
            recipients: v.recipients,
            pending: to_token(v.pending),
            armed: v.armed,
            delivered: v.delivered,
            captured: v.captured,
            retired: v.retired,
            rejected: v.rejected,
            closing_present: v.closing.is_some(),
            closing: match v.closing {
                Some(core::Close::Release) => 0,
                Some(core::Close::Abort) => 1,
                None => 0,
            },
            published: v.published,
            delivery_outstanding: v.delivery_outstanding,
            stalled: v.stalled,
            buffered_bytes: v.buffered_bytes,
            maximum_buffered_bytes: v.maximum_buffered_bytes,
            rejected_packets: v.rejected_packets,
        })
    })
}
pub fn peer_snapshot_poll(
    s: &PeerSnapshotCandidate,
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
pub fn peer_snapshot_receive(
    s: &PeerSnapshotCandidate,
    p: u32,
    bytes: &[u8],
    now: u64,
) -> Result<bool, String> {
    protect(|| Ok(s.0.receive(p, bytes, now)))
}
pub fn peer_snapshot_is_packet(bytes: &[u8]) -> bool {
    std::panic::catch_unwind(|| core::is_packet(bytes)).unwrap_or(false)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn raw_context_close_and_window_operands_do_not_alias() {
        let mut c = ffi::SourceAdmissionsContext {
            session: [1; 32],
            epoch: 0,
            membership_revision: 0,
            peers: 17,
            host: 0,
        };
        let l = ffi::NativeSnapshotLimits {
            maximum_archive_bytes: 4096,
            retry_ms: 10,
            idle_ms: 100,
            window_chunks: 32,
        };
        for p in [16, 256, 257, u32::MAX] {
            assert!(new_peer_snapshot_candidate(p, 124, &c, &l).is_err());
        }
        c.host = 256;
        assert!(new_peer_snapshot_candidate(0, 124, &c, &l).is_err());
        c.host = 0;
        let s = new_peer_snapshot_candidate(0, 124, &c, &l).unwrap();
        let t = peer_snapshot_begin(&s, 0).unwrap();
        for mode in [2, 256, 257, u32::MAX] {
            assert!(!peer_snapshot_close(&s, &t, mode).unwrap());
        }
        let mut bad = l;
        bad.window_chunks = 65537;
        assert!(new_peer_snapshot_candidate(0, 124, &c, &bad).is_err());
    }
}
