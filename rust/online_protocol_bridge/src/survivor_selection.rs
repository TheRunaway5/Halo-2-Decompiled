//! Typed metadata conversion; native inventory leases remain caller-owned.
use crate::ffi;
use halo2_online_protocol::survivor_selection as protocol;

pub struct SurvivorSelection {
    inner: protocol::Agreement,
}

fn protect<T>(call: impl FnOnce() -> Result<T, protocol::Error>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(call))
        .map_err(|_| "Rust survivor selection failed".to_owned())?
        .map_err(|error| error.to_string())
}

fn context(value: &ffi::HostAgreementContext) -> protocol::Context {
    protocol::Context {
        session: value.session,
        epoch: value.epoch,
        membership_revision: value.membership_revision,
        peers: value.peers,
        host: value.host,
    }
}

fn cut(value: Option<protocol::Cut>) -> ffi::MigrationPreparationCut {
    let present = value.is_some();
    let value = value.unwrap_or(protocol::Cut {
        id: 0,
        tick: 0,
        bytes: 0,
        digest: [0; 32],
    });
    ffi::MigrationPreparationCut {
        present,
        id: value.id,
        tick: value.tick,
        bytes: value.bytes,
        digest: value.digest,
    }
}

fn inventory(value: &ffi::SurvivorInventory) -> protocol::Inventory {
    protocol::Inventory {
        lease: value.lease,
        cuts: std::array::from_fn(|index| {
            crate::preparation_cut(&value.cuts[index].cut).map(|cut| protocol::InventoryEntry {
                cut,
                irreversible_common: value.cuts[index].irreversible_common,
            })
        }),
    }
}

fn admission(value: protocol::InventoryAdmission) -> ffi::SurvivorInventoryAdmission {
    match value {
        protocol::InventoryAdmission::Admit => ffi::SurvivorInventoryAdmission::Admit,
        protocol::InventoryAdmission::Refuse => ffi::SurvivorInventoryAdmission::Refuse,
        protocol::InventoryAdmission::Invalidate => ffi::SurvivorInventoryAdmission::Invalidate,
    }
}

pub fn survivor_inventory_admission(
    binding: &ffi::HostAgreementContext,
    value: &ffi::SurvivorInventory,
    common_present: bool,
    common: &ffi::CommonCutState,
) -> Result<ffi::SurvivorInventoryAdmission, String> {
    protect(|| {
        let common = common_present.then(|| crate::common_cut::snapshot(common));
        Ok(admission(protocol::inventory_admission(
            context(binding),
            &inventory(value),
            common.as_ref(),
        )))
    })
}

pub fn survivor_retained_inventory(
    binding: &ffi::HostAgreementContext,
    lease: u64,
    retained: &[ffi::MigrationPreparationCut],
    common_present: bool,
    common: &ffi::CommonCutState,
) -> Result<ffi::SurvivorInventoryPreparation, String> {
    protect(|| {
        if retained.len() != protocol::MAX_INVENTORY_CUTS {
            return Err(protocol::Error);
        }
        let common = common_present.then(|| crate::common_cut::snapshot(common));
        let value = protocol::retained_inventory(
            context(binding),
            lease,
            std::array::from_fn(|index| crate::preparation_cut(&retained[index])),
            common.as_ref(),
        );
        Ok(ffi::SurvivorInventoryPreparation {
            inventory: ffi::SurvivorInventory {
                lease: value.inventory.lease,
                cuts: value
                    .inventory
                    .cuts
                    .map(|value| ffi::SurvivorInventoryEntry {
                        cut: cut(value.map(|entry| entry.cut)),
                        irreversible_common: value.is_some_and(|entry| entry.irreversible_common),
                    }),
            },
            admission: admission(value.admission),
        })
    })
}
pub fn new_survivor_selection(
    self_peer: u8,
    binding: &ffi::HostAgreementContext,
    wire_protocol: u16,
) -> Result<Box<SurvivorSelection>, String> {
    protect(|| {
        protocol::Agreement::new(self_peer, context(binding), wire_protocol)
            .map(|inner| Box::new(SurvivorSelection { inner }))
    })
}

