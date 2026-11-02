//! Typed publication ABI. The protocol driver owns all caller decisions.
use crate::{ffi, matchmaking_native_session::NativeSession};
use halo2_online_protocol::{
    host_agreement as host, matchmaking_native_publication as core,
    migration_preparation::PreparationCut as Cut, peer_source_admissions::Player,
};
pub(super) fn context(v: &ffi::NativePublicationContext) -> Option<host::Context> {
    Some(host::Context {
        session: v.session,
        epoch: v.epoch,
        membership_revision: v.membership_revision,
        peers: v.peers,
        host: v.host.try_into().ok()?,
    })
}
pub(super) fn to_context(v: Option<host::Context>) -> ffi::NativePublicationContext {
    let v = v.unwrap_or(host::Context {
        session: [0; 32],
        epoch: 0,
        membership_revision: 0,
        peers: 0,
        host: 16,
    });
    ffi::NativePublicationContext {
        session: v.session,
        epoch: v.epoch,
        membership_revision: v.membership_revision,
        peers: v.peers,
        host: v.host as u32,
    }
}
pub(super) fn cut(v: Option<Cut>) -> ffi::MigrationPreparationCut {
    let present = v.is_some();
    let v = v.unwrap_or(Cut {
        id: 0,
        tick: 0,
        bytes: 0,
        digest: [0; 32],
    });
    ffi::MigrationPreparationCut {
        present,
        id: v.id,
        tick: v.tick,
        bytes: v.bytes,
        digest: v.digest,
    }
}
fn decision(v: &ffi::NativePublicationDecision) -> Option<host::Decision> {
    if !v.present {
        return None;
    }
    Some(host::Decision {
        previous: context(&v.previous)?,
        next: context(&v.next)?,
        ballot: host::Ballot {
            round: v.round,
            proposer: v.proposer.try_into().ok()?,
        },
        value: host::Value {
            host: v.host.try_into().ok()?,
            cut: crate::preparation_cut(&v.cut)?,
        },
        voters: v.voters,
    })
}
fn to_decision(v: Option<host::Decision>) -> ffi::NativePublicationDecision {
    ffi::NativePublicationDecision {
        present: v.is_some(),
        previous: to_context(v.map(|d| d.previous)),
        next: to_context(v.map(|d| d.next)),
        round: v.map_or(0, |d| d.ballot.round),
        proposer: v.map_or(16, |d| d.ballot.proposer as u32),
        host: v.map_or(16, |d| d.value.host as u32),
        cut: cut(v.map(|d| d.value.cut)),
        voters: v.map_or(0, |d| d.voters),
    }
}
fn players(v: Vec<ffi::SourceAdmissionsPlayer>) -> Vec<Player> {
    v.into_iter()
        .map(|p| Player {
            slot: p.slot,
            player: p.player,
            identity: p.identity,
        })
        .collect()
}
fn checked<T>(v: core::Result<T>) -> Result<T, String> {
    v.map_err(|e| e.0)
}
fn io<T>(v: Result<T, cxx::Exception>) -> core::Result<T> {
    v.map_err(|e| core::Error(e.to_string()))
}
fn invalid() -> core::Error {
    core::Error("Invalid native publication observation".into())
}
pub fn native_publication_session(
    s: &NativeSession,
    d: &ffi::NativePublicationDecision,
) -> ffi::NativePublicationSession {
    let v = s.0.publication_session(decision(d));
    ffi::NativePublicationSession {
        self_peer: v.self_peer as u32,
        handoff: v.handoff,
        selection: v.selection,
        snapshots: v.snapshots,
        commit_allowed: v.commit_allowed,
    }
}
struct Io<'a>(&'a ffi::NativePublicationIo);
impl core::Io for Io<'_> {
    fn now_ms(&self) -> u64 {
        self.0.publication_now_ms()
    }
    fn session(&self, d: Option<host::Decision>) -> core::Result<core::Session> {
        let v = io(self.0.publication_session(&to_decision(d)))?;
        Ok(core::Session {
            self_peer: v.self_peer.try_into().map_err(|_| invalid())?,
            handoff: v.handoff,
            selection: v.selection,
            snapshots: v.snapshots,
            commit_allowed: v.commit_allowed,
        })
    }
    fn resources(&self) -> core::Result<core::Resources> {
        let v = io(self.0.publication_resources())?;
        Ok(core::Resources {
            agreement_present: v.agreement_present,
            agreement_context: if v.agreement_context_present {
                Some(context(&v.agreement_context).ok_or_else(invalid)?)
            } else {
                None
            },
            authority: if v.authority_present {
                Some(context(&v.authority).ok_or_else(invalid)?)
            } else {
                None
            },
            source_present: v.source_present,
            history: v.history,
        })
    }
    fn selection(&self, c: Option<Cut>) -> core::Result<core::Selection> {
        let v = io(self.0.publication_selection(&cut(c)))?;
        Ok(core::Selection {
            present: v.present,
            started: v.started,
            supported: v.supported,
            complete: v.complete,
            failed: v.failed,
            survivors: v.survivors,
            allows_ready: v.allows_ready,
        })
    }
    fn agreement(&self, now: u64) -> core::Result<Option<host::State>> {
        let v = io(self.0.publication_agreement(now))?;
        if !v.present {
            return Ok(None);
        }
        let s = v.state;
        let d = crate::matchmaking_native_session::decision(&s.decision);
        if s.decision.present && d.is_none() {
            return Err(invalid());
        }
        Ok(Some(host::State {
            suspected: s.suspected,
            fenced: s.fenced,
            electorate: s.electorate,
            silent: s.silent,
            majority: s.majority,
            ready: std::array::from_fn(|i| crate::preparation_cut(&s.ready[i])),
            decision: d,
            rejected: s.rejected,
        }))
    }
    fn common(&self) -> core::Result<core::Common> {
        let v = io(self.0.publication_common())?;
        Ok(core::Common {
            abort: v.abort,
            pending: if v.pending {
                Some((
                    context(&v.context).ok_or_else(invalid)?,
                    crate::preparation_cut(&v.cut).ok_or_else(invalid)?,
                ))
            } else {
                None
            },
        })
    }
    fn admission(&self) -> core::Result<core::Admission> {
        let v = io(self.0.publication_admission())?;
        Ok(core::Admission {
            accepted: if v.present {
                Some((context(&v.context).ok_or_else(invalid)?, players(v.players)))
            } else {
                None
            },
            continuity: v.continuity_present.then(|| players(v.continuity)),
        })
    }
    fn audio(&self) -> core::Result<Option<bool>> {
        let v = io(self.0.publication_audio())?;
        Ok(v.present.then_some(v.ready))
    }
    fn snapshot(&self) -> core::Result<core::Snapshot> {
        let v = io(self.0.publication_snapshot())?;
        Ok(core::Snapshot {
            present: v.present,
            pending: v.pending,
            retired: v.retired,
        })
    }
    fn blocks_publication(&self) -> bool {
        self.0.publication_blocks()
    }
    fn invalidate_selection(&self, c: Cut) -> core::Result<()> {
        io(self.0.publication_invalidate(&cut(Some(c))))
    }
    fn observe_ready(&self, p: u8, c: Cut) -> core::Result<bool> {
        io(self.0.publication_observe_ready(p, &cut(Some(c))))
    }
    fn fence_selection(&self, failed: bool) -> core::Result<()> {
        io(self.0.publication_fence(failed))
    }
    fn publish_ready(&self) -> core::Result<bool> {
        io(self.0.publication_ready())
    }
    fn propose(&self, p: u32, now: u64) -> core::Result<bool> {
        io(self.0.publication_propose(p, now))
    }
    fn candidate(&self, now: u64) -> core::Result<Option<u8>> {
        let p = io(self.0.publication_candidate(now))?;
        Ok(if p < 16 { Some(p as u8) } else { None })
    }
    fn allocate(&self, p: core::Successor) -> core::Result<()> {
        io(self
            .0
            .publication_allocate(&ffi::NativePublicationSuccessor {
                multiple: p.multiple,
                selection: p.selection,
                snapshot: p.snapshot,
                selection_feature: p.selection_feature,
                snapshot_feature: p.snapshot_feature,
                limits: crate::matchmaking_native_access::limits(p.transfer_limits()),
            }))
    }
    fn has_native_commit(&self) -> bool {
        self.0.publication_has_commit()
    }
    fn commit_native(&self) -> core::Result<bool> {
        io(self.0.publication_commit())
    }
    fn publish_successor(&self) -> core::Result<()> {
        io(self.0.publication_publish())
    }
}
pub fn native_publication_abort(v: &ffi::NativePublicationIo) -> Result<(), String> {
    checked(core::selection_abort(&Io(v)))
}
pub fn native_publication_progress(v: &ffi::NativePublicationIo) -> Result<(), String> {
    checked(core::selection_progress(&Io(v)))
}
pub fn native_publication_fence(v: &ffi::NativePublicationIo) -> Result<(), String> {
    checked(core::fence(&Io(v)))
}
pub fn native_publication_prepare(
    v: &ffi::NativePublicationIo,
    c: &ffi::NativePublicationContext,
    k: &ffi::MigrationPreparationCut,
) -> Result<bool, String> {
    let (Some(c), Some(k)) = (context(c), crate::preparation_cut(k)) else {
        return Ok(false);
    };
    checked(core::prepare(&Io(v), c, k))
}
pub fn native_publication_propose(v: &ffi::NativePublicationIo, p: u32) -> Result<bool, String> {
    checked(core::propose(&Io(v), p))
}
pub fn native_publication_propose_automatic(v: &ffi::NativePublicationIo) -> Result<bool, String> {
    checked(core::propose_automatic(&Io(v)))
}
pub fn native_publication_finish(
    v: &ffi::NativePublicationIo,
    d: &ffi::NativePublicationDecision,
) -> Result<bool, String> {
    let Some(d) = decision(d) else {
        return Ok(false);
    };
    checked(core::finish(&Io(v), d))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn full_width_publication_context_and_decision_indices_do_not_alias_zero() {
        let mut c = to_context(Some(host::Context {
            session: [1; 32],
            epoch: 1,
            membership_revision: 2,
            peers: 7,
            host: 0,
        }));
        c.host = 256;
        assert!(context(&c).is_none());
        let d = host::Decision {
            previous: host::Context {
                session: [1; 32],
                epoch: 1,
                membership_revision: 2,
                peers: 7,
                host: 0,
            },
            next: host::Context {
                session: [1; 32],
                epoch: 2,
                membership_revision: 3,
                peers: 6,
                host: 1,
            },
            ballot: host::Ballot {
                round: 1,
                proposer: 1,
            },
            value: host::Value {
                host: 1,
                cut: Cut {
                    id: 1,
                    tick: 4,
                    bytes: 20,
                    digest: [2; 32],
                },
            },
            voters: 6,
        };
        for field in 0..4 {
            let mut raw = to_decision(Some(d));
            match field {
                0 => raw.previous.host = 256,
                1 => raw.next.host = 257,
                2 => raw.host = 257,
                _ => raw.proposer = 257,
            };
            assert!(decision(&raw).is_none());
        }
        assert_eq!(decision(&to_decision(Some(d))), Some(d));
    }
}
