//! Native readiness and successor publication caller policy.
//!
//! The enclosing caller holds its genuine generation mutex for the entire
//! operation. IO owns real resources, leases, device observations and native
//! commit callbacks. Metadata never creates Ready, a lease or a native world.
use crate::{
    host_agreement::{Context, Decision, State as Agreement},
    migration_preparation::{PreparationCut as Cut, all_survivors_prepared},
    peer_source_admissions::Player,
};

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Error(pub String);
pub type Result<T> = std::result::Result<T, Error>;
#[derive(Clone, Copy, Debug, Default)]
pub struct Session {
    pub self_peer: u8,
    pub handoff: bool,
    pub selection: bool,
    pub snapshots: bool,
    pub commit_allowed: bool,
}
#[derive(Clone, Copy, Debug, Default)]
pub struct Resources {
    pub agreement_present: bool,
    pub agreement_context: Option<Context>,
    pub authority: Option<Context>,
    pub source_present: bool,
    pub history: u32,
}
#[derive(Clone, Copy, Debug, Default)]
pub struct Selection {
    pub present: bool,
    pub started: bool,
    pub supported: bool,
    pub complete: bool,
    pub failed: bool,
    pub survivors: u16,
    pub allows_ready: bool,
}
#[derive(Clone, Copy, Debug, Default)]
pub struct Common {
    pub abort: bool,
    pub pending: Option<(Context, Cut)>,
}
#[derive(Clone, Debug, Default)]
pub struct Admission {
    pub accepted: Option<(Context, Vec<Player>)>,
    pub continuity: Option<Vec<Player>>,
}
impl Admission {
    fn matches(&self, context: Context) -> bool {
        self.accepted.as_ref().is_some_and(|(c, rows)| {
            *c == context && self.continuity.as_ref().is_none_or(|before| before == rows)
        })
    }
}
#[derive(Clone, Copy, Debug, Default)]
pub struct Snapshot {
    pub present: bool,
    pub pending: bool,
    pub retired: u16,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Successor {
    pub multiple: bool,
    pub selection: bool,
    pub snapshot: bool,
    pub selection_feature: bool,
    pub snapshot_feature: bool,
}
impl Successor {
    pub fn transfer_limits(self) -> crate::peer_authority_transfer::Limits {
        // The prior successor caller omitted terminal mode even when building
        // an exact host/membership continuation; preserve that operand here.
        crate::matchmaking_native_access::transfer_limits(
            self.snapshot_feature,
            self.selection_feature,
            false,
        )
    }
}
pub trait Io {
    fn now_ms(&self) -> u64;
    /// Copied from the authoritative Rust Session, with no borrow retained
    /// while calling back into actual native owners.
    fn session(&self, decision: Option<Decision>) -> Result<Session>;
    fn resources(&self) -> Result<Resources>;
    fn selection(&self, cut: Option<Cut>) -> Result<Selection>;
    fn agreement(&self, now: u64) -> Result<Option<Agreement>>;
    fn common(&self) -> Result<Common>;
    fn admission(&self) -> Result<Admission>;
    fn audio(&self) -> Result<Option<bool>>;
    fn snapshot(&self) -> Result<Snapshot>;
    fn blocks_publication(&self) -> bool;
    fn invalidate_selection(&self, cut: Cut) -> Result<()>;
    fn observe_ready(&self, peer: u8, cut: Cut) -> Result<bool>;
    fn fence_selection(&self, failed: bool) -> Result<()>;
    fn publish_ready(&self) -> Result<bool>;
    fn propose(&self, peer: u32, now: u64) -> Result<bool>;
    fn candidate(&self, now: u64) -> Result<Option<u8>>;
    /// All fallible successor allocations and retained row copies occur here,
    /// before native commit. Failure leaves the live generation unchanged.
    fn allocate(&self, plan: Successor) -> Result<()>;
    fn has_native_commit(&self) -> bool;
    fn commit_native(&self) -> Result<bool>;
    /// Swaps the actual preallocated native owners/history and records the
    /// committed decision in the authoritative Rust Session under the same lock.
    fn publish_successor(&self) -> Result<()>;
}
pub fn selection_abort(io: &impl Io) -> Result<()> {
    if !io.selection(None)?.present {
        return Ok(());
    }
    let common = io.common()?;
    if common.abort {
        if let Some((_, cut)) = common.pending {
            io.invalidate_selection(cut)?;
        }
    }
    Ok(())
}
pub fn selection_progress(io: &impl Io) -> Result<()> {
    let mut selected = io.selection(None)?;
    if !selected.present {
        return Ok(());
    }
    if selected.complete && !selected.failed {
        if let Some(agreement) = io.agreement(io.now_ms())? {
            for peer in 0..16 {
                if selected.survivors & (1u16 << peer) != 0 {
                    if let Some(cut) = agreement.ready[peer as usize] {
                        io.observe_ready(peer, cut)?;
                    }
                }
            }
            selected = io.selection(None)?;
        }
    }
    if selected.started {
        io.fence_selection(selected.failed)?;
    }
    Ok(())
}
pub fn fence(io: &impl Io) -> Result<()> {
    let selected = io.selection(None)?;
    io.fence_selection(selected.started && selected.present && selected.failed)
}
pub fn prepare(io: &impl Io, context: Context, cut: Cut) -> Result<bool> {
    if io.blocks_publication() {
        return Ok(false);
    }
    let session = io.session(None)?;
    if !session.handoff {
        return Ok(false);
    }
    let resources = io.resources()?;
    if !resources.agreement_present
        || !resources.source_present
        || !io.admission()?.matches(context)
    {
        return Ok(false);
    }
    if session.selection {
        selection_abort(io)?;
        selection_progress(io)?;
        let selected = io.selection(Some(cut))?;
        if !selected.started || !selected.supported || !selected.allows_ready {
            return Ok(false);
        }
    }
    let result = io.publish_ready()?;
    if result && session.selection {
        return io.observe_ready(session.self_peer, cut);
    }
    Ok(result)
}
pub fn propose(io: &impl Io, peer: u32) -> Result<bool> {
    let session = io.session(None)?;
    if !session.handoff || !io.resources()?.agreement_present {
        return Ok(false);
    }
    if session.selection {
        let selected = io.selection(None)?;
        if !selected.started || !selected.supported || !selected.complete || selected.failed {
            return Ok(false);
        }
    }
    let result = io.propose(peer, io.now_ms())?;
    fence(io)?;
    Ok(result)
}
pub fn propose_automatic(io: &impl Io) -> Result<bool> {
    let now = io.now_ms();
    let Some(peer) = io.candidate(now)? else {
        return Ok(false);
    };
    let result = io.propose(peer as u32, now)?;
    fence(io)?;
    Ok(result)
}
pub fn finish(io: &impl Io, decision: Decision) -> Result<bool> {
    let session = io.session(Some(decision))?;
    if !session.commit_allowed || !session.handoff || session.self_peer >= 16 {
        return Ok(false);
    }
    let resources = io.resources()?;
    if !resources.agreement_present
        || resources.agreement_context.is_none()
        || resources.authority.is_none()
        || !resources.source_present
    {
        return Ok(false);
    }
    if session.selection {
        selection_abort(io)?;
        selection_progress(io)?;
        let selected = io.selection(Some(decision.value.cut))?;
        if !selected.started || !selected.supported || !selected.allows_ready {
            return Ok(false);
        }
    }
    let Some(agreement) = io.agreement(io.now_ms())? else {
        return Ok(false);
    };
    if agreement.decision != Some(decision)
        || resources.agreement_context != Some(decision.previous)
        || resources.history >= 15
        || decision.next.peers & (1u16 << session.self_peer) == 0
        || resources.authority != Some(decision.previous)
    {
        return Ok(false);
    }
    let common = io.common()?;
    if common.abort && common.pending == Some((decision.previous, decision.value.cut)) {
        return Ok(false);
    }
    if all_survivors_prepared(
        decision.previous.peers,
        decision.next.peers,
        decision.previous.host,
        decision.value.host,
        &decision.value.cut,
        &agreement.ready,
    )
    .is_err()
    {
        return Ok(false);
    }
    if !io.admission()?.matches(decision.previous) {
        return Ok(false);
    }
    if io.audio()? == Some(false) {
        return Ok(false);
    }
    let snapshot = io.snapshot()?;
    if snapshot.present && snapshot.pending && snapshot.retired & (1u16 << session.self_peer) == 0 {
        return Ok(false);
    }
    let multiple = decision.next.peers.count_ones() > 1;
    io.allocate(Successor {
        multiple,
        selection: multiple && session.selection && session.self_peer != decision.next.host,
        snapshot: multiple && session.snapshots,
        selection_feature: session.selection,
        snapshot_feature: session.snapshots,
    })?;
    if io.has_native_commit() && !io.commit_native()? {
        return Ok(false);
    }
    io.publish_successor()?;
    Ok(true)
}

#[cfg(test)]
mod tests {
    // Declared metadata/IO fixtures only. No retained native lease or world.
    use super::*;
    use std::cell::{Cell, RefCell};
    fn decision() -> Decision {
        let previous = Context {
            session: [3; 32],
            epoch: 4,
            membership_revision: 8,
            peers: 7,
            host: 0,
        };
        Decision {
            previous,
            next: Context {
                epoch: 5,
                membership_revision: 9,
                peers: 6,
                host: 1,
                ..previous
            },
            ballot: crate::host_agreement::Ballot {
                round: 1,
                proposer: 1,
            },
            value: crate::host_agreement::Value {
                host: 1,
                cut: Cut {
                    id: 2,
                    tick: 100,
                    bytes: 40,
                    digest: [7; 32],
                },
            },
            voters: 6,
        }
    }
    struct Fixture {
        session: Cell<Session>,
        resources: Cell<Resources>,
        selection: Cell<Selection>,
        common: Cell<Common>,
        agreement: RefCell<Option<Agreement>>,
        admission: RefCell<Admission>,
        audio: Cell<Option<bool>>,
        snapshot: Cell<Snapshot>,
        blocked: Cell<bool>,
        native: Cell<bool>,
        accept: Cell<bool>,
        allocate_error: Cell<bool>,
        candidate: Cell<Option<u8>>,
        ready_result: Cell<bool>,
        observe_fails: Cell<bool>,
        events: RefCell<Vec<String>>,
        plans: RefCell<Vec<Successor>>,
    }
    impl Fixture {
        fn new() -> Self {
            let d = decision();
            let players = vec![Player {
                slot: 0,
                player: 0x10000,
                identity: [8; 12],
            }];
            Self {
                session: Cell::new(Session {
                    self_peer: 1,
                    handoff: true,
                    selection: false,
                    snapshots: false,
                    commit_allowed: true,
                }),
                resources: Cell::new(Resources {
                    agreement_present: true,
                    agreement_context: Some(d.previous),
                    authority: Some(d.previous),
                    source_present: true,
                    history: 0,
                }),
                selection: Cell::new(Selection::default()),
                common: Cell::new(Common::default()),
                agreement: RefCell::new(Some(Agreement {
                    suspected: true,
                    fenced: true,
                    electorate: 6,
                    silent: 6,
                    majority: 2,
                    ready: std::array::from_fn(|p| (p == 1 || p == 2).then_some(d.value.cut)),
                    decision: Some(d),
                    rejected: 0,
                })),
                admission: RefCell::new(Admission {
                    accepted: Some((d.previous, players.clone())),
                    continuity: Some(players),
                }),
                audio: Cell::new(None),
                snapshot: Cell::new(Snapshot::default()),
                blocked: Cell::new(false),
                native: Cell::new(true),
                accept: Cell::new(true),
                allocate_error: Cell::new(false),
                candidate: Cell::new(Some(1)),
                ready_result: Cell::new(true),
                observe_fails: Cell::new(false),
                events: RefCell::new(Vec::new()),
                plans: RefCell::new(Vec::new()),
            }
        }
        fn event(&self, v: impl Into<String>) {
            self.events.borrow_mut().push(v.into());
        }
        fn selection_on(&self) {
            self.session.set(Session {
                selection: true,
                snapshots: true,
                ..self.session.get()
            });
            self.selection.set(Selection {
                present: true,
                started: true,
                supported: true,
                complete: true,
                failed: false,
                survivors: 6,
                allows_ready: true,
            });
        }
        fn never_committed(&self) {
            assert!(
                !self
                    .events
                    .borrow()
                    .iter()
                    .any(|e| e == "commit" || e == "publish")
            );
        }
    }
    impl Io for Fixture {
        fn now_ms(&self) -> u64 {
            1000
        }
        fn session(&self, _: Option<Decision>) -> Result<Session> {
            self.event("session");
            Ok(self.session.get())
        }
        fn resources(&self) -> Result<Resources> {
            self.event("resources");
            Ok(self.resources.get())
        }
        fn selection(&self, _: Option<Cut>) -> Result<Selection> {
            self.event("selection");
            Ok(self.selection.get())
        }
        fn agreement(&self, _: u64) -> Result<Option<Agreement>> {
            self.event("agreement");
            Ok(*self.agreement.borrow())
        }
        fn common(&self) -> Result<Common> {
            self.event("common");
            Ok(self.common.get())
        }
        fn admission(&self) -> Result<Admission> {
            self.event("admission");
            Ok(self.admission.borrow().clone())
        }
        fn audio(&self) -> Result<Option<bool>> {
            self.event("audio");
            Ok(self.audio.get())
        }
        fn snapshot(&self) -> Result<Snapshot> {
            self.event("snapshot");
            Ok(self.snapshot.get())
        }
        fn blocks_publication(&self) -> bool {
            self.event("blocks");
            self.blocked.get()
        }
        fn invalidate_selection(&self, c: Cut) -> Result<()> {
            assert_eq!(c, decision().value.cut);
            self.event("invalidate");
            Ok(())
        }
        fn observe_ready(&self, p: u8, c: Cut) -> Result<bool> {
            assert_eq!(c, decision().value.cut);
            self.event(format!("observe:{p}"));
            if self.observe_fails.get() {
                self.selection.set(Selection {
                    failed: true,
                    ..self.selection.get()
                });
            }
            Ok(true)
        }
        fn fence_selection(&self, failed: bool) -> Result<()> {
            self.event(format!("fence:{failed}"));
            Ok(())
        }
        fn publish_ready(&self) -> Result<bool> {
            self.event("ready");
            Ok(self.ready_result.get())
        }
        fn propose(&self, p: u32, t: u64) -> Result<bool> {
            self.event(format!("propose:{p}:{t}"));
            Ok(self.accept.get())
        }
        fn candidate(&self, t: u64) -> Result<Option<u8>> {
            self.event(format!("candidate:{t}"));
            Ok(self.candidate.get())
        }
        fn allocate(&self, p: Successor) -> Result<()> {
            self.event("allocate");
            self.plans.borrow_mut().push(p);
            if self.allocate_error.get() {
                Err(Error("declared allocation refusal".into()))
            } else {
                Ok(())
            }
        }
        fn has_native_commit(&self) -> bool {
            self.event("has_commit");
            self.native.get()
        }
        fn commit_native(&self) -> Result<bool> {
            self.event("commit");
            Ok(self.accept.get())
        }
        fn publish_successor(&self) -> Result<()> {
            self.event("publish");
            Ok(())
        }
    }
    #[test]
    fn all_admission_device_retirement_and_allocations_precede_native_commit_and_publication() {
        let io = Fixture::new();
        assert!(finish(&io, decision()).unwrap());
        assert_eq!(
            *io.events.borrow(),
            [
                "session",
                "resources",
                "agreement",
                "common",
                "admission",
                "audio",
                "snapshot",
                "allocate",
                "has_commit",
                "commit",
                "publish"
            ]
        );
        assert_eq!(
            *io.plans.borrow(),
            [Successor {
                multiple: true,
                selection: false,
                snapshot: false,
                selection_feature: false,
                snapshot_feature: false
            }]
        );
        let io = Fixture::new();
        io.native.set(false);
        assert!(finish(&io, decision()).unwrap());
        assert!(!io.events.borrow().iter().any(|e| e == "commit"));
        assert_eq!(io.events.borrow().last().unwrap(), "publish");
    }
    #[test]
    fn missing_fixed_survivor_forged_decision_stale_context_or_full_history_never_allocate() {
        for bad in 0..6 {
            let io = Fixture::new();
            match bad {
                0 => io.agreement.borrow_mut().as_mut().unwrap().ready[2] = None,
                1 => {
                    io.agreement.borrow_mut().as_mut().unwrap().ready[2]
                        .as_mut()
                        .unwrap()
                        .tick += 1
                }
                2 => {
                    io.agreement
                        .borrow_mut()
                        .as_mut()
                        .unwrap()
                        .decision
                        .as_mut()
                        .unwrap()
                        .ballot
                        .round += 1
                }
                3 => io.resources.set(Resources {
                    history: 15,
                    ..io.resources.get()
                }),
                4 => io.resources.set(Resources {
                    authority: None,
                    ..io.resources.get()
                }),
                _ => io.session.set(Session {
                    commit_allowed: false,
                    ..io.session.get()
                }),
            };
            assert!(!finish(&io, decision()).unwrap());
            io.never_committed();
            assert!(io.plans.borrow().is_empty());
        }
    }
    #[test]
    fn aborted_exact_cut_continuity_mismatch_audio_and_snapshot_borrowers_block_publication() {
        for bad in 0..5 {
            let io = Fixture::new();
            match bad {
                0 => io.common.set(Common {
                    abort: true,
                    pending: Some((decision().previous, decision().value.cut)),
                }),
                1 => io.admission.borrow_mut().continuity.as_mut().unwrap()[0].identity[0] ^= 1,
                2 => io.admission.borrow_mut().accepted = None,
                3 => io.audio.set(Some(false)),
                _ => io.snapshot.set(Snapshot {
                    present: true,
                    pending: true,
                    retired: 4,
                }),
            };
            assert!(!finish(&io, decision()).unwrap());
            io.never_committed();
            assert!(io.plans.borrow().is_empty());
        }
        let io = Fixture::new();
        io.snapshot.set(Snapshot {
            present: true,
            pending: true,
            retired: 2,
        });
        assert!(finish(&io, decision()).unwrap());
    }
    #[test]
    fn allocation_error_or_native_refusal_never_publishes_a_successor() {
        let io = Fixture::new();
        io.allocate_error.set(true);
        assert!(finish(&io, decision()).is_err());
        io.never_committed();
        let io = Fixture::new();
        io.accept.set(false);
        assert!(!finish(&io, decision()).unwrap());
        assert_eq!(io.events.borrow().last().unwrap(), "commit");
        assert!(!io.events.borrow().iter().any(|e| e == "publish"));
        io.accept.set(true);
        assert!(finish(&io, decision()).unwrap());
        io.session.set(Session {
            commit_allowed: false,
            ..io.session.get()
        });
        io.events.borrow_mut().clear();
        assert!(!finish(&io, decision()).unwrap());
        assert_eq!(*io.events.borrow(), ["session"]);
    }
    #[test]
    fn successor_plan_preserves_sole_survivor_and_selected_host_resource_boundaries() {
        let io = Fixture::new();
        io.selection_on();
        assert!(finish(&io, decision()).unwrap());
        assert_eq!(
            io.plans.borrow()[0],
            Successor {
                multiple: true,
                selection: false,
                snapshot: true,
                selection_feature: true,
                snapshot_feature: true
            }
        );
        let io = Fixture::new();
        io.selection_on();
        io.session.set(Session {
            self_peer: 2,
            ..io.session.get()
        });
        assert!(finish(&io, decision()).unwrap());
        assert!(io.plans.borrow()[0].selection);
        let io = Fixture::new();
        let mut d = decision();
        d.previous.peers = 3;
        d.next.peers = 2;
        d.voters = 2;
        io.resources.set(Resources {
            agreement_context: Some(d.previous),
            authority: Some(d.previous),
            ..io.resources.get()
        });
        io.agreement.borrow_mut().as_mut().unwrap().decision = Some(d);
        io.admission.borrow_mut().accepted.as_mut().unwrap().0 = d.previous;
        assert!(finish(&io, d).unwrap());
        assert!(!io.plans.borrow()[0].multiple);
    }
    #[test]
    fn ready_requires_actual_admission_and_selector_then_observes_only_successful_publication() {
        let io = Fixture::new();
        io.blocked.set(true);
        assert!(!prepare(&io, decision().previous, decision().value.cut).unwrap());
        assert_eq!(*io.events.borrow(), ["blocks"]);
        let io = Fixture::new();
        io.admission.borrow_mut().accepted = None;
        assert!(!prepare(&io, decision().previous, decision().value.cut).unwrap());
        assert!(!io.events.borrow().iter().any(|e| e == "ready"));
        let io = Fixture::new();
        io.selection_on();
        io.ready_result.set(false);
        assert!(!prepare(&io, decision().previous, decision().value.cut).unwrap());
        let observed = io
            .events
            .borrow()
            .iter()
            .filter(|e| e.starts_with("observe:"))
            .count();
        io.events.borrow_mut().clear();
        io.ready_result.set(true);
        assert!(prepare(&io, decision().previous, decision().value.cut).unwrap());
        assert_eq!(
            io.events
                .borrow()
                .iter()
                .filter(|e| e.starts_with("observe:"))
                .count(),
            observed + 1
        );
        assert_eq!(io.events.borrow().last().unwrap(), "observe:1");
    }
    #[test]
    fn explicit_and_measured_proposals_preserve_fencing_and_one_clock_observation() {
        let io = Fixture::new();
        io.accept.set(false);
        assert!(!propose(&io, 2).unwrap());
        assert_eq!(io.events.borrow().last().unwrap(), "fence:false");
        let io = Fixture::new();
        assert!(propose_automatic(&io).unwrap());
        assert_eq!(
            *io.events.borrow(),
            [
                "candidate:1000",
                "propose:1:1000",
                "selection",
                "fence:false"
            ]
        );
        let io = Fixture::new();
        io.candidate.set(None);
        assert!(!propose_automatic(&io).unwrap());
        assert_eq!(*io.events.borrow(), ["candidate:1000"]);
        let io = Fixture::new();
        io.selection_on();
        io.selection.set(Selection {
            failed: true,
            ..io.selection.get()
        });
        assert!(!propose(&io, 1).unwrap());
        assert!(!io.events.borrow().iter().any(|e| e.starts_with("propose:")));
    }
    #[test]
    fn selection_progress_imports_only_actual_fixed_survivor_ready_and_rechecks_failure() {
        let io = Fixture::new();
        io.selection_on();
        io.observe_fails.set(true);
        io.common.set(Common {
            abort: true,
            pending: Some((decision().previous, decision().value.cut)),
        });
        selection_abort(&io).unwrap();
        assert_eq!(io.events.borrow().last().unwrap(), "invalidate");
        io.events.borrow_mut().clear();
        selection_progress(&io).unwrap();
        let events = io.events.borrow();
        assert!(events.iter().any(|e| e == "observe:1") && events.iter().any(|e| e == "observe:2"));
        assert!(!events.iter().any(|e| e == "observe:0"));
        assert_eq!(events.last().unwrap(), "fence:true");
    }
}
