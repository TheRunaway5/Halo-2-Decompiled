//! Session access ABI only; policy and orchestration stay in the safe core.
use crate::{
    ffi,
    matchmaking_native_session::{NativeSession, seal},
};
use halo2_online_protocol::{matchmaking_native_access as core, peer_authority_transfer::Limits};
pub(super) fn limits(l: Limits) -> ffi::AuthorityTransferLimits {
    ffi::AuthorityTransferLimits {
        maximum_bytes: l.maximum_bytes,
        bytes_per_second: l.bytes_per_second,
        burst_bytes: l.burst_bytes,
        retry_ms: l.retry_ms,
        offer_ms: l.offer_ms,
        idle_ms: l.idle_ms,
        lifetime_ms: l.lifetime_ms,
        window_chunks: l.window_chunks,
        features: l.features,
    }
}
pub fn native_session_authority_limits(s: &NativeSession) -> ffi::AuthorityTransferLimits {
    limits(s.0.authority_limits())
}
pub fn native_session_local_slots(s: &NativeSession) -> Vec<u32> {
    s.0.local_slots().into_iter().map(u32::from).collect()
}
pub fn native_session_physical_peers(s: &NativeSession) -> Vec<u32> {
    s.0.physical_peers().into_iter().map(u32::from).collect()
}
pub fn native_session_presence(s: &NativeSession) -> ffi::NativeSessionPresence {
    let p = s.0.presence();
    ffi::NativeSessionPresence {
        present: p.is_some(),
        members: p.unwrap_or(0),
    }
}
pub fn native_session_continuity_view(s: &NativeSession) -> ffi::NativeSessionContinuity {
    let c = s.0.continuity_view();
    ffi::NativeSessionContinuity {
        present: c.is_some(),
        epoch: c.as_ref().map_or(0, |c| c.epoch),
        host: c.as_ref().map_or(16, |c| u32::from(c.host)),
        members: c.as_ref().map_or(0, |c| c.members),
        selected: seal(c.and_then(|c| c.selected)),
    }
}
pub fn native_session_control_postgame(direct_present: bool) -> bool {
    core::control_postgame_pump(direct_present)
}
fn io<T>(v: Result<T, cxx::Exception>) -> core::Result<T> {
    v.map_err(|_| core::Error)
}
fn protect<T>(f: impl FnOnce() -> core::Result<T>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(f))
        .map_err(|_| "Native Session access failed".to_owned())?
        .map_err(|e| e.to_string())
}
struct Snapshot<'a>(&'a ffi::NativeSnapshotViewIo);
impl core::SnapshotIo for Snapshot<'_> {
    fn common_phase(&self) -> bool {
        self.0.snapshot_view_common()
    }
    fn supported(&self) -> core::Result<bool> {
        io(self.0.snapshot_view_supported())
    }
    fn select(&self) -> core::Result<()> {
        io(self.0.snapshot_view_select())
    }
}
pub fn native_session_snapshot_view(v: &ffi::NativeSnapshotViewIo) -> Result<(), String> {
    protect(|| core::snapshot_view(&Snapshot(v)))
}
struct Postgame<'a>(&'a ffi::NativePostgameIo);
impl core::PostgameIo for Postgame<'_> {
    type Frames = Vec<ffi::NativeSessionAudio>;
    fn pin_audio(&self) -> core::Result<bool> {
        io(self.0.postgame_pin_audio())
    }
    fn phase(&self) -> core::Result<u32> {
        io(self.0.postgame_phase())
    }
    fn enable(&self, b: bool) -> core::Result<()> {
        io(self.0.postgame_enable(b))
    }
    fn focused(&self) -> bool {
        self.0.postgame_focused()
    }
    fn focus(&self, b: bool) -> core::Result<()> {
        io(self.0.postgame_focus(b))
    }
    fn radio(&self, a: bool, b: bool) -> core::Result<()> {
        io(self.0.postgame_radio(a, b))
    }
    fn blocks(&self) -> core::Result<u16> {
        io(self.0.postgame_blocks())
    }
    fn set_blocks(&self, b: u16) -> core::Result<()> {
        io(self.0.postgame_set_blocks(b))
    }
    fn preferences(&self) -> core::Result<core::Preferences> {
        let p = io(self.0.postgame_preferences())?;
        Ok(core::Preferences {
            enabled: p.enabled,
            muted: p.muted,
            blocked: p.blocked,
        })
    }
    fn outgoing(&self) -> core::Result<Self::Frames> {
        io(self.0.postgame_outgoing())
    }
    fn lock(&self) -> core::Result<()> {
        io(self.0.postgame_lock())
    }
    fn unlock(&self) -> core::Result<()> {
        io(self.0.postgame_unlock())
    }
    fn fence(&self) -> core::Result<()> {
        io(self.0.postgame_fence())
    }
    fn exchange(&self, p: core::Preferences, f: Self::Frames) -> core::Result<Self::Frames> {
        io(self.0.postgame_exchange(
            &ffi::NativePostgamePreferences {
                enabled: p.enabled,
                muted: p.muted,
                blocked: p.blocked,
            },
            f,
        ))
    }
    fn deliver(&self, f: Self::Frames) -> core::Result<()> {
        io(self.0.postgame_deliver(f))
    }
}
pub fn native_session_pump_postgame(v: &ffi::NativePostgameIo) -> Result<(), String> {
    protect(|| core::postgame_voice(&Postgame(v)))
}
