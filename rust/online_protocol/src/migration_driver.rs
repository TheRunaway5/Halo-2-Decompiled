//! Session-owned scheduling for genuine, unpublished native preparation.
//!
//! Every observation is captured under the actual Session generation lock.
//! This module has no clock: suspicion comes from the existing HostAgreement.
//! Requests and numeric leases are metadata, never native owners or Ready.
//! Retiring is a reservation held across real outer-owner veto/destruction;
//! only authentic completion may leave it. Published selection never resets.

use crate::host_agreement::{Context, State as HostState};
use crate::survivor_selection::{self, Inventory, State as SelectionState};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Phase {
    Connecting,
    Loading,
    Running,
    Reviewing,
    Postgame,
    Failed,
    AwaitingWorld,
    Migrating,
    Rebinding,
}
impl TryFrom<u8> for Phase {
    type Error = Error;
    fn try_from(value: u8) -> Result<Self, Error> {
        match value {
            0 => Ok(Self::Connecting),
            1 => Ok(Self::Loading),
            2 => Ok(Self::Running),
            3 => Ok(Self::Reviewing),
            4 => Ok(Self::Postgame),
            5 => Ok(Self::Failed),
            6 => Ok(Self::AwaitingWorld),
            7 => Ok(Self::Migrating),
            8 => Ok(Self::Rebinding),
            _ => Err(Error),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Observation {
    pub context: Option<Context>,
    pub admitted: Option<Context>,
    pub self_peer: u8,
    pub phase: Phase,
    pub supported: bool,
    pub selection_started: bool,
    pub agreement: Option<HostState>,
    pub selection: Option<SelectionState>,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Ticket {
    pub context: Context,
    pub self_peer: u8,
    pub id: u64,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Reason {
    None,
    Healthy,
    Unavailable,
    Stale,
    Provenance,
    Aborted,
    Published,
    Fenced,
    Exhausted,
    NativeFailure,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ActionKind {
    Wait,
    Prepare,
    Retire,
    Published,
    Terminal,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Action {
    pub kind: ActionKind,
    pub ticket: Option<Ticket>,
    pub reason: Reason,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FreezeKind {
    Publish,
    Retire,
    Refuse,
    Invalidate,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct FreezeAdmission {
    pub kind: FreezeKind,
    pub reason: Reason,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Retirement {
    Retryable,
    RetiredButFenced,
    Stale,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Invalid migration preparation generation or observation")
    }
}
impl std::error::Error for Error {}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Stage {
    Idle,
    Preparing {
        ticket: Ticket,
        lease: Option<u64>,
        retire: Option<Reason>,
    },
    Retiring {
        ticket: Ticket,
        reason: Reason,
    },
    Published(Option<Ticket>),
    Terminal(Option<Ticket>, Reason),
}
pub struct Driver {
    context: Context,
    self_peer: u8,
    last_ticket: u64,
    stage: Stage,
}
impl Driver {
    pub fn new(self_peer: u8, context: Context) -> Result<Self, Error> {
        if self_peer >= 16
            || context.host >= 16
            || self_peer == context.host
            || context.peers.count_ones() < 2
            || context.peers & (1u16 << self_peer) == 0
            || context.peers & (1u16 << context.host) == 0
            || context.session.iter().all(|&v| v == 0)
            || context.epoch == u32::MAX
            || context.membership_revision == u64::MAX
        {
            return Err(Error);
        }
        Ok(Self {
            context,
            self_peer,
            last_ticket: 0,
            stage: Stage::Idle,
        })
    }
    fn action(&self) -> Action {
        let (kind, ticket, reason) = match self.stage {
            Stage::Idle => (ActionKind::Wait, None, Reason::None),
            Stage::Preparing {
                ticket,
                retire: Some(reason),
                ..
            } => (ActionKind::Retire, Some(ticket), reason),
            Stage::Preparing { ticket, .. } => (ActionKind::Prepare, Some(ticket), Reason::None),
            Stage::Retiring { ticket, reason } => (ActionKind::Retire, Some(ticket), reason),
            Stage::Published(ticket) => (ActionKind::Published, ticket, Reason::Published),
            Stage::Terminal(ticket, reason) => (ActionKind::Terminal, ticket, reason),
        };
        Action {
            kind,
            ticket,
            reason,
        }
    }
    fn binding(&self, observation: &Observation) -> Result<(), Reason> {
        if observation.self_peer != self.self_peer || observation.context != Some(self.context) {
            return Err(Reason::Stale);
        }
        if observation.admitted != Some(self.context)
            || !observation.supported
            || !matches!(observation.phase, Phase::Running | Phase::AwaitingWorld)
            || observation.agreement.is_none()
            || observation.selection.is_none()
        {
            return Err(Reason::Unavailable);
        }
        let selection = observation.selection.as_ref().ok_or(Reason::Unavailable)?;
        let survivors = self.context.peers & !(1u16 << self.context.host);
        let agreement = observation.agreement.as_ref().ok_or(Reason::Unavailable)?;
        if selection.context != self.context
            || selection.survivors != survivors
            || agreement.electorate != survivors
        {
            return Err(Reason::Stale);
        }
        Ok(())
    }
    fn published(&self, observation: &Observation) -> bool {
        observation.selection_started
            || observation.selection.is_some_and(|s| {
                s.context == self.context && s.inventory_peers & (1u16 << self.self_peer) != 0
            })
    }
    fn fenced(&self, observation: &Observation) -> bool {
        observation.agreement.is_some_and(|s| {
            s.fenced || s.decision.is_some() || s.ready.iter().any(Option::is_some)
        }) || observation.selection.is_some_and(|s| s.ready_observed != 0)
    }
    fn failed(&self, observation: &Observation) -> bool {
        observation.selection.is_some_and(|s| s.failed)
    }
    fn eligible(&self, observation: &Observation) -> Result<(), Reason> {
        self.binding(observation)?;
        if self.failed(observation) {
            return Err(Reason::Aborted);
        }
        if self.published(observation) {
            return Err(Reason::Published);
        }
        if self.fenced(observation) {
            return Err(Reason::Fenced);
        }
        Ok(())
    }
    /// Poll an actual Session observation. Repeated requests retain one ticket;
    /// no second capture may start until real retirement completes.
    pub fn step(&mut self, observation: &Observation) -> Action {
        if matches!(
            self.stage,
            Stage::Retiring { .. } | Stage::Published(_) | Stage::Terminal(..)
        ) {
            return self.action();
        }
        let ticket = match self.stage {
            Stage::Preparing { ticket, .. } => Some(ticket),
            _ => None,
        };
        if self.failed(observation) {
            self.stage = Stage::Terminal(ticket, Reason::Aborted);
        } else if self.published(observation) {
            self.stage = Stage::Published(ticket);
        } else if self.fenced(observation) {
            self.stage = Stage::Terminal(ticket, Reason::Fenced);
        } else if let Err(reason) = self.binding(observation) {
            if reason == Reason::Stale {
                self.stage = Stage::Terminal(ticket, reason);
            } else if let Stage::Preparing { retire, .. } = &mut self.stage {
                *retire = Some(reason);
            }
        } else if !observation.agreement.is_some_and(|s| s.suspected) {
            if let Stage::Preparing { retire, .. } = &mut self.stage {
                *retire = Some(Reason::Healthy);
            }
        } else if self.stage == Stage::Idle {
            if let Some(id) = self.last_ticket.checked_add(1) {
                self.last_ticket = id;
                let ticket = Ticket {
                    context: self.context,
                    self_peer: self.self_peer,
                    id,
                };
                self.stage = Stage::Preparing {
                    ticket,
                    lease: None,
                    retire: None,
                };
            } else {
                self.stage = Stage::Terminal(None, Reason::Exhausted);
            }
        }
        self.action()
    }
    /// Complete actual capture. Zero reports failure, requesting retirement of
    /// any partial owners. A real preexisting lease may differ from ticket id.
    pub fn retained(&mut self, expected: Ticket, retained_lease: u64) -> bool {
        let Stage::Preparing {
            ticket,
            lease,
            retire,
        } = &mut self.stage
        else {
            return false;
        };
        if *ticket != expected {
            return false;
        }
        if retained_lease == 0 {
            *retire = Some(Reason::NativeFailure);
            return true;
        }
        if lease.is_some_and(|value| value != retained_lease) {
            return false;
        }
        *lease = Some(retained_lease);
        true
    }
    fn provenance(
        &self,
        observation: &Observation,
        inventory: &Inventory,
        common: Option<&crate::common_cut::State>,
    ) -> FreezeAdmission {
        if let Err(reason) = self.eligible(observation) {
            return FreezeAdmission {
                kind: FreezeKind::Refuse,
                reason,
            };
        }
        let admission = survivor_selection::inventory_admission(self.context, inventory, common);
        // Preserve the established exact-Abort priority. Healthy return may
        // cancel an unpublished capture, never erase an actual matching Abort.
        if admission == survivor_selection::InventoryAdmission::Invalidate {
            return FreezeAdmission {
                kind: FreezeKind::Invalidate,
                reason: Reason::Aborted,
            };
        }
        if !observation.agreement.is_some_and(|s| s.suspected) {
            return FreezeAdmission {
                kind: FreezeKind::Retire,
                reason: Reason::Healthy,
            };
        }
        match admission {
            survivor_selection::InventoryAdmission::Admit => FreezeAdmission {
                kind: FreezeKind::Publish,
                reason: Reason::None,
            },
            survivor_selection::InventoryAdmission::Refuse => FreezeAdmission {
                kind: FreezeKind::Retire,
                reason: Reason::Provenance,
            },
            survivor_selection::InventoryAdmission::Invalidate => FreezeAdmission {
                kind: FreezeKind::Invalidate,
                reason: Reason::Aborted,
            },
        }
    }
    /// Called under the same Session lock as the immediately following real
    /// selector publication. A prior suspected snapshot cannot grant this.
    pub fn admit_freeze(
        &mut self,
        observation: &Observation,
        expected: Ticket,
        inventory: &Inventory,
        common: Option<&crate::common_cut::State>,
    ) -> FreezeAdmission {
        let Stage::Preparing {
            ticket,
            lease,
            retire,
        } = self.stage
        else {
            return FreezeAdmission {
                kind: FreezeKind::Refuse,
                reason: Reason::Unavailable,
            };
        };
        if ticket != expected || lease != Some(inventory.lease) || inventory.lease == 0 {
            return FreezeAdmission {
                kind: FreezeKind::Refuse,
                reason: Reason::Stale,
            };
        }
        // Reclassify actual Abort even after an earlier healthy/provenance
        // refusal latched retirement. The local row may still be unpublished,
        // so selector-side relevance filtering cannot replace this check.
        let result = self.provenance(observation, inventory, common);
        if result.kind == FreezeKind::Invalidate {
            self.stage = Stage::Terminal(Some(ticket), Reason::Aborted);
            return result;
        }
        if result.kind == FreezeKind::Refuse {
            return result;
        }
        if let Some(reason) = retire {
            return FreezeAdmission {
                kind: FreezeKind::Retire,
                reason,
            };
        }
        if result.kind == FreezeKind::Retire {
            self.stage = Stage::Preparing {
                ticket,
                lease,
                retire: Some(result.reason),
            };
        }
        result
    }
    /// Compatibility entry for authentic native callers without a driver
    /// ticket. It still requires fresh suspicion; active requests exclude it.
    pub fn admit_external_freeze(
        &self,
        observation: &Observation,
        inventory: &Inventory,
        common: Option<&crate::common_cut::State>,
    ) -> FreezeAdmission {
        if self.stage != Stage::Idle {
            return FreezeAdmission {
                kind: FreezeKind::Refuse,
                reason: Reason::Unavailable,
            };
        }
        self.provenance(observation, inventory, common)
    }
    /// Reserve only unpublished local owners. Caller holds this reservation
    /// through genuine outer gates/destruction, never a boolean check-then-drop.
    pub fn reserve_retirement(&mut self, observation: &Observation, expected: Ticket) -> bool {
        if let Stage::Retiring { ticket, .. } = self.stage {
            return ticket == expected;
        }
        let Stage::Preparing { ticket, retire, .. } = self.stage else {
            return false;
        };
        // Losing support/admission or leaving a runnable phase prohibits a
        // retry, not destruction of these still-unadvertised native owners.
        // Actual generation and both metadata owners must remain identifiable.
        if ticket != expected
            || retire.is_none()
            || observation.context != Some(self.context)
            || observation.self_peer != self.self_peer
            || observation.agreement.is_none()
            || observation
                .selection
                .is_none_or(|s| s.context != self.context)
            || self.published(observation)
            || self.fenced(observation)
            || self.failed(observation)
        {
            return false;
        }
        self.stage = Stage::Retiring {
            ticket,
            reason: retire.unwrap_or(Reason::NativeFailure),
        };
        true
    }
    /// Invoke only after real native/outer owners have retired. A new fence or
    /// generation cannot be undone by completing an older reservation.
    pub fn retired(&mut self, observation: &Observation, expected: Ticket) -> Retirement {
        let Stage::Retiring { ticket, .. } = self.stage else {
            return Retirement::Stale;
        };
        if ticket != expected {
            return Retirement::Stale;
        }
        if self.eligible(observation).is_err() {
            self.stage = Stage::Terminal(Some(ticket), Reason::Fenced);
            Retirement::RetiredButFenced
        } else {
            self.stage = Stage::Idle;
            Retirement::Retryable
        }
    }
    pub fn blocks_publication(&self) -> bool {
        matches!(self.stage, Stage::Preparing { .. } | Stage::Retiring { .. })
    }
    pub fn retirement_pending(&self) -> bool {
        matches!(
            self.stage,
            Stage::Preparing {
                retire: Some(_),
                ..
            } | Stage::Retiring { .. }
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{common_cut, host_agreement};

    fn context() -> Context {
        Context {
            session: [0x71; 32],
            epoch: 7,
            membership_revision: 0xfedcba9876543210,
            peers: 0x111,
            host: 0,
        }
    }
    fn cut() -> host_agreement::Cut {
        host_agreement::Cut {
            id: u64::MAX,
            tick: u64::MAX,
            bytes: 64 * 1024 * 1024,
            digest: [0xa5; 32],
        }
    }
    fn inventory(lease: u64, irreversible_common: bool) -> Inventory {
        Inventory {
            lease,
            cuts: [
                Some(survivor_selection::InventoryEntry {
                    cut: cut(),
                    irreversible_common,
                }),
                None,
                None,
            ],
        }
    }
    fn observe(
        host: &host_agreement::Agreement,
        selector: &survivor_selection::Agreement,
        now: u64,
    ) -> Observation {
        Observation {
            context: Some(context()),
            admitted: Some(context()),
            self_peer: 4,
            phase: Phase::Running,
            supported: true,
            selection_started: false,
            agreement: Some(host.state(now)),
            selection: Some(selector.state()),
        }
    }
    fn owners() -> (
        host_agreement::Agreement,
        survivor_selection::Agreement,
        Driver,
    ) {
        (
            host_agreement::Agreement::new(4, context(), 97, 0).unwrap(),
            survivor_selection::Agreement::new(4, context(), 97).unwrap(),
            Driver::new(4, context()).unwrap(),
        )
    }
    fn common() -> common_cut::State {
        common_cut::State {
            context: context(),
            recipients: 0x110,
            committed: Some(common_cut::CommonCut {
                context: context(),
                seal: cut(),
                recipients: 0x110,
            }),
            pending: None,
            associated: 0,
            applied: 0,
            released: 0,
            aborted: 0,
            native_all_validated: false,
            commit: false,
            release: false,
            abort: false,
            rejected: 0,
        }
    }
    #[test]
    fn actual_host_suspicion_is_strict_and_repeated_steps_keep_one_ticket() {
        let (mut host, selector, mut driver) = owners();
        for now in [0, 499, 500] {
            assert_eq!(
                driver.step(&observe(&host, &selector, now)).kind,
                ActionKind::Wait
            );
        }
        let request = driver.step(&observe(&host, &selector, 501));
        assert_eq!(request.kind, ActionKind::Prepare);
        let ticket = request.ticket.unwrap();
        assert_eq!(
            (ticket.context, ticket.self_peer, ticket.id),
            (context(), 4, 1)
        );
        assert_eq!(driver.step(&observe(&host, &selector, 502)), request);
        host.observe_host(600);
        let cancel = driver.step(&observe(&host, &selector, 600));
        assert_eq!(
            (cancel.kind, cancel.ticket, cancel.reason),
            (ActionKind::Retire, Some(ticket), Reason::Healthy)
        );
        // Renewed suspicion cannot silently discard a required retirement.
        assert_eq!(driver.step(&observe(&host, &selector, 1101)), cancel);
    }
    #[test]
    fn host_return_between_capture_and_publication_requires_real_retirement() {
        let (mut host, selector, mut driver) = owners();
        let ticket = driver.step(&observe(&host, &selector, 501)).ticket.unwrap();
        assert!(driver.retained(ticket, 0xfedcba9876543210));
        host.observe_host(600);
        let observed = observe(&host, &selector, 600);
        assert_eq!(
            driver.admit_freeze(
                &observed,
                ticket,
                &inventory(0xfedcba9876543210, false),
                None
            ),
            FreezeAdmission {
                kind: FreezeKind::Retire,
                reason: Reason::Healthy
            }
        );
        assert!(driver.blocks_publication());
        assert_eq!(driver.retired(&observed, ticket), Retirement::Stale);
        assert!(driver.reserve_retirement(&observed, ticket));
        assert_eq!(
            driver.step(&observe(&host, &selector, 1200)).kind,
            ActionKind::Retire
        );
        assert_eq!(driver.retired(&observed, ticket), Retirement::Retryable);
        assert!(!driver.blocks_publication());
        assert_eq!(driver.step(&observed).kind, ActionKind::Wait);
        let next = driver
            .step(&observe(&host, &selector, 1101))
            .ticket
            .unwrap();
        assert_eq!(next.id, 2);
        assert!(!driver.retained(ticket, 1));
        assert_eq!(driver.retired(&observed, ticket), Retirement::Stale);
        assert_eq!(
            driver.step(&observe(&host, &selector, 1101)).ticket,
            Some(next)
        );
    }
    #[test]
    fn actual_lease_is_bound_once_without_forcing_ticket_identity() {
        let (host, selector, mut driver) = owners();
        let observed = observe(&host, &selector, 501);
        let ticket = driver.step(&observed).ticket.unwrap();
        assert!(driver.retained(ticket, u64::MAX));
        assert!(driver.retained(ticket, u64::MAX));
        assert!(!driver.retained(ticket, u64::MAX - 1));
        assert_eq!(
            driver
                .admit_freeze(&observed, ticket, &inventory(ticket.id, false), None)
                .kind,
            FreezeKind::Refuse
        );
        assert_eq!(
            driver
                .admit_freeze(&observed, ticket, &inventory(u64::MAX, false), None)
                .kind,
            FreezeKind::Publish
        );
        assert_eq!(
            driver
                .admit_external_freeze(&observed, &inventory(u64::MAX, false), None)
                .kind,
            FreezeKind::Refuse
        );
    }
    #[test]
    fn current_witness_rotation_retires_before_a_fresh_ticket_can_retry() {
        let (host, selector, mut driver) = owners();
        let observed = observe(&host, &selector, 501);
        let ticket = driver.step(&observed).ticket.unwrap();
        assert!(driver.retained(ticket, 99));
        let retained = inventory(99, true);
        let original = common();
        assert_eq!(
            driver
                .admit_freeze(&observed, ticket, &retained, Some(&original))
                .kind,
            FreezeKind::Publish
        );
        let mut rotated = original;
        rotated.committed.as_mut().unwrap().seal.id -= 1;
        assert_eq!(
            driver.admit_freeze(&observed, ticket, &retained, Some(&rotated)),
            FreezeAdmission {
                kind: FreezeKind::Retire,
                reason: Reason::Provenance
            }
        );
        // Restoring metadata does not revive a ticket already marked retire.
        assert_eq!(
            driver
                .admit_freeze(&observed, ticket, &retained, Some(&original))
                .kind,
            FreezeKind::Retire
        );
        assert!(driver.reserve_retirement(&observed, ticket));
        assert_eq!(driver.retired(&observed, ticket), Retirement::Retryable);
        assert!(driver.step(&observed).ticket.unwrap().id > ticket.id);
    }
    #[test]
    fn exact_advertised_abort_is_terminal_without_a_selector_reset() {
        let (host, selector, mut driver) = owners();
        let observed = observe(&host, &selector, 501);
        let ticket = driver.step(&observed).ticket.unwrap();
        assert!(driver.retained(ticket, 99));
        let mut aborted = common();
        aborted.pending = aborted.committed;
        aborted.abort = true;
        assert_eq!(
            driver
                .admit_freeze(&observed, ticket, &inventory(99, true), Some(&aborted))
                .kind,
            FreezeKind::Invalidate
        );
        assert_eq!(driver.step(&observed).kind, ActionKind::Terminal);
        assert!(!driver.reserve_retirement(&observed, ticket));
        let (mut host, selector, mut driver) = owners();
        let ticket = driver.step(&observe(&host, &selector, 501)).ticket.unwrap();
        assert!(driver.retained(ticket, 99));
        host.observe_host(600);
        assert_eq!(
            driver
                .admit_freeze(
                    &observe(&host, &selector, 600),
                    ticket,
                    &inventory(99, true),
                    Some(&aborted)
                )
                .kind,
            FreezeKind::Invalidate
        );
        for healthy in [true, false] {
            let (mut host, selector, mut driver) = owners();
            let ticket = driver.step(&observe(&host, &selector, 501)).ticket.unwrap();
            assert!(driver.retained(ticket, 99));
            if healthy {
                host.observe_host(600);
            }
            let observed = observe(&host, &selector, 600);
            assert_eq!(
                driver
                    .admit_freeze(&observed, ticket, &inventory(99, true), None)
                    .kind,
                FreezeKind::Retire
            );
            assert_eq!(
                driver
                    .admit_freeze(&observed, ticket, &inventory(99, true), Some(&aborted))
                    .kind,
                FreezeKind::Invalidate
            );
            assert_eq!(driver.step(&observed).kind, ActionKind::Terminal);
        }
    }
    #[test]
    fn own_inventory_install_is_irreversible_before_any_packet_is_polled() {
        let (host, mut selector, mut driver) = owners();
        let mut observed = observe(&host, &selector, 501);
        let ticket = driver.step(&observed).ticket.unwrap();
        assert!(driver.retained(ticket, 88));
        assert!(selector.publish_inventory(inventory(88, false)).unwrap());
        observed.selection = Some(selector.state());
        assert_eq!(driver.step(&observed).kind, ActionKind::Published);
        assert!(!driver.reserve_retirement(&observed, ticket));
        assert_eq!(driver.retired(&observed, ticket), Retirement::Stale);
        assert_eq!(
            driver
                .admit_external_freeze(&observed, &inventory(89, false), None)
                .kind,
            FreezeKind::Refuse
        );
        assert_eq!(selector.state().inventory_peers, 1 << 4);
    }
    #[test]
    fn remote_rows_are_not_local_publication_but_early_actual_ready_is_a_fence() {
        let (mut host, selector, mut driver) = owners();
        let mut observed = observe(&host, &selector, 501);
        observed.selection.as_mut().unwrap().inventory_peers = 1 << 8;
        let ticket = driver.step(&observed).ticket.unwrap();
        assert!(driver.retained(ticket, 7));
        host.publish_ready(4, context(), cut()).unwrap();
        observed.agreement = Some(host.state(501));
        // The selector has not been told Ready: the real agreement bank wins.
        assert_eq!(observed.selection.unwrap().ready_observed, 0);
        assert_eq!(
            driver
                .admit_freeze(&observed, ticket, &inventory(7, false), None)
                .reason,
            Reason::Fenced
        );
        assert_eq!(driver.step(&observed).kind, ActionKind::Terminal);
        assert!(!driver.reserve_retirement(&observed, ticket));
    }
    #[test]
    fn reserved_retirement_survives_veto_and_new_ready_but_never_resumes() {
        let (mut host, selector, mut driver) = owners();
        let observed = observe(&host, &selector, 501);
        let ticket = driver.step(&observed).ticket.unwrap();
        assert!(driver.retained(ticket, 0));
        assert_eq!(driver.step(&observed).reason, Reason::NativeFailure);
        assert!(driver.reserve_retirement(&observed, ticket));
        host.publish_ready(4, context(), cut()).unwrap();
        let changed = observe(&host, &selector, 501);
        assert_eq!(driver.step(&changed).kind, ActionKind::Retire);
        assert!(driver.reserve_retirement(&changed, ticket));
        assert!(driver.blocks_publication());
        assert_eq!(
            driver
                .admit_external_freeze(&changed, &inventory(99, false), None)
                .kind,
            FreezeKind::Refuse
        );
        assert_eq!(
            driver.retired(&changed, ticket),
            Retirement::RetiredButFenced
        );
        assert_eq!(driver.step(&observed).kind, ActionKind::Terminal);
    }
    #[test]
    fn unavailable_preparation_can_retire_but_cannot_resume_or_retry() {
        for field in 0..3 {
            let (host, selector, mut driver) = owners();
            let observed = observe(&host, &selector, 501);
            let ticket = driver.step(&observed).ticket.unwrap();
            assert!(driver.retained(ticket, 99));
            let mut unavailable = observed;
            match field {
                0 => unavailable.supported = false,
                1 => unavailable.phase = Phase::Postgame,
                _ => unavailable.admitted = None,
            }
            let action = driver.step(&unavailable);
            assert_eq!(
                (action.kind, action.reason),
                (ActionKind::Retire, Reason::Unavailable)
            );
            assert!(driver.reserve_retirement(&unavailable, ticket));
            assert!(driver.blocks_publication());
            assert_eq!(
                driver.retired(&unavailable, ticket),
                Retirement::RetiredButFenced
            );
            assert_eq!(driver.step(&observed).kind, ActionKind::Terminal);
        }
    }
    #[test]
    fn stale_completion_and_every_generation_field_cannot_release_a_reservation() {
        let (host, selector, mut driver) = owners();
        let observed = observe(&host, &selector, 501);
        let ticket = driver.step(&observed).ticket.unwrap();
        assert!(driver.retained(ticket, 0));
        assert!(driver.reserve_retirement(&observed, ticket));
        for field in 0..8 {
            let mut wrong = ticket;
            match field {
                0 => wrong.context.session[31] ^= 1,
                1 => wrong.context.epoch += 1,
                2 => wrong.context.membership_revision += 1,
                3 => wrong.context.peers ^= 1 << 8,
                4 => wrong.context.host = 8,
                5 => wrong.self_peer = 8,
                6 => wrong.id += 1,
                _ => wrong.id = 0,
            }
            assert_eq!(driver.retired(&observed, wrong), Retirement::Stale);
            assert!(!driver.retained(wrong, 5));
            assert_eq!(driver.step(&observed).ticket, Some(ticket));
        }
        let mut stale = observed;
        stale.context.as_mut().unwrap().epoch += 1;
        assert_eq!(driver.retired(&stale, ticket), Retirement::RetiredButFenced);
        assert_eq!(driver.step(&observed).kind, ActionKind::Terminal);
    }
    #[test]
    fn generic_freeze_uses_fresh_suspicion_and_unavailable_inputs_fail_closed() {
        let (host, selector, mut driver) = owners();
        let healthy = observe(&host, &selector, 500);
        assert_eq!(
            driver
                .admit_external_freeze(&healthy, &inventory(7, false), None)
                .reason,
            Reason::Healthy
        );
        let observed = observe(&host, &selector, 501);
        assert_eq!(
            driver
                .admit_external_freeze(&observed, &inventory(7, false), None)
                .kind,
            FreezeKind::Publish
        );
        for field in 0..7 {
            let mut invalid = observed;
            match field {
                0 => invalid.admitted = None,
                1 => invalid.supported = false,
                2 => invalid.phase = Phase::Failed,
                3 => invalid.agreement = None,
                4 => invalid.selection = None,
                5 => invalid.self_peer = 16,
                _ => invalid.context.as_mut().unwrap().peers ^= 1 << 8,
            }
            assert_eq!(
                driver
                    .admit_external_freeze(&invalid, &inventory(7, false), None)
                    .kind,
                FreezeKind::Refuse
            );
        }
        driver.last_ticket = u64::MAX;
        assert_eq!(
            driver.step(&observed),
            Action {
                kind: ActionKind::Terminal,
                ticket: None,
                reason: Reason::Exhausted
            }
        );
    }
    #[test]
    fn constructors_and_phase_indices_preserve_fixed_physical_membership_bounds() {
        for peer in 0..16 {
            let c = Context {
                peers: u16::MAX,
                ..context()
            };
            assert_eq!(Driver::new(peer, c).is_ok(), peer != c.host);
        }
        for field in 0..7 {
            let mut c = context();
            let mut peer = 4;
            match field {
                0 => peer = 16,
                1 => c.host = 16,
                2 => c.peers &= !(1 << 4),
                3 => c.peers &= !1,
                4 => c.session = [0; 32],
                5 => c.epoch = u32::MAX,
                _ => c.membership_revision = u64::MAX,
            }
            assert!(Driver::new(peer, c).is_err());
        }
        assert!(Phase::try_from(9).is_err());
        assert!(Phase::try_from(255).is_err());
    }
}
