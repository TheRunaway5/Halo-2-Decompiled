//! Typed native exchange IO. Rust owns the complete public caller sequence.
use crate::{ffi, matchmaking_native_session::NativeSession};
use halo2_online_protocol::{
    matchmaking_native_exchange as core,
    peer_source_admissions::{Context, Player},
};
fn context(v: &ffi::NativePublicationContext) -> Context {
    Context {
        session: v.session,
        epoch: v.epoch,
        membership_revision: v.membership_revision,
        peers: v.peers,
        host: v.host,
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
fn action(v: ffi::NativeExchangeCommonAction) -> Option<core::CommonAction> {
    use core::CommonAction as C;
    use ffi::NativeExchangeCommonAction as F;
    Some(match v {
        F::Offer => C::Offer,
        F::ConfirmNative => C::ConfirmNative,
        F::Associate => C::Associate,
        F::Apply => C::Apply,
        F::Release => C::Release,
        F::Reject => C::Reject,
        F::Abort => C::Abort,
        F::AcknowledgeAbort => C::AcknowledgeAbort,
        _ => return None,
    })
}
fn to_action(v: core::CommonAction) -> ffi::NativeExchangeCommonAction {
    use core::CommonAction as C;
    use ffi::NativeExchangeCommonAction as F;
    match v {
        C::Offer => F::Offer,
        C::ConfirmNative => F::ConfirmNative,
        C::Associate => F::Associate,
        C::Apply => F::Apply,
        C::Release => F::Release,
        C::Reject => F::Reject,
        C::Abort => F::Abort,
        C::AcknowledgeAbort => F::AcknowledgeAbort,
    }
}
fn capability(v: ffi::NativeExchangeCapability) -> Option<core::Capability> {
    use core::Capability as C;
    use ffi::NativeExchangeCapability as F;
    Some(match v {
        F::Authority => C::Authority,
        F::Replay => C::Replay,
        F::Pipeline => C::Pipeline,
        F::HostContinuation => C::HostContinuation,
        F::AdmittedCamera => C::AdmittedCamera,
        F::SourceConfirmed => C::SourceConfirmed,
        _ => return None,
    })
}
pub fn native_exchange_session(s: &NativeSession) -> ffi::NativeExchangeSession {
    let v = s.0.exchange_session();
    ffi::NativeExchangeSession {
        self_peer: v.self_peer,
        host: v.host,
        direct: v.direct,
        running: v.running,
        common_phase: v.common_phase,
        candidate_phase: v.candidate_phase,
        terminal_recovery: v.terminal_recovery,
    }
}
struct Io<'a>(&'a ffi::NativeExchangeIo);
impl core::Io for Io<'_> {
    fn session(&self) -> core::Session {
        let v = self.0.exchange_session();
        core::Session {
            self_peer: v.self_peer,
            host: v.host,
            direct: v.direct,
            running: v.running,
            common_phase: v.common_phase,
            candidate_phase: v.candidate_phase,
            terminal_recovery: v.terminal_recovery,
        }
    }
    fn now_ms(&self) -> u64 {
        self.0.exchange_now_ms()
    }
    fn direct_ready(&self) -> u16 {
        self.0.exchange_direct_ready()
    }
    fn source_present(&self) -> bool {
        self.0.exchange_source_present()
    }
    fn source_continuity(&self) -> core::Result<Option<Vec<Player>>> {
        let v = io(self.0.exchange_continuity())?;
        Ok(v.continuity_present.then(|| players(v.continuity)))
    }
    fn source_publish(&self) -> core::Result<bool> {
        io(self.0.exchange_publish_source())
    }
    fn admission(&self) -> core::Result<core::Admission> {
        let v = io(self.0.exchange_admission())?;
        Ok(core::Admission {
            accepted: v.present.then(|| players(v.players)),
            continuity: v.continuity_present.then(|| players(v.continuity)),
        })
    }
    fn source_confirmed(&self, p: u32) -> core::Result<bool> {
        io(self.0.exchange_source_confirmed(p))
    }
    fn authority_present(&self) -> bool {
        self.0.exchange_authority_present()
    }
    fn authority_publish(
        &self,
        ahead: bool,
        now: u64,
    ) -> core::Result<halo2_online_protocol::migration_preparation::PreparationCut> {
        let v = io(self.0.exchange_publish_authority(ahead, now))?;
        crate::preparation_cut(&v)
            .ok_or_else(|| core::Error("Missing native authority publication".into()))
    }
    fn authority_supported(&self, p: u32, f: u32) -> core::Result<bool> {
        io(self.0.exchange_authority_supported(p, f))
    }
    fn common(&self) -> core::Result<core::Common> {
        let v = io(self.0.exchange_common())?;
        Ok(core::Common {
            supported: v.supported,
            context: v.present.then(|| context(&v.context)),
        })
    }
    fn common_perform(&self, a: core::CommonAction) -> core::Result<bool> {
        io(self.0.exchange_common_perform(to_action(a)))
    }
    fn common_view(&self) -> core::Result<()> {
        io(self.0.exchange_common_view())
    }
    fn selection_abort(&self) -> core::Result<()> {
        io(self.0.exchange_selection_abort())
    }
    fn selection_progress(&self) -> core::Result<()> {
        io(self.0.exchange_selection_progress())
    }
    fn quality_context(&self) -> core::Result<Option<Context>> {
        let v = io(self.0.exchange_quality())?;
        Ok(v.present.then(|| context(&v.context)))
    }
    fn quality_candidates(&self, n: u64) -> core::Result<()> {
        io(self.0.exchange_candidates(n))
    }
}
pub fn native_exchange_source_publish(
    v: &ffi::NativeExchangeIo,
    rows: &[ffi::SourceAdmissionsPlayer],
) -> Result<bool, String> {
    let rows = rows
        .iter()
        .map(|p| Player {
            slot: p.slot,
            player: p.player,
            identity: p.identity,
        })
        .collect::<Vec<_>>();
    checked(core::publish_source(&Io(v), &rows))
}
pub fn native_exchange_source_visible(v: &ffi::NativeExchangeIo) -> Result<bool, String> {
    checked(core::source_visible(&Io(v)))
}
pub fn native_exchange_authority_publish(
    v: &ffi::NativeExchangeIo,
    ahead: bool,
) -> Result<ffi::MigrationPreparationCut, String> {
    checked(core::publish_authority(&Io(v), ahead))
        .map(|c| crate::matchmaking_native_publication::cut(Some(c)))
}
pub fn native_exchange_capability(
    v: &ffi::NativeExchangeIo,
    kind: ffi::NativeExchangeCapability,
    peer: u32,
) -> Result<bool, String> {
    let Some(kind) = capability(kind) else {
        return Ok(false);
    };
    checked(core::capability(&Io(v), kind, peer))
}
pub fn native_exchange_common(
    v: &ffi::NativeExchangeIo,
    c: &ffi::NativePublicationContext,
    a: ffi::NativeExchangeCommonAction,
) -> Result<bool, String> {
    let Some(a) = action(a) else { return Ok(false) };
    checked(core::common_action(&Io(v), context(c), a))
}
pub fn native_exchange_common_view(v: &ffi::NativeExchangeIo) -> Result<(), String> {
    checked(core::common_view(&Io(v)))
}
pub fn native_exchange_candidates(v: &ffi::NativeExchangeIo) -> Result<(), String> {
    checked(core::host_candidates(&Io(v)))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn raw_context_and_enum_tags_preserve_full_width_and_refuse_unknown_actions() {
        let c = ffi::NativePublicationContext {
            session: [2; 32],
            epoch: 4,
            membership_revision: 7,
            peers: 7,
            host: 257,
        };
        assert_eq!(context(&c).host, 257);
        for v in [
            ffi::NativeExchangeCommonAction::Offer,
            ffi::NativeExchangeCommonAction::ConfirmNative,
            ffi::NativeExchangeCommonAction::Associate,
            ffi::NativeExchangeCommonAction::Apply,
            ffi::NativeExchangeCommonAction::Release,
            ffi::NativeExchangeCommonAction::Reject,
            ffi::NativeExchangeCommonAction::Abort,
            ffi::NativeExchangeCommonAction::AcknowledgeAbort,
        ] {
            assert_eq!(to_action(action(v).unwrap()).repr, v.repr);
        }
        assert!(action(ffi::NativeExchangeCommonAction { repr: 255 }).is_none());
        assert!(capability(ffi::NativeExchangeCapability { repr: 255 }).is_none());
    }
}
