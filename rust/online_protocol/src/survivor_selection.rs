//! A fixed-survivor, single-attempt selection of one actually retained cut.
//!
//! The caller freezes genuine native/foreground owners before publishing its
//! inventory and retains that lease until this generation is retired. An
//! irreversible observation means an actual CommonCut Commit/Release receipt,
//! never merely an export, association, import or full migration readiness.
//! This module authenticates no transport and creates no owner or election
//! Ready. Every inventory row must arrive independently from its admitted
//! physical owner; an Accept authenticates none of the rows it hashes.
//!
//! Any invalidation/conflict is terminal for this attempt and Context. There
//! is intentionally no reset, replacement inventory or automatic resume. An
//! actual externally published/learned Ready must be reported while holding
//! the enclosing session's generation lock, and the native freeze must remain
//! held. A future restart protocol requires a separate reviewed owner-retirement
//! barrier; constructing a replacement Agreement is not such a protocol.

pub use crate::host_agreement::{Context, Cut, Packet};
use sha2::{Digest, Sha256};

pub const MAX_INVENTORY_CUTS: usize = 3;
pub const DATAGRAM_KIND: u8 = 13;
pub const MAX_PACKET_BYTES: usize = 246;
const PEERS: usize = 16;
const TYPES: usize = 3;
const PENDING: usize = PEERS * TYPES;
const ATTEMPT: u64 = 1;
const VERSION: u8 = 1;
const RETRY_MS: u64 = 100;
const DOMAIN: &[u8] = b"H2 survivor cut selection\0v1\0";

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct InventoryEntry {
    pub cut: Cut,
    pub irreversible_common: bool,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Inventory {
    /// Nonzero caller-owned identifier of this actually frozen inventory.
    pub lease: u64,
    pub cuts: [Option<InventoryEntry>; MAX_INVENTORY_CUTS],
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum InventoryAdmission {
    Admit,
    Refuse,
    Invalidate,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct InventoryPreparation {
    pub inventory: Inventory,
    pub admission: InventoryAdmission,
}

fn irreversible_observed(context: Context, cut: Cut, common: &crate::common_cut::State) -> bool {
    let names = |value: Option<crate::common_cut::CommonCut>| {
        value.is_some_and(|value| value.context == context && value.seal == cut)
    };
    common.context == context
        && (names(common.committed) || (names(common.pending) && common.commit && !common.abort))
}

/// Classify the Session's existing provenance gate without changing any owner
/// or agreement. Session/context/admission checks precede this operation; the
/// ordinary inventory validator still checks leases, cut bounds and duplicates.
/// Exact advertised pending Abort takes precedence over witness refusal, as in
/// the existing Session path. It compares the full seal before witness context.
pub fn inventory_admission(
    context: Context,
    inventory: &Inventory,
    common: Option<&crate::common_cut::State>,
) -> InventoryAdmission {
    if common.is_some_and(|common| {
        common.abort
            && common.pending.is_some_and(|pending| {
                inventory
                    .cuts
                    .iter()
                    .flatten()
                    .any(|entry| entry.cut == pending.seal)
            })
    }) {
        return InventoryAdmission::Invalidate;
    }
    if inventory.cuts.iter().flatten().any(|entry| {
        entry.irreversible_common
            && !common.is_some_and(|common| irreversible_observed(context, entry.cut, common))
    }) {
        return InventoryAdmission::Refuse;
    }
    InventoryAdmission::Admit
}

/// Build only metadata for already retained caller-owned pairs. Neither a cut
/// nor a lease value proves a native freeze. Irreversible flags come exclusively
/// from the actual Common snapshot, never from importability or byte delivery.
pub fn retained_inventory(
    context: Context,
    lease: u64,
    retained: [Option<Cut>; MAX_INVENTORY_CUTS],
    common: Option<&crate::common_cut::State>,
) -> InventoryPreparation {
    let inventory = Inventory {
        lease,
        cuts: retained.map(|value| {
            value.map(|cut| InventoryEntry {
                cut,
                irreversible_common: common
                    .is_some_and(|common| irreversible_observed(context, cut, common)),
            })
        }),
    };
    InventoryPreparation {
        admission: inventory_admission(context, &inventory, common),
        inventory,
    }
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub enum Failure {
    LocalInvalidation = 0,
    NoCommonIrreversibleCut = 1,
    ConflictingInventory = 2,
    ConflictingAcceptance = 3,
    Abort = 4,
    UnexpectedReady = 5,
}
impl TryFrom<u8> for Failure {
    type Error = Error;
    fn try_from(value: u8) -> Result<Self, Error> {
        match value {
            0 => Ok(Self::LocalInvalidation),
            1 => Ok(Self::NoCommonIrreversibleCut),
            2 => Ok(Self::ConflictingInventory),
            3 => Ok(Self::ConflictingAcceptance),
            4 => Ok(Self::Abort),
            5 => Ok(Self::UnexpectedReady),
            _ => Err(Error),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct State {
    pub context: Context,
    pub survivors: u16,
    pub inventory_peers: u16,
    pub accepted: u16,
    pub selected: Option<Cut>,
    pub transcript: [u8; 32],
    /// Selection metadata only, never full preparation or election Ready.
    pub selection_complete: bool,
    pub failed: bool,
    pub failure: Option<Failure>,
    pub ready_observed: u16,
    pub rejected: u64,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Invalid survivor cut selection inventory, context or packet")
    }
}
impl std::error::Error for Error {}
fn need(value: bool) -> Result<(), Error> {
    if value { Ok(()) } else { Err(Error) }
}
fn member(peers: u16, peer: u8) -> bool {
    peer < PEERS as u8 && peers & (1u16 << peer) != 0
}
fn valid(cut: Cut) -> bool {
    cut.id != 0
        && cut.bytes != 0
        && cut.bytes <= crate::migration_preparation::MAX_PREPARATION_BYTES
        && cut.digest.iter().any(|&byte| byte != 0)
}
fn canonical(mut value: Inventory) -> Result<Inventory, Error> {
    need(value.lease != 0)?;
    for entry in value.cuts.iter().flatten() {
        need(valid(entry.cut))?;
    }
    value
        .cuts
        .sort_by_key(|entry| entry.map(|entry| entry.cut.id));
    // None sorts first; the canonical wire representation has a dense prefix.
    let empty = value
        .cuts
        .iter()
        .take_while(|entry| entry.is_none())
        .count();
    value.cuts.rotate_left(empty);
    let mut previous = None;
    for entry in value.cuts.iter().flatten() {
        need(previous.is_none_or(|id| entry.cut.id > id))?;
        previous = Some(entry.cut.id);
    }
    Ok(value)
}
fn put(bytes: &mut Vec<u8>, value: u64, width: usize) {
    bytes.extend_from_slice(&value.to_le_bytes()[..width]);
}
fn write_context(bytes: &mut Vec<u8>, value: Context) {
    bytes.extend_from_slice(&value.session);
    put(bytes, u64::from(value.epoch), 4);
    put(bytes, value.membership_revision, 8);
    put(bytes, u64::from(value.peers), 2);
    bytes.push(value.host);
}
fn write_cut(bytes: &mut Vec<u8>, value: Cut) {
    put(bytes, value.id, 8);
    put(bytes, value.tick, 8);
    put(bytes, value.bytes, 8);
    bytes.extend_from_slice(&value.digest);
}
fn write_inventory(bytes: &mut Vec<u8>, value: Inventory) {
    put(bytes, value.lease, 8);
    bytes.push(value.cuts.iter().flatten().count() as u8);
    for entry in value.cuts.iter().flatten() {
        write_cut(bytes, entry.cut);
        bytes.push(u8::from(entry.irreversible_common));
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Body {
    Inventory(Inventory),
    Accept(Cut, [u8; 32]),
    Invalidate(Failure, Option<Cut>),
}
impl Body {
    fn kind(self) -> usize {
        match self {
            Self::Inventory(_) => 0,
            Self::Accept(..) => 1,
            Self::Invalidate(..) => 2,
        }
    }
}
struct Message {
    context: Context,
    source: u8,
    target: u8,
    body: Body,
}
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl<'a> Reader<'a> {
    fn raw(&mut self, width: usize) -> Result<&'a [u8], Error> {
        let end = self.at.checked_add(width).ok_or(Error)?;
        let value = self.bytes.get(self.at..end).ok_or(Error)?;
        self.at = end;
        Ok(value)
    }
    fn word(&mut self, width: usize) -> Result<u64, Error> {
        let mut bytes = [0; 8];
        bytes
            .get_mut(..width)
            .ok_or(Error)?
            .copy_from_slice(self.raw(width)?);
        Ok(u64::from_le_bytes(bytes))
    }
    fn byte(&mut self) -> Result<u8, Error> {
        Ok(self.word(1)? as u8)
    }
    fn boolean(&mut self) -> Result<bool, Error> {
        match self.byte()? {
            0 => Ok(false),
            1 => Ok(true),
            _ => Err(Error),
        }
    }
    fn context(&mut self) -> Result<Context, Error> {
        Ok(Context {
            session: self.raw(32)?.try_into().map_err(|_| Error)?,
            epoch: self.word(4)? as u32,
            membership_revision: self.word(8)?,
            peers: self.word(2)? as u16,
            host: self.byte()?,
        })
    }
    fn cut(&mut self) -> Result<Cut, Error> {
        let cut = Cut {
            id: self.word(8)?,
            tick: self.word(8)?,
            bytes: self.word(8)?,
            digest: self.raw(32)?.try_into().map_err(|_| Error)?,
        };
        need(valid(cut))?;
        Ok(cut)
    }
    fn inventory(&mut self) -> Result<Inventory, Error> {
        let lease = self.word(8)?;
        let count = usize::from(self.byte()?);
        need(count <= MAX_INVENTORY_CUTS)?;
        let mut cuts = [None; MAX_INVENTORY_CUTS];
        for entry in cuts.iter_mut().take(count) {
            *entry = Some(InventoryEntry {
                cut: self.cut()?,
                irreversible_common: self.boolean()?,
            });
        }
        let value = Inventory { lease, cuts };
        need(canonical(value)? == value)?;
        Ok(value)
    }
}
fn decode(bytes: &[u8], protocol: u16) -> Result<Message, Error> {
    need(bytes.len() <= MAX_PACKET_BYTES)?;
    let mut reader = Reader { bytes, at: 0 };
    need(reader.raw(4)? == b"H2N2" && reader.word(2)? == u64::from(protocol))?;
    need(reader.byte()? == DATAGRAM_KIND && reader.byte()? == VERSION)?;
    let kind = reader.byte()?;
    let context = reader.context()?;
    let source = reader.byte()?;
    let target = reader.byte()?;
    need(reader.word(8)? == ATTEMPT)?;
    let body = match kind {
        0 => Body::Inventory(reader.inventory()?),
        1 => Body::Accept(
            reader.cut()?,
            reader.raw(32)?.try_into().map_err(|_| Error)?,
        ),
        2 => {
            let reason = Failure::try_from(reader.byte()?)?;
            let cut = if reader.boolean()? {
                Some(reader.cut()?)
            } else {
                None
            };
            need((reason == Failure::Abort) == cut.is_some())?;
            Body::Invalidate(reason, cut)
        }
        _ => return Err(Error),
    };
    need(reader.at == bytes.len())?;
    Ok(Message {
        context,
        source,
        target,
        body,
    })
}

pub struct Agreement {
    self_peer: u8,
    protocol: u16,
    value: State,
    inventories: [Option<Inventory>; PEERS],
    invalidated_cut: Option<Cut>,
    clock: u64,
    emitted_at: u64,
    emitted: bool,
    dirty: bool,
    packets: [[Option<Vec<u8>>; TYPES]; PEERS],
    cursor: usize,
}
impl Agreement {
    pub fn new(self_peer: u8, context: Context, protocol: u16) -> Result<Self, Error> {
        need(
            member(context.peers, context.host)
                && context.peers.count_ones() >= 2
                && context.session.iter().any(|&byte| byte != 0),
        )?;
        let survivors = context.peers & !(1u16 << context.host);
        need(member(survivors, self_peer))?;
        Ok(Self {
            self_peer,
            protocol,
            value: State {
                context,
                survivors,
                inventory_peers: 0,
                accepted: 0,
                selected: None,
                transcript: [0; 32],
                selection_complete: false,
                failed: false,
                failure: None,
                ready_observed: 0,
                rejected: 0,
            },
            inventories: [None; PEERS],
            invalidated_cut: None,
            clock: 0,
            emitted_at: 0,
            emitted: false,
            dirty: true,
            packets: std::array::from_fn(|_| std::array::from_fn(|_| None)),
            cursor: 0,
        })
    }
    pub fn state(&self) -> State {
        self.value
    }
    /// Read-only metadata gate. The caller still owes actual complete native
    /// preparation and must atomically pair this check with Ready publication.
    pub fn allows_ready(&self, peer: u8, cut: Cut) -> bool {
        member(self.value.survivors, peer) && self.can_finish(cut)
    }
    pub fn can_finish(&self, cut: Cut) -> bool {
        !self.value.failed && self.value.selection_complete && self.value.selected == Some(cut)
    }
    fn fail(&mut self, reason: Failure, cut: Option<Cut>) {
        if self.value.failed {
            return;
        }
        self.value.failed = true;
        self.value.failure = Some(reason);
        self.value.selection_complete = false;
        self.invalidated_cut = cut;
        self.packets.iter_mut().for_each(|row| row.fill(None));
        self.dirty = true;
    }
    /// Terminal local refusal. Some(cut) reports a genuine exact Common Abort
    /// relevant to a known frozen inventory/selection; an unrelated cut is
    /// ignored. None reports any other caller failure. No action retires owners.
    pub fn invalidate(&mut self, cut: Option<Cut>) -> bool {
        if cut.is_some_and(|cut| !valid(cut)) {
            return false;
        }
        if cut.is_some_and(|cut| {
            self.value.selected != Some(cut)
                && !self.inventories.iter().flatten().any(|inventory| {
                    inventory
                        .cuts
                        .iter()
                        .flatten()
                        .any(|entry| entry.cut == cut)
                })
        }) {
            return false;
        }
        self.fail(
            if cut.is_some() {
                Failure::Abort
            } else {
                Failure::LocalInvalidation
            },
            cut,
        );
        true
    }
    /// Report an actual published/learned election Ready, never request one.
    /// The enclosing session must serialize this with Ready publication and
    /// incoming controls. A legitimately earlier remote Ready may stay in the
    /// actual election owner's bank until selection completes; report that
    /// retained observation then, rather than inventing another Ready bank.
    /// No restart or inventory replacement is available.
    pub fn observe_ready(&mut self, peer: u8, cut: Cut) -> bool {
        if !member(self.value.survivors, peer) || !valid(cut) {
            return false;
        }
        self.value.ready_observed |= 1u16 << peer;
        if self.value.failed || !self.value.selection_complete || self.value.selected != Some(cut) {
            self.fail(Failure::UnexpectedReady, None);
            return false;
        }
        true
    }
    pub fn publish_inventory(&mut self, inventory: Inventory) -> Result<bool, Error> {
        let inventory = canonical(inventory)?;
        if self.value.failed {
            return Ok(false);
        }
        self.install(self.self_peer, inventory);
        Ok(!self.value.failed)
    }
    fn install(&mut self, peer: u8, inventory: Inventory) {
        let slot = &mut self.inventories[peer as usize];
        if let Some(prior) = slot {
            if *prior != inventory {
                self.fail(Failure::ConflictingInventory, None);
            }
            return;
        }
        *slot = Some(inventory);
        self.value.inventory_peers |= 1u16 << peer;
        self.dirty = true;
        self.progress();
    }
    fn progress(&mut self) {
        if self.value.failed || self.value.inventory_peers != self.value.survivors {
            return;
        }
        let Some(own) = self.inventories[self.self_peer as usize] else {
            return;
        };
        let selected = own
            .cuts
            .iter()
            .flatten()
            .map(|entry| entry.cut)
            .filter(|cut| {
                let mut irreversible = false;
                for peer in 0..PEERS as u8 {
                    if !member(self.value.survivors, peer) {
                        continue;
                    }
                    let Some(inventory) = self.inventories[peer as usize] else {
                        return false;
                    };
                    let Some(entry) = inventory
                        .cuts
                        .iter()
                        .flatten()
                        .find(|entry| entry.cut == *cut)
                    else {
                        return false;
                    };
                    irreversible |= entry.irreversible_common;
                }
                irreversible
            })
            .max_by_key(|cut| cut.id);
        let Some(selected) = selected else {
            self.fail(Failure::NoCommonIrreversibleCut, None);
            return;
        };
        let mut transcript = Vec::with_capacity(3200);
        transcript.extend_from_slice(DOMAIN);
        transcript.push(VERSION);
        put(&mut transcript, u64::from(self.protocol), 2);
        write_context(&mut transcript, self.value.context);
        put(&mut transcript, ATTEMPT, 8);
        put(&mut transcript, u64::from(self.value.survivors), 2);
        for (peer, inventory) in self.inventories.iter().enumerate() {
            transcript.push(peer as u8);
            transcript.push(u8::from(inventory.is_some()));
            if let Some(inventory) = inventory {
                write_inventory(&mut transcript, *inventory);
            }
        }
        write_cut(&mut transcript, selected);
        self.value.transcript = Sha256::digest(&transcript).into();
        self.value.selected = Some(selected);
        self.value.accepted |= 1u16 << self.self_peer;
        self.value.selection_complete = self.value.accepted == self.value.survivors;
        self.dirty = true;
    }
    fn send(&mut self, peer: u8, body: Body) {
        let mut bytes = Vec::with_capacity(MAX_PACKET_BYTES);
        bytes.extend_from_slice(b"H2N2");
        put(&mut bytes, u64::from(self.protocol), 2);
        bytes.extend_from_slice(&[DATAGRAM_KIND, VERSION, body.kind() as u8]);
        write_context(&mut bytes, self.value.context);
        bytes.extend_from_slice(&[self.self_peer, peer]);
        put(&mut bytes, ATTEMPT, 8);
        match body {
            Body::Inventory(value) => write_inventory(&mut bytes, value),
            Body::Accept(cut, digest) => {
                write_cut(&mut bytes, cut);
                bytes.extend_from_slice(&digest);
            }
            Body::Invalidate(reason, cut) => {
                bytes.extend_from_slice(&[reason as u8, u8::from(cut.is_some())]);
                if let Some(cut) = cut {
                    write_cut(&mut bytes, cut);
                }
            }
        }
        self.packets[peer as usize][body.kind()] = Some(bytes);
    }
    fn refresh(&mut self) {
        for peer in 0..PEERS as u8 {
            if peer == self.self_peer || !member(self.value.survivors, peer) {
                continue;
            }
            if let Some(reason) = self.value.failure {
                self.send(peer, Body::Invalidate(reason, self.invalidated_cut));
                continue;
            }
            if let Some(inventory) = self.inventories[self.self_peer as usize] {
                self.send(peer, Body::Inventory(inventory));
            }
            if let Some(selected) = self.value.selected {
                self.send(peer, Body::Accept(selected, self.value.transcript));
            }
        }
    }
    pub fn poll(&mut self, now: u64, budget: u32) -> Vec<Packet> {
        self.clock = self.clock.max(now);
        if self.dirty || !self.emitted || self.clock - self.emitted_at >= RETRY_MS {
            self.refresh();
            self.dirty = false;
            self.emitted = true;
            self.emitted_at = self.clock;
        }
        let mut result = Vec::new();
        for _ in 0..PENDING {
            if result.len() >= budget.min(128) as usize {
                break;
            }
            let index = self.cursor;
            self.cursor = (self.cursor + 1) % PENDING;
            if let Some(bytes) = self.packets[index / TYPES][index % TYPES].take() {
                result.push(Packet {
                    peer: (index / TYPES) as u8,
                    bytes,
                });
            }
        }
        result
    }
    fn apply_packet(&mut self, peer: u8, bytes: &[u8]) -> Result<(), Error> {
        let message = decode(bytes, self.protocol)?;
        need(
            message.context == self.value.context
                && message.source == peer
                && message.target == self.self_peer
                && peer != self.self_peer
                && member(self.value.survivors, peer),
        )?;
        if let Body::Invalidate(reason, cut) = message.body {
            self.fail(reason, cut);
            return Ok(());
        }
        if self.value.failed {
            return Ok(());
        }
        match message.body {
            Body::Inventory(inventory) => self.install(peer, inventory),
            Body::Accept(cut, digest) => {
                // A hashed transcript never authenticates or installs rows.
                // Early Accept is dropped; normal retries supply it later.
                if self.value.inventory_peers != self.value.survivors {
                    return Ok(());
                }
                if self.value.selected != Some(cut) || self.value.transcript != digest {
                    self.fail(Failure::ConflictingAcceptance, None);
                } else {
                    self.value.accepted |= 1u16 << peer;
                    self.value.selection_complete = self.value.accepted == self.value.survivors;
                }
            }
            Body::Invalidate(..) => {}
        }
        Ok(())
    }
    pub fn receive(&mut self, peer: u8, bytes: &[u8], now: u64) -> bool {
        if !is_packet(bytes) {
            return false;
        }
        self.clock = self.clock.max(now);
        if self.apply_packet(peer, bytes).is_err() {
            self.value.rejected = self.value.rejected.saturating_add(1);
        }
        true
    }
}

pub fn is_packet(bytes: &[u8]) -> bool {
    bytes.len() >= 7 && bytes.get(..4) == Some(b"H2N2") && bytes[6] == DATAGRAM_KIND
}

#[cfg(test)]
mod inventory_admission_tests {
    use super::*;

    fn context() -> Context {
        Context {
            session: [0x80; 32],
            epoch: u32::MAX,
            membership_revision: u64::MAX,
            peers: 0x8111,
            host: 0,
        }
    }
    fn cut(id: u64) -> Cut {
        Cut {
            id,
            tick: u64::MAX,
            bytes: crate::migration_preparation::MAX_PREPARATION_BYTES,
            digest: [0xa5; 32],
        }
    }
    fn common() -> crate::common_cut::State {
        crate::common_cut::State {
            context: context(),
            recipients: 0x8110,
            committed: None,
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
    fn value(seal: Cut) -> crate::common_cut::CommonCut {
        crate::common_cut::CommonCut {
            context: context(),
            seal,
            recipients: 0x8110,
        }
    }
    fn inventory(witness: bool) -> Inventory {
        Inventory {
            lease: u64::MAX,
            cuts: [
                Some(InventoryEntry {
                    cut: cut(u64::MAX),
                    irreversible_common: witness,
                }),
                None,
                None,
            ],
        }
    }

    #[test]
    fn only_exact_committed_or_nonaborted_pending_commit_supports_irreversible_claim() {
        let claimed = inventory(true);
        assert_eq!(
            inventory_admission(context(), &claimed, None),
            InventoryAdmission::Refuse
        );
        let mut state = common();
        state.pending = Some(value(cut(u64::MAX)));
        state.associated = state.recipients;
        state.native_all_validated = true;
        assert_eq!(
            inventory_admission(context(), &claimed, Some(&state)),
            InventoryAdmission::Refuse
        );
        state.commit = true;
        assert_eq!(
            inventory_admission(context(), &claimed, Some(&state)),
            InventoryAdmission::Admit
        );
        state.committed = state.pending;
        state.pending = None;
        state.commit = false;
        state.abort = true;
        assert_eq!(
            inventory_admission(context(), &claimed, Some(&state)),
            InventoryAdmission::Admit
        );
        for field in 0..4 {
            let mut changed = state;
            match field {
                0 => changed.committed.as_mut().unwrap().context.epoch -= 1,
                1 => changed.context.membership_revision -= 1,
                2 => changed.committed.as_mut().unwrap().seal.tick -= 1,
                _ => changed.committed.as_mut().unwrap().seal.digest[31] ^= 1,
            }
            assert_eq!(
                inventory_admission(context(), &claimed, Some(&changed)),
                InventoryAdmission::Refuse
            );
        }
    }

    #[test]
    fn exact_advertised_abort_precedes_refusal_and_does_not_invalidate_other_seals() {
        let mut claimed = inventory(false);
        claimed.cuts[1] = Some(InventoryEntry {
            cut: cut(7),
            irreversible_common: true,
        });
        let mut state = common();
        state.abort = true;
        state.pending = Some(value(cut(u64::MAX)));
        assert_eq!(
            inventory_admission(context(), &claimed, Some(&state)),
            InventoryAdmission::Invalidate
        );
        // Preserve the original Session's full-seal comparison before context
        // provenance even for an inconsistent externally supplied snapshot.
        state.pending.as_mut().unwrap().context.epoch -= 1;
        state.context.epoch -= 1;
        assert_eq!(
            inventory_admission(context(), &claimed, Some(&state)),
            InventoryAdmission::Invalidate
        );
        state.pending.as_mut().unwrap().seal.bytes -= 1;
        assert_eq!(
            inventory_admission(context(), &claimed, Some(&state)),
            InventoryAdmission::Refuse
        );
        claimed.cuts[1] = None;
        assert_eq!(
            inventory_admission(context(), &claimed, Some(&state)),
            InventoryAdmission::Admit
        );
    }

    #[test]
    fn metadata_admission_does_not_replace_existing_inventory_validation_or_make_ready() {
        let mut malformed = inventory(false);
        malformed.lease = 0;
        malformed.cuts[0].as_mut().unwrap().cut.id = 0;
        assert_eq!(
            inventory_admission(context(), &malformed, None),
            InventoryAdmission::Admit
        );
        let mut owner = Agreement::new(4, context(), 100).unwrap();
        assert!(owner.publish_inventory(malformed).is_err());
        assert!(!owner.state().failed && owner.state().ready_observed == 0);
        assert_eq!(
            inventory_admission(context(), &inventory(true), None),
            InventoryAdmission::Refuse
        );
        assert!(!owner.state().failed && owner.state().inventory_peers == 0);
    }

    #[test]
    fn retained_builder_uses_real_common_provenance_and_preserves_partial_release_inventory() {
        let retained = [Some(cut(7)), Some(cut(u64::MAX)), Some(cut(11))];
        let mut state = common();
        state.committed = Some(value(cut(7)));
        state.pending = Some(value(cut(u64::MAX)));
        state.commit = true;
        state.release = true;
        let prepared = retained_inventory(context(), u64::MAX, retained, Some(&state));
        assert_eq!(prepared.admission, InventoryAdmission::Admit);
        assert_eq!(prepared.inventory.lease, u64::MAX);
        assert_eq!(
            prepared.inventory.cuts.map(|entry| entry.unwrap().cut),
            retained.map(Option::unwrap)
        );
        assert_eq!(
            prepared
                .inventory
                .cuts
                .map(|entry| entry.unwrap().irreversible_common),
            [true, true, false]
        );
        state.commit = false;
        assert!(
            !retained_inventory(context(), 1, retained, Some(&state))
                .inventory
                .cuts[1]
                .unwrap()
                .irreversible_common
        );
        state.abort = true;
        assert_eq!(
            retained_inventory(context(), 1, retained, Some(&state)).admission,
            InventoryAdmission::Invalidate
        );
        let unrelated_abort =
            retained_inventory(context(), 1, [Some(cut(7)), None, None], Some(&state));
        assert_eq!(unrelated_abort.admission, InventoryAdmission::Admit);
        assert!(
            unrelated_abort.inventory.cuts[0]
                .unwrap()
                .irreversible_common
        );
        assert!(
            retained_inventory(context(), 1, retained, None)
                .inventory
                .cuts
                .iter()
                .flatten()
                .all(|entry| !entry.irreversible_common)
        );
        assert_eq!(
            retained_inventory(context(), 1, [None; 3], None).admission,
            InventoryAdmission::Admit
        );
    }

    #[test]
    fn every_context_and_full_seal_field_is_part_of_the_actual_witness() {
        for field in 0..14 {
            let mut state = common();
            state.pending = Some(value(cut(u64::MAX)));
            state.commit = true;
            let pending = state.pending.as_mut().unwrap();
            match field {
                0 => state.context.session[31] ^= 1,
                1 => state.context.epoch -= 1,
                2 => state.context.membership_revision -= 1,
                3 => state.context.peers ^= 2,
                4 => state.context.host = 4,
                5 => pending.context.session[31] ^= 1,
                6 => pending.context.epoch -= 1,
                7 => pending.context.membership_revision -= 1,
                8 => pending.context.peers ^= 2,
                9 => pending.context.host = 4,
                10 => pending.seal.id -= 1,
                11 => pending.seal.tick -= 1,
                12 => pending.seal.bytes -= 1,
                _ => pending.seal.digest[31] ^= 1,
            }
            assert_eq!(
                inventory_admission(context(), &inventory(true), Some(&state)),
                InventoryAdmission::Refuse
            );
            let built = retained_inventory(
                context(),
                1,
                [Some(cut(u64::MAX)), None, None],
                Some(&state),
            );
            assert_eq!(built.admission, InventoryAdmission::Admit);
            assert!(!built.inventory.cuts[0].unwrap().irreversible_common);
        }
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::BTreeMap;

    fn context(peers: u16, host: u8) -> Context {
        let mut session = [0; 32];
        session[0] = 0x15;
        Context {
            session,
            epoch: 2,
            membership_revision: 5,
            peers,
            host,
        }
    }
    fn cut(id: u64) -> Cut {
        Cut {
            id,
            tick: id + 4,
            bytes: 4096,
            digest: [0xa5; 32],
        }
    }
    fn inventory(lease: u64, cuts: &[(u64, bool)]) -> Inventory {
        let mut entries = [None; 3];
        for (slot, &(id, irreversible_common)) in entries.iter_mut().zip(cuts) {
            *slot = Some(InventoryEntry {
                cut: cut(id),
                irreversible_common,
            });
        }
        Inventory {
            lease,
            cuts: entries,
        }
    }
    fn hex(value: &str) -> Vec<u8> {
        let value: String = value.chars().filter(|ch| !ch.is_whitespace()).collect();
        assert_eq!(value.len() % 2, 0);
        value
            .as_bytes()
            .chunks_exact(2)
            .map(|pair| u8::from_str_radix(std::str::from_utf8(pair).unwrap(), 16).unwrap())
            .collect()
    }
    fn literal_inventory() -> Vec<u8> {
        hex("48324e32 6100 0d 01 00
            1500000000000000000000000000000000000000000000000000000000000000
            02000000 0500000000000000 1101 00 04 08 0100000000000000
            0900000000000000 01
            0700000000000000 0b00000000000000 0010000000000000
            a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5
            01")
    }
    fn packet(owner: &mut Agreement, target: u8, kind: u8) -> Vec<u8> {
        owner
            .poll(1000, 128)
            .into_iter()
            .find(|packet| packet.peer == target && packet.bytes[8] == kind)
            .unwrap()
            .bytes
    }
    struct Mesh {
        peers: BTreeMap<u8, Agreement>,
        now: u64,
    }
    impl Mesh {
        fn new(context: Context) -> Self {
            let peers = (0..16)
                .filter(|&peer| peer != context.host && member(context.peers, peer))
                .map(|peer| (peer, Agreement::new(peer, context, 97).unwrap()))
                .collect();
            Self { peers, now: 0 }
        }
        fn step(&mut self, lossy: bool, budget: u32) {
            self.now += 20;
            let mut packets = Vec::new();
            for (&source, owner) in &mut self.peers {
                packets.extend(
                    owner
                        .poll(self.now, budget)
                        .into_iter()
                        .map(|packet| (source, packet)),
                );
            }
            packets.reverse();
            for (source, packet) in packets {
                assert!(packet.bytes.len() <= MAX_PACKET_BYTES);
                if lossy
                    && self.now < 5000
                    && (self.now / 20
                        + u64::from(source)
                        + u64::from(packet.peer)
                        + u64::from(packet.bytes[8]))
                    .is_multiple_of(7)
                {
                    continue;
                }
                let receiver = self.peers.get_mut(&packet.peer).unwrap();
                assert!(receiver.receive(source, &packet.bytes, self.now));
                if lossy {
                    assert!(receiver.receive(source, &packet.bytes, self.now.saturating_sub(13)));
                }
            }
        }
        fn complete(&mut self, lossy: bool, budget: u32) {
            for _ in 0..2000 {
                self.step(lossy, budget);
                assert!(self.peers.values().all(|peer| !peer.state().failed));
                if self
                    .peers
                    .values()
                    .all(|peer| peer.state().selection_complete)
                {
                    return;
                }
            }
            panic!("fixed survivor selection did not converge");
        }
    }

    #[test]
    fn literal_codec_and_fixed_physical_inventory_bounds() {
        let c = context(0x111, 0);
        let mut owner = Agreement::new(4, c, 97).unwrap();
        assert!(owner.publish_inventory(inventory(9, &[(7, true)])).unwrap());
        assert_eq!(packet(&mut owner, 8, 0), literal_inventory());
        assert_eq!(literal_inventory().len(), 132);
        assert_eq!(owner.state().survivors, 0x110);
        assert_eq!(owner.state().inventory_peers, 1 << 4);
        assert!(!owner.state().selection_complete);
        assert!(!owner.allows_ready(4, cut(7)));
        for peer in [0, 1, 16, 255] {
            assert!(Agreement::new(peer, c, 97).is_err());
        }
        for bad in [
            Context { peers: 1, ..c },
            Context { host: 16, ..c },
            Context {
                session: [0; 32],
                ..c
            },
        ] {
            assert!(Agreement::new(4, bad, 97).is_err());
        }
        assert!(
            Agreement::new(
                4,
                Context {
                    epoch: u32::MAX,
                    membership_revision: u64::MAX,
                    ..c
                },
                97
            )
            .is_ok()
        );
        for mutation in 0..6 {
            let mut bad = inventory(9, &[(7, true)]);
            match mutation {
                0 => bad.lease = 0,
                1 => bad.cuts[0].as_mut().unwrap().cut.id = 0,
                2 => bad.cuts[0].as_mut().unwrap().cut.bytes = 0,
                3 => bad.cuts[0].as_mut().unwrap().cut.bytes = 64 * 1024 * 1024 + 1,
                4 => bad.cuts[0].as_mut().unwrap().cut.digest = [0; 32],
                _ => bad.cuts[1] = bad.cuts[0],
            }
            assert!(
                Agreement::new(4, c, 97)
                    .unwrap()
                    .publish_inventory(bad)
                    .is_err()
            );
        }
        let mut maximum = Agreement::new(4, c, 97).unwrap();
        maximum
            .publish_inventory(inventory(1, &[(1, true), (2, false), (3, true)]))
            .unwrap();
        assert_eq!(packet(&mut maximum, 8, 0).len(), MAX_PACKET_BYTES);
        // Four local couch seats do not add inventory rows: only physical4/8.
        assert_eq!(maximum.state().survivors.count_ones(), 2);
    }

    #[test]
    fn missed_offer_partial_release_and_real_irreversible_witness() {
        for (left, right, expected) in [
            (vec![(7, true), (9, false)], vec![(7, true)], 7),
            (vec![(9, true)], vec![(7, true), (9, true)], 9),
            (vec![(7, true), (9, true)], vec![(7, true), (9, false)], 9),
            (
                vec![(11, true), (7, true), (9, true)],
                vec![(9, true), (7, true)],
                9,
            ),
        ] {
            let mut mesh = Mesh::new(context(0x111, 0));
            mesh.peers
                .get_mut(&4)
                .unwrap()
                .publish_inventory(inventory(41, &left))
                .unwrap();
            mesh.peers
                .get_mut(&8)
                .unwrap()
                .publish_inventory(inventory(81, &right))
                .unwrap();
            mesh.complete(true, 2);
            let digest = mesh.peers[&4].state().transcript;
            for peer in mesh.peers.values() {
                assert_eq!(peer.state().selected, Some(cut(expected)));
                assert_eq!(peer.state().accepted, 0x110);
                assert_eq!(peer.state().transcript, digest);
                assert!(peer.can_finish(cut(expected)));
                assert!(peer.allows_ready(4, cut(expected)) && peer.allows_ready(8, cut(expected)));
                assert!(
                    !peer.allows_ready(0, cut(expected)) && !peer.allows_ready(1, cut(expected))
                );
                assert_eq!(peer.state().ready_observed, 0);
            }
        }
        for (left, right) in [
            (vec![(7, false)], vec![(7, false)]),
            (vec![(7, true)], vec![(9, true)]),
            (vec![], vec![(7, true)]),
        ] {
            let mut mesh = Mesh::new(context(0x111, 0));
            mesh.peers
                .get_mut(&4)
                .unwrap()
                .publish_inventory(inventory(1, &left))
                .unwrap();
            mesh.peers
                .get_mut(&8)
                .unwrap()
                .publish_inventory(inventory(2, &right))
                .unwrap();
            for _ in 0..30 {
                mesh.step(true, 2);
            }
            for peer in mesh.peers.values_mut() {
                assert!(peer.state().failed && !peer.state().selection_complete);
                assert!(!peer.publish_inventory(inventory(3, &[(7, true)])).unwrap());
                assert!(!peer.can_finish(cut(7)));
            }
        }
        let mut solo = Agreement::new(15, context(0x8001, 0), 97).unwrap();
        solo.publish_inventory(inventory(1, &[(7, true)])).unwrap();
        assert!(solo.state().selection_complete);
        assert_eq!(solo.state().accepted, 0x8000);
    }

    #[test]
    fn accepts_never_supply_missing_authenticated_rows() {
        let c = context(0x111, 0);
        let mut left = Agreement::new(4, c, 97).unwrap();
        let mut right = Agreement::new(8, c, 97).unwrap();
        left.publish_inventory(inventory(9, &[(7, true)])).unwrap();
        right
            .publish_inventory(inventory(10, &[(7, false)]))
            .unwrap();
        let left_row = packet(&mut left, 8, 0);
        let right_row = packet(&mut right, 4, 0);
        right.receive(4, &left_row, 10);
        let accept = packet(&mut right, 4, 1);
        assert_eq!(accept.len(), 154);
        left.receive(8, &accept, 20);
        assert_eq!(left.state().inventory_peers, 1 << 4);
        assert_eq!(left.state().accepted, 0);
        assert!(!left.state().failed && !left.state().selection_complete);
        left.receive(4, &right_row, 21); // authenticated sender substitution
        assert_eq!(left.state().inventory_peers, 1 << 4);
        left.receive(8, &right_row, 22);
        assert_eq!(left.state().accepted, 1 << 4);
        left.receive(8, &accept, 23);
        assert!(left.state().selection_complete);
        assert_eq!(left.state().transcript, right.state().transcript);
        let mut changed = right_row.clone();
        changed[66] ^= 1; // authentic owner reuses the round with another lease
        left.receive(8, &changed, 24);
        assert_eq!(left.state().failure, Some(Failure::ConflictingInventory));
        assert!(!left.state().selection_complete);
        left.receive(8, &accept, u64::MAX);
        assert!(left.state().failed && !left.state().selection_complete);
    }

    #[test]
    fn canonical_transcript_binds_all_fields_and_conflicts_fail_terminal() {
        fn transcript(c: Context, second: Inventory) -> [u8; 32] {
            let mut left = Agreement::new(4, c, 97).unwrap();
            let mut right = Agreement::new(8, c, 97).unwrap();
            left.publish_inventory(inventory(1, &[(7, true)])).unwrap();
            right.publish_inventory(second).unwrap();
            left.receive(8, &packet(&mut right, 4, 0), 1);
            left.state().transcript
        }
        let c = context(0x111, 0);
        let base = transcript(c, inventory(2, &[(7, false), (9, false)]));
        // Independently constructed field bytes + Python hashlib, retained in
        // the owned validation receipt. This pins domain/version/protocol,
        // row indices/presence/counts/lease, observation bits and selected seal.
        let golden: [u8; 32] =
            hex("05c394111ef295893f3566324b2ced30fe400f9fee09357f82410f174bfe2a5e")
                .try_into()
                .unwrap();
        assert_eq!(base, golden);
        let mut changed = inventory(2, &[(7, false), (9, false)]);
        changed.cuts[1].as_mut().unwrap().cut.digest[31] ^= 1;
        for (context, row) in [
            (c, inventory(3, &[(7, false), (9, false)])),
            (c, inventory(2, &[(7, true), (9, false)])),
            (c, inventory(2, &[(7, false)])),
            (c, changed),
            (
                Context { epoch: 3, ..c },
                inventory(2, &[(7, false), (9, false)]),
            ),
            (
                Context {
                    membership_revision: 6,
                    ..c
                },
                inventory(2, &[(7, false), (9, false)]),
            ),
        ] {
            assert_ne!(base, transcript(context, row));
        }
        assert_eq!(base, transcript(c, inventory(2, &[(9, false), (7, false)])));
        let mut mesh = Mesh::new(c);
        for (&peer, owner) in &mut mesh.peers {
            owner
                .publish_inventory(inventory(u64::from(peer), &[(7, true)]))
                .unwrap();
        }
        mesh.complete(false, 128);
        let mut wrong = packet(mesh.peers.get_mut(&8).unwrap(), 4, 1);
        *wrong.last_mut().unwrap() ^= 1;
        mesh.peers.get_mut(&4).unwrap().receive(8, &wrong, 2000);
        assert_eq!(
            mesh.peers[&4].state().failure,
            Some(Failure::ConflictingAcceptance)
        );
        assert!(!mesh.peers[&4].can_finish(cut(7)));
    }

    #[test]
    fn delayed_abort_and_actual_ready_observation_never_resume_or_overwrite() {
        let mut mesh = Mesh::new(context(0x111, 0));
        for (&peer, owner) in &mut mesh.peers {
            owner
                .publish_inventory(inventory(u64::from(peer), &[(7, true)]))
                .unwrap();
        }
        mesh.complete(false, 4);
        assert!(!mesh.peers.get_mut(&4).unwrap().invalidate(Some(cut(901))));
        assert!(mesh.peers[&4].can_finish(cut(7)));
        assert!(mesh.peers.get_mut(&4).unwrap().invalidate(Some(cut(7))));
        assert!(!mesh.peers[&4].allows_ready(4, cut(7)));
        for _ in 0..50 {
            mesh.step(true, 2);
        }
        for owner in mesh.peers.values_mut() {
            assert!(owner.state().failed && !owner.state().selection_complete);
            assert!(
                !owner
                    .publish_inventory(inventory(99, &[(9, true)]))
                    .unwrap()
            );
            assert!(!owner.observe_ready(4, cut(7)));
            for packet in owner.poll(u64::MAX, 128) {
                assert_eq!(packet.bytes[8], 2);
            }
        }
        let mut solo = Agreement::new(4, context(0x11, 0), 97).unwrap();
        solo.publish_inventory(inventory(1, &[(7, true)])).unwrap();
        assert!(solo.observe_ready(4, cut(7)));
        assert_eq!(solo.state().ready_observed, 1 << 4);
        assert!(solo.invalidate(None));
        assert!(!solo.can_finish(cut(7)));
        let mut early = Agreement::new(4, context(0x111, 0), 97).unwrap();
        assert!(!early.observe_ready(8, cut(7)));
        assert_eq!(early.state().failure, Some(Failure::UnexpectedReady));
        assert_eq!(early.state().ready_observed, 1 << 8);
    }

    #[test]
    fn malformed_codec_sender_context_and_dispatch_are_bounded() {
        let c = context(0x111, 0);
        let valid = literal_inventory();
        let mut node = Agreement::new(8, c, 97).unwrap();
        assert!(!node.receive(4, b"unknown", u64::MAX));
        assert_eq!(node.clock, 0);
        for length in 0..valid.len() {
            let recognized = node.receive(4, &valid[..length], 20);
            assert_eq!(recognized, length >= 7);
        }
        assert_eq!(node.state().inventory_peers, 0);
        for offset in [4, 7, 8, 9, 41, 45, 53, 55, 56, 57, 58, 74, 66, 75, 92, 131] {
            let mut bad = valid.clone();
            bad[offset] = match offset {
                66 | 75 | 92 => 0,
                131 => 2,
                _ => 0xff,
            };
            node.receive(4, &bad, 30);
            assert_eq!(node.state().inventory_peers, 0, "offset{offset}");
        }
        let mut zero_digest = valid.clone();
        zero_digest[99..131].fill(0);
        node.receive(4, &zero_digest, 30);
        assert_eq!(node.state().inventory_peers, 0);
        for peer in [0, 1, 8, 16, 255] {
            node.receive(peer, &valid, 31);
            assert_eq!(node.state().inventory_peers, 0);
        }
        for extra in [1, MAX_PACKET_BYTES, 4096] {
            let mut bad = valid.clone();
            bad.resize(bad.len() + extra, 0);
            node.receive(4, &bad, 32);
            assert_eq!(node.state().inventory_peers, 0);
        }
        let mut unsorted = Agreement::new(4, c, 97).unwrap();
        unsorted
            .publish_inventory(inventory(1, &[(7, true), (9, true)]))
            .unwrap();
        let mut bad = packet(&mut unsorted, 8, 0);
        let (first, second) = bad[75..].split_at_mut(57);
        first.swap_with_slice(second);
        node.receive(4, &bad, 33);
        assert_eq!(node.state().inventory_peers, 0);
        let before = node.state().rejected;
        assert!(before > 100);
        node.value.rejected = u64::MAX;
        node.receive(4, &valid[..7], u64::MAX);
        assert_eq!(node.state().rejected, u64::MAX);
        assert_eq!(node.clock, u64::MAX);
        node.poll(0, 0);
        assert_eq!(node.clock, u64::MAX);
        assert!(!node.state().failed && !node.state().selection_complete);
    }

    #[test]
    fn all_fifteen_survivors_loss_retry_reorder_and_no_timeout_membership_change() {
        for budget in [1, 2, 4, 128, u32::MAX] {
            let mut mesh = Mesh::new(context(u16::MAX, 15));
            for (&peer, owner) in &mut mesh.peers {
                let entries = if peer % 2 == 0 {
                    vec![(7, peer == 0), (9, true)]
                } else {
                    vec![(7, false)]
                };
                owner
                    .publish_inventory(inventory(u64::from(peer) + 1, &entries))
                    .unwrap();
                assert!(owner.poll(1, 0).is_empty());
            }
            mesh.complete(true, budget);
            for peer in mesh.peers.values() {
                assert_eq!(peer.state().survivors, 0x7fff);
                assert_eq!(peer.state().accepted, 0x7fff);
                assert_eq!(peer.state().selected, Some(cut(7)));
                assert_eq!(peer.state().ready_observed, 0);
            }
        }
        let mut missing = Agreement::new(4, context(0x111, 0), 97).unwrap();
        missing
            .publish_inventory(inventory(1, &[(7, true)]))
            .unwrap();
        missing.poll(u64::MAX, 128);
        assert_eq!(missing.state().survivors, 0x110);
        assert_eq!(missing.state().inventory_peers, 1 << 4);
        assert_eq!(missing.state().accepted, 0);
        assert!(!missing.state().failed && !missing.state().selection_complete);
    }

    #[test]
    fn every_control_has_strict_lengths_booleans_variants_and_attempt() {
        let c = context(0x111, 0);
        let mut mesh = Mesh::new(c);
        for (&peer, owner) in &mut mesh.peers {
            owner
                .publish_inventory(inventory(u64::from(peer), &[(7, true)]))
                .unwrap();
        }
        mesh.complete(false, 128);
        let accept = packet(mesh.peers.get_mut(&4).unwrap(), 8, 1);
        mesh.peers.get_mut(&4).unwrap().invalidate(Some(cut(7)));
        let abort = packet(mesh.peers.get_mut(&4).unwrap(), 8, 2);
        mesh.peers.get_mut(&8).unwrap().invalidate(None);
        let failure = packet(mesh.peers.get_mut(&8).unwrap(), 4, 2);
        assert_eq!((accept.len(), abort.len(), failure.len()), (154, 124, 68));
        for bytes in [literal_inventory(), accept, abort.clone(), failure.clone()] {
            assert!(decode(&bytes, 97).is_ok());
            for length in 0..bytes.len() {
                assert!(decode(&bytes[..length], 97).is_err());
            }
            let mut trailing = bytes.clone();
            trailing.push(0);
            assert!(decode(&trailing, 97).is_err());
            for offset in [7, 8, 58] {
                let mut unsupported = bytes.clone();
                unsupported[offset] = 0xff;
                assert!(decode(&unsupported, 97).is_err());
            }
        }
        for offset in [66, 67] {
            let mut malformed = failure.clone();
            malformed[offset] = 255;
            assert!(decode(&malformed, 97).is_err());
        }
        let mut wrong_presence = abort.clone();
        wrong_presence[67] = 2;
        assert!(decode(&wrong_presence, 97).is_err());
        let mut wrong_reason = abort;
        wrong_reason[66] = Failure::LocalInvalidation as u8;
        assert!(decode(&wrong_reason, 97).is_err());
    }

    #[test]
    fn legitimate_ready_can_be_reported_from_actual_bank_after_accept_reordering() {
        let c = context(0x111, 0);
        let mut left = Agreement::new(4, c, 97).unwrap();
        let mut right = Agreement::new(8, c, 97).unwrap();
        left.publish_inventory(inventory(4, &[(7, true)])).unwrap();
        right.publish_inventory(inventory(8, &[(7, true)])).unwrap();
        let left_row = packet(&mut left, 8, 0);
        let right_row = packet(&mut right, 4, 0);
        left.receive(8, &right_row, 1001);
        right.receive(4, &left_row, 1001);
        let left_accept = packet(&mut left, 8, 1);
        let right_accept = packet(&mut right, 4, 1);
        right.receive(4, &left_accept, 1002);
        assert!(right.allows_ready(8, cut(7)));
        assert!(right.observe_ready(8, cut(7)));
        // The external real election owner may learn this authenticated Ready
        // before the last selection Accept. It retains its existing bank; the
        // session reports that observation here only after local completion.
        let actual_election_bank = (8, cut(7));
        assert!(!left.state().failed && !left.state().selection_complete);
        assert_eq!(left.state().ready_observed, 0);
        left.receive(8, &right_accept, 1003);
        let (peer, cut) = actual_election_bank;
        assert!(left.observe_ready(peer, cut));
        assert_eq!(left.state().ready_observed, 1 << 8);
        assert!(left.can_finish(cut));
    }
}
