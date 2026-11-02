//! Complete Session receive caller. Native IO retains the real generation lock,
//! pinned resource owners and history. This is the modern replacement protocol,
//! not recovered original Live/XNet or evidence of a prepared native world.
use crate::{
    common_cut, host_agreement, peer_authority_transfer, peer_mesh_quality,
    peer_snapshot_candidate, peer_source_admissions, survivor_selection,
};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Native receive dispatch failed")
    }
}
impl std::error::Error for Error {}
pub type Result<T> = std::result::Result<T, Error>;
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Target {
    Selection,
    Snapshot,
    Common,
    Agreement,
    Quality,
    Admissions,
    Authority,
}
#[derive(Clone, Copy, Debug, Default)]
pub struct Session {
    pub common: bool,
    pub native_control: bool,
    pub handoff: bool,
    pub direct: bool,
    pub epochs: bool,
}
/// Native callbacks borrow no mutable Rust Session reference. They only observe
/// its existing projection, pin actual resource owners, or perform the chosen
/// operation. The native adapter must release its lock on unexpected unwinding.
pub trait Io {
    fn lock(&self) -> Result<()>;
    fn unlock(&self) -> Result<()>;
    fn session(&self) -> Result<Session>;
    fn supported(&self, target: Target) -> Result<bool>;
    fn pin(&self, target: Target) -> Result<()>;
    fn present(&self, target: Target) -> Result<bool>;
    fn epoch(&self, target: Target) -> Result<u32>;
    fn agreement_binding(&self) -> Result<Option<u32>>;
    fn history_len(&self) -> Result<usize>;
    fn history_epoch(&self, index: usize) -> Result<u32>;
    fn history_receive(&self, index: usize, source: u32, packet: &[u8]) -> Result<()>;
    fn receive(&self, target: Target, source: u32, packet: &[u8]) -> Result<()>;
    fn selection_abort(&self) -> Result<()>;
    fn selection_progress(&self) -> Result<()>;
    fn fence(&self) -> Result<()>;
    fn general_receive(&self, source: u32, epoch: u32, packet: &[u8]) -> Result<bool>;
    fn observe_host(&self) -> Result<()>;
    fn refresh(&self) -> Result<()>;
}
pub fn classify(packet: &[u8]) -> Option<Target> {
    if survivor_selection::is_packet(packet) {
        Some(Target::Selection)
    } else if peer_snapshot_candidate::is_packet(packet) {
        Some(Target::Snapshot)
    } else if common_cut::is_packet(packet) {
        Some(Target::Common)
    } else if host_agreement::is_packet(packet) {
        Some(Target::Agreement)
    } else if peer_mesh_quality::is_packet(packet) {
        Some(Target::Quality)
    } else if peer_source_admissions::is_packet(packet) {
        Some(Target::Admissions)
    } else if peer_authority_transfer::is_packet(packet) {
        Some(Target::Authority)
    } else {
        None
    }
}
pub fn dispatch(io: &impl Io, source: u32, packet: &[u8], epoch: u32) -> Result<()> {
    let target = classify(packet);
    io.lock()?;
    // Decide locked work and which actual owner can outlive the generation lock.
    // Unlock is attempted on every ordinary IO error, as native lock_guard did.
    let next = (|| -> Result<Option<(Target, bool)>> {
        let s = io.session()?;
        match target {
            Some(Target::Selection) => {
                if s.common && io.supported(Target::Selection)? {
                    io.pin(Target::Selection)?;
                    if epoch == io.epoch(Target::Selection)? {
                        io.receive(Target::Selection, source, packet)?;
                        io.selection_abort()?;
                        io.selection_progress()?;
                    }
                }
            }
            Some(Target::Common) => {
                if s.common && io.supported(Target::Common)? {
                    io.pin(Target::Common)?;
                    if !s.epochs || epoch == io.epoch(Target::Common)? {
                        io.receive(Target::Common, source, packet)?;
                    }
                }
                io.selection_abort()?;
                io.selection_progress()?;
            }
            Some(Target::Agreement) => {
                io.pin(Target::Agreement)?;
                if s.handoff
                    && io.agreement_binding()? == Some(epoch)
                    && io.present(Target::Agreement)?
                {
                    io.receive(Target::Agreement, source, packet)?;
                    io.selection_progress()?;
                    io.fence()?;
                } else {
                    for index in 0..io.history_len()? {
                        if io.history_epoch(index)? == epoch {
                            io.history_receive(index, source, packet)?;
                            break;
                        }
                    }
                }
            }
            Some(
                t @ (Target::Snapshot | Target::Quality | Target::Admissions | Target::Authority),
            ) => {
                let enabled = match t {
                    Target::Snapshot => s.common && io.supported(t)?,
                    Target::Quality => s.direct,
                    _ => s.direct && s.native_control,
                };
                if enabled {
                    io.pin(t)?;
                    if io.present(t)? {
                        return Ok(Some((t, t == Target::Snapshot || s.epochs)));
                    }
                }
            }
            None => {
                io.fence()?;
                if io.general_receive(source, epoch, packet)? {
                    io.pin(Target::Agreement)?;
                    if io.present(Target::Agreement)? {
                        io.observe_host()?;
                    }
                }
                io.refresh()?;
            }
        }
        Ok(None)
    })();
    let unlocked = io.unlock();
    let next = next.and_then(|next| unlocked.map(|_| next))?;
    if let Some((target, check_epoch)) = next {
        if !check_epoch || epoch == io.epoch(target)? {
            io.receive(target, source, packet)?;
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::cell::{Cell, RefCell};
    struct Fixture {
        s: Session,
        supported: bool,
        present: bool,
        epoch: u32,
        binding: Option<u32>,
        history: Vec<u32>,
        general: bool,
        held: Cell<bool>,
        fail: Option<&'static str>,
        calls: RefCell<Vec<String>>,
    }
    impl Default for Fixture {
        fn default() -> Self {
            Self {
                s: Session {
                    common: true,
                    native_control: true,
                    handoff: true,
                    direct: true,
                    epochs: true,
                },
                supported: true,
                present: true,
                epoch: 3,
                binding: Some(3),
                history: vec![2, 3, 3, 4],
                general: true,
                held: Cell::new(false),
                fail: None,
                calls: RefCell::new(vec![]),
            }
        }
    }
    impl Fixture {
        fn call(&self, name: &str) -> Result<()> {
            self.calls
                .borrow_mut()
                .push(format!("{name}:{}", self.held.get()));
            if self.fail == Some(name) {
                Err(Error)
            } else {
                Ok(())
            }
        }
        fn contains(&self, name: &str) -> bool {
            self.calls.borrow().iter().any(|s| s.starts_with(name))
        }
    }
    impl Io for Fixture {
        fn lock(&self) -> Result<()> {
            assert!(!self.held.replace(true));
            self.call("lock")
        }
        fn unlock(&self) -> Result<()> {
            assert!(self.held.get());
            self.call("unlock")?;
            self.held.set(false);
            Ok(())
        }
        fn session(&self) -> Result<Session> {
            self.call("session")?;
            Ok(self.s)
        }
        fn supported(&self, _: Target) -> Result<bool> {
            self.call("supported")?;
            Ok(self.supported)
        }
        fn pin(&self, _: Target) -> Result<()> {
            assert!(self.held.get());
            self.call("pin")
        }
        fn present(&self, _: Target) -> Result<bool> {
            self.call("present")?;
            Ok(self.present)
        }
        fn epoch(&self, t: Target) -> Result<u32> {
            self.call(&format!("epoch:{t:?}"))?;
            Ok(self.epoch)
        }
        fn agreement_binding(&self) -> Result<Option<u32>> {
            Ok(self.binding)
        }
        fn history_len(&self) -> Result<usize> {
            Ok(self.history.len())
        }
        fn history_epoch(&self, i: usize) -> Result<u32> {
            Ok(self.history[i])
        }
        fn history_receive(&self, i: usize, p: u32, _: &[u8]) -> Result<()> {
            self.call(&format!("history:{i}:{p}"))
        }
        fn receive(&self, t: Target, p: u32, _: &[u8]) -> Result<()> {
            self.call(&format!("receive:{t:?}:{p}"))
        }
        fn selection_abort(&self) -> Result<()> {
            self.call("abort")
        }
        fn selection_progress(&self) -> Result<()> {
            self.call("progress")
        }
        fn fence(&self) -> Result<()> {
            self.call("fence")
        }
        fn general_receive(&self, p: u32, e: u32, _: &[u8]) -> Result<bool> {
            self.call(&format!("general:{p}:{e}"))?;
            Ok(self.general)
        }
        fn observe_host(&self) -> Result<()> {
            self.call("observe")
        }
        fn refresh(&self) -> Result<()> {
            self.call("refresh")
        }
    }
    fn packet(t: Target) -> Vec<u8> {
        if t == Target::Quality {
            return b"H2QO".to_vec();
        }
        let kind = match t {
            Target::Selection => 13,
            Target::Snapshot => 12,
            Target::Common => 11,
            Target::Agreement => 10,
            Target::Admissions => 8,
            _ => 7,
        };
        vec![b'H', b'2', b'N', b'2', 124, 0, kind]
    }
    #[test]
    fn raw_recognition_uses_each_existing_protocol_boundary() {
        for t in [
            Target::Selection,
            Target::Snapshot,
            Target::Common,
            Target::Agreement,
            Target::Quality,
            Target::Admissions,
            Target::Authority,
        ] {
            let mut p = packet(t);
            assert_eq!(classify(&p), Some(t));
            if t != Target::Quality {
                p[4] = 0;
                assert_eq!(
                    classify(&p),
                    if t == Target::Authority {
                        None
                    } else {
                        Some(t)
                    }
                );
            }
            for n in 0..p.len() {
                assert_eq!(classify(&p[..n]), None);
            }
        }
    }
    #[test]
    fn locked_barriers_and_effect_order_survive_epoch_refusal() {
        for t in [Target::Selection, Target::Common] {
            let f = Fixture::default();
            dispatch(&f, 4, &packet(t), 3).unwrap();
            assert!(f.contains(&format!("receive:{t:?}:4:true")));
            let calls = f.calls.borrow();
            let r = calls.iter().position(|s| s.starts_with("receive")).unwrap();
            assert_eq!(&calls[r + 1..r + 3], ["abort:true", "progress:true"]);
            let f = Fixture {
                epoch: 4,
                ..Fixture::default()
            };
            dispatch(&f, 4, &packet(t), 3).unwrap();
            assert!(!f.contains("receive"));
            assert_eq!(f.contains("abort"), t == Target::Common);
            assert_eq!(f.contains("progress"), t == Target::Common);
            assert!(!f.held.get());
        }
    }
    #[test]
    fn pinned_owners_receive_after_unlock_with_exact_epoch_policy() {
        for t in [
            Target::Snapshot,
            Target::Quality,
            Target::Admissions,
            Target::Authority,
        ] {
            let f = Fixture::default();
            dispatch(&f, 256, &packet(t), 3).unwrap();
            assert!(f.contains(&format!("receive:{t:?}:256:false")));
            let f = Fixture {
                epoch: 4,
                ..Fixture::default()
            };
            dispatch(&f, 256, &packet(t), 3).unwrap();
            assert!(!f.contains("receive"));
            let f = Fixture {
                s: Session {
                    epochs: false,
                    ..Fixture::default().s
                },
                epoch: 4,
                ..Fixture::default()
            };
            dispatch(&f, u32::MAX, &packet(t), 3).unwrap();
            assert_eq!(f.contains("receive"), t != Target::Snapshot);
            assert_eq!(f.contains("epoch:"), t == Target::Snapshot);
        }
    }
    #[test]
    fn phase_and_missing_resource_refusal_do_not_fall_through_to_gameplay() {
        for t in [
            Target::Selection,
            Target::Snapshot,
            Target::Common,
            Target::Quality,
            Target::Admissions,
            Target::Authority,
        ] {
            let f = Fixture {
                s: Session::default(),
                supported: false,
                present: false,
                ..Fixture::default()
            };
            dispatch(&f, 4, &packet(t), 3).unwrap();
            assert!(!f.contains("receive"));
            assert!(!f.contains("general"));
            assert_eq!(f.contains("abort"), t == Target::Common);
        }
    }
    #[test]
    fn current_agreement_precedes_history_and_history_uses_first_exact_epoch() {
        let p = packet(Target::Agreement);
        let f = Fixture::default();
        dispatch(&f, 4, &p, 3).unwrap();
        assert!(f.contains("receive:Agreement:4:true"));
        assert!(!f.contains("history"));
        assert!(f.contains("progress:true") && f.contains("fence:true"));
        for (binding, present, handoff) in [
            (None, true, true),
            (Some(2), true, true),
            (Some(3), false, true),
            (Some(3), true, false),
        ] {
            let f = Fixture {
                binding,
                present,
                s: Session {
                    handoff,
                    ..Fixture::default().s
                },
                ..Fixture::default()
            };
            dispatch(&f, 4, &p, 3).unwrap();
            assert!(f.contains("history:1:4:true"));
            assert!(!f.contains("history:2"));
            assert!(!f.contains("receive"));
            assert!(!f.contains("fence"));
        }
    }
    #[test]
    fn generic_fence_receive_observe_refresh_order_and_typed_error_unlock() {
        let f = Fixture::default();
        dispatch(&f, u32::MAX, b"ordinary", u32::MAX).unwrap();
        let calls = f.calls.borrow();
        let filtered = calls
            .iter()
            .filter(|s| {
                ["fence", "general", "observe", "refresh"]
                    .iter()
                    .any(|p| s.starts_with(p))
            })
            .map(String::as_str)
            .collect::<Vec<_>>();
        assert_eq!(
            filtered,
            [
                "fence:true",
                "general:4294967295:4294967295:true",
                "observe:true",
                "refresh:true"
            ]
        );
        drop(calls);
        for fail in ["session", "supported", "pin", "abort", "progress"] {
            let f = Fixture {
                fail: Some(fail),
                ..Fixture::default()
            };
            assert!(dispatch(&f, 4, &packet(Target::Selection), 3).is_err());
            assert!(!f.held.get());
            assert!(f.contains("unlock"));
        }
    }
}
