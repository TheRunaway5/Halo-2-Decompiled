//! Typed metadata conversion only; native common-pair owners stay outside Rust.
use crate::ffi;
use halo2_online_protocol::common_cut as protocol;

pub struct CommonCutAgreement {
    inner: protocol::Agreement,
}

fn protect<T>(call: impl FnOnce() -> Result<T, protocol::Error>) -> Result<T, String> {
    // Keep validation errors and unexpected unwinding inside the Rust ABI.
    // Allocation exhaustion remains process-fatal, as for the other bridges.
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(call))
        .map_err(|_| "Rust peer common-cut agreement failed".to_owned())?
        .map_err(|error| error.to_string())
}

fn from_context(context: &ffi::HostAgreementContext) -> protocol::Context {
    protocol::Context {
        session: context.session,
        epoch: context.epoch,
        membership_revision: context.membership_revision,
        peers: context.peers,
        host: context.host,
    }
}

fn to_context(context: protocol::Context) -> ffi::HostAgreementContext {
    ffi::HostAgreementContext {
        session: context.session,
        epoch: context.epoch,
        membership_revision: context.membership_revision,
        peers: context.peers,
        host: context.host,
    }
}

fn from_cut(cut: &ffi::CommonCutValue) -> Option<protocol::CommonCut> {
    if !cut.present {
        return None;
    }
    Some(protocol::CommonCut {
        context: from_context(&cut.context),
        seal: crate::preparation_cut(&cut.seal)?,
        recipients: cut.recipients,
    })
}

fn to_cut(cut: Option<protocol::CommonCut>) -> ffi::CommonCutValue {
    let absent = cut.is_none();
    let cut = cut.unwrap_or(protocol::CommonCut {
        context: protocol::Context {
            session: [0; 32],
            epoch: 0,
            membership_revision: 0,
            peers: 0,
            host: 16,
        },
        seal: protocol::Cut {
            id: 0,
            tick: 0,
            bytes: 0,
            digest: [0; 32],
        },
        recipients: 0,
    });
    ffi::CommonCutValue {
        present: !absent,
        context: to_context(cut.context),
        seal: ffi::MigrationPreparationCut {
            present: !absent,
            id: cut.seal.id,
            tick: cut.seal.tick,
            bytes: cut.seal.bytes,
            digest: cut.seal.digest,
        },
        recipients: cut.recipients,
    }
}

pub(crate) fn snapshot(value: &ffi::CommonCutState) -> protocol::State {
    protocol::State {
        context: from_context(&value.context),
        recipients: value.recipients,
        committed: from_cut(&value.committed),
        pending: from_cut(&value.pending),
        associated: value.associated,
        applied: value.applied,
        released: value.released,
        aborted: value.aborted,
        native_all_validated: value.native_all_validated,
        commit: value.commit,
        release: value.release,
        abort: value.abort,
        rejected: value.rejected,
    }
}

pub fn new_common_cut_agreement(
    self_peer: u8,
    context: &ffi::HostAgreementContext,
    wire_protocol: u16,
) -> Result<Box<CommonCutAgreement>, String> {
    protect(|| {
        protocol::Agreement::new(self_peer, from_context(context), wire_protocol)
            .map(|inner| Box::new(CommonCutAgreement { inner }))
    })
}

pub fn common_cut_context(
    agreement: &CommonCutAgreement,
) -> Result<ffi::HostAgreementContext, String> {
    protect(|| Ok(to_context(agreement.inner.context())))
}

pub fn common_cut_offer(
    agreement: &mut CommonCutAgreement,
    cut: &ffi::CommonCutValue,
) -> Result<bool, String> {
    protect(|| Ok(from_cut(cut).is_some_and(|cut| agreement.inner.offer(cut))))
}

pub fn common_cut_confirm_native(
    agreement: &mut CommonCutAgreement,
    cut: &ffi::CommonCutValue,
) -> Result<bool, String> {
    protect(|| Ok(from_cut(cut).is_some_and(|cut| agreement.inner.confirm_native(cut))))
}

pub fn common_cut_associate(
    agreement: &mut CommonCutAgreement,
    cut: &ffi::CommonCutValue,
) -> Result<bool, String> {
    protect(|| Ok(from_cut(cut).is_some_and(|cut| agreement.inner.associate(cut))))
}

pub fn common_cut_apply(
    agreement: &mut CommonCutAgreement,
    cut: &ffi::CommonCutValue,
) -> Result<bool, String> {
    protect(|| Ok(from_cut(cut).is_some_and(|cut| agreement.inner.apply(cut))))
}

