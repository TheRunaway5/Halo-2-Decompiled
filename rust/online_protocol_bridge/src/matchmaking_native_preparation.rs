//! Typed native caller ABI. The protocol crate owns orchestration and admission.
use crate::{
    ffi,
    matchmaking_native_publication::{context, cut, to_context},
    matchmaking_native_session::NativeSession,
};
use halo2_online_protocol::{
    host_agreement as host, matchmaking_native_preparation as core,
    matchmaking_native_publication as publication, migration_driver as driver,
    peer_source_admissions::Player,
};
fn checked<T>(v: core::Result<T>) -> Result<T, String> {
    v.map_err(|e| e.0)
}
fn io<T>(v: Result<T, cxx::Exception>) -> core::Result<T> {
    v.map_err(|e| core::Error(e.to_string()))
}
fn invalid() -> core::Error {
    core::Error("Invalid native preparation observation".into())
}
fn ticket(v: &ffi::NativePreparationTicket) -> Option<driver::Ticket> {
    v.present
        .then(|| {
            Some(driver::Ticket {
                context: context(&v.context)?,
                self_peer: v.self_peer.try_into().ok()?,
                id: v.id,
            })
        })
        .flatten()
}
fn to_ticket(v: Option<driver::Ticket>) -> ffi::NativePreparationTicket {
    ffi::NativePreparationTicket {
        present: v.is_some(),
        context: to_context(v.map(|t| t.context)),
        self_peer: v.map_or(16, |t| t.self_peer as u32),
        id: v.map_or(0, |t| t.id),
    }
}
fn outcome(v: core::Outcome) -> ffi::NativePreparationOutcome {
    match v {
        core::Outcome::Inactive => ffi::NativePreparationOutcome::Inactive,
        core::Outcome::Preparing => ffi::NativePreparationOutcome::Preparing,
        core::Outcome::Retiring => ffi::NativePreparationOutcome::Retiring,
        core::Outcome::Published => ffi::NativePreparationOutcome::Published,
        core::Outcome::Retryable => ffi::NativePreparationOutcome::Retryable,
        core::Outcome::Terminal => ffi::NativePreparationOutcome::Terminal,
        core::Outcome::Refused => ffi::NativePreparationOutcome::Refused,
    }
}
fn action(v: ffi::MigrationDriverAction) -> core::Result<driver::Action> {
    let kind = match v.kind {
        ffi::MigrationDriverActionKind::Wait => driver::ActionKind::Wait,
        ffi::MigrationDriverActionKind::Prepare => driver::ActionKind::Prepare,
        ffi::MigrationDriverActionKind::Retire => driver::ActionKind::Retire,
        ffi::MigrationDriverActionKind::Published => driver::ActionKind::Published,
        ffi::MigrationDriverActionKind::Terminal => driver::ActionKind::Terminal,
        _ => return Err(invalid()),
    };
    let reason = match v.reason {
        ffi::MigrationDriverReason::None => driver::Reason::None,
        ffi::MigrationDriverReason::Healthy => driver::Reason::Healthy,
        ffi::MigrationDriverReason::Unavailable => driver::Reason::Unavailable,
        ffi::MigrationDriverReason::Stale => driver::Reason::Stale,
        ffi::MigrationDriverReason::Provenance => driver::Reason::Provenance,
        ffi::MigrationDriverReason::Aborted => driver::Reason::Aborted,
        ffi::MigrationDriverReason::Published => driver::Reason::Published,
        ffi::MigrationDriverReason::Fenced => driver::Reason::Fenced,
        ffi::MigrationDriverReason::Exhausted => driver::Reason::Exhausted,
        ffi::MigrationDriverReason::NativeFailure => driver::Reason::NativeFailure,
        _ => return Err(invalid()),
    };
    let v = v.ticket;
    let ticket = v.present.then_some(driver::Ticket {
        context: host::Context {
            session: v.context.session,
            epoch: v.context.epoch,
            membership_revision: v.context.membership_revision,
            peers: v.context.peers,
            host: v.context.host,
        },
        self_peer: v.self_peer,
        id: v.id,
    });
    Ok(driver::Action {
        kind,
        ticket,
        reason,
    })
}
fn freeze(v: ffi::MigrationDriverFreezeKind) -> core::Result<driver::FreezeKind> {
    Ok(match v {
        ffi::MigrationDriverFreezeKind::Publish => driver::FreezeKind::Publish,
        ffi::MigrationDriverFreezeKind::Retire => driver::FreezeKind::Retire,
        ffi::MigrationDriverFreezeKind::Refuse => driver::FreezeKind::Refuse,
        ffi::MigrationDriverFreezeKind::Invalidate => driver::FreezeKind::Invalidate,
        _ => return Err(invalid()),
    })
}
pub fn native_preparation_authorize(
    s: &NativeSession,
    r: &ffi::NativePublicationResources,
    a: &ffi::NativePublicationAdmission,
    selected: bool,
) -> Result<ffi::NativePreparationAuthorization, String> {
    let players = |v: &[ffi::SourceAdmissionsPlayer]| {
        v.iter()
            .map(|p| Player {
                slot: p.slot,
                player: p.player,
                identity: p.identity,
            })
            .collect()
    };
    checked((|| {
        let r = publication::Resources {
            agreement_present: r.agreement_present,
            agreement_context: if r.agreement_context_present {
                Some(context(&r.agreement_context).ok_or_else(invalid)?)
            } else {
                None
            },
            authority: if r.authority_present {
                Some(context(&r.authority).ok_or_else(invalid)?)
            } else {
                None
            },
            source_present: r.source_present,
            history: r.history,
        };
        let a = publication::Admission {
            accepted: if a.present {
                Some((
                    context(&a.context).ok_or_else(invalid)?,
                    players(&a.players),
                ))
            } else {
                None
            },
            continuity: a.continuity_present.then(|| players(&a.continuity)),
        };
        let v = s.0.preparation_authorization(r, a, selected);
        Ok(ffi::NativePreparationAuthorization {
            supported: v.supported,
            admitted_present: v.admitted.is_some(),
            admitted: to_context(v.admitted),
        })
    })())
}
struct Io<'a>(&'a ffi::NativePreparationIo);
impl core::Io for Io<'_> {
    fn driver_present(&self) -> bool {
        self.0.preparation_driver_present()
    }
    fn owner_matches(&self) -> bool {
        self.0.preparation_owner_matches()
    }
    fn observe(&self) -> core::Result<()> {
        io(self.0.preparation_observe())
    }
    fn step(&self) -> core::Result<driver::Action> {
        action(io(self.0.preparation_step())?)
    }
    fn reserve_retirement(&self, t: driver::Ticket) -> core::Result<bool> {
        io(self.0.preparation_reserve(&to_ticket(Some(t))))
    }
    fn retained(&self, t: driver::Ticket, l: u64) -> core::Result<bool> {
        io(self.0.preparation_retained(&to_ticket(Some(t)), l))
    }
    fn retired(&self, t: driver::Ticket) -> core::Result<driver::Retirement> {
        Ok(match io(self.0.preparation_retired(&to_ticket(Some(t))))? {
            ffi::MigrationDriverRetirement::Retryable => driver::Retirement::Retryable,
            ffi::MigrationDriverRetirement::RetiredButFenced => {
                driver::Retirement::RetiredButFenced
            }
            ffi::MigrationDriverRetirement::Stale => driver::Retirement::Stale,
            _ => return Err(invalid()),
        })
    }
    fn admit_freeze(&self, t: driver::Ticket) -> core::Result<driver::FreezeKind> {
        freeze(io(self.0.preparation_admit(&to_ticket(Some(t))))?)
    }
    fn admit_external(&self) -> core::Result<driver::FreezeKind> {
        freeze(io(self.0.preparation_admit_external())?)
    }
    fn selection(&self) -> core::Result<core::Selection> {
        let v = io(self.0.preparation_selection())?;
        Ok(core::Selection {
            context: if v.present {
                Some(context(&v.context).ok_or_else(invalid)?)
            } else {
                None
            },
            started: v.started,
            failed: v.failed,
        })
    }
    fn publish_inventory(&self) -> core::Result<bool> {
        io(self.0.preparation_publish())
    }
    fn invalidate(
        &self,
        c: Option<halo2_online_protocol::migration_preparation::PreparationCut>,
    ) -> core::Result<bool> {
        io(self.0.preparation_invalidate(&cut(c)))
    }
    fn abort(&self) -> core::Result<()> {
        io(self.0.preparation_abort())
    }
    fn progress(&self) -> core::Result<()> {
        io(self.0.preparation_progress())
    }
    fn begin_selection(&self) -> core::Result<()> {
        io(self.0.preparation_begin())
    }
    fn terminal(&self) -> core::Result<()> {
        io(self.0.preparation_terminal())
    }
    fn retirement_pending(&self) -> bool {
        self.0.preparation_retirement_pending()
    }
}
pub fn native_preparation_request(
    v: &ffi::NativePreparationIo,
) -> Result<ffi::NativePreparationStep, String> {
    checked(core::request(&Io(v))).map(|v| ffi::NativePreparationStep {
        outcome: outcome(v.outcome),
        kind: if v
            .request
            .is_some_and(|(kind, _)| kind == core::Kind::Retire)
        {
            ffi::NativePreparationKind::Retire
        } else {
            ffi::NativePreparationKind::Prepare
        },
        ticket: to_ticket(v.request.map(|(_, t)| t)),
    })
}
pub fn native_preparation_complete(
    v: &ffi::NativePreparationIo,
    kind: ffi::NativePreparationKind,
    t: &ffi::NativePreparationTicket,
    present: bool,
    lease: u64,
) -> Result<ffi::NativePreparationOutcome, String> {
    let Some(t) = ticket(t) else {
        return Ok(ffi::NativePreparationOutcome::Refused);
    };
    let kind = match kind {
        ffi::NativePreparationKind::Prepare => core::Kind::Prepare,
        ffi::NativePreparationKind::Retire => core::Kind::Retire,
        _ => return Ok(ffi::NativePreparationOutcome::Refused),
    };
    checked(core::complete(&Io(v), kind, t, present, lease)).map(outcome)
}
pub fn native_preparation_freeze(
    v: &ffi::NativePreparationIo,
    c: &ffi::NativePublicationContext,
) -> Result<bool, String> {
    let Some(c) = context(c) else {
        return Ok(false);
    };
    checked(core::freeze_external(&Io(v), c))
}
pub fn native_preparation_invalidate(
    v: &ffi::NativePreparationIo,
    c: &ffi::NativePublicationContext,
    k: &ffi::MigrationPreparationCut,
) -> Result<bool, String> {
    let Some(c) = context(c) else {
        return Ok(false);
    };
    checked(core::invalidate(&Io(v), c, crate::preparation_cut(k)))
}
pub fn native_preparation_retiring(v: &ffi::NativePreparationIo) -> bool {
    core::retiring(&Io(v))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn full_width_native_request_context_and_owner_do_not_alias_valid_ticket() {
        let mut t = to_ticket(Some(driver::Ticket {
            context: host::Context {
                session: [1; 32],
                epoch: 2,
                membership_revision: 3,
                peers: 3,
                host: 0,
            },
            self_peer: 1,
            id: 7,
        }));
        assert!(ticket(&t).is_some());
        t.self_peer = 257;
        assert!(ticket(&t).is_none());
        t.self_peer = 1;
        t.context.host = 256;
        assert!(ticket(&t).is_none());
    }
}
