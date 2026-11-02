//! Session feature limits, local ownership/views and postgame caller policy.
//! Actual mutexes, audio devices, buffers and native resource handles remain IO.
use crate::{migration_driver::Phase, peer_authority_transfer::Limits};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Native Session access failed")
    }
}
impl std::error::Error for Error {}
pub type Result<T> = std::result::Result<T, Error>;
pub fn admission_owner(slot: u32, peer: u32) -> Result<u8> {
    u8::try_from(if peer == u32::MAX { slot } else { peer }).map_err(|_| Error)
}
pub fn transfer_limits(snapshot: bool, selection: bool, terminal: bool) -> Limits {
    let mut features = 1 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256 | 512 | 8192;
    if snapshot {
        features |= 1024;
    }
    if selection {
        features |= 2048;
    }
    if terminal {
        features = (features & !512) | 4096;
    }
    Limits {
        features,
        ..Limits::default()
    }
}
pub fn control_postgame_pump(direct_present: bool) -> bool {
    !direct_present
}

/// The caller holds the real generation mutex. IO retains the selected actual
/// snapshot owner; no mutable Rust Session borrow spans these callbacks.
pub trait SnapshotIo {
    fn common_phase(&self) -> bool;
    fn supported(&self) -> Result<bool>;
    fn select(&self) -> Result<()>;
}
pub fn snapshot_view(io: &impl SnapshotIo) -> Result<()> {
    if io.common_phase() && io.supported()? {
        io.select()?;
    }
    Ok(())
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Preferences {
    pub enabled: bool,
    pub muted: u16,
    pub blocked: u16,
}
/// Device IO is performed on the same pinned genuine native audio owner as the
/// prior caller. Only exchange holds the generation mutex. Native RAII must
/// release it on unexpected unwinding, without holding a Rust Session borrow.
pub trait PostgameIo {
    type Frames;
    fn pin_audio(&self) -> Result<bool>;
    fn phase(&self) -> Result<u32>;
    fn enable(&self, enabled: bool) -> Result<()>;
    fn focused(&self) -> bool;
    fn focus(&self, focused: bool) -> Result<()>;
    fn radio(&self, pressed: bool, active: bool) -> Result<()>;
    fn blocks(&self) -> Result<u16>;
    fn set_blocks(&self, blocked: u16) -> Result<()>;
    fn preferences(&self) -> Result<Preferences>;
    fn outgoing(&self) -> Result<Self::Frames>;
    fn lock(&self) -> Result<()>;
    fn unlock(&self) -> Result<()>;
    fn fence(&self) -> Result<()>;
    /// IO samples the actual retirement readiness/time and calls the existing
    /// Rust Session exchange. That owner rechecks the current postgame phase.
    fn exchange(&self, prefs: Preferences, frames: Self::Frames) -> Result<Self::Frames>;
    fn deliver(&self, frames: Self::Frames) -> Result<()>;
}
pub fn postgame_voice(io: &impl PostgameIo) -> Result<()> {
    if !io.pin_audio()? {
        return Ok(());
    }
    let phase = io.phase()?;
    if phase == Phase::Failed as u32 {
        return io.enable(false);
    }
    if phase != Phase::Reviewing as u32 && phase != Phase::Postgame as u32 {
        return Ok(());
    }
    io.focus(io.focused())?;
    io.radio(false, true)?;
    io.set_blocks(io.blocks()?)?;
    let selected = io.preferences()?;
    let mut outgoing = Some(io.outgoing()?);
    io.lock()?;
    let received = match io.fence() {
        Ok(()) => io.exchange(selected, outgoing.take().unwrap()),
        Err(error) => Err(error),
    };
    let unlocked = io.unlock();
    let received = received.and_then(|frames| unlocked.map(|_| frames))?;
    io.deliver(received)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::cell::{Cell, RefCell};
    use std::rc::Rc;
    #[test]
    fn legacy_owner_normalization_preserves_wide_invalid_operands() {
        for slot in 0..16 {
            assert_eq!(admission_owner(slot, u32::MAX).unwrap(), slot as u8);
        }
        for p in [256, 257, u32::MAX - 1] {
            assert!(admission_owner(0, p).is_err());
        }
        assert!(admission_owner(256, u32::MAX).is_err());
        assert_eq!(admission_owner(15, 4).unwrap(), 4);
    }
    #[test]
    fn all_feature_combinations_and_successor_terminal_operand() {
        for bits in 0..8 {
            let l = transfer_limits(bits & 1 != 0, bits & 2 != 0, bits & 4 != 0);
            let mut expected = 9215;
            if bits & 1 != 0 {
                expected |= 1024;
            }
            if bits & 2 != 0 {
                expected |= 2048;
            }
            if bits & 4 != 0 {
                expected = (expected & !512) | 4096;
            }
            assert_eq!(l.features, expected);
            assert_eq!(l.maximum_bytes, 33554432);
            assert_eq!(l.bytes_per_second, 524288);
            assert_eq!(
                (
                    l.burst_bytes,
                    l.retry_ms,
                    l.offer_ms,
                    l.idle_ms,
                    l.lifetime_ms,
                    l.window_chunks
                ),
                (16384, 250, 500, 15000, 300000, 64)
            );
            let p = crate::matchmaking_native_publication::Successor {
                multiple: true,
                selection: false,
                snapshot: false,
                selection_feature: bits & 2 != 0,
                snapshot_feature: bits & 1 != 0,
            };
            assert_eq!(
                p.transfer_limits().features,
                transfer_limits(bits & 1 != 0, bits & 2 != 0, false).features
            );
        }
        assert!(control_postgame_pump(false));
        assert!(!control_postgame_pump(true));
    }
    #[test]
    fn snapshot_view_retains_phase_short_circuit_and_exact_selection() {
        struct S {
            common: bool,
            supported: bool,
            calls: RefCell<Vec<&'static str>>,
        }
        impl SnapshotIo for S {
            fn common_phase(&self) -> bool {
                self.common
            }
            fn supported(&self) -> Result<bool> {
                self.calls.borrow_mut().push("supported");
                Ok(self.supported)
            }
            fn select(&self) -> Result<()> {
                self.calls.borrow_mut().push("select");
                Ok(())
            }
        }
        for common in [false, true] {
            for supported in [false, true] {
                let s = S {
                    common,
                    supported,
                    calls: RefCell::new(vec![]),
                };
                snapshot_view(&s).unwrap();
                assert_eq!(s.calls.borrow().contains(&"supported"), common);
                assert_eq!(s.calls.borrow().contains(&"select"), common && supported);
            }
        }
    }
    struct Frames {
        values: Vec<u8>,
        held: Rc<Cell<bool>>,
        drops: Rc<RefCell<Vec<bool>>>,
    }
    impl Drop for Frames {
        fn drop(&mut self) {
            self.drops.borrow_mut().push(self.held.get());
        }
    }
    struct F {
        audio: bool,
        phase: u32,
        held: Rc<Cell<bool>>,
        calls: RefCell<Vec<String>>,
        fail: Option<&'static str>,
        drops: Rc<RefCell<Vec<bool>>>,
        delivered: RefCell<Vec<u8>>,
    }
    impl Default for F {
        fn default() -> Self {
            Self {
                audio: true,
                phase: Phase::Reviewing as u32,
                held: Rc::new(Cell::new(false)),
                calls: RefCell::new(vec![]),
                fail: None,
                drops: Rc::new(RefCell::new(vec![])),
                delivered: RefCell::new(vec![]),
            }
        }
    }
    impl F {
        fn call(&self, n: &str) -> Result<()> {
            self.calls
                .borrow_mut()
                .push(format!("{n}:{}", self.held.get()));
            if self.fail == Some(n) {
                Err(Error)
            } else {
                Ok(())
            }
        }
        fn frames(&self, v: Vec<u8>) -> Frames {
            Frames {
                values: v,
                held: self.held.clone(),
                drops: self.drops.clone(),
            }
        }
    }
    impl PostgameIo for F {
        type Frames = Frames;
        fn pin_audio(&self) -> Result<bool> {
            self.call("pin")?;
            Ok(self.audio)
        }
        fn phase(&self) -> Result<u32> {
            self.call("phase")?;
            Ok(self.phase)
        }
        fn enable(&self, b: bool) -> Result<()> {
            assert!(!b);
            self.call("disable")
        }
        fn focused(&self) -> bool {
            true
        }
        fn focus(&self, b: bool) -> Result<()> {
            assert!(b);
            self.call("focus")
        }
        fn radio(&self, a: bool, b: bool) -> Result<()> {
            assert!(!a && b);
            self.call("radio")
        }
        fn blocks(&self) -> Result<u16> {
            self.call("blocks")?;
            Ok(0x8001)
        }
        fn set_blocks(&self, b: u16) -> Result<()> {
            assert_eq!(b, 0x8001);
            self.call("set_blocks")
        }
        fn preferences(&self) -> Result<Preferences> {
            self.call("preferences")?;
            Ok(Preferences {
                enabled: true,
                muted: 2,
                blocked: 0x8001,
            })
        }
        fn outgoing(&self) -> Result<Frames> {
            self.call("outgoing")?;
            Ok(self.frames(vec![1, 2, 3]))
        }
        fn lock(&self) -> Result<()> {
            assert!(!self.held.get());
            self.call("lock")?;
            self.held.set(true);
            Ok(())
        }
        fn unlock(&self) -> Result<()> {
            assert!(self.held.get());
            self.call("unlock")?;
            self.held.set(false);
            Ok(())
        }
        fn fence(&self) -> Result<()> {
            assert!(self.held.get());
            self.call("fence")
        }
        fn exchange(&self, p: Preferences, f: Frames) -> Result<Frames> {
            assert!(self.held.get());
            assert_eq!(f.values, [1, 2, 3]);
            assert_eq!(p.muted, 2);
            self.call("exchange")?;
            Ok(self.frames(vec![8, 9]))
        }
        fn deliver(&self, f: Frames) -> Result<()> {
            assert!(!self.held.get());
            self.call("deliver")?;
            *self.delivered.borrow_mut() = f.values.clone();
            Ok(())
        }
    }
    #[test]
    fn postgame_missing_failed_inactive_and_wide_phases() {
        let f = F {
            audio: false,
            ..F::default()
        };
        postgame_voice(&f).unwrap();
        assert_eq!(*f.calls.borrow(), ["pin:false"]);
        for phase in [0, 1, 2, 5, 6, 7, 8, 256, 257, u32::MAX] {
            let f = F {
                phase,
                ..F::default()
            };
            postgame_voice(&f).unwrap();
            assert_eq!(
                f.calls.borrow().iter().any(|s| s.starts_with("disable")),
                phase == 5
            );
            assert!(!f.calls.borrow().iter().any(|s| s.starts_with("focus")));
        }
    }
    #[test]
    fn postgame_device_order_fence_and_frame_delivery() {
        for phase in [Phase::Reviewing, Phase::Postgame] {
            let f = F {
                phase: phase as u32,
                ..F::default()
            };
            postgame_voice(&f).unwrap();
            assert_eq!(
                *f.calls.borrow(),
                [
                    "pin:false",
                    "phase:false",
                    "focus:false",
                    "radio:false",
                    "blocks:false",
                    "set_blocks:false",
                    "preferences:false",
                    "outgoing:false",
                    "lock:false",
                    "fence:true",
                    "exchange:true",
                    "unlock:true",
                    "deliver:false"
                ]
            );
            assert_eq!(*f.delivered.borrow(), [8, 9]);
            assert!(!f.held.get());
        }
    }
    #[test]
    fn postgame_io_errors_unlock_and_fence_refusal_drops_caller_frames_after_unlock() {
        for fail in [
            "pin",
            "phase",
            "focus",
            "radio",
            "blocks",
            "set_blocks",
            "preferences",
            "outgoing",
            "lock",
            "fence",
            "exchange",
            "deliver",
        ] {
            let f = F {
                fail: Some(fail),
                ..F::default()
            };
            assert!(postgame_voice(&f).is_err());
            assert!(!f.held.get());
            if fail == "fence" {
                assert_eq!(*f.drops.borrow(), [false]);
            }
            if fail == "exchange" {
                assert!(f.calls.borrow().iter().any(|s| s == "unlock:true"));
            }
        }
    }
}