pub fn common_cut_release(
    agreement: &mut CommonCutAgreement,
    cut: &ffi::CommonCutValue,
) -> Result<bool, String> {
    protect(|| Ok(from_cut(cut).is_some_and(|cut| agreement.inner.release(cut))))
}

pub fn common_cut_reject(
    agreement: &mut CommonCutAgreement,
    cut: &ffi::CommonCutValue,
) -> Result<bool, String> {
    protect(|| Ok(from_cut(cut).is_some_and(|cut| agreement.inner.reject(cut))))
}

pub fn common_cut_abort(
    agreement: &mut CommonCutAgreement,
    cut: &ffi::CommonCutValue,
) -> Result<bool, String> {
    protect(|| Ok(from_cut(cut).is_some_and(|cut| agreement.inner.abort(cut))))
}

pub fn common_cut_acknowledge_abort(
    agreement: &mut CommonCutAgreement,
    cut: &ffi::CommonCutValue,
) -> Result<bool, String> {
    protect(|| Ok(from_cut(cut).is_some_and(|cut| agreement.inner.acknowledge_abort(cut))))
}

pub fn common_cut_poll(
    agreement: &mut CommonCutAgreement,
    now_ms: u64,
    packet_budget: u32,
) -> Result<Vec<ffi::CommonCutPacket>, String> {
    protect(|| {
        Ok(agreement
            .inner
            .poll(now_ms, packet_budget)
            .into_iter()
            .map(|packet| ffi::CommonCutPacket {
                peer: packet.peer,
                bytes: packet.bytes,
            })
            .collect())
    })
}

pub fn common_cut_receive(
    agreement: &mut CommonCutAgreement,
    authenticated_peer: u8,
    packet: &[u8],
    now_ms: u64,
) -> Result<bool, String> {
    protect(|| Ok(agreement.inner.receive(authenticated_peer, packet, now_ms)))
}

pub fn common_cut_state(agreement: &CommonCutAgreement) -> Result<ffi::CommonCutState, String> {
    protect(|| {
        let state = agreement.inner.state();
        Ok(ffi::CommonCutState {
            context: to_context(state.context),
            recipients: state.recipients,
            committed: to_cut(state.committed),
            pending: to_cut(state.pending),
            associated: state.associated,
            applied: state.applied,
            released: state.released,
            aborted: state.aborted,
            native_all_validated: state.native_all_validated,
            commit: state.commit,
            release: state.release,
            abort: state.abort,
            rejected: state.rejected,
        })
    })
}

