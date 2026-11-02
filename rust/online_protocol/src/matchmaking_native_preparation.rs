//! Complete native preparation request/completion and inventory callers.
//!
//! IO observes genuine native owner identity and retained inventory under the
//! actual Session generation lock. Metadata cannot supply native quiescence,
//! leases or a prepared world. Observations are cached only for one leaf call
//! sequence so retirement reservation uses the same observation as its request.
use crate::{host_agreement::Context, migration_driver as driver};

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Error(pub String);
pub type Result<T> = std::result::Result<T, Error>;
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kind {
    Prepare,
    Retire,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Outcome {
    Inactive,
    Preparing,
    Retiring,
    Published,
    Retryable,
    Terminal,
    Refused,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Step {
    pub outcome: Outcome,
    pub request: Option<(Kind, driver::Ticket)>,
}
impl Step {
    fn outcome(outcome: Outcome) -> Self {
        Self {
            outcome,
            request: None,
        }
    }
}
#[derive(Clone, Copy, Debug, Default)]
pub struct Selection {
    pub context: Option<Context>,
    pub started: bool,
    pub failed: bool,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Authorization {
    pub supported: bool,
    pub admitted: Option<Context>,
}
pub trait Io {
    fn driver_present(&self) -> bool;
    fn owner_matches(&self) -> bool;
    /// Captures one actual authority/admission/HostAgreement/selector observation
    /// with one native clock sample. Leaf calls consume this exact snapshot.
    fn observe(&self) -> Result<()>;
    fn step(&self) -> Result<driver::Action>;
    fn reserve_retirement(&self, ticket: driver::Ticket) -> Result<bool>;
    fn retained(&self, ticket: driver::Ticket, lease: u64) -> Result<bool>;
    fn retired(&self, ticket: driver::Ticket) -> Result<driver::Retirement>;
    fn admit_freeze(&self, ticket: driver::Ticket) -> Result<driver::FreezeKind>;
    fn admit_external(&self) -> Result<driver::FreezeKind>;
    fn selection(&self) -> Result<Selection>;
    fn publish_inventory(&self) -> Result<bool>;
    fn invalidate(&self, cut: Option<crate::migration_preparation::PreparationCut>)
    -> Result<bool>;
    fn abort(&self) -> Result<()>;
    fn progress(&self) -> Result<()>;
    fn begin_selection(&self) -> Result<()>;
    fn terminal(&self) -> Result<()>;
    fn retirement_pending(&self) -> bool;
}
pub fn request(io: &impl Io) -> Result<Step> {
    if !io.driver_present() {
        return Ok(Step::outcome(Outcome::Inactive));
    }
    io.abort()?;
    io.observe()?;
    let action = io.step()?;
    match action.kind {
        driver::ActionKind::Terminal => {
            io.terminal()?;
            Ok(Step::outcome(Outcome::Terminal))
        }
        driver::ActionKind::Published => Ok(Step::outcome(Outcome::Published)),
        driver::ActionKind::Wait => Ok(Step::outcome(Outcome::Inactive)),
        driver::ActionKind::Prepare | driver::ActionKind::Retire => {
            let Some(ticket) = action.ticket else {
                return Ok(Step::outcome(Outcome::Refused));
            };
            if action.kind == driver::ActionKind::Retire && !io.reserve_retirement(ticket)? {
                return Ok(Step::outcome(Outcome::Refused));
            }
            let (kind, outcome) = if action.kind == driver::ActionKind::Prepare {
                (Kind::Prepare, Outcome::Preparing)
            } else {
                (Kind::Retire, Outcome::Retiring)
            };
            Ok(Step {
                outcome,
                request: Some((kind, ticket)),
            })
        }
    }
}
fn after(io: &impl Io) -> Result<Outcome> {
    io.observe()?;
    match io.step()?.kind {
        driver::ActionKind::Terminal => {
            io.terminal()?;
            Ok(Outcome::Terminal)
        }
        driver::ActionKind::Retire => Ok(Outcome::Retiring),
        driver::ActionKind::Published => Ok(Outcome::Published),
        _ => Ok(Outcome::Refused),
    }
}
pub fn complete(
    io: &impl Io,
    kind: Kind,
    ticket: driver::Ticket,
    inventory_present: bool,
    lease: u64,
) -> Result<Outcome> {
    if !io.driver_present() || !io.owner_matches() {
        return Ok(Outcome::Refused);
    }
    io.abort()?;
    if kind == Kind::Retire {
        if inventory_present {
            return Ok(Outcome::Refused);
        }
        io.observe()?;
        return match io.retired(ticket)? {
            driver::Retirement::Retryable => Ok(Outcome::Retryable),
            driver::Retirement::RetiredButFenced => {
                io.terminal()?;
                Ok(Outcome::Terminal)
            }
            driver::Retirement::Stale => Ok(Outcome::Refused),
        };
    }
    if !io.retained(ticket, if inventory_present { lease } else { 0 })? {
        return Ok(Outcome::Refused);
    }
    io.observe()?;
    if inventory_present {
        match io.admit_freeze(ticket)? {
            driver::FreezeKind::Publish => {
                let published = io.publish_inventory()?;
                if published || io.selection()?.failed {
                    io.begin_selection()?;
                }
                io.abort()?;
                io.progress()?;
                io.observe()?;
                let action = io.step()?;
                if action.kind == driver::ActionKind::Terminal {
                    io.terminal()?;
                    return Ok(Outcome::Terminal);
                }
                return Ok(if action.kind == driver::ActionKind::Published {
                    Outcome::Published
                } else {
                    Outcome::Refused
                });
            }
            driver::FreezeKind::Invalidate => {
                io.invalidate(None)?;
                io.begin_selection()?;
                io.progress()?;
            }
            driver::FreezeKind::Retire | driver::FreezeKind::Refuse => {}
        }
    }
    after(io)
}
pub fn freeze_external(io: &impl Io, context: Context) -> Result<bool> {
    if !io.driver_present() || io.selection()?.context != Some(context) {
        return Ok(false);
    }
    io.observe()?;
    match io.admit_external()? {
        driver::FreezeKind::Invalidate => {
            io.invalidate(None)?;
            io.begin_selection()?;
            io.progress()?;
            io.observe()?;
            io.step()?;
            return Ok(false);
        }
        driver::FreezeKind::Refuse | driver::FreezeKind::Retire => return Ok(false),
        driver::FreezeKind::Publish => {}
    }
    let result = io.publish_inventory()?;
    if result || io.selection()?.failed {
        io.begin_selection()?;
    }
    io.abort()?;
    io.progress()?;
    io.observe()?;
    if io.step()?.kind == driver::ActionKind::Terminal {
        io.terminal()?;
    }
    Ok(result && !io.selection()?.failed)
}
pub fn invalidate(
    io: &impl Io,
    context: Context,
    cut: Option<crate::migration_preparation::PreparationCut>,
) -> Result<bool> {
    let selected = io.selection()?;
    if !selected.started || selected.context != Some(context) {
        return Ok(false);
    }
    let result = io.invalidate(cut)?;
    io.progress()?;
    Ok(result)
}
pub fn retiring(io: &impl Io) -> bool {
    io.driver_present() && io.retirement_pending()
}

#[cfg(test)]
mod tests {
    // Declared IO facts, never genuine native resources/leases/quiescence.
    use super::*;
    use std::{
        cell::{Cell, RefCell},
        collections::VecDeque,
    };
    fn ticket() -> driver::Ticket {
        driver::Ticket {
            context: Context {
                session: [2; 32],
                epoch: 5,
                membership_revision: 7,
                peers: 7,
                host: 0,
            },
            self_peer: 1,
            id: 41,
        }
    }
    fn action(kind: driver::ActionKind) -> driver::Action {
        driver::Action {
            kind,
            ticket: matches!(
                kind,
                driver::ActionKind::Prepare | driver::ActionKind::Retire
            )
            .then_some(ticket()),
            reason: driver::Reason::None,
        }
    }
    struct Fixture {
        present: Cell<bool>,
        owner: Cell<bool>,
        reserved: Cell<bool>,
        retained: Cell<bool>,
        retirement: Cell<driver::Retirement>,
        admission: Cell<driver::FreezeKind>,
        selected: Cell<Selection>,
        published: Cell<bool>,
        pending: Cell<bool>,
        fail_publish: Cell<bool>,
        steps: RefCell<VecDeque<driver::Action>>,
        events: RefCell<Vec<String>>,
        observation: Cell<u32>,
    }
    impl Fixture {
        fn new(kinds: &[driver::ActionKind]) -> Self {
            Self {
                present: Cell::new(true),
                owner: Cell::new(true),
                reserved: Cell::new(true),
                retained: Cell::new(true),
                retirement: Cell::new(driver::Retirement::Retryable),
                admission: Cell::new(driver::FreezeKind::Publish),
                selected: Cell::new(Selection {
                    context: Some(ticket().context),
                    started: false,
                    failed: false,
                }),
                published: Cell::new(true),
                pending: Cell::new(false),
                fail_publish: Cell::new(false),
                steps: RefCell::new(kinds.iter().copied().map(action).collect()),
                events: RefCell::new(Vec::new()),
                observation: Cell::new(0),
            }
        }
        fn event(&self, s: impl Into<String>) {
            self.events.borrow_mut().push(s.into());
        }
    }
    impl Io for Fixture {
        fn driver_present(&self) -> bool {
            self.event("driver");
            self.present.get()
        }
        fn owner_matches(&self) -> bool {
            self.event("owner");
            self.owner.get()
        }
        fn observe(&self) -> Result<()> {
            self.observation.set(self.observation.get() + 1);
            self.event(format!("observe:{}", self.observation.get()));
            Ok(())
        }
        fn step(&self) -> Result<driver::Action> {
            self.event(format!("step:{}", self.observation.get()));
            Ok(self
                .steps
                .borrow_mut()
                .pop_front()
                .unwrap_or(action(driver::ActionKind::Wait)))
        }
        fn reserve_retirement(&self, t: driver::Ticket) -> Result<bool> {
            assert_eq!(t, ticket());
            self.event(format!("reserve:{}", self.observation.get()));
            Ok(self.reserved.get())
        }
        fn retained(&self, t: driver::Ticket, l: u64) -> Result<bool> {
            assert_eq!(t, ticket());
            self.event(format!("retained:{l}"));
            Ok(self.retained.get())
        }
        fn retired(&self, t: driver::Ticket) -> Result<driver::Retirement> {
            assert_eq!(t, ticket());
            self.event(format!("retired:{}", self.observation.get()));
            Ok(self.retirement.get())
        }
        fn admit_freeze(&self, t: driver::Ticket) -> Result<driver::FreezeKind> {
            assert_eq!(t, ticket());
            self.event(format!("admit:{}", self.observation.get()));
            Ok(self.admission.get())
        }
        fn admit_external(&self) -> Result<driver::FreezeKind> {
            self.event(format!("external:{}", self.observation.get()));
            Ok(self.admission.get())
        }
        fn selection(&self) -> Result<Selection> {
            self.event("selection");
            Ok(self.selected.get())
        }
        fn publish_inventory(&self) -> Result<bool> {
            self.event("publish");
            if self.fail_publish.get() {
                Err(Error("declared metadata failure".into()))
            } else {
                Ok(self.published.get())
            }
        }
        fn invalidate(
            &self,
            _: Option<crate::migration_preparation::PreparationCut>,
        ) -> Result<bool> {
            self.event("invalidate");
            Ok(true)
        }
        fn abort(&self) -> Result<()> {
            self.event("abort");
            Ok(())
        }
        fn progress(&self) -> Result<()> {
            self.event("progress");
            Ok(())
        }
        fn begin_selection(&self) -> Result<()> {
            self.event("begin");
            Ok(())
        }
        fn terminal(&self) -> Result<()> {
            self.event("terminal");
            Ok(())
        }
        fn retirement_pending(&self) -> bool {
            self.event("pending");
            self.pending.get()
        }
    }
    #[test]
    fn requests_preserve_exact_ticket_and_one_observation_for_retirement_reservation() {
        let io = Fixture::new(&[driver::ActionKind::Prepare]);
        let s = request(&io).unwrap();
        assert_eq!(
            s,
            Step {
                outcome: Outcome::Preparing,
                request: Some((Kind::Prepare, ticket()))
            }
        );
        assert_eq!(
            *io.events.borrow(),
            ["driver", "abort", "observe:1", "step:1"]
        );
        let io = Fixture::new(&[driver::ActionKind::Retire]);
        assert_eq!(
            request(&io).unwrap().request,
            Some((Kind::Retire, ticket()))
        );
        assert_eq!(io.events.borrow().last().unwrap(), "reserve:1");
        assert_eq!(io.observation.get(), 1);
        let io = Fixture::new(&[driver::ActionKind::Retire]);
        io.reserved.set(false);
        assert_eq!(request(&io).unwrap(), Step::outcome(Outcome::Refused));
    }
    #[test]
    fn inactive_published_terminal_and_missing_ticket_requests_do_not_create_native_requests() {
        for (kind, expected) in [
            (driver::ActionKind::Wait, Outcome::Inactive),
            (driver::ActionKind::Published, Outcome::Published),
            (driver::ActionKind::Terminal, Outcome::Terminal),
        ] {
            let io = Fixture::new(&[kind]);
            assert_eq!(request(&io).unwrap(), Step::outcome(expected));
            assert_eq!(
                io.events.borrow().iter().any(|v| v == "terminal"),
                expected == Outcome::Terminal
            );
        }
        let io = Fixture::new(&[]);
        io.present.set(false);
        assert_eq!(request(&io).unwrap().outcome, Outcome::Inactive);
        assert_eq!(*io.events.borrow(), ["driver"]);
        let io = Fixture::new(&[]);
        io.steps.borrow_mut().push_back(driver::Action {
            ticket: None,
            ..action(driver::ActionKind::Prepare)
        });
        assert_eq!(request(&io).unwrap().outcome, Outcome::Refused);
    }
    #[test]
    fn foreign_owner_and_retirement_with_inventory_cannot_retain_or_publish() {
        let io = Fixture::new(&[]);
        io.owner.set(false);
        assert_eq!(
            complete(&io, Kind::Prepare, ticket(), true, 90).unwrap(),
            Outcome::Refused
        );
        assert_eq!(*io.events.borrow(), ["driver", "owner"]);
        let io = Fixture::new(&[]);
        assert_eq!(
            complete(&io, Kind::Retire, ticket(), true, 90).unwrap(),
            Outcome::Refused
        );
        assert_eq!(*io.events.borrow(), ["driver", "owner", "abort"]);
        for (retired, result) in [
            (driver::Retirement::Retryable, Outcome::Retryable),
            (driver::Retirement::RetiredButFenced, Outcome::Terminal),
            (driver::Retirement::Stale, Outcome::Refused),
        ] {
            let io = Fixture::new(&[]);
            io.retirement.set(retired);
            assert_eq!(
                complete(&io, Kind::Retire, ticket(), false, 0).unwrap(),
                result
            );
            assert_eq!(io.observation.get(), 1);
            assert_eq!(
                io.events.borrow().iter().any(|v| v == "terminal"),
                result == Outcome::Terminal
            );
        }
    }
    #[test]
    fn retained_identity_precedes_admission_and_zero_lease_failure_does_not_publish() {
        let io = Fixture::new(&[]);
        io.retained.set(false);
        assert_eq!(
            complete(&io, Kind::Prepare, ticket(), false, 999).unwrap(),
            Outcome::Refused
        );
        assert_eq!(
            *io.events.borrow(),
            ["driver", "owner", "abort", "retained:0"]
        );
        let io = Fixture::new(&[driver::ActionKind::Published]);
        assert_eq!(
            complete(&io, Kind::Prepare, ticket(), true, 99).unwrap(),
            Outcome::Published
        );
        assert_eq!(
            *io.events.borrow(),
            [
                "driver",
                "owner",
                "abort",
                "retained:99",
                "observe:1",
                "admit:1",
                "publish",
                "begin",
                "abort",
                "progress",
                "observe:2",
                "step:2"
            ]
        );
        let io = Fixture::new(&[driver::ActionKind::Retire]);
        assert_eq!(
            complete(&io, Kind::Prepare, ticket(), true, 99).unwrap(),
            Outcome::Refused
        );
    }
    #[test]
    fn invalidation_and_admission_refusal_reobserve_before_mapping_terminal_retirement_or_publication()
     {
        for freeze in [
            driver::FreezeKind::Invalidate,
            driver::FreezeKind::Retire,
            driver::FreezeKind::Refuse,
        ] {
            for (kind, result) in [
                (driver::ActionKind::Terminal, Outcome::Terminal),
                (driver::ActionKind::Retire, Outcome::Retiring),
                (driver::ActionKind::Published, Outcome::Published),
                (driver::ActionKind::Wait, Outcome::Refused),
            ] {
                let io = Fixture::new(&[kind]);
                io.admission.set(freeze);
                assert_eq!(
                    complete(&io, Kind::Prepare, ticket(), true, 9).unwrap(),
                    result
                );
                assert_eq!(io.observation.get(), 2);
                assert_eq!(
                    io.events.borrow().iter().any(|e| e == "invalidate"),
                    freeze == driver::FreezeKind::Invalidate
                );
                assert!(!io.events.borrow().iter().any(|e| e == "publish"));
            }
        }
    }
    #[test]
    fn failed_selector_begins_fencing_even_when_inventory_publish_is_refused() {
        let io = Fixture::new(&[driver::ActionKind::Terminal]);
        io.published.set(false);
        io.selected.set(Selection {
            failed: true,
            ..io.selected.get()
        });
        assert_eq!(
            complete(&io, Kind::Prepare, ticket(), true, 9).unwrap(),
            Outcome::Terminal
        );
        assert!(io.events.borrow().iter().any(|e| e == "begin"));
        assert_eq!(io.events.borrow().last().unwrap(), "terminal");
        let io = Fixture::new(&[]);
        io.fail_publish.set(true);
        assert!(complete(&io, Kind::Prepare, ticket(), true, 9).is_err());
        assert_eq!(io.events.borrow().last().unwrap(), "publish");
    }
    #[test]
    fn external_inventory_paths_preserve_invalidation_and_terminal_publication_outcomes() {
        let io = Fixture::new(&[driver::ActionKind::Terminal]);
        io.admission.set(driver::FreezeKind::Invalidate);
        assert!(!freeze_external(&io, ticket().context).unwrap());
        assert_eq!(
            *io.events.borrow(),
            [
                "driver",
                "selection",
                "observe:1",
                "external:1",
                "invalidate",
                "begin",
                "progress",
                "observe:2",
                "step:2"
            ]
        );
        let io = Fixture::new(&[driver::ActionKind::Terminal]);
        assert!(freeze_external(&io, ticket().context).unwrap());
        assert!(io.events.borrow().iter().any(|e| e == "terminal"));
        let io = Fixture::new(&[driver::ActionKind::Published]);
        io.selected.set(Selection {
            failed: true,
            ..io.selected.get()
        });
        assert!(!freeze_external(&io, ticket().context).unwrap());
        let io = Fixture::new(&[]);
        let mut wrong = ticket().context;
        wrong.epoch += 1;
        assert!(!freeze_external(&io, wrong).unwrap());
        assert_eq!(io.observation.get(), 0);
    }
    #[test]
    fn invalidation_and_retirement_views_require_the_actual_current_owner_state() {
        let io = Fixture::new(&[]);
        assert!(!invalidate(&io, ticket().context, None).unwrap());
        io.selected.set(Selection {
            started: true,
            ..io.selected.get()
        });
        assert!(invalidate(&io, ticket().context, None).unwrap());
        assert_eq!(io.events.borrow().last().unwrap(), "progress");
        assert!(!retiring(&io));
        io.pending.set(true);
        assert!(retiring(&io));
        io.present.set(false);
        assert!(!retiring(&io));
    }
}