pub fn survivor_selection_publish_inventory(
    owner: &mut SurvivorSelection,
    inventory: &ffi::SurvivorInventory,
) -> Result<bool, String> {
    protect(|| {
        owner.inner.publish_inventory(self::inventory(inventory))
    })
}

pub fn survivor_selection_invalidate(
    owner: &mut SurvivorSelection,
    value: &ffi::MigrationPreparationCut,
) -> Result<bool, String> {
    protect(|| Ok(owner.inner.invalidate(crate::preparation_cut(value))))
}

pub fn survivor_selection_allows_ready(
    owner: &SurvivorSelection,
    peer: u8,
    value: &ffi::MigrationPreparationCut,
) -> Result<bool, String> {
    protect(|| {
        Ok(crate::preparation_cut(value).is_some_and(|cut| owner.inner.allows_ready(peer, cut)))
    })
}

pub fn survivor_selection_observe_ready(
    owner: &mut SurvivorSelection,
    peer: u8,
    value: &ffi::MigrationPreparationCut,
) -> Result<bool, String> {
    protect(|| {
        Ok(crate::preparation_cut(value).is_some_and(|cut| owner.inner.observe_ready(peer, cut)))
    })
}

pub fn survivor_selection_poll(
    owner: &mut SurvivorSelection,
    now: u64,
    budget: u32,
) -> Result<Vec<ffi::CommonCutPacket>, String> {
    protect(|| {
        Ok(owner
            .inner
            .poll(now, budget)
            .into_iter()
            .map(|packet| ffi::CommonCutPacket {
                peer: packet.peer,
                bytes: packet.bytes,
            })
            .collect())
    })
}

pub fn survivor_selection_receive(
    owner: &mut SurvivorSelection,
    peer: u8,
    packet: &[u8],
    now: u64,
) -> Result<bool, String> {
    protect(|| Ok(owner.inner.receive(peer, packet, now)))
}

pub fn survivor_selection_state(
    owner: &SurvivorSelection,
) -> Result<ffi::SurvivorSelectionState, String> {
    protect(|| {
        let value = owner.inner.state();
        Ok(ffi::SurvivorSelectionState {
            context: ffi::HostAgreementContext {
                session: value.context.session,
                epoch: value.context.epoch,
                membership_revision: value.context.membership_revision,
                peers: value.context.peers,
                host: value.context.host,
            },
            survivors: value.survivors,
            inventory_peers: value.inventory_peers,
            accepted: value.accepted,
            selected: cut(value.selected),
            transcript: value.transcript,
            selection_complete: value.selection_complete,
            failed: value.failed,
            ready_observed: value.ready_observed,
            rejected: value.rejected,
        })
    })
}