pub fn common_cut_is_packet(packet: &[u8]) -> bool {
    std::panic::catch_unwind(|| protocol::is_packet(packet)).unwrap_or(false)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn context() -> ffi::HostAgreementContext {
        ffi::HostAgreementContext {
            session: std::array::from_fn(|i| 0x80 + i as u8),
            // Common-cut metadata does not inherit election's increment limits.
            epoch: u32::MAX,
            membership_revision: u64::MAX,
            peers: 0x8001,
            host: 15,
        }
    }

    fn cut() -> ffi::CommonCutValue {
        ffi::CommonCutValue {
            present: true,
            context: context(),
            seal: ffi::MigrationPreparationCut {
                present: true,
                id: u64::MAX,
                tick: u64::MAX,
                bytes: 64 * 1024 * 1024,
                digest: std::array::from_fn(|i| i as u8 + 1),
            },
            recipients: 1,
        }
    }

    fn deliver(
        sender: &mut CommonCutAgreement,
        receiver: &mut CommonCutAgreement,
        from: u8,
        to: u8,
        now: u64,
    ) {
        for packet in common_cut_poll(sender, now, 128).unwrap() {
            assert_eq!(packet.peer, to);
            assert_eq!(packet.bytes.len(), 116);
            assert!(common_cut_is_packet(&packet.bytes));
            assert!(common_cut_receive(receiver, from, &packet.bytes, now).unwrap());
        }
    }

    #[test]
    fn complete_metadata_lifecycle_retains_exact_context_seal_and_explicit_phase_bits() {
        let context = context();
        let cut = cut();
        let mut host = new_common_cut_agreement(15, &context, 97).unwrap();
        let mut recipient = new_common_cut_agreement(0, &context, 97).unwrap();
        let initial = common_cut_state(&host).unwrap();
        assert!(!initial.pending.present && !initial.committed.present);
        assert_eq!(common_cut_context(&host).unwrap().session, context.session);
        assert!(common_cut_offer(&mut host, &cut).unwrap());
        deliver(&mut host, &mut recipient, 15, 0, 100);
        assert!(!common_cut_apply(&mut recipient, &cut).unwrap());
        assert!(common_cut_associate(&mut recipient, &cut).unwrap());
        deliver(&mut recipient, &mut host, 0, 15, 200);
        let associated = common_cut_state(&host).unwrap();
        assert_eq!(associated.associated, 1);
        assert!(!associated.native_all_validated && !associated.commit);
        assert!(common_cut_confirm_native(&mut host, &cut).unwrap());
        deliver(&mut host, &mut recipient, 15, 0, 300);
        assert!(common_cut_apply(&mut recipient, &cut).unwrap());
        assert!(!common_cut_release(&mut recipient, &cut).unwrap());
        deliver(&mut recipient, &mut host, 0, 15, 400);
        deliver(&mut host, &mut recipient, 15, 0, 500);
        assert!(common_cut_release(&mut recipient, &cut).unwrap());
        deliver(&mut recipient, &mut host, 0, 15, 600);
        for endpoint in [&host, &recipient] {
            let state = common_cut_state(endpoint).unwrap();
            assert_eq!(state.context.session, context.session);
            assert_eq!(state.context.epoch, u32::MAX);
            assert_eq!(state.context.membership_revision, u64::MAX);
            assert_eq!(state.context.peers, 0x8001);
            assert_eq!(state.context.host, 15);
            assert_eq!(state.recipients, 1);
            assert!(state.pending.present && state.committed.present);
            assert_eq!(state.associated, 1);
            assert_eq!(state.applied, 1);
            assert_eq!(state.released, 1);
            assert_eq!(state.aborted, 0);
            assert!(state.native_all_validated && state.commit && state.release && !state.abort);
            assert_eq!(state.rejected, 0);
            let committed = state.committed;
            assert_eq!(committed.context.session, context.session);
            assert_eq!(committed.context.epoch, context.epoch);
            assert_eq!(committed.context.membership_revision, context.membership_revision);
            assert_eq!(committed.context.peers, context.peers);
            assert_eq!(committed.context.host, context.host);
            assert_eq!(committed.recipients, 1);
            assert!(committed.seal.present);
            assert_eq!(committed.seal.id, cut.seal.id);
            assert_eq!(committed.seal.tick, cut.seal.tick);
            assert_eq!(committed.seal.bytes, cut.seal.bytes);
            assert_eq!(committed.seal.digest, cut.seal.digest);
        }
    }

    #[test]
    fn absent_or_invalid_metadata_refuses_without_replacing_pending_cut() {
        let mut host = new_common_cut_agreement(15, &context(), 97).unwrap();
        let valid = cut();
        assert!(common_cut_offer(&mut host, &valid).unwrap());
        let mut invalid = cut();
        invalid.seal.bytes = 0;
        type Action = fn(&mut CommonCutAgreement, &ffi::CommonCutValue) -> Result<bool, String>;
        let actions: [Action; 8] = [
            common_cut_offer,
            common_cut_confirm_native,
            common_cut_associate,
            common_cut_apply,
            common_cut_release,
            common_cut_reject,
            common_cut_abort,
            common_cut_acknowledge_abort,
        ];
        for action in actions {
            assert!(!action(&mut host, &invalid).unwrap());
            assert!(!action(&mut host, &to_cut(None)).unwrap());
            let state = common_cut_state(&host).unwrap();
            assert!(state.pending.present && !state.committed.present);
            assert_eq!(state.pending.seal.id, valid.seal.id);
            assert_eq!(state.associated, 0);
            assert!(!state.commit && !state.release && !state.abort);
        }
        assert!(new_common_cut_agreement(16, &context(), 97).is_err());
        let mut invalid_context = context();
        invalid_context.host = 16;
        assert!(new_common_cut_agreement(0, &invalid_context, 97).is_err());
    }

    #[test]
    fn recognized_malformed_wire_increments_rejections_without_phase_or_owner_claims() {
        let mut recipient = new_common_cut_agreement(0, &context(), 97).unwrap();
        let invalid = [0x48, 0x32, 0x4e, 0x32, 0xff, 0xff, 11];
        assert!(common_cut_is_packet(&invalid));
        assert!(common_cut_receive(&mut recipient, 16, &invalid, u64::MAX).unwrap());
        assert!(!common_cut_receive(&mut recipient, 15, &[], 0).unwrap());
        let state = common_cut_state(&recipient).unwrap();
        assert_eq!(state.rejected, 1);
        assert!(!state.pending.present && !state.committed.present);
        assert!(!state.native_all_validated && !state.commit && !state.release && !state.abort);
        assert_eq!((state.associated, state.applied, state.released, state.aborted), (0, 0, 0, 0));
        assert!(common_cut_poll(&mut recipient, u64::MAX, 128).unwrap().is_empty());
    }
}
