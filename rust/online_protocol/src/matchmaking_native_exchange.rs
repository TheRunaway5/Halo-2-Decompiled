//! Complete native publication and capability-view callers.
//!
//! Native IO pins the exact real owner under the Session generation mutex.
//! Source/authority publication then runs outside that mutex, as before; Common
//! actions retain it through Abort/progress. Metadata grants no native readiness.
use crate::{
    migration_preparation::PreparationCut,
    peer_source_admissions::{Context, Player},
};

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Error(pub String);
pub type Result<T> = std::result::Result<T, Error>;
#[derive(Clone, Copy, Debug, Default)]
pub struct Session {
    pub self_peer: u32,
    pub host: u32,
    pub direct: bool,
    pub running: bool,
    pub common_phase: bool,
    pub candidate_phase: bool,
    pub terminal_recovery: bool,
}
#[derive(Clone, Debug, Default)]
pub struct Admission {
    pub accepted: Option<Vec<Player>>,
    pub continuity: Option<Vec<Player>>,
}
#[derive(Clone, Copy, Debug, Default)]
pub struct Common {
    pub supported: bool,
    pub context: Option<Context>,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CommonAction {
    Offer,
    ConfirmNative,
    Associate,
    Apply,
    Release,
    Reject,
    Abort,
    AcknowledgeAbort,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Capability {
    Authority,
    Replay,
    Pipeline,
    HostContinuation,
    AdmittedCamera,
    SourceConfirmed,
}
pub trait Io {
    fn session(&self) -> Session;
    fn now_ms(&self) -> u64;
    fn direct_ready(&self) -> u16;
    fn source_present(&self) -> bool;
    fn source_continuity(&self) -> Result<Option<Vec<Player>>>;
    fn source_publish(&self) -> Result<bool>;
    fn admission(&self) -> Result<Admission>;
    fn source_confirmed(&self, peer: u32) -> Result<bool>;
    fn authority_present(&self) -> bool;
    fn authority_publish(&self, ahead: bool, now: u64) -> Result<PreparationCut>;
    fn authority_supported(&self, peer: u32, features: u32) -> Result<bool>;
    fn common(&self) -> Result<Common>;
    fn common_perform(&self, action: CommonAction) -> Result<bool>;
    fn common_view(&self) -> Result<()>;
    fn selection_abort(&self) -> Result<()>;
    fn selection_progress(&self) -> Result<()>;
    fn quality_context(&self) -> Result<Option<Context>>;
    fn quality_candidates(&self, now: u64) -> Result<()>;
}
fn need(ok: bool) -> Result<()> {
    if ok {
        Ok(())
    } else {
        Err(Error(
            "ONLINE match connection failed or ended. Search again.".into(),
        ))
    }
}
fn publish_allowed(s: Session) -> bool {
    s.direct && s.running && s.host == s.self_peer
}
pub fn publish_source(io: &impl Io, players: &[Player]) -> Result<bool> {
    need(publish_allowed(io.session()))?;
    if io
        .source_continuity()?
        .is_some_and(|prior| prior != players)
    {
        return Ok(false);
    }
    if !io.source_present() {
        return Ok(false);
    }
    io.source_publish()
}
/// IO retains the genuine accepted value; the return selects that exact value.
pub fn source_visible(io: &impl Io) -> Result<bool> {
    let admission = io.admission()?;
    Ok(admission
        .accepted
        .is_some_and(|rows| admission.continuity.is_none_or(|prior| prior == rows)))
}
pub fn publish_authority(io: &impl Io, ahead: bool) -> Result<PreparationCut> {
    need(publish_allowed(io.session()))?;
    need(io.authority_present())?;
    io.authority_publish(ahead, io.now_ms())
}
pub fn capability(io: &impl Io, kind: Capability, peer: u32) -> Result<bool> {
    if kind == Capability::AdmittedCamera && !io.session().terminal_recovery {
        return Ok(false);
    }
    if peer >= 16 || io.direct_ready() & (1u16 << peer) == 0 {
        return Ok(false);
    }
    if kind == Capability::SourceConfirmed {
        return if io.source_present() {
            io.source_confirmed(peer)
        } else {
            Ok(false)
        };
    }
    if !io.authority_present() {
        return Ok(false);
    }
    // Same native authority feature bits; include all presented model operands.
    let features = match kind {
        Capability::Authority => 0,
        Capability::Replay => 1 | 2 | 4 | 8 | 16 | 32 | 64 | 8192,
        Capability::Pipeline => 128,
        Capability::HostContinuation => 256,
        Capability::AdmittedCamera => 4096,
        Capability::SourceConfirmed => unreachable!(),
    };
    io.authority_supported(peer, features)
}
pub fn common_action(io: &impl Io, context: Context, action: CommonAction) -> Result<bool> {
    let s = io.session();
    if !s.running && !(action == CommonAction::AcknowledgeAbort && s.common_phase) {
        return Ok(false);
    }
    let common = io.common()?;
    if !common.supported || common.context != Some(context) {
        return Ok(false);
    }
    let result = io.common_perform(action)?;
    // The prior caller drives Abort and progress even when a leaf refuses.
    io.selection_abort()?;
    io.selection_progress()?;
    Ok(result)
}
pub fn common_view(io: &impl Io) -> Result<()> {
    if io.common()?.supported {
        io.common_view()?;
    }
    Ok(())
}
pub fn host_candidates(io: &impl Io) -> Result<()> {
    let Some(context) = io.quality_context()? else {
        return Ok(());
    };
    let s = io.session();
    if s.candidate_phase && context.host == s.host {
        io.quality_candidates(io.now_ms())?;
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    // All owner/readiness/identity inputs below are declared metadata fixtures.
    use super::*;
    use std::cell::{Cell, RefCell};
    fn context() -> Context {
        Context {
            session: [2; 32],
            epoch: 4,
            membership_revision: 7,
            peers: 7,
            host: 0,
        }
    }
    fn player() -> Player {
        Player {
            slot: 0,
            player: 0x10000,
            identity: [3; 12],
        }
    }
    fn cut() -> PreparationCut {
        PreparationCut {
            id: 4,
            tick: 100,
            bytes: 128,
            digest: [5; 32],
        }
    }
    struct Fixture {
        s: Cell<Session>,
        admission: RefCell<Admission>,
        common: Cell<Common>,
        source: Cell<bool>,
        authority: Cell<bool>,
        ready: Cell<u16>,
        quality: Cell<Option<Context>>,
        result: Cell<bool>,
        fail: Cell<bool>,
        events: RefCell<Vec<String>>,
    }
    impl Fixture {
        fn new() -> Self {
            Self {
                s: Cell::new(Session {
                    self_peer: 0,
                    host: 0,
                    direct: true,
                    running: true,
                    common_phase: true,
                    candidate_phase: true,
                    terminal_recovery: true,
                }),
                admission: RefCell::new(Admission {
                    accepted: Some(vec![player()]),
                    continuity: Some(vec![player()]),
                }),
                common: Cell::new(Common {
                    supported: true,
                    context: Some(context()),
                }),
                source: Cell::new(true),
                authority: Cell::new(true),
                ready: Cell::new(7),
                quality: Cell::new(Some(context())),
                result: Cell::new(true),
                fail: Cell::new(false),
                events: RefCell::new(vec![]),
            }
        }
        fn event(&self, v: impl Into<String>) {
            self.events.borrow_mut().push(v.into());
        }
    }
    impl Io for Fixture {
        fn session(&self) -> Session {
            self.event("session");
            self.s.get()
        }
        fn now_ms(&self) -> u64 {
            self.event("clock");
            1000
        }
        fn direct_ready(&self) -> u16 {
            self.event("direct");
            self.ready.get()
        }
        fn source_present(&self) -> bool {
            self.event("source");
            self.source.get()
        }
        fn source_continuity(&self) -> Result<Option<Vec<Player>>> {
            self.event("continuity");
            Ok(self.admission.borrow().continuity.clone())
        }
        fn source_publish(&self) -> Result<bool> {
            self.event("publish-source");
            if self.fail.get() {
                Err(Error("source IO failed".into()))
            } else {
                Ok(self.result.get())
            }
        }
        fn admission(&self) -> Result<Admission> {
            self.event("admission");
            Ok(self.admission.borrow().clone())
        }
        fn source_confirmed(&self, p: u32) -> Result<bool> {
            self.event(format!("confirmed:{p}"));
            Ok(self.result.get())
        }
        fn authority_present(&self) -> bool {
            self.event("authority");
            self.authority.get()
        }
        fn authority_publish(&self, a: bool, n: u64) -> Result<PreparationCut> {
            self.event(format!("publish-authority:{a}:{n}"));
            if self.fail.get() {
                Err(Error("authority IO failed".into()))
            } else {
                Ok(cut())
            }
        }
        fn authority_supported(&self, p: u32, f: u32) -> Result<bool> {
            self.event(format!("supported:{p}:{f}"));
            Ok(self.result.get())
        }
        fn common(&self) -> Result<Common> {
            self.event("common");
            Ok(self.common.get())
        }
        fn common_perform(&self, a: CommonAction) -> Result<bool> {
            self.event(format!("action:{a:?}"));
            if self.fail.get() {
                Err(Error("common IO failed".into()))
            } else {
                Ok(self.result.get())
            }
        }
        fn common_view(&self) -> Result<()> {
            self.event("view");
            Ok(())
        }
        fn selection_abort(&self) -> Result<()> {
            self.event("abort");
            Ok(())
        }
        fn selection_progress(&self) -> Result<()> {
            self.event("progress");
            Ok(())
        }
        fn quality_context(&self) -> Result<Option<Context>> {
            self.event("quality");
            Ok(self.quality.get())
        }
        fn quality_candidates(&self, n: u64) -> Result<()> {
            self.event(format!("candidates:{n}"));
            Ok(())
        }
    }
    #[test]
    fn source_checks_exact_full_rows_before_real_publication() {
        let io = Fixture::new();
        assert!(publish_source(&io, &[player()]).unwrap());
        assert_eq!(
            *io.events.borrow(),
            ["session", "continuity", "source", "publish-source"]
        );
        for field in 0..3 {
            let mut wrong = player();
            match field {
                0 => wrong.slot += 256,
                1 => wrong.player ^= 0x10000,
                _ => wrong.identity[11] ^= 1,
            };
            io.events.borrow_mut().clear();
            assert!(!publish_source(&io, &[wrong]).unwrap());
            assert_eq!(*io.events.borrow(), ["session", "continuity"]);
        }
        io.admission.borrow_mut().continuity = None;
        assert!(publish_source(&io, &[]).unwrap());
        io.source.set(false);
        assert!(!publish_source(&io, &[]).unwrap());
    }
    #[test]
    fn publication_gates_and_errors_do_not_reach_or_replace_native_owner_work() {
        for change in 0..3 {
            let io = Fixture::new();
            let mut s = io.s.get();
            match change {
                0 => s.direct = false,
                1 => s.running = false,
                _ => s.host = 1,
            };
            io.s.set(s);
            assert!(publish_source(&io, &[player()]).is_err());
            assert!(publish_authority(&io, false).is_err());
            assert!(io.events.borrow().iter().all(|e| e == "session"));
        }
        let io = Fixture::new();
        io.authority.set(false);
        assert!(publish_authority(&io, false).is_err());
        assert_eq!(*io.events.borrow(), ["session", "authority"]);
        let io = Fixture::new();
        io.fail.set(true);
        assert_eq!(
            publish_source(&io, &[player()]),
            Err(Error("source IO failed".into()))
        );
        assert_eq!(
            publish_authority(&io, true),
            Err(Error("authority IO failed".into()))
        );
    }
    #[test]
    fn authority_full_and_ahead_use_one_clock_after_capture_and_admission() {
        for ahead in [false, true] {
            let io = Fixture::new();
            assert_eq!(publish_authority(&io, ahead).unwrap(), cut());
            assert_eq!(
                *io.events.borrow(),
                [
                    "session".to_string(),
                    "authority".into(),
                    "clock".into(),
                    format!("publish-authority:{ahead}:1000")
                ]
            );
        }
    }
    #[test]
    fn accepted_projection_preserves_absence_and_immutable_continuity() {
        let io = Fixture::new();
        assert!(source_visible(&io).unwrap());
        io.admission.borrow_mut().accepted.as_mut().unwrap()[0].identity[11] ^= 1;
        assert!(!source_visible(&io).unwrap());
        io.admission.borrow_mut().continuity = None;
        assert!(source_visible(&io).unwrap());
        io.admission.borrow_mut().accepted = None;
        assert!(!source_visible(&io).unwrap());
    }
    #[test]
    fn capability_checks_full_width_peers_readiness_and_exact_feature_masks() {
        for p in [16, 256, 257, u32::MAX] {
            let io = Fixture::new();
            assert!(!capability(&io, Capability::Authority, p).unwrap());
            assert!(io.events.borrow().is_empty());
        }
        let io = Fixture::new();
        io.ready.set(0);
        assert!(!capability(&io, Capability::Replay, 1).unwrap());
        assert_eq!(*io.events.borrow(), ["direct"]);
        for (kind, bits) in [
            (Capability::Authority, 0),
            (Capability::Replay, 8319),
            (Capability::Pipeline, 128),
            (Capability::HostContinuation, 256),
            (Capability::AdmittedCamera, 4096),
        ] {
            let io = Fixture::new();
            assert!(capability(&io, kind, 1).unwrap());
            assert_eq!(
                io.events.borrow().last().unwrap(),
                &format!("supported:1:{bits}")
            );
        }
        let io = Fixture::new();
        io.s.set(Session {
            terminal_recovery: false,
            ..io.s.get()
        });
        assert!(!capability(&io, Capability::AdmittedCamera, 1).unwrap());
        assert_eq!(*io.events.borrow(), ["session"]);
        let io = Fixture::new();
        assert!(capability(&io, Capability::SourceConfirmed, 1).unwrap());
        assert_eq!(*io.events.borrow(), ["direct", "source", "confirmed:1"]);
    }
    #[test]
    fn common_actions_keep_abort_progress_even_on_refusal_but_not_on_io_exception() {
        for action in [
            CommonAction::Offer,
            CommonAction::ConfirmNative,
            CommonAction::Associate,
            CommonAction::Apply,
            CommonAction::Release,
            CommonAction::Reject,
            CommonAction::Abort,
            CommonAction::AcknowledgeAbort,
        ] {
            let io = Fixture::new();
            io.result.set(false);
            assert!(!common_action(&io, context(), action).unwrap());
            assert_eq!(
                *io.events.borrow(),
                [
                    "session".to_string(),
                    "common".into(),
                    format!("action:{action:?}"),
                    "abort".into(),
                    "progress".into()
                ]
            );
        }
        let io = Fixture::new();
        io.fail.set(true);
        assert!(common_action(&io, context(), CommonAction::Offer).is_err());
        assert!(
            !io.events
                .borrow()
                .iter()
                .any(|e| e == "abort" || e == "progress")
        );
    }
    #[test]
    fn common_retirement_phase_and_full_context_never_alias_or_synthesize_readiness() {
        let io = Fixture::new();
        io.s.set(Session {
            running: false,
            ..io.s.get()
        });
        assert!(!common_action(&io, context(), CommonAction::Release).unwrap());
        assert_eq!(*io.events.borrow(), ["session"]);
        assert!(common_action(&io, context(), CommonAction::AcknowledgeAbort).unwrap());
        io.s.set(Session {
            running: false,
            common_phase: false,
            ..io.s.get()
        });
        assert!(!common_action(&io, context(), CommonAction::AcknowledgeAbort).unwrap());
        let io = Fixture::new();
        let mut wrong = context();
        wrong.host += 256;
        assert!(!common_action(&io, wrong, CommonAction::Offer).unwrap());
        assert_eq!(*io.events.borrow(), ["session", "common"]);
        io.common.set(Common {
            supported: false,
            ..io.common.get()
        });
        assert!(!common_action(&io, context(), CommonAction::Offer).unwrap());
    }
    #[test]
    fn common_view_and_host_candidates_preserve_context_phase_and_clock_order() {
        let io = Fixture::new();
        common_view(&io).unwrap();
        assert_eq!(*io.events.borrow(), ["common", "view"]);
        io.events.borrow_mut().clear();
        host_candidates(&io).unwrap();
        assert_eq!(
            *io.events.borrow(),
            ["quality", "session", "clock", "candidates:1000"]
        );
        for change in 0..3 {
            let io = Fixture::new();
            match change {
                0 => io.quality.set(None),
                1 => io.s.set(Session {
                    candidate_phase: false,
                    ..io.s.get()
                }),
                _ => io.s.set(Session {
                    host: 1,
                    ..io.s.get()
                }),
            };
            host_candidates(&io).unwrap();
            assert!(!io.events.borrow().iter().any(|e| e == "clock"));
        }
    }
}