pub fn survivor_selection_is_packet(packet: &[u8]) -> bool {
    std::panic::catch_unwind(|| protocol::is_packet(packet)).unwrap_or(false)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn binding() -> ffi::HostAgreementContext {
        ffi::HostAgreementContext {
            session: std::array::from_fn(|i| 0x80 + i as u8),
            epoch: u32::MAX,
            membership_revision: u64::MAX,
            peers: 0x8003,
            host: 15,
        }
    }
    fn seal() -> ffi::MigrationPreparationCut {
        ffi::MigrationPreparationCut {
            present: true,
            id: u64::MAX,
            tick: u64::MAX,
            bytes: 64 * 1024 * 1024,
            digest: std::array::from_fn(|i| i as u8 + 1),
        }
    }
    fn inventory(lease: u64, witness: bool) -> ffi::SurvivorInventory {
        ffi::SurvivorInventory {
            lease,
            cuts: std::array::from_fn(|i| ffi::SurvivorInventoryEntry {
                cut: if i == 0 { seal() } else { cut(None) },
                irreversible_common: i == 0 && witness,
            }),
        }
    }
    fn common_snapshot() -> ffi::CommonCutState {
        ffi::CommonCutState {
            context: binding(),
            recipients: 3,
            committed: ffi::CommonCutValue {
                present: false,
                context: binding(),
                seal: cut(None),
                recipients: 0,
            },
            pending: ffi::CommonCutValue {
                present: true,
                context: binding(),
                seal: seal(),
                recipients: 3,
            },
            associated: 3,
            applied: 0,
            released: 0,
            aborted: 0,
            native_all_validated: true,
            commit: false,
            release: false,
            abort: false,
            rejected: u64::MAX,
        }
    }

    #[test]
    fn inventory_provenance_keeps_nonterminal_refusal_and_exact_abort_precedence() {
        let mut common = common_snapshot();
        let claimed = inventory(u64::MAX, true);
        assert!(
            survivor_inventory_admission(&binding(), &claimed, true, &common).unwrap()
                == ffi::SurvivorInventoryAdmission::Refuse
        );
        common.commit = true;
        assert!(
            survivor_inventory_admission(&binding(), &claimed, true, &common).unwrap()
                == ffi::SurvivorInventoryAdmission::Admit
        );
        common.context.membership_revision -= 1;
        assert!(
            survivor_inventory_admission(&binding(), &claimed, true, &common).unwrap()
                == ffi::SurvivorInventoryAdmission::Refuse
        );
        common.abort = true;
        assert!(
            survivor_inventory_admission(&binding(), &claimed, true, &common).unwrap()
                == ffi::SurvivorInventoryAdmission::Invalidate
        );
        common.pending.seal.digest[31] ^= 1;
        assert!(
            survivor_inventory_admission(&binding(), &claimed, true, &common).unwrap()
                == ffi::SurvivorInventoryAdmission::Refuse
        );
        assert!(
            survivor_inventory_admission(&binding(), &inventory(0, false), false, &common).unwrap()
                == ffi::SurvivorInventoryAdmission::Admit
        );
        // Classification neither mutates the selector nor supplies real owners.
        let owner = new_survivor_selection(0, &binding(), 100).unwrap();
        let state = survivor_selection_state(&owner).unwrap();
        assert!(!state.failed && state.inventory_peers == 0 && state.ready_observed == 0);
    }

    #[test]
    fn retained_inventory_derives_full_width_witnesses_from_existing_snapshot_only() {
        let mut common = common_snapshot();
        common.commit = true;
        let mut imported = seal();
        imported.id -= 1;
        let retained = [seal(), imported, cut(None)];
        let result =
            survivor_retained_inventory(&binding(), u64::MAX, &retained, true, &common).unwrap();
        assert!(result.admission == ffi::SurvivorInventoryAdmission::Admit);
        assert_eq!(result.inventory.lease, u64::MAX);
        let exact = &result.inventory.cuts[0];
        assert!(exact.cut.present && exact.irreversible_common);
        assert_eq!(
            (
                exact.cut.id,
                exact.cut.tick,
                exact.cut.bytes,
                exact.cut.digest
            ),
            (u64::MAX, u64::MAX, 64 * 1024 * 1024, retained[0].digest)
        );
        assert!(
            result.inventory.cuts[1].cut.present && !result.inventory.cuts[1].irreversible_common
        );
        assert_eq!(result.inventory.cuts[1].cut.id, u64::MAX - 1);
        assert!(
            !result.inventory.cuts[2].cut.present && !result.inventory.cuts[2].irreversible_common
        );
        let absent = survivor_retained_inventory(&binding(), 1, &retained, false, &common).unwrap();
        assert!(
            absent
                .inventory
                .cuts
                .iter()
                .all(|entry| !entry.irreversible_common)
        );
        common.abort = true;
        let aborted = survivor_retained_inventory(&binding(), 1, &retained, true, &common).unwrap();
        assert!(aborted.admission == ffi::SurvivorInventoryAdmission::Invalidate);
        assert!(!aborted.inventory.cuts[0].irreversible_common);
        for count in [0, 1, 2] {
            assert!(
                survivor_retained_inventory(&binding(), 1, &retained[..count], true, &common)
                    .is_err()
            );
        }
        let too_many = std::array::from_fn::<_, 4, _>(|_| cut(None));
        assert!(survivor_retained_inventory(&binding(), 1, &too_many, true, &common).is_err());
    }

    fn deliver(
        sender: &mut SurvivorSelection,
        receiver: &mut SurvivorSelection,
        peer: u8,
        now: u64,
    ) {
        for packet in survivor_selection_poll(sender, now, 128).unwrap() {
            assert!(packet.bytes.len() <= 246);
            assert!(survivor_selection_is_packet(&packet.bytes));
            assert!(survivor_selection_receive(receiver, peer, &packet.bytes, now).unwrap());
        }
    }

    #[test]
    fn authenticated_rows_and_exact_full_width_selection_do_not_create_ready() {
        let mut a = new_survivor_selection(0, &binding(), 100).unwrap();
        let mut b = new_survivor_selection(1, &binding(), 100).unwrap();
        assert!(survivor_selection_publish_inventory(&mut a, &inventory(u64::MAX, true)).unwrap());
        deliver(&mut a, &mut b, 0, 0);
        let before = survivor_selection_state(&b).unwrap();
        assert_eq!(before.inventory_peers, 1);
        assert!(!before.selection_complete && !before.selected.present);
        assert_eq!(before.ready_observed, 0);
        assert!(survivor_selection_publish_inventory(&mut b, &inventory(1, false)).unwrap());
        for now in [0, 101, 202, 303] {
            deliver(&mut a, &mut b, 0, now);
            deliver(&mut b, &mut a, 1, now);
        }
        for owner in [&a, &b] {
            let state = survivor_selection_state(owner).unwrap();
            assert_eq!(state.context.session, binding().session);
            assert_eq!(state.context.epoch, u32::MAX);
            assert_eq!(state.context.membership_revision, u64::MAX);
            assert_eq!(state.survivors, 3);
            assert_eq!(state.inventory_peers, 3);
            assert_eq!(state.accepted, 3);
            assert!(state.selection_complete && !state.failed && state.selected.present);
            assert_eq!(state.selected.id, u64::MAX);
            assert_eq!(state.selected.tick, u64::MAX);
            assert_eq!(state.selected.bytes, 64 * 1024 * 1024);
            assert_eq!(state.selected.digest, seal().digest);
            assert!(state.transcript.iter().any(|&v| v != 0));
            assert_eq!(state.ready_observed, 0);
            assert!(survivor_selection_allows_ready(owner, 0, &seal()).unwrap());
            assert!(!survivor_selection_allows_ready(owner, 15, &seal()).unwrap());
            assert!(!survivor_selection_allows_ready(owner, 255, &seal()).unwrap());
        }
        assert!(survivor_selection_invalidate(&mut a, &cut(None)).unwrap());
        deliver(&mut a, &mut b, 0, 404);
        assert!(survivor_selection_state(&b).unwrap().failed);
        assert!(!survivor_selection_allows_ready(&b, 1, &seal()).unwrap());
        assert!(!survivor_selection_publish_inventory(&mut b, &inventory(1, false)).unwrap());
    }

    #[test]
    fn invalid_constructor_inventory_and_recognized_wire_preserve_metadata_boundaries() {
        assert!(new_survivor_selection(255, &binding(), 100).is_err());
        let mut owner = new_survivor_selection(0, &binding(), 100).unwrap();
        assert!(survivor_selection_publish_inventory(&mut owner, &inventory(0, true)).is_err());
        assert!(!survivor_selection_receive(&mut owner, 1, b"foreign", u64::MAX).unwrap());
        assert!(survivor_selection_receive(&mut owner, 255, b"H2N2\0\0\x0d", 1).unwrap());
        let state = survivor_selection_state(&owner).unwrap();
        assert_eq!(state.rejected, 1);
        assert_eq!(state.inventory_peers, 0);
        assert!(!state.selected.present && !state.selection_complete && !state.failed);
        assert_eq!(state.ready_observed, 0);
    }
}
