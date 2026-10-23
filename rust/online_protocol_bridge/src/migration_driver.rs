//! Typed scheduling metadata only. Session owns serialization and native
//! callers retain genuine resources; the protocol crate owns admission.
use crate::ffi;
use halo2_online_protocol::{
    host_agreement as host, migration_driver as protocol, survivor_selection as selection,
};

pub struct MigrationDriver {
    inner: protocol::Driver,
}
fn protect<T>(call: impl FnOnce() -> Result<T, protocol::Error>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(call))
        .map_err(|_| "Rust migration preparation failed".to_owned())?
        .map_err(|error| error.to_string())
}
fn context(v: &ffi::HostAgreementContext) -> host::Context {
    host::Context {
        session: v.session,
        epoch: v.epoch,
        membership_revision: v.membership_revision,
        peers: v.peers,
        host: v.host,
    }
}
fn to_context(v: host::Context) -> ffi::HostAgreementContext {
    ffi::HostAgreementContext {
        session: v.session,
        epoch: v.epoch,
        membership_revision: v.membership_revision,
        peers: v.peers,
        host: v.host,
    }
}
fn ticket(v: &ffi::MigrationDriverTicket) -> protocol::Ticket {
    protocol::Ticket {
        context: context(&v.context),
        self_peer: v.self_peer,
        id: v.id,
    }
}
fn to_ticket(v: Option<protocol::Ticket>) -> ffi::MigrationDriverTicket {
    let present = v.is_some();
    let v = v.unwrap_or(protocol::Ticket {
        context: host::Context {
            session: [0; 32],
            epoch: 0,
            membership_revision: 0,
            peers: 0,
            host: 16,
        },
        self_peer: 16,
        id: 0,
    });
    ffi::MigrationDriverTicket {
        present,
        context: to_context(v.context),
        self_peer: v.self_peer,
        id: v.id,
    }
}
fn observation(
    v: &ffi::MigrationDriverObservation,
) -> Result<protocol::Observation, protocol::Error> {
    let agreement = if v.agreement_present {
        let a = &v.agreement;
        let decision = if a.decision.present {
            let d = &a.decision;
            Some(host::Decision {
                previous: context(&d.previous),
                next: context(&d.next),
                ballot: host::Ballot {
                    round: d.ballot.round,
                    proposer: d.ballot.proposer,
                },
                value: host::Value {
                    host: d.value.host,
                    cut: crate::preparation_cut(&d.value.cut).ok_or(protocol::Error)?,
                },
                voters: d.voters,
            })
        } else {
            None
        };
        Some(host::State {
            suspected: a.suspected,
            fenced: a.fenced,
            electorate: a.electorate,
            silent: a.silent,
            majority: a.majority,
            ready: a.ready.each_ref().map(crate::preparation_cut),
            decision,
            rejected: a.rejected,
        })
    } else {
        None
    };
    let selected = &v.selection;
    let selection = v.selection_present.then(|| selection::State {
        context: context(&selected.context),
        survivors: selected.survivors,
        inventory_peers: selected.inventory_peers,
        accepted: selected.accepted,
        selected: crate::preparation_cut(&selected.selected),
        transcript: selected.transcript,
        selection_complete: selected.selection_complete,
        failed: selected.failed,
        failure: None,
        ready_observed: selected.ready_observed,
        rejected: selected.rejected,
    });
    Ok(protocol::Observation {
        context: v.context_present.then(|| context(&v.context)),
        admitted: v.admitted_present.then(|| context(&v.admitted)),
        self_peer: v.self_peer,
        phase: protocol::Phase::try_from(v.phase)?,
        supported: v.supported,
        selection_started: v.selection_started,
        agreement,
        selection,
    })
}
fn inventory(v: &ffi::SurvivorInventory) -> selection::Inventory {
    selection::Inventory {
        lease: v.lease,
        cuts: v.cuts.each_ref().map(|v| {
            crate::preparation_cut(&v.cut).map(|cut| selection::InventoryEntry {
                cut,
                irreversible_common: v.irreversible_common,
            })
        }),
    }
}
fn reason(v: protocol::Reason) -> ffi::MigrationDriverReason {
    match v {
        protocol::Reason::None => ffi::MigrationDriverReason::None,
        protocol::Reason::Healthy => ffi::MigrationDriverReason::Healthy,
        protocol::Reason::Unavailable => ffi::MigrationDriverReason::Unavailable,
        protocol::Reason::Stale => ffi::MigrationDriverReason::Stale,
        protocol::Reason::Provenance => ffi::MigrationDriverReason::Provenance,
        protocol::Reason::Aborted => ffi::MigrationDriverReason::Aborted,
        protocol::Reason::Published => ffi::MigrationDriverReason::Published,
        protocol::Reason::Fenced => ffi::MigrationDriverReason::Fenced,
        protocol::Reason::Exhausted => ffi::MigrationDriverReason::Exhausted,
        protocol::Reason::NativeFailure => ffi::MigrationDriverReason::NativeFailure,
    }
}
fn freeze(v: protocol::FreezeAdmission) -> ffi::MigrationDriverFreeze {
    let kind = match v.kind {
        protocol::FreezeKind::Publish => ffi::MigrationDriverFreezeKind::Publish,
        protocol::FreezeKind::Retire => ffi::MigrationDriverFreezeKind::Retire,
        protocol::FreezeKind::Refuse => ffi::MigrationDriverFreezeKind::Refuse,
        protocol::FreezeKind::Invalidate => ffi::MigrationDriverFreezeKind::Invalidate,
    };
    ffi::MigrationDriverFreeze {
        kind,
        reason: reason(v.reason),
    }
}
pub fn new_migration_driver(
    self_peer: u8,
    binding: &ffi::HostAgreementContext,
) -> Result<Box<MigrationDriver>, String> {
    protect(|| {
        protocol::Driver::new(self_peer, context(binding))
            .map(|inner| Box::new(MigrationDriver { inner }))
    })
}
pub fn migration_driver_step(
    driver: &mut MigrationDriver,
    value: &ffi::MigrationDriverObservation,
) -> Result<ffi::MigrationDriverAction, String> {
    protect(|| {
        let value = driver.inner.step(&observation(value)?);
        let kind = match value.kind {
            protocol::ActionKind::Wait => ffi::MigrationDriverActionKind::Wait,
            protocol::ActionKind::Prepare => ffi::MigrationDriverActionKind::Prepare,
            protocol::ActionKind::Retire => ffi::MigrationDriverActionKind::Retire,
            protocol::ActionKind::Published => ffi::MigrationDriverActionKind::Published,
            protocol::ActionKind::Terminal => ffi::MigrationDriverActionKind::Terminal,
        };
        Ok(ffi::MigrationDriverAction {
            kind,
            ticket: to_ticket(value.ticket),
            reason: reason(value.reason),
        })
    })
}
pub fn migration_driver_retained(
    driver: &mut MigrationDriver,
    value: &ffi::MigrationDriverTicket,
    lease: u64,
) -> Result<bool, String> {
    protect(|| Ok(value.present && driver.inner.retained(ticket(value), lease)))
}
pub fn migration_driver_admit_freeze(
    driver: &mut MigrationDriver,
    observed: &ffi::MigrationDriverObservation,
    value: &ffi::MigrationDriverTicket,
    retained: &ffi::SurvivorInventory,
    common_present: bool,
    common: &ffi::CommonCutState,
) -> Result<ffi::MigrationDriverFreeze, String> {
    protect(|| {
        let observed = observation(observed)?;
        let common = common_present.then(|| crate::common_cut::snapshot(common));
        if !value.present {
            return Ok(freeze(protocol::FreezeAdmission {
                kind: protocol::FreezeKind::Refuse,
                reason: protocol::Reason::Stale,
            }));
        }
        Ok(freeze(driver.inner.admit_freeze(
            &observed,
            ticket(value),
            &inventory(retained),
            common.as_ref(),
        )))
    })
}
pub fn migration_driver_admit_external_freeze(
    driver: &MigrationDriver,
    observed: &ffi::MigrationDriverObservation,
    retained: &ffi::SurvivorInventory,
    common_present: bool,
    common: &ffi::CommonCutState,
) -> Result<ffi::MigrationDriverFreeze, String> {
    protect(|| {
        let common = common_present.then(|| crate::common_cut::snapshot(common));
        Ok(freeze(driver.inner.admit_external_freeze(
            &observation(observed)?,
            &inventory(retained),
            common.as_ref(),
        )))
    })
}
pub fn migration_driver_reserve_retirement(
    driver: &mut MigrationDriver,
    observed: &ffi::MigrationDriverObservation,
    value: &ffi::MigrationDriverTicket,
) -> Result<bool, String> {
    protect(|| {
        Ok(value.present
            && driver
                .inner
                .reserve_retirement(&observation(observed)?, ticket(value)))
    })
}
pub fn migration_driver_retired(
    driver: &mut MigrationDriver,
    observed: &ffi::MigrationDriverObservation,
    value: &ffi::MigrationDriverTicket,
) -> Result<ffi::MigrationDriverRetirement, String> {
    protect(|| {
        let result = if value.present {
            driver.inner.retired(&observation(observed)?, ticket(value))
        } else {
            protocol::Retirement::Stale
        };
        Ok(match result {
            protocol::Retirement::Retryable => ffi::MigrationDriverRetirement::Retryable,
            protocol::Retirement::RetiredButFenced => {
                ffi::MigrationDriverRetirement::RetiredButFenced
            }
            protocol::Retirement::Stale => ffi::MigrationDriverRetirement::Stale,
        })
    })
}
pub fn migration_driver_blocks_publication(driver: &MigrationDriver) -> bool {
    driver.inner.blocks_publication()
}
pub fn migration_driver_retirement_pending(driver: &MigrationDriver) -> bool {
    driver.inner.retirement_pending()
}

