//! Ownership and typed value conversion only; the protocol crate owns consensus.
use crate::ffi;
use halo2_online_protocol::host_agreement as protocol;

pub struct HostAgreement {
    inner: protocol::Agreement,
}

// Expected validation errors become C++ exceptions through CXX. An unexpected
// Rust unwind must also stop at this boundary, never cross the generated shim.
// Allocation exhaustion remains process-fatal, as for the existing bridge.
fn protect<T>(call: impl FnOnce() -> Result<T, protocol::Error>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(call))
        .map_err(|_| "Rust peer host agreement failed".to_owned())?
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

fn to_cut(cut: Option<protocol::Cut>) -> ffi::MigrationPreparationCut {
    match cut {
        Some(cut) => ffi::MigrationPreparationCut {
            present: true,
            id: cut.id,
            tick: cut.tick,
            bytes: cut.bytes,
            digest: cut.digest,
        },
        None => ffi::MigrationPreparationCut {
            present: false,
            id: 0,
            tick: 0,
            bytes: 0,
            digest: [0; 32],
        },
    }
}

fn to_decision(decision: Option<protocol::Decision>) -> ffi::HostAgreementDecision {
    match decision {
        Some(decision) => ffi::HostAgreementDecision {
            present: true,
            previous: to_context(decision.previous),
            next: to_context(decision.next),
            ballot: ffi::HostAgreementBallot {
                round: decision.ballot.round,
                proposer: decision.ballot.proposer,
            },
            value: ffi::HostAgreementValue {
                host: decision.value.host,
                cut: to_cut(Some(decision.value.cut)),
            },
            voters: decision.voters,
        },
        None => {
            let empty = protocol::Context {
                session: [0; 32],
                epoch: 0,
                membership_revision: 0,
                peers: 0,
                host: 16,
            };
            ffi::HostAgreementDecision {
                present: false,
                previous: to_context(empty),
                next: to_context(empty),
                ballot: ffi::HostAgreementBallot {
                    round: 0,
                    proposer: 16,
                },
                value: ffi::HostAgreementValue {
                    host: 16,
                    cut: to_cut(None),
                },
                voters: 0,
            }
        }
    }
}

pub fn new_host_agreement(
    self_peer: u8,
    context: &ffi::HostAgreementContext,
    wire_protocol: u16,
    last_host_ms: u64,
) -> Result<Box<HostAgreement>, String> {
    protect(|| {
        protocol::Agreement::new(self_peer, from_context(context), wire_protocol, last_host_ms)
            .map(|inner| Box::new(HostAgreement { inner }))
    })
}

pub fn host_agreement_observe_host(
    agreement: &mut HostAgreement,
    now_ms: u64,
) -> Result<(), String> {
    protect(|| {
        agreement.inner.observe_host(now_ms);
        Ok(())
    })
}

pub fn host_agreement_publish_ready(
    agreement: &mut HostAgreement,
    peer: u8,
    context: &ffi::HostAgreementContext,
    cut: &ffi::MigrationPreparationCut,
) -> Result<bool, String> {
    protect(|| {
        // The native adapter always supplies a present cut. Keep absence a
        // refused bridge argument rather than fabricating an empty seal.
        let cut = crate::preparation_cut(cut).ok_or(protocol::Error)?;
        agreement.inner.publish_ready(peer, from_context(context), cut)
    })
}

pub fn host_agreement_propose(
    agreement: &mut HostAgreement,
    candidate: u8,
    now_ms: u64,
) -> Result<bool, String> {
    protect(|| agreement.inner.propose(candidate, now_ms))
}

pub fn host_agreement_drive(
    agreement: &mut HostAgreement,
    candidate: u8,
    now_ms: u64,
) -> Result<bool, String> {
    protect(|| agreement.inner.drive(candidate, now_ms))
}

pub fn host_agreement_poll(
    agreement: &mut HostAgreement,
    now_ms: u64,
    packet_budget: u32,
) -> Result<Vec<ffi::HostAgreementPacket>, String> {
    protect(|| {
        Ok(agreement
            .inner
            .poll(now_ms, packet_budget)
            .into_iter()
            .map(|packet| ffi::HostAgreementPacket {
                peer: packet.peer,
                bytes: packet.bytes,
            })
            .collect())
    })
}

pub fn host_agreement_receive(
    agreement: &mut HostAgreement,
    authenticated_peer: u8,
    packet: &[u8],
    now_ms: u64,
) -> Result<bool, String> {
    protect(|| Ok(agreement.inner.receive(authenticated_peer, packet, now_ms)))
}

pub fn host_agreement_state(
    agreement: &HostAgreement,
    now_ms: u64,
) -> Result<ffi::HostAgreementState, String> {
    protect(|| {
        let state = agreement.inner.state(now_ms);
        Ok(ffi::HostAgreementState {
            suspected: state.suspected,
            fenced: state.fenced,
            electorate: state.electorate,
            silent: state.silent,
            majority: state.majority,
            ready: state.ready.map(to_cut),
            decision: to_decision(state.decision),
            rejected: state.rejected,
        })
    })
}