#[cfg(test)]
mod tests {
    use super::*;
    fn binding() -> ffi::HostAgreementContext {
        ffi::HostAgreementContext {
            session: [0x71; 32],
            epoch: 7,
            membership_revision: u64::MAX - 1,
            peers: 0x111,
            host: 0,
        }
    }
    fn cut() -> ffi::MigrationPreparationCut {
        ffi::MigrationPreparationCut {
            present: true,
            id: u64::MAX,
            tick: u64::MAX,
            bytes: 64 * 1024 * 1024,
            digest: [0xa5; 32],
        }
    }
    fn inventory() -> ffi::SurvivorInventory {
        ffi::SurvivorInventory {
            lease: u64::MAX,
            cuts: std::array::from_fn(|i| ffi::SurvivorInventoryEntry {
                cut: if i == 0 {
                    cut()
                } else {
                    ffi::MigrationPreparationCut {
                        present: false,
                        id: 0,
                        tick: 0,
                        bytes: 0,
                        digest: [0; 32],
                    }
                },
                irreversible_common: false,
            }),
        }
    }
    fn observed(now: u64) -> ffi::MigrationDriverObservation {
        let agreement = crate::host_agreement::new_host_agreement(4, &binding(), 97, 0).unwrap();
        let selection =
            crate::survivor_selection::new_survivor_selection(4, &binding(), 97).unwrap();
        ffi::MigrationDriverObservation {
            context_present: true,
            context: binding(),
            admitted_present: true,
            admitted: binding(),
            self_peer: 4,
            phase: 2,
            supported: true,
            selection_started: false,
            agreement_present: true,
            agreement: crate::host_agreement::host_agreement_state(&agreement, now).unwrap(),
            selection_present: true,
            selection: crate::survivor_selection::survivor_selection_state(&selection).unwrap(),
        }
    }
    fn empty_common() -> ffi::CommonCutState {
        let owner = crate::common_cut::new_common_cut_agreement(0, &binding(), 97).unwrap();
        crate::common_cut::common_cut_state(&owner).unwrap()
    }
    #[test]
    fn bridge_preserves_full_ticket_lease_and_actual_row_before_transport_poll() {
        let mut driver = new_migration_driver(4, &binding()).unwrap();
        assert!(
            migration_driver_step(&mut driver, &observed(500))
                .unwrap()
                .kind
                == ffi::MigrationDriverActionKind::Wait
        );
        let request = migration_driver_step(&mut driver, &observed(501)).unwrap();
        assert!(request.kind == ffi::MigrationDriverActionKind::Prepare && request.ticket.present);
        assert_eq!(request.ticket.context.membership_revision, u64::MAX - 1);
        assert!(migration_driver_retained(&mut driver, &request.ticket, u64::MAX).unwrap());
        assert!(
            migration_driver_admit_freeze(
                &mut driver,
                &observed(501),
                &request.ticket,
                &inventory(),
                false,
                &empty_common()
            )
            .unwrap()
            .kind
                == ffi::MigrationDriverFreezeKind::Publish
        );
        let mut published = observed(501);
        published.selection.inventory_peers = 1 << 4;
        assert!(
            migration_driver_step(&mut driver, &published).unwrap().kind
                == ffi::MigrationDriverActionKind::Published
        );
        assert!(
            !migration_driver_reserve_retirement(&mut driver, &published, &request.ticket).unwrap()
        );
    }
    #[test]
    fn bridge_reservation_blocks_until_exact_retirement_and_does_not_reset_ready() {
        let mut driver = new_migration_driver(4, &binding()).unwrap();
        let request = migration_driver_step(&mut driver, &observed(501)).unwrap();
        assert!(migration_driver_retained(&mut driver, &request.ticket, 0).unwrap());
        assert!(migration_driver_retirement_pending(&driver));
        assert!(
            migration_driver_reserve_retirement(&mut driver, &observed(501), &request.ticket)
                .unwrap()
        );
        let mut changed = observed(501);
        changed.agreement.ready[8] = cut();
        assert!(migration_driver_blocks_publication(&driver));
        assert!(
            migration_driver_retired(&mut driver, &changed, &request.ticket).unwrap()
                == ffi::MigrationDriverRetirement::RetiredButFenced
        );
        assert!(
            migration_driver_step(&mut driver, &observed(501))
                .unwrap()
                .kind
                == ffi::MigrationDriverActionKind::Terminal
        );
        for peer in [16, 255] {
            assert!(new_migration_driver(peer, &binding()).is_err());
        }
        let mut malformed = observed(501);
        malformed.phase = 255;
        assert!(migration_driver_step(&mut driver, &malformed).is_err());
    }
}