pub fn host_agreement_is_packet(packet: &[u8]) -> bool {
    // This pure predicate performs bounded reads only. Unexpected unwinding
    // cannot escape its infallible native classification interface.
    std::panic::catch_unwind(|| protocol::is_packet(packet)).unwrap_or(false)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn context() -> ffi::HostAgreementContext {
        ffi::HostAgreementContext {
            session: std::array::from_fn(|i| i as u8 + 1),
            epoch: 0x7f00ff01,
            membership_revision: 0x123456789abcdef0,
            peers: 0x8001,
            host: 0,
        }
    }

    fn cut() -> ffi::MigrationPreparationCut {
        ffi::MigrationPreparationCut {
            present: true,
            id: 0xfedcba9876543210,
            tick: 0x1122334455667788,
            bytes: 8193,
            digest: std::array::from_fn(|i| 0x80 + i as u8),
        }
    }

    #[test]
    fn native_state_preserves_full_context_exact_cut_and_physical_slot_fifteen() {
        let context = context();
        let cut = cut();
        let mut agreement = new_host_agreement(15, &context, 97, 0).unwrap();
        let empty = host_agreement_state(&agreement, 0).unwrap();
        assert!(empty.ready.iter().all(|value| !value.present));
        assert!(!empty.decision.present);
        assert!(host_agreement_publish_ready(&mut agreement, 15, &context, &cut).unwrap());
        assert!(host_agreement_propose(&mut agreement, 15, 501).unwrap());
        let state = host_agreement_state(&agreement, 501).unwrap();
        assert!(state.suspected && state.fenced && state.decision.present);
        assert_eq!(state.electorate, 0x8000);
        assert_eq!(state.majority, 1);
        for (peer, ready) in state.ready.iter().enumerate() {
            assert_eq!(ready.present, peer == 15);
        }
        let decision = state.decision;
        assert_eq!(decision.previous.session, context.session);
        assert_eq!(decision.previous.epoch, context.epoch);
        assert_eq!(decision.previous.membership_revision, context.membership_revision);
        assert_eq!(decision.previous.peers, context.peers);
        assert_eq!(decision.previous.host, context.host);
        assert_eq!(decision.next.session, context.session);
        assert_eq!(decision.next.epoch, context.epoch + 1);
        assert_eq!(decision.next.membership_revision, context.membership_revision + 1);
        assert_eq!(decision.next.peers, 0x8000);
        assert_eq!(decision.next.host, 15);
        assert_eq!(decision.ballot.round, 1);
        assert_eq!(decision.ballot.proposer, 15);
        assert_eq!(decision.value.host, 15);
        assert!(decision.value.cut.present);
        assert_eq!(decision.value.cut.id, cut.id);
        assert_eq!(decision.value.cut.tick, cut.tick);
        assert_eq!(decision.value.cut.bytes, cut.bytes);
        assert_eq!(decision.value.cut.digest, cut.digest);
        assert_eq!(decision.voters, 0x8000);
    }

    #[test]
    fn ineligible_native_calls_refuse_while_invalid_seals_keep_error_behavior() {
        let context = context();
        let mut agreement = new_host_agreement(15, &context, 97, 1000).unwrap();
        assert!(!host_agreement_publish_ready(&mut agreement, 16, &context, &cut()).unwrap());
        let mut wrong = to_context(from_context(&context));
        wrong.session[31] ^= 1;
        assert!(!host_agreement_publish_ready(&mut agreement, 15, &wrong, &cut()).unwrap());
        let mut invalid = cut();
        invalid.id = 0;
        assert!(host_agreement_publish_ready(&mut agreement, 15, &context, &invalid).is_err());
        assert!(!host_agreement_propose(&mut agreement, 16, 900).unwrap());
        assert!(!host_agreement_drive(&mut agreement, 16, 900).unwrap());
        let state = host_agreement_state(&agreement, 900).unwrap();
        assert!(!state.suspected && !state.fenced && !state.decision.present);
        assert!(state.ready.iter().all(|value| !value.present));
        assert!(new_host_agreement(16, &context, 97, 0).is_err());
    }

    #[test]
    fn malformed_recognized_wire_keeps_rejection_count_and_returns_owned_packets() {
        let context = context();
        let mut agreement = new_host_agreement(15, &context, 97, 0).unwrap();
        let packet = [0x48, 0x32, 0x4e, 0x32, 0xff, 0xff, 10];
        assert!(host_agreement_is_packet(&packet));
        assert!(host_agreement_receive(&mut agreement, 16, &packet, 600).unwrap());
        assert_eq!(host_agreement_state(&agreement, 600).unwrap().rejected, 1);
        assert!(!host_agreement_receive(&mut agreement, 0, &[], 900).unwrap());
        assert_eq!(host_agreement_state(&agreement, 600).unwrap().rejected, 1);
        assert!(host_agreement_publish_ready(&mut agreement, 15, &context, &cut()).unwrap());
        let packets = host_agreement_poll(&mut agreement, 600, 128).unwrap();
        assert!(!packets.is_empty());
        for packet in packets {
            assert_eq!(packet.peer, 0);
            assert!(host_agreement_is_packet(&packet.bytes));
            assert_eq!(&packet.bytes[4..6], &97u16.to_le_bytes());
        }
    }
}
