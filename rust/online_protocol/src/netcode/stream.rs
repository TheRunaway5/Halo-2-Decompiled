//! Acknowledged delta snapshot stream between the native host and one client.
//!
//! Generic snapshot state is coded losslessly against the newest message the
//! client acknowledged, so the reconstructed state (and its digest) is exactly
//! the host's. Skeletal poses are presentation state replicated Halo-style:
//! each body has a priority that grows while it differs from what the client
//! is known to hold; the host spends the remaining budget on the highest
//! priorities. The client keeps the latest refresh per body and carries held
//! poses along with cheap root updates. Pose values are excluded from the
//! digest, while the pose bank structure is included.

use super::bandwidth::{Bandwidth, Profile};
use super::generic::{self, View};
use super::pose::{self, Key, Pose, Reference, Root, Velocity};
use super::range::{Bit, Decoder, Encoder, Error, Uint};
use sha2::{Digest, Sha256};
use std::collections::{HashMap, HashSet, VecDeque};

/// Messages a client retains as baselines and pose references.
pub const RING: u32 = 64;
pub const MAX_MESSAGE: usize = 8 * 1024 * 1024;
/// Reconstructed snapshot bound, matching the native snapshot limit.
pub const MAX_BODY: usize = 8 * 1024 * 1024;
pub const FRAGMENT_HEADER: usize = 8;
pub const ACK_KEYFRAME: u8 = 1;
const FLAG_KEYFRAME: u8 = 1;
const FLAG_POSES: u8 = 2;
const FLAG_RESET: u8 = 4;
const MAX_ASSEMBLIES: usize = 4;
const ASSEMBLY_TIMEOUT_MS: u64 = 1_000;
const ACK_WINDOW: u32 = 32;

fn newer(a: u32, b: u32) -> bool {
    (a.wrapping_sub(b) as i32) > 0
}

fn put_varint(out: &mut Vec<u8>, mut value: u64) {
    loop {
        let byte = (value & 0x7F) as u8;
        value >>= 7;
        if value == 0 {
            out.push(byte);
            return;
        }
        out.push(byte | 0x80);
    }
}

struct Cursor<'a> {
    bytes: &'a [u8],
    at: usize,
}

impl<'a> Cursor<'a> {
    fn u8(&mut self) -> Result<u8, Error> {
        let value = *self.bytes.get(self.at).ok_or(Error)?;
        self.at += 1;
        Ok(value)
    }
    fn u32(&mut self) -> Result<u32, Error> {
        let bytes = self.bytes.get(self.at..self.at + 4).ok_or(Error)?;
        self.at += 4;
        Ok(u32::from_le_bytes(bytes.try_into().map_err(|_| Error)?))
    }
    fn varint(&mut self) -> Result<u64, Error> {
        let mut value = 0u64;
        for shift in (0..64).step_by(7) {
            let byte = self.u8()?;
            value |= u64::from(byte & 0x7F) << shift;
            if byte & 0x80 == 0 {
                return Ok(value);
            }
        }
        Err(Error)
    }
    fn take(&mut self, len: usize) -> Result<&'a [u8], Error> {
        let end = self.at.checked_add(len).ok_or(Error)?;
        let bytes = self.bytes.get(self.at..end).ok_or(Error)?;
        self.at = end;
        Ok(bytes)
    }
}

fn digest(generic: &[u8], poses: Option<(usize, u8, &[Key])>) -> [u8; 32] {
    let mut hash = Sha256::new();
    hash.update(b"H2SD1");
    hash.update((generic.len() as u64).to_le_bytes());
    hash.update(generic);
    if let Some((offset, authoritative, keys)) = poses {
        hash.update((offset as u64).to_le_bytes());
        hash.update([authoritative]);
        hash.update((keys.len() as u64).to_le_bytes());
        for key in keys {
            hash.update(key.object.to_le_bytes());
            hash.update(key.nodes.to_le_bytes());
        }
    }
    hash.finalize().into()
}

/// One pose refresh carried by a message.
#[derive(Clone)]
struct Entry {
    key: Key,
    full: Option<Pose>,
    root: Root,
    velocity: Option<Velocity>,
    tick: u32,
}

#[derive(Default, Clone)]
struct StreamModels {
    more: Bit,
    index: Uint,
    root_kind: Bit,
    reference: [Bit; 3],
    age: Uint,
    entry: Uint,
    same_key: Bit,
    skip: Bit,
    skip_len: Uint,
    count: Uint,
    nodes: Uint,
}

const REF_NONE: u8 = 0;
const REF_SAME: u8 = 1;
const REF_CROSS: u8 = 2;
const REF_LOCAL: u8 = 3;

fn encode_reference_kind(e: &mut Encoder, m: &mut StreamModels, kind: u8) {
    e.bit(&mut m.reference[0], kind & 2 != 0);
    e.bit(&mut m.reference[1 + usize::from(kind >> 1)], kind & 1 != 0);
}

fn decode_reference_kind(d: &mut Decoder<'_>, m: &mut StreamModels) -> u8 {
    let high = d.bit(&mut m.reference[0]);
    let low = d.bit(&mut m.reference[1 + usize::from(high)]);
    (u8::from(high) << 1) | u8::from(low)
}

fn encode_layout(e: &mut Encoder, m: &mut StreamModels, authoritative: u8, keys: &[Key], base: &[Key]) {
    e.direct(u64::from(authoritative), 8);
    m.count.encode(e, keys.len() as u64);
    let positions: HashMap<Key, usize> = base.iter().enumerate().map(|(n, k)| (*k, n)).collect();
    let mut cursor = 0usize;
    for key in keys {
        let same = base.get(cursor) == Some(key);
        e.bit(&mut m.same_key, same);
        if same {
            cursor += 1;
            continue;
        }
        match positions.get(key).filter(|&&at| at > cursor) {
            Some(&at) => {
                e.bit(&mut m.skip, true);
                m.skip_len.encode(e, (at - cursor - 1) as u64);
                cursor = at + 1;
            }
            None => {
                e.bit(&mut m.skip, false);
                e.direct(u64::from(key.object), 32);
                m.nodes.encode(e, u64::from(key.nodes));
            }
        }
    }
}

fn decode_layout(d: &mut Decoder<'_>, m: &mut StreamModels, base: &[Key]) -> Result<(u8, Vec<Key>), Error> {
    let authoritative = d.direct(8) as u8;
    let count = usize::try_from(m.count.decode(d)?).map_err(|_| Error)?;
    if count > 2048 || authoritative > 1 {
        return Err(Error);
    }
    let mut keys = Vec::with_capacity(count);
    let mut cursor = 0usize;
    for _ in 0..count {
        if d.bit(&mut m.same_key) {
            keys.push(*base.get(cursor).ok_or(Error)?);
            cursor += 1;
        } else if d.bit(&mut m.skip) {
            let at = cursor
                .checked_add(usize::try_from(m.skip_len.decode(d)?).map_err(|_| Error)?)
                .and_then(|v| v.checked_add(1))
                .ok_or(Error)?;
            keys.push(*base.get(at).ok_or(Error)?);
            cursor = at + 1;
        } else {
            let object = d.direct(32) as u32;
            let nodes = u16::try_from(m.nodes.decode(d)?).map_err(|_| Error)?;
            if nodes == 0 || usize::from(nodes) > 0x7fff / 0x34 {
                return Err(Error);
            }
            keys.push(Key { object, nodes });
        }
    }
    let unique: HashSet<u32> = keys.iter().map(|k| k.object).collect();
    if unique.len() != keys.len() {
        return Err(Error);
    }
    Ok((authoritative, keys))
}

fn similarity(a: &Pose, b: &Pose) -> u64 {
    a.nodes
        .iter()
        .zip(&b.nodes)
        .map(|(x, y)| {
            let pos: u64 = (0..3).map(|k| u64::from(x.pos[k].abs_diff(y.pos[k]))).sum();
            let rot = match (x.rot, y.rot) {
                (pose::Rot::Quat { largest: la, c: ca }, pose::Rot::Quat { largest: lb, c: cb }) if la == lb => {
                    (0..3).map(|k| u64::from(ca[k].abs_diff(cb[k]))).sum()
                }
                _ => 8_192,
            };
            pos + rot
        })
        .sum()
}

#[derive(Clone)]
struct Stored {
    seq: u32,
    entry: u32,
    tick: u32,
    pose: Option<Pose>,
    root: Root,
    velocity: Option<Velocity>,
}

#[derive(Default)]
struct Object {
    acked_full: Option<Stored>,
    acked_root: Option<Stored>,
    pending: Vec<Stored>,
    priority_full: f32,
    priority_root: f32,
}

impl Object {
    fn last_full(&self) -> Option<&Pose> {
        self.pending.iter().rev().find_map(|s| s.pose.as_ref()).or(self.acked_full.as_ref().and_then(|s| s.pose.as_ref()))
    }
    fn last_root(&self) -> Option<&Root> {
        self.pending.last().map(|s| &s.root).or(self.acked_root.as_ref().map(|s| &s.root))
    }
}

struct Sent {
    seq: u32,
    sent_ms: u64,
    view: Option<View>,
    layout: Vec<Key>,
    entries: Vec<Entry>,
    resolved: bool,
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SenderStats {
    pub messages: u64,
    pub wire_bytes: u64,
    pub packets: u64,
    pub keyframes: u64,
    pub over_budget: u64,
    pub full_refreshes: u64,
    pub root_updates: u64,
    pub forced_refreshes: u64,
    pub held_refreshes: u64,
    pub lost_messages: u64,
    pub resets: u64,
    pub budget_bps: u32,
    pub rtt_ms: u64,
    pub loss_percent: f32,
}

pub struct Frame<'a> {
    pub body: &'a [u8],
    pub floats: &'a [u32],
    /// Keyed list/entry anchors, sorted by offset.
    pub anchors: &'a [(u32, u64)],
    /// Byte range of the native WORLD pose bank inside `body`, if present.
    pub poses: Option<(usize, usize)>,
    pub tick: u32,
    pub sequence: u32,
    pub now_ms: u64,
    pub viewers: &'a [[f32; 3]],
    pub own: &'a [u32],
    /// Bodies playing a short action (melee, grenade throw) and their held
    /// weapons: refreshed every tick while hot, so actions are never held.
    pub hot: &'a [u32],
}

pub struct Emitted {
    pub digest: [u8; 32],
    pub view_len: u32,
    pub packets: Vec<Vec<u8>>,
    pub keyframe: bool,
}

#[derive(Clone, Copy, Debug)]
pub struct SenderConfig {
    pub profile: Profile,
    /// Fragment payload bytes per datagram, excluding the fragment header.
    pub chunk: usize,
    /// Wire bytes added to each datagram outside its payload.
    pub overhead: usize,
}

struct Split {
    generic: Vec<u8>,
    floats: Vec<u32>,
    anchors: Vec<(u32, u64)>,
    offset: usize,
    bank: pose::RawBank,
}

fn split(frame: &Frame<'_>) -> Option<Split> {
    let (start, end) = frame.poses?;
    if start > end || end > frame.body.len() {
        return None;
    }
    let bank = pose::parse(&frame.body[start..end])?;
    let mut generic = Vec::with_capacity(frame.body.len() - (end - start));
    generic.extend_from_slice(&frame.body[..start]);
    generic.extend_from_slice(&frame.body[end..]);
    let shift = (end - start) as u32;
    let floats = frame
        .floats
        .iter()
        .filter_map(|&f| {
            let at = f as usize;
            if at + 4 <= start {
                Some(f)
            } else if at >= end {
                Some(f - shift)
            } else {
                None
            }
        })
        .collect();
    let anchors = frame
        .anchors
        .iter()
        .filter_map(|&(f, key)| {
            let at = f as usize;
            if at < start {
                Some((f, key))
            } else if at >= end {
                Some((f - shift, key))
            } else {
                None
            }
        })
        .collect();
    Some(Split { generic, floats, anchors, offset: start, bank })
}

/// Float offsets and keyed anchors for the generic codec.
type Hints = (Vec<u32>, Vec<(u32, u64)>);

/// The frame's hints plus the pose bank's own floats and per-body anchors,
/// so an exact stream codes poses as lossless residuals.
fn exact_layout(frame: &Frame<'_>) -> Option<Hints> {
    let (start, end) = frame.poses?;
    if start > end || end > frame.body.len() {
        return None;
    }
    let bank = pose::parse(&frame.body[start..end])?;
    let (bank_floats, bank_anchors) = pose::bank_layout(&bank, u32::try_from(start).ok()?);
    let mut floats: Vec<u32> = frame.floats.iter().copied().filter(|&f| (f as usize) < start || f as usize >= end).collect();
    floats.extend(bank_floats);
    floats.sort_unstable();
    let mut anchors: Vec<(u32, u64)> =
        frame.anchors.iter().copied().filter(|&(f, _)| (f as usize) < start || f as usize >= end).collect();
    anchors.extend(bank_anchors);
    anchors.sort_unstable_by_key(|&(offset, _)| offset);
    Some((floats, anchors))
}

fn relevance(world: [f32; 3], viewers: &[[f32; 3]], own: bool) -> f32 {
    if own {
        return 3.0;
    }
    if viewers.is_empty() {
        return 1.0;
    }
    let nearest = viewers
        .iter()
        .map(|v| (0..3).map(|a| (v[a] - world[a]) * (v[a] - world[a])).sum::<f32>())
        .fold(f32::MAX, f32::min);
    (1.0 / (1.0 + nearest / 64.0)).max(0.02)
}

pub struct Sender {
    config: SenderConfig,
    bandwidth: Bandwidth,
    sent: VecDeque<Sent>,
    newest_acked: Option<u32>,
    objects: HashMap<Key, Object>,
    keyframe_requested: bool,
    /// Every message is self-contained until the client acknowledges one.
    resetting: bool,
    reset_seq: Option<u32>,
    stats: SenderStats,
}

struct Candidate {
    index: usize,
    priority: f32,
    full: bool,
    root: bool,
    forced: bool,
}

/// A body dirty for about a second near a viewer may overdraw the budget
/// once per message, so complete refreshes cannot starve behind root updates.
const OVERDRAFT_PRIORITY: f32 = 30.0;

impl Sender {
    pub fn new(config: SenderConfig) -> Self {
        Self {
            bandwidth: Bandwidth::new(config.profile),
            config,
            sent: VecDeque::new(),
            newest_acked: None,
            objects: HashMap::new(),
            keyframe_requested: false,
            resetting: true,
            reset_seq: None,
            stats: SenderStats::default(),
        }
    }

    pub fn stats(&self) -> SenderStats {
        SenderStats {
            budget_bps: self.bandwidth.budget_bps(),
            rtt_ms: self.bandwidth.rtt_ms().unwrap_or(0),
            loss_percent: (self.bandwidth.loss() * 100.0) as f32,
            ..self.stats
        }
    }

    fn message(&self, seq: u32) -> Option<&Sent> {
        self.sent.iter().find(|m| m.seq == seq)
    }

    fn resolve_lost(&mut self, index: usize) {
        let message = &mut self.sent[index];
        if message.resolved {
            return;
        }
        message.resolved = true;
        let seq = message.seq;
        for entry in &message.entries {
            if let Some(object) = self.objects.get_mut(&entry.key) {
                object.pending.retain(|s| s.seq != seq);
            }
        }
        self.bandwidth.outcome(false);
        self.stats.lost_messages += 1;
    }

    fn resolve_delivered(&mut self, index: usize, now_ms: u64) {
        let message = &mut self.sent[index];
        if message.resolved {
            return;
        }
        message.resolved = true;
        let seq = message.seq;
        self.bandwidth.rtt_sample(now_ms.saturating_sub(message.sent_ms));
        self.bandwidth.outcome(true);
        for (n, entry) in message.entries.iter().enumerate() {
            let Some(object) = self.objects.get_mut(&entry.key) else { continue };
            object.pending.retain(|s| s.seq != seq);
            let stored = Stored {
                seq,
                entry: n as u32,
                tick: entry.tick,
                pose: entry.full.clone(),
                root: entry.root,
                velocity: entry.velocity.clone(),
            };
            if entry.full.is_some() && object.acked_full.as_ref().is_none_or(|s| newer(seq, s.seq)) {
                object.acked_full = Some(stored.clone());
            }
            if object.acked_root.as_ref().is_none_or(|s| newer(seq, s.seq)) {
                object.acked_root = Some(stored);
            }
        }
        if self.newest_acked.is_none_or(|s| newer(seq, s)) {
            self.newest_acked = Some(seq);
        }
        self.resetting = false;
    }

    /// Applies the client's cumulative acknowledgement from an input packet.
    pub fn ack(&mut self, latest: u32, mask: u32, flags: u8, now_ms: u64) {
        // A request is current only if the client already holds a message
        // from the latest reset; older requests are still in flight.
        if flags & ACK_KEYFRAME != 0 && !self.resetting && self.reset_seq.is_none_or(|r| !newer(r, latest)) {
            self.keyframe_requested = true;
        }
        if latest == 0 || self.message(latest).is_none() {
            return;
        }
        let received = |seq: u32| {
            let age = latest.wrapping_sub(seq);
            age == 0 || (1..=ACK_WINDOW).contains(&age) && mask & (1 << (age - 1)) != 0
        };
        for index in 0..self.sent.len() {
            let seq = self.sent[index].seq;
            if newer(seq, latest) {
                continue;
            }
            if received(seq) {
                self.resolve_delivered(index, now_ms);
            } else {
                self.resolve_lost(index);
            }
        }
        // Only the newest acknowledged view can become a baseline.
        if let Some(acked) = self.newest_acked {
            for message in &mut self.sent {
                if newer(acked, message.seq) {
                    message.view = None;
                }
            }
        }
    }

    fn expire(&mut self, sequence: u32) {
        while let Some(front) = self.sent.front() {
            if sequence.wrapping_sub(front.seq) < RING {
                break;
            }
            self.resolve_lost(0);
            self.sent.pop_front();
        }
        if self.newest_acked.is_some_and(|s| sequence.wrapping_sub(s) >= RING) {
            self.newest_acked = None;
        }
    }

    fn within_ring(&self, sequence: u32, seq: u32) -> bool {
        let age = sequence.wrapping_sub(seq);
        (1..RING).contains(&age) && self.message(seq).is_some()
    }

    /// Encodes one tick for this client, or skips it when the budget is spent.
    pub fn encode(&mut self, frame: &Frame<'_>) -> Result<Option<Emitted>, Error> {
        self.bandwidth.refill(frame.now_ms);
        self.expire(frame.sequence);
        // Game state goes out every tick, as the original sent player state
        // every tick: skipping one would delay the final postgame state and
        // unpair migration cuts from client receipts. The budget governs the
        // optional presentation stream (pose refreshes) and loss adaptation.
        let allowance = match self.bandwidth.allowance() {
            Some(allowance) => allowance,
            None => {
                self.bandwidth.limited();
                self.stats.over_budget += 1;
                0
            }
        };
        if self.keyframe_requested {
            self.keyframe_requested = false;
            self.resetting = true;
            self.reset_seq = Some(frame.sequence);
            self.objects.clear();
            self.newest_acked = None;
            // Nothing sent before the reset may become a baseline or reference.
            self.sent.clear();
            self.stats.resets += 1;
        }
        let reset = self.resetting;
        let base_seq = self.newest_acked.filter(|&s| {
            self.within_ring(frame.sequence, s) && self.message(s).is_some_and(|m| m.view.is_some())
        });
        let empty = View::default();
        let base = base_seq.and_then(|s| self.message(s));
        let base_view = base.and_then(|m| m.view.as_ref()).unwrap_or(&empty);
        let base_layout: Vec<Key> = base.map(|m| m.layout.clone()).unwrap_or_default();

        let exact = if self.config.profile.exact { exact_layout(frame) } else { None };
        let split = if self.config.profile.exact { None } else { split(frame) };
        let (generic_bytes, floats, anchors): (&[u8], &[u32], &[(u32, u64)]) = match (&split, &exact) {
            (Some(s), _) => (&s.generic, &s.floats, &s.anchors),
            (None, Some((floats, anchors))) => (frame.body, floats, anchors),
            (None, None) => (frame.body, frame.floats, frame.anchors),
        };
        let mut ge = Encoder::default();
        let input = generic::Input { bytes: generic_bytes, floats, anchors };
        let view = generic::encode(&mut ge, &input, base_view, frame.tick);
        let generic_stream = ge.finish();

        let mut header = Vec::with_capacity(32);
        let mut flags = 0u8;
        if base_seq.is_none() {
            flags |= FLAG_KEYFRAME;
        }
        if reset {
            flags |= FLAG_RESET;
        }
        if split.is_some() {
            flags |= FLAG_POSES;
        }
        let keys: Vec<Key> = split.as_ref().map(|s| s.bank.poses.iter().map(|p| p.key).collect()).unwrap_or_default();
        let digest = digest(&view.bytes, split.as_ref().map(|s| (s.offset, s.bank.authoritative, keys.as_slice())));
        header.push(flags);
        header.extend_from_slice(&frame.tick.to_le_bytes());
        header.push(base_seq.map_or(0, |s| frame.sequence.wrapping_sub(s) as u8));
        header.extend_from_slice(&digest[..4]);
        put_varint(&mut header, view.bytes.len() as u64);
        if let Some(s) = &split {
            put_varint(&mut header, s.offset as u64);
        }
        put_varint(&mut header, generic_stream.len() as u64);

        let mut entries = Vec::new();
        let mut pose_stream = Vec::new();
        if let Some(s) = &split {
            let fixed = header.len() + generic_stream.len();
            let (stream, chosen) = self.encode_poses(frame, s, &keys, &base_layout, fixed, allowance);
            pose_stream = stream;
            entries = chosen;
        }
        let mut message = header;
        message.extend_from_slice(&generic_stream);
        message.extend_from_slice(&pose_stream);
        if message.len() > MAX_MESSAGE {
            return Err(Error);
        }
        let chunk = self.config.chunk.max(1);
        let count = message.len().div_ceil(chunk).max(1);
        let count16 = u16::try_from(count).map_err(|_| Error)?;
        let packets: Vec<Vec<u8>> = (0..count)
            .map(|n| {
                let part = &message[n * chunk..((n + 1) * chunk).min(message.len())];
                let mut packet = Vec::with_capacity(FRAGMENT_HEADER + part.len());
                packet.extend_from_slice(&frame.sequence.to_le_bytes());
                packet.extend_from_slice(&(n as u16).to_le_bytes());
                packet.extend_from_slice(&count16.to_le_bytes());
                packet.extend_from_slice(part);
                packet
            })
            .collect();
        let wire = message.len() + count * (FRAGMENT_HEADER + self.config.overhead);
        self.bandwidth.charge(wire);
        self.stats.messages += 1;
        self.stats.wire_bytes += wire as u64;
        self.stats.packets += count as u64;
        if base_seq.is_none() {
            self.stats.keyframes += 1;
        }
        for (n, entry) in entries.iter().enumerate() {
            if let Some(object) = self.objects.get_mut(&entry.key) {
                object.pending.push(Stored {
                    seq: frame.sequence,
                    entry: n as u32,
                    tick: entry.tick,
                    pose: entry.full.clone(),
                    root: entry.root,
                    velocity: entry.velocity.clone(),
                });
            }
        }
        let view_len = (view.bytes.len() + if split.is_some() { pose::bank_len(&keys) } else { 0 }) as u32;
        self.sent.push_back(Sent {
            seq: frame.sequence,
            sent_ms: frame.now_ms,
            view: Some(view),
            layout: keys,
            entries,
            resolved: false,
        });
        Ok(Some(Emitted { digest, view_len, packets, keyframe: base_seq.is_none() }))
    }

    fn encode_poses(
        &mut self,
        frame: &Frame<'_>,
        split: &Split,
        keys: &[Key],
        base_layout: &[Key],
        fixed: usize,
        allowance: usize,
    ) -> (Vec<u8>, Vec<Entry>) {
        let current: Vec<Pose> = split.bank.poses.iter().map(pose::quantize).collect();
        let present: HashSet<Key> = keys.iter().copied().collect();
        self.objects.retain(|k, _| present.contains(k));
        let mut candidates: Vec<Candidate> = Vec::new();
        for (index, key) in keys.iter().enumerate() {
            let object = self.objects.entry(*key).or_default();
            if object.acked_full.is_none() {
                candidates.push(Candidate { index, priority: f32::INFINITY, full: true, root: false, forced: true });
                continue;
            }
            if frame.hot.contains(&key.object) && object.last_full().is_none_or(|p| pose::distance(p, &current[index]) != 0) {
                candidates.push(Candidate { index, priority: f32::INFINITY, full: true, root: false, forced: true });
                continue;
            }
            let weight = relevance(current[index].world(), frame.viewers, frame.own.contains(&key.object));
            let full_dirty = object.last_full().is_none_or(|p| pose::distance(p, &current[index]) != 0);
            let root_dirty = object.last_root().is_none_or(|r| pose::root_distance(r, &current[index].root()) != 0);
            object.priority_full = if full_dirty { object.priority_full + weight } else { 0.0 };
            object.priority_root = if root_dirty { object.priority_root + weight * 4.0 } else { 0.0 };
            if full_dirty || root_dirty {
                let priority = object.priority_full + object.priority_root;
                candidates.push(Candidate { index, priority, full: full_dirty, root: root_dirty, forced: false });
            }
        }
        candidates.sort_by(|a, b| b.priority.total_cmp(&a.priority));
        let unlimited = self.bandwidth.unlimited();
        let overdraft_limit = self.bandwidth.overdraft(allowance);
        let mut e = Encoder::default();
        let mut sm = StreamModels::default();
        let mut pm = pose::Models::default();
        encode_layout(&mut e, &mut sm, split.bank.authoritative, keys, base_layout);
        let mut entries: Vec<Entry> = Vec::new();
        let (mut full_open, mut overdraft) = (true, true);
        for candidate in candidates {
            let index = candidate.index;
            let key = keys[index];
            let mut accepted = None;
            if candidate.full && (full_open || candidate.forced) {
                let priority = self.objects.get(&key).map_or(0.0, |o| o.priority_full);
                let limit = if candidate.forced || unlimited {
                    usize::MAX
                } else if overdraft && priority >= OVERDRAFT_PRIORITY {
                    overdraft_limit
                } else {
                    allowance
                };
                let point = e.checkpoint();
                let (saved_sm, saved_pm) = (sm.clone(), pm.clone());
                match self.encode_full_entry(&mut e, &mut sm, &mut pm, frame, key, index, &current[index], &entries) {
                    Some(entry) if fixed.saturating_add(e.estimate()) <= limit => {
                        if fixed + e.estimate() > allowance && !candidate.forced {
                            overdraft = false;
                        }
                        accepted = Some(entry);
                    }
                    result => {
                        e.rollback(point);
                        sm = saved_sm;
                        pm = saved_pm;
                        if result.is_some() {
                            full_open = false;
                            self.bandwidth.limited();
                        }
                    }
                }
            }
            // Every moving body's root goes out every tick, like the original's
            // object positions: a few bytes each, and all a rigid object (a
            // falling barrel) needs. Only articulated refreshes are budgeted.
            if accepted.is_none() && candidate.root {
                let point = e.checkpoint();
                let (saved_sm, saved_pm) = (sm.clone(), pm.clone());
                match self.encode_root_entry(&mut e, &mut sm, &mut pm, frame, key, index, &current[index]) {
                    Some(entry) => accepted = Some(entry),
                    None => {
                        e.rollback(point);
                        sm = saved_sm;
                        pm = saved_pm;
                    }
                }
            }
            let object = self.objects.get_mut(&key).expect("present object");
            match accepted {
                Some(entry) => {
                    if entry.full.is_some() {
                        object.priority_full = 0.0;
                        object.priority_root = 0.0;
                        if candidate.forced {
                            self.stats.forced_refreshes += 1;
                        } else {
                            self.stats.full_refreshes += 1;
                        }
                    } else {
                        object.priority_root = 0.0;
                        self.stats.root_updates += 1;
                    }
                    entries.push(entry);
                }
                None => self.stats.held_refreshes += 1,
            }
        }
        e.bit(&mut sm.more, false);
        (e.finish(), entries)
    }

    #[allow(clippy::too_many_arguments)]
    fn encode_full_entry(
        &self,
        e: &mut Encoder,
        sm: &mut StreamModels,
        pm: &mut pose::Models,
        frame: &Frame<'_>,
        key: Key,
        index: usize,
        current: &Pose,
        local: &[Entry],
    ) -> Option<Entry> {
        let object = self.objects.get(&key)?;
        e.bit(&mut sm.more, true);
        sm.index.encode(e, index as u64);
        e.bit(&mut sm.root_kind, false);
        let same = object.acked_full.as_ref().filter(|s| self.within_ring(frame.sequence, s.seq));
        if let Some(stored) = same {
            let base = stored.pose.as_ref()?;
            encode_reference_kind(e, sm, REF_SAME);
            sm.age.encode(e, u64::from(frame.sequence.wrapping_sub(stored.seq) - 1));
            let elapsed = frame.tick.wrapping_sub(stored.tick);
            let reference = Reference { pose: base, velocity: stored.velocity.as_ref(), elapsed };
            let residuals = pose::encode_full(e, pm, current, Some(&reference));
            let velocity = (elapsed > 0).then_some(Velocity { residuals, span: elapsed });
            return Some(Entry { key, full: Some(current.clone()), root: current.root(), velocity, tick: frame.tick });
        }
        // Another body of the same model: an acknowledged one, or one earlier
        // in this message. Respawns and keyframes then cost a pose delta.
        let mut best: Option<(u64, u8, u32, u32)> = None;
        for (n, entry) in local.iter().enumerate() {
            if let Some(pose) = entry.full.as_ref().filter(|_| entry.key.nodes == key.nodes) {
                let score = similarity(pose, current);
                if best.is_none_or(|b| score < b.0) {
                    best = Some((score, REF_LOCAL, 0, n as u32));
                }
            }
        }
        let acknowledged = self
            .objects
            .iter()
            .filter(|(k, _)| k.nodes == key.nodes && **k != key)
            .filter_map(|(_, object)| object.acked_full.as_ref())
            .filter(|stored| self.within_ring(frame.sequence, stored.seq))
            .filter_map(|stored| stored.pose.as_ref().map(|pose| (stored, pose)))
            .take(16);
        for (stored, pose) in acknowledged {
            let score = similarity(pose, current);
            if best.is_none_or(|b| score < b.0) {
                best = Some((score, REF_CROSS, stored.seq, stored.entry));
            }
        }
        let residuals_target = Entry { key, full: Some(current.clone()), root: current.root(), velocity: None, tick: frame.tick };
        match best {
            Some((_, REF_LOCAL, _, n)) => {
                encode_reference_kind(e, sm, REF_LOCAL);
                sm.entry.encode(e, u64::from(n));
                let base = local[n as usize].full.as_ref()?;
                pose::encode_full(e, pm, current, Some(&Reference { pose: base, velocity: None, elapsed: 0 }));
            }
            Some((_, _, seq, n)) => {
                let base = self.message(seq)?.entries.get(n as usize)?.full.as_ref()?;
                encode_reference_kind(e, sm, REF_CROSS);
                sm.age.encode(e, u64::from(frame.sequence.wrapping_sub(seq) - 1));
                sm.entry.encode(e, u64::from(n));
                pose::encode_full(e, pm, current, Some(&Reference { pose: base, velocity: None, elapsed: 0 }));
            }
            None => {
                encode_reference_kind(e, sm, REF_NONE);
                pose::encode_full(e, pm, current, None);
            }
        }
        Some(residuals_target)
    }

    #[allow(clippy::too_many_arguments)]
    fn encode_root_entry(
        &self,
        e: &mut Encoder,
        sm: &mut StreamModels,
        pm: &mut pose::Models,
        frame: &Frame<'_>,
        key: Key,
        index: usize,
        current: &Pose,
    ) -> Option<Entry> {
        let object = self.objects.get(&key)?;
        let stored = object.acked_root.as_ref().filter(|s| self.within_ring(frame.sequence, s.seq))?;
        e.bit(&mut sm.more, true);
        sm.index.encode(e, index as u64);
        e.bit(&mut sm.root_kind, true);
        sm.age.encode(e, u64::from(frame.sequence.wrapping_sub(stored.seq) - 1));
        let elapsed = frame.tick.wrapping_sub(stored.tick);
        let root = current.root();
        let residuals = pose::encode_root(e, pm, &root, &stored.root, stored.velocity.as_ref(), elapsed);
        let velocity = (elapsed > 0).then_some(Velocity { residuals, span: elapsed });
        Some(Entry { key, full: None, root, velocity, tick: frame.tick })
    }
}

/// Fragments are stored sparsely: a declared count allocates nothing.
struct Assembly {
    seq: u32,
    count: u16,
    started_ms: u64,
    parts: std::collections::BTreeMap<u16, Vec<u8>>,
}

struct Message {
    seq: u32,
    view: View,
    poses: bool,
    layout: Vec<Key>,
    entries: Vec<Entry>,
}

impl Message {
    fn entry_for(&self, key: Key) -> Option<&Entry> {
        self.entries.iter().find(|e| e.key == key)
    }
    fn full_for(&self, key: Key) -> Option<&Entry> {
        self.entries.iter().find(|e| e.key == key && e.full.is_some())
    }
}

struct Cached {
    full_seq: u32,
    full: Pose,
    root_seq: u32,
    root: Root,
}

struct Pending {
    message: Message,
    reset: bool,
}

pub struct Delivered {
    pub sequence: u32,
    pub tick: u32,
    pub body: Vec<u8>,
    pub digest: [u8; 32],
    pub view_len: u32,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ReceiverStats {
    pub messages: u64,
    pub failures: u64,
    pub held_poses: u64,
    pub missing_poses: u64,
}

pub struct Receiver {
    chunk: usize,
    assemblies: Vec<Assembly>,
    ring: VecDeque<Message>,
    latest: Option<u32>,
    cache: HashMap<Key, Cached>,
    keyframe_needed: bool,
    pending: Option<Pending>,
    stats: ReceiverStats,
}

impl Receiver {
    pub fn new(chunk: usize) -> Self {
        Self {
            chunk: chunk.max(1),
            assemblies: Vec::new(),
            ring: VecDeque::new(),
            latest: None,
            cache: HashMap::new(),
            keyframe_needed: false,
            pending: None,
            stats: ReceiverStats::default(),
        }
    }

    pub fn stats(&self) -> ReceiverStats {
        self.stats
    }

    /// Fragment bytes held by incomplete messages.
    pub fn buffered_bytes(&self) -> usize {
        self.assemblies.iter().flat_map(|a| a.parts.values()).map(Vec::len).sum()
    }

    /// Drops every baseline and asks the host for a self-contained reset.
    pub fn request_reset(&mut self) {
        self.fail();
    }

    /// Forgets every baseline, e.g. after a host change.
    pub fn reset(&mut self) {
        *self = Self::new(self.chunk);
    }

    fn fail(&mut self) {
        self.stats.failures += 1;
        self.keyframe_needed = true;
        self.ring.clear();
        self.cache.clear();
        self.pending = None;
    }

    /// Cumulative acknowledgement for the next input packet.
    pub fn ack(&self) -> (u32, u32, u8) {
        let flags = if self.keyframe_needed { ACK_KEYFRAME } else { 0 };
        let Some(latest) = self.latest else { return (0, 0, flags) };
        let mut mask = 0u32;
        for message in &self.ring {
            let age = latest.wrapping_sub(message.seq);
            if (1..=ACK_WINDOW).contains(&age) {
                mask |= 1 << (age - 1);
            }
        }
        (latest, mask, flags)
    }

    fn find(&self, seq: u32) -> Option<&Message> {
        self.ring.iter().find(|m| m.seq == seq)
    }

    /// Accepts one fragment. A completed message is decoded but stays
    /// uncommitted until the native decoder accepts or rejects its body.
    pub fn push(&mut self, packet: &[u8], now_ms: u64) -> Result<Option<Delivered>, Error> {
        let mut cursor = Cursor { bytes: packet, at: 0 };
        let seq = cursor.u32()?;
        let index = u16::from_le_bytes([cursor.u8()?, cursor.u8()?]);
        let count = u16::from_le_bytes([cursor.u8()?, cursor.u8()?]);
        let data = &packet[FRAGMENT_HEADER..];
        let last = count.checked_sub(1).ok_or(Error)?;
        if index > last
            || usize::from(count) > MAX_MESSAGE.div_ceil(self.chunk)
            || data.is_empty()
            || data.len() > self.chunk
            || (index < last && data.len() != self.chunk)
        {
            return Err(Error);
        }
        if self.latest.is_some_and(|latest| !newer(seq, latest)) {
            return Ok(None);
        }
        self.pending = None;
        let latest = self.latest;
        self.assemblies.retain(|a| {
            now_ms.saturating_sub(a.started_ms) <= ASSEMBLY_TIMEOUT_MS && latest.is_none_or(|l| newer(a.seq, l))
        });
        let position = match self.assemblies.iter().position(|a| a.seq == seq) {
            Some(at) => at,
            None => {
                if self.assemblies.len() == MAX_ASSEMBLIES {
                    let oldest = (0..self.assemblies.len()).min_by_key(|&n| self.assemblies[n].started_ms).unwrap_or(0);
                    self.assemblies.remove(oldest);
                }
                self.assemblies.push(Assembly { seq, count, started_ms: now_ms, parts: Default::default() });
                self.assemblies.len() - 1
            }
        };
        let assembly = &mut self.assemblies[position];
        if assembly.count != count {
            self.assemblies.remove(position);
            return Err(Error);
        }
        match assembly.parts.get(&index) {
            Some(existing) if existing.as_slice() != data => {
                self.assemblies.remove(position);
                return Err(Error);
            }
            Some(_) => return Ok(None),
            None => {
                assembly.parts.insert(index, data.to_vec());
            }
        }
        if assembly.parts.len() != usize::from(assembly.count) {
            return Ok(None);
        }
        let assembly = self.assemblies.remove(position);
        let mut message = Vec::new();
        for part in assembly.parts.into_values() {
            message.extend_from_slice(&part);
        }
        match self.decode(seq, &message) {
            Ok((pending, delivered)) => {
                self.pending = Some(pending);
                self.stats.messages += 1;
                Ok(Some(delivered))
            }
            Err(error) => {
                self.fail();
                Err(error)
            }
        }
    }

    fn decode(&mut self, seq: u32, bytes: &[u8]) -> Result<(Pending, Delivered), Error> {
        let mut cursor = Cursor { bytes, at: 0 };
        let flags = cursor.u8()?;
        if flags & !(FLAG_KEYFRAME | FLAG_POSES | FLAG_RESET) != 0 {
            return Err(Error);
        }
        let tick = cursor.u32()?;
        let base_age = cursor.u8()?;
        let expected: [u8; 4] = cursor.take(4)?.try_into().map_err(|_| Error)?;
        let generic_len = usize::try_from(cursor.varint()?).map_err(|_| Error)?;
        if generic_len > MAX_BODY {
            return Err(Error);
        }
        let offset = if flags & FLAG_POSES != 0 {
            let offset = usize::try_from(cursor.varint()?).map_err(|_| Error)?;
            if offset > generic_len {
                return Err(Error);
            }
            Some(offset)
        } else {
            None
        };
        let generic_stream_len = usize::try_from(cursor.varint()?).map_err(|_| Error)?;
        let generic_stream = cursor.take(generic_stream_len)?;
        let pose_stream = &bytes[cursor.at..];
        if (base_age == 0) != (flags & FLAG_KEYFRAME != 0) || u32::from(base_age) >= RING {
            return Err(Error);
        }
        let base = if base_age == 0 { None } else { Some(self.find(seq.wrapping_sub(u32::from(base_age))).ok_or(Error)?) };
        let empty = View::default();
        let view = generic::decode(
            &mut Decoder::new(generic_stream),
            generic_len,
            base.map_or(&empty, |m| &m.view),
            tick,
        )?;
        let base_layout: &[Key] = base.map_or(&[], |m| &m.layout);
        let mut layout = Vec::new();
        let mut entries: Vec<Entry> = Vec::new();
        let mut authoritative = 0u8;
        if offset.is_some() {
            let mut d = Decoder::new(pose_stream);
            let mut sm = StreamModels::default();
            let mut pm = pose::Models::default();
            let (auth, keys) = decode_layout(&mut d, &mut sm, base_layout)?;
            authoritative = auth;
            layout = keys;
            let mut seen = HashSet::new();
            while d.bit(&mut sm.more) {
                if entries.len() >= 2 * layout.len() {
                    return Err(Error);
                }
                let index = usize::try_from(sm.index.decode(&mut d)?).map_err(|_| Error)?;
                let key = *layout.get(index).ok_or(Error)?;
                let root_only = d.bit(&mut sm.root_kind);
                if !seen.insert(key) {
                    return Err(Error);
                }
                let entry = if root_only {
                    let age = u32::try_from(sm.age.decode(&mut d)?).map_err(|_| Error)?.checked_add(1).ok_or(Error)?;
                    if age >= RING {
                        return Err(Error);
                    }
                    let source = self.find(seq.wrapping_sub(age)).and_then(|m| m.entry_for(key)).ok_or(Error)?;
                    let elapsed = tick.wrapping_sub(source.tick);
                    let (root, residuals) = pose::decode_root(&mut d, &mut pm, &source.root, source.velocity.as_ref(), elapsed)?;
                    let velocity = (elapsed > 0).then_some(Velocity { residuals, span: elapsed });
                    Entry { key, full: None, root, velocity, tick }
                } else {
                    let kind = decode_reference_kind(&mut d, &mut sm);
                    let (full, velocity) = match kind {
                        REF_SAME => {
                            let age = u32::try_from(sm.age.decode(&mut d)?).map_err(|_| Error)?.checked_add(1).ok_or(Error)?;
                            if age >= RING {
                                return Err(Error);
                            }
                            let source = self.find(seq.wrapping_sub(age)).and_then(|m| m.full_for(key)).ok_or(Error)?;
                            let base = source.full.as_ref().ok_or(Error)?;
                            let elapsed = tick.wrapping_sub(source.tick);
                            let reference = Reference { pose: base, velocity: source.velocity.as_ref(), elapsed };
                            let (pose, residuals) = pose::decode_full(&mut d, &mut pm, key.nodes, Some(&reference))?;
                            (pose, (elapsed > 0).then_some(Velocity { residuals, span: elapsed }))
                        }
                        REF_CROSS => {
                            let age = u32::try_from(sm.age.decode(&mut d)?).map_err(|_| Error)?.checked_add(1).ok_or(Error)?;
                            let n = usize::try_from(sm.entry.decode(&mut d)?).map_err(|_| Error)?;
                            if age >= RING {
                                return Err(Error);
                            }
                            let source = self.find(seq.wrapping_sub(age)).and_then(|m| m.entries.get(n)).ok_or(Error)?;
                            let base = source.full.as_ref().filter(|_| source.key.nodes == key.nodes).ok_or(Error)?;
                            let reference = Reference { pose: base, velocity: None, elapsed: 0 };
                            (pose::decode_full(&mut d, &mut pm, key.nodes, Some(&reference))?.0, None)
                        }
                        REF_LOCAL => {
                            let n = usize::try_from(sm.entry.decode(&mut d)?).map_err(|_| Error)?;
                            let source = entries.get(n).ok_or(Error)?;
                            let base = source.full.as_ref().filter(|_| source.key.nodes == key.nodes).ok_or(Error)?;
                            let reference = Reference { pose: base, velocity: None, elapsed: 0 };
                            (pose::decode_full(&mut d, &mut pm, key.nodes, Some(&reference))?.0, None)
                        }
                        _ => (pose::decode_full(&mut d, &mut pm, key.nodes, None)?.0, None),
                    };
                    Entry { key, root: full.root(), full: Some(full), velocity, tick }
                };
                entries.push(entry);
            }
        }
        let digest = digest(&view.bytes, offset.map(|o| (o, authoritative, layout.as_slice())));
        if digest[..4] != expected {
            return Err(Error);
        }
        let view_len = view.bytes.len() + offset.map_or(0, |_| pose::bank_len(&layout));
        if view_len > MAX_BODY {
            return Err(Error);
        }
        let mut body = Vec::with_capacity(view_len);
        if let Some(offset) = offset {
            body.extend_from_slice(&view.bytes[..offset]);
            self.materialize_bank(seq, authoritative, &layout, &entries, &mut body);
            body.extend_from_slice(&view.bytes[offset..]);
        } else {
            body.extend_from_slice(&view.bytes);
        }
        let message = Message { seq, view, poses: offset.is_some(), layout, entries };
        Ok((
            Pending { message, reset: flags & FLAG_RESET != 0 },
            Delivered { sequence: seq, tick, body, digest, view_len: u32::try_from(view_len).map_err(|_| Error)? },
        ))
    }

    fn materialize_bank(&mut self, seq: u32, authoritative: u8, layout: &[Key], entries: &[Entry], out: &mut Vec<u8>) {
        let count_at = out.len();
        out.push(authoritative);
        out.extend_from_slice(&[0, 0]);
        let mut included = 0u16;
        for key in layout {
            let full_entry = entries.iter().find(|e| e.key == *key && e.full.is_some());
            let root_entry = entries.iter().find(|e| e.key == *key && e.full.is_none());
            let cached = self.cache.get(key);
            let full = full_entry.and_then(|e| e.full.as_ref()).or(cached.map(|c| &c.full));
            let Some(full) = full else {
                self.stats.missing_poses += 1;
                continue;
            };
            let full_seq = if full_entry.is_some() { seq } else { cached.map_or(seq, |c| c.full_seq) };
            let root = match (root_entry, cached) {
                (Some(entry), _) => Some(&entry.root),
                (None, Some(c)) if full_entry.is_none() && newer(c.root_seq, full_seq) => Some(&c.root),
                _ => None,
            };
            if full_entry.is_none() {
                self.stats.held_poses += 1;
            }
            pose::materialize(*key, full, root, out);
            included += 1;
        }
        out[count_at + 1..count_at + 3].copy_from_slice(&included.to_le_bytes());
    }

    /// The native decoder accepted this body: it becomes a baseline/reference.
    pub fn commit(&mut self, seq: u32) -> bool {
        let Some(pending) = self.pending.take().filter(|p| p.message.seq == seq) else { return false };
        let message = pending.message;
        for entry in &message.entries {
            match &entry.full {
                Some(full) => {
                    self.cache.insert(
                        entry.key,
                        Cached { full_seq: seq, full: full.clone(), root_seq: seq, root: entry.root },
                    );
                }
                None => {
                    if let Some(cached) = self.cache.get_mut(&entry.key) {
                        cached.root_seq = seq;
                        cached.root = entry.root;
                    }
                }
            }
        }
        if message.poses {
            let present: HashSet<Key> = message.layout.iter().copied().collect();
            self.cache.retain(|k, _| present.contains(k));
        }
        if pending.reset {
            self.keyframe_needed = false;
        }
        self.latest = Some(seq);
        self.ring.push_back(message);
        while self.ring.front().is_some_and(|m| seq.wrapping_sub(m.seq) >= RING) {
            self.ring.pop_front();
        }
        true
    }

    /// The native decoder rejected the body: discard it and resynchronize.
    pub fn reject(&mut self, seq: u32) {
        if self.pending.as_ref().is_some_and(|p| p.message.seq == seq) {
            self.fail();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::netcode::generic::tests::{Lcg, World};
    use crate::netcode::pose::tests::body;

    const CHUNK: usize = 1200;

    struct Scene {
        world: World,
        bodies: Vec<(u32, [f32; 3], f32)>,
        statics: Vec<(u32, [f32; 3])>,
    }

    impl Scene {
        fn new(rng: &mut Lcg, players: usize) -> Self {
            Self {
                world: World::new(rng, players, 60),
                bodies: (0..players).map(|n| (0x0100_0000 | n as u32, [n as f32 * 3.0, 0.0, 0.0], 0.0)).collect(),
                statics: (0..10).map(|n| (0x0200_0000 | n as u32, [n as f32, 20.0, 0.0])).collect(),
            }
        }
        fn step(&mut self, rng: &mut Lcg) {
            self.world.step(rng);
            for body in &mut self.bodies {
                body.1[0] += 0.05;
                body.2 += 0.1;
            }
        }
        fn respawn(&mut self, n: usize, salt: u32) {
            self.bodies[n].0 = (salt << 16) | n as u32;
        }
        fn frame(&self) -> (Vec<u8>, Vec<u32>, (usize, usize)) {
            let (bytes, floats, _, poses) = self.anchored_frame();
            (bytes, floats, poses)
        }
        fn anchored_frame(&self) -> (Vec<u8>, Vec<u32>, Vec<(u32, u64)>, (usize, usize)) {
            let (mut bytes, floats, mut anchors) = self.world.encode_anchored();
            let mut bank = vec![1u8];
            let poses: Vec<_> = self
                .bodies
                .iter()
                .map(|&(object, center, phase)| body(object, 40, center, phase))
                .chain(self.statics.iter().map(|&(object, center)| body(object, 6, center, 0.0)))
                .collect();
            bank.extend_from_slice(&(poses.len() as u16).to_le_bytes());
            for pose in &poses {
                bank.extend_from_slice(&pose.key.object.to_le_bytes());
                bank.extend_from_slice(&(pose.nodes.len() as u16).to_le_bytes());
                for v in pose.sphere {
                    bank.extend_from_slice(&v.to_le_bytes());
                }
                for node in &pose.nodes {
                    for v in node {
                        bank.extend_from_slice(&v.to_le_bytes());
                    }
                }
            }
            let start = bytes.len();
            bytes.extend_from_slice(&bank);
            let end = bytes.len();
            anchors.push((end as u32, 9 << 40));
            bytes.extend_from_slice(b"contrail tail");
            (bytes, floats, anchors, (start, end))
        }
    }

    struct Link {
        sender: Sender,
        receiver: Receiver,
        sequence: u32,
        now: u64,
        hot: Vec<u32>,
    }

    impl Link {
        fn new(profile: Profile) -> Self {
            Self {
                sender: Sender::new(SenderConfig { profile, chunk: CHUNK, overhead: 80 }),
                receiver: Receiver::new(CHUNK),
                sequence: 0,
                now: 1_000,
                hot: Vec::new(),
            }
        }

        /// One tick: maybe emit, deliver unless dropped, then acknowledge.
        fn tick(&mut self, scene: &Scene, drop: bool) -> Option<Delivered> {
            self.now += 33;
            let (body, floats, anchors, poses) = scene.anchored_frame();
            let viewers = [[0.0f32, 0.0, 0.0]];
            let frame = Frame {
                body: &body,
                floats: &floats,
                anchors: &anchors,
                poses: Some(poses),
                tick: scene.world.tick,
                sequence: self.sequence + 1,
                now_ms: self.now,
                viewers: &viewers,
                own: &[scene.bodies[0].0],
                hot: &self.hot,
            };
            let emitted = self.sender.encode(&frame).unwrap();
            let mut delivered = None;
            if let Some(emitted) = emitted {
                self.sequence += 1;
                if !drop {
                    for packet in emitted.packets.iter().rev() {
                        if let Ok(Some(value)) = self.receiver.push(packet, self.now) {
                            delivered = Some(value);
                        }
                    }
                    if let Some(value) = &delivered {
                        assert_eq!(value.digest, emitted.digest);
                        assert_eq!(value.view_len, emitted.view_len);
                        assert_eq!(value.view_len as usize, body.len());
                        // Generic state is exact; the bank is presentation.
                        assert_eq!(value.body[..poses.0], body[..poses.0]);
                        assert_eq!(value.body[value.body.len() - 13..], body[body.len() - 13..]);
                        assert!(self.receiver.commit(value.sequence));
                    }
                }
            }
            let (latest, mask, flags) = self.receiver.ack();
            self.sender.ack(latest, mask, flags, self.now);
            delivered
        }
    }

    fn bank_count(body: &[u8], start: usize) -> u16 {
        u16::from_le_bytes([body[start + 1], body[start + 2]])
    }

    const QUANTIZED_UNLIMITED: Profile = Profile { exact: false, ..Profile::UNLIMITED };

    #[test]
    fn exact_profile_reconstructs_every_byte_including_poses() {
        let mut rng = Lcg(29);
        let mut scene = Scene::new(&mut rng, 8);
        let mut link = Link::new(Profile::UNLIMITED);
        let mut total = 0usize;
        for tick in 0..60 {
            scene.step(&mut rng);
            if tick == 30 {
                scene.respawn(2, 0x0555);
            }
            let delivered = link.tick(&scene, false).expect("every tick delivered");
            assert_eq!(delivered.body, scene.anchored_frame().0);
            total += 1;
        }
        assert_eq!(total, 60);
        assert_eq!(link.receiver.stats().held_poses, 0);
    }

    #[test]
    fn unlimited_stream_keeps_every_pose_fresh_and_state_exact() {
        let mut rng = Lcg(31);
        let mut scene = Scene::new(&mut rng, 8);
        let mut link = Link::new(QUANTIZED_UNLIMITED);
        for tick in 0..90 {
            scene.step(&mut rng);
            if tick == 40 {
                scene.respawn(3, 0x0777);
            }
            let delivered = link.tick(&scene, false).expect("every tick delivered");
            let (_, _, poses) = scene.frame();
            assert_eq!(bank_count(&delivered.body, poses.0), 18);
        }
        let stats = link.sender.stats();
        assert_eq!(stats.keyframes, 1);
        assert_eq!(link.receiver.stats().failures, 0);
    }

    #[test]
    fn original_budget_is_respected_and_poses_degrade_gracefully() {
        let mut rng = Lcg(37);
        let mut scene = Scene::new(&mut rng, 8);
        let mut link = Link::new(Profile::ORIGINAL);
        let mut delivered = 0;
        for tick in 0..900 {
            scene.step(&mut rng);
            if tick % 150 == 75 {
                scene.respawn(tick % 8, 0x0100 + tick as u32);
            }
            if let Some(value) = link.tick(&scene, false) {
                delivered += 1;
                let (_, _, poses) = scene.frame();
                // Every body remains present: held poses fill the bank.
                assert_eq!(bank_count(&value.body, poses.0), 18);
            }
        }
        let stats = link.sender.stats();
        eprintln!("original budget: delivered {delivered}/900 {stats:?} {:?}", link.receiver.stats());
        let seconds = 900.0 / 30.0;
        let rate = stats.wire_bytes as f64 * 8.0 / seconds;
        // Average within the probe ceiling plus the keyframe burst.
        assert!(rate < 71_680.0 * 1.15, "rate {rate} stats {stats:?}");
        // Game state reaches the client every tick; only poses are budgeted.
        assert_eq!(delivered, 900, "stats {stats:?}");
        // Roots are never held; only articulated refreshes wait for budget.
        assert_eq!(stats.held_refreshes, 0, "{stats:?}");
        assert!(stats.over_budget > 0, "{stats:?}");
        assert!(stats.root_updates > stats.full_refreshes, "{stats:?}");
        assert_eq!(link.receiver.stats().failures, 0);
    }

    #[test]
    fn every_moving_root_reaches_the_client_every_tick_under_the_budget() {
        let mut rng = Lcg(67);
        let mut scene = Scene::new(&mut rng, 8);
        let mut link = Link::new(Profile::ORIGINAL);
        for _ in 0..90 {
            scene.step(&mut rng);
            link.tick(&scene, false);
        }
        // Every body moves each tick; each tick carries every one of them.
        let before = link.sender.stats();
        for _ in 0..30 {
            scene.step(&mut rng);
            assert!(link.tick(&scene, false).is_some());
        }
        let after = link.sender.stats();
        let entries = (after.root_updates - before.root_updates) + (after.full_refreshes - before.full_refreshes)
            + (after.forced_refreshes - before.forced_refreshes);
        assert_eq!(entries, 30 * 8, "{after:?}");
    }

    #[test]
    fn bodies_playing_short_actions_refresh_every_tick_under_the_budget() {
        let mut rng = Lcg(61);
        let mut scene = Scene::new(&mut rng, 8);
        let mut link = Link::new(Profile::ORIGINAL);
        for _ in 0..90 {
            scene.step(&mut rng);
            link.tick(&scene, false);
        }
        // A distant body starts a melee: every tick must carry its full pose.
        link.hot = vec![scene.bodies[7].0];
        let before = link.sender.stats().forced_refreshes;
        for _ in 0..12 {
            scene.step(&mut rng);
            assert!(link.tick(&scene, false).is_some());
        }
        assert_eq!(link.sender.stats().forced_refreshes - before, 12);
        link.hot.clear();
        let before = link.sender.stats().forced_refreshes;
        for _ in 0..12 {
            scene.step(&mut rng);
            link.tick(&scene, false);
        }
        assert_eq!(link.sender.stats().forced_refreshes, before);
    }

    #[test]
    fn loss_reordering_and_resets_recover() {
        let mut rng = Lcg(41);
        let mut scene = Scene::new(&mut rng, 4);
        let mut link = Link::new(Profile::BROADBAND);
        let mut recovered_after_reset = false;
        for tick in 0..400 {
            scene.step(&mut rng);
            if tick % 60 == 30 {
                scene.respawn(tick % 4, 0x0300 + tick as u32);
            }
            let drop = rng.next() % 5 == 0;
            if tick == 200 {
                // Force a client-side failure: the next ack requests a reset.
                link.receiver.fail();
            }
            if link.tick(&scene, drop).is_some() && tick > 200 && link.receiver.ack().2 == 0 {
                recovered_after_reset = true;
            }
        }
        assert!(recovered_after_reset);
        assert!(link.sender.stats().lost_messages > 0);
    }

    #[test]
    fn duplicate_conflicting_and_stale_fragments_are_contained() {
        let mut rng = Lcg(43);
        let scene = Scene::new(&mut rng, 16);
        let mut sender = Sender::new(SenderConfig { profile: Profile::UNLIMITED, chunk: 256, overhead: 0 });
        let (body, floats, poses) = scene.frame();
        let frame = Frame { body: &body, floats: &floats, anchors: &[], poses: Some(poses), tick: 1, sequence: 1, now_ms: 0, viewers: &[], own: &[], hot: &[] };
        let emitted = sender.encode(&frame).unwrap().unwrap();
        assert!(emitted.packets.len() > 2);
        let mut receiver = Receiver::new(256);
        assert!(receiver.push(&emitted.packets[0], 0).unwrap().is_none());
        assert!(receiver.push(&emitted.packets[0], 0).unwrap().is_none());
        let mut forged = emitted.packets[1].clone();
        forged[FRAGMENT_HEADER] ^= 1;
        assert!(receiver.push(&forged, 0).unwrap().is_none());
        assert!(receiver.push(&emitted.packets[1], 0).is_err());
        let mut short = emitted.packets[0].clone();
        short.pop();
        assert!(receiver.push(&short, 0).is_err());
        let mut done = None;
        for packet in &emitted.packets {
            if let Some(value) = receiver.push(packet, 0).unwrap() {
                done = Some(value);
            }
        }
        let done = done.unwrap();
        assert!(receiver.commit(done.sequence));
        assert!(receiver.push(&emitted.packets[0], 0).unwrap().is_none());
        assert_eq!(receiver.ack(), (1, 0, 0));
    }

    #[test]
    fn missing_pose_bank_falls_back_to_generic_coding() {
        let mut rng = Lcg(47);
        let scene = Scene::new(&mut rng, 2);
        let (body, floats, _) = scene.frame();
        let mut sender = Sender::new(SenderConfig { profile: Profile::UNLIMITED, chunk: CHUNK, overhead: 0 });
        let mut receiver = Receiver::new(CHUNK);
        for (seq, poses) in [(1, None), (2, Some((5, 3)))] {
            let frame = Frame { body: &body, floats: &floats, anchors: &[], poses, tick: seq, sequence: seq, now_ms: 0, viewers: &[], own: &[], hot: &[] };
            let emitted = sender.encode(&frame).unwrap().unwrap();
            let mut delivered = None;
            for packet in &emitted.packets {
                delivered = receiver.push(packet, 0).unwrap().or(delivered);
            }
            let delivered = delivered.unwrap();
            assert_eq!(delivered.body, body);
            assert!(receiver.commit(seq));
            let (latest, mask, flags) = receiver.ack();
            sender.ack(latest, mask, flags, 0);
        }
    }

    /// Replays bodies captured from a native host (`H2_NET_CORPUS` directory of
    /// `p<slot>_<tick>.bin`: body, anchors, pose range, floats) through the
    /// stream with every message acknowledged, and reports payload sizes.
    #[test]
    #[ignore]
    fn captured_corpus_payload() {
        let Ok(dir) = std::env::var("H2_NET_CORPUS") else { return };
        let mut by_peer: std::collections::BTreeMap<u32, Vec<(u32, std::path::PathBuf)>> = Default::default();
        for entry in std::fs::read_dir(&dir).unwrap() {
            let path = entry.unwrap().path();
            let name = path.file_stem().unwrap().to_string_lossy().to_string();
            let Some((peer, tick)) = name.trim_start_matches('p').split_once('_') else { continue };
            by_peer.entry(peer.parse().unwrap()).or_default().push((tick.parse().unwrap(), path));
        }
        let word = |b: &[u8], at: &mut usize, n: usize| {
            let v = (0..n).fold(0u64, |v, k| v | (u64::from(b[*at + k]) << (8 * k)));
            *at += n;
            v
        };
        for (peer, mut files) in by_peer {
            files.sort();
            let mut link = Link::new(QUANTIZED_UNLIMITED);
            let (mut total, mut count) = (0usize, 0usize);
            for (tick, path) in &files {
                let b = std::fs::read(path).unwrap();
                let mut at = 0;
                let len = word(&b, &mut at, 4) as usize;
                let body = b[4..4 + len].to_vec();
                at = 4 + len;
                let anchors: Vec<(u32, u64)> =
                    (0..word(&b, &mut at, 4)).map(|_| (word(&b, &mut at, 4) as u32, word(&b, &mut at, 8))).collect();
                let poses = (word(&b, &mut at, 4) as usize, word(&b, &mut at, 4) as usize);
                let floats: Vec<u32> = (0..word(&b, &mut at, 4)).map(|_| word(&b, &mut at, 4) as u32).collect();
                link.now += 33;
                let frame = Frame {
                    body: &body,
                    floats: &floats,
                    anchors: &anchors,
                    poses: (poses.1 > poses.0).then_some(poses),
                    tick: *tick,
                    sequence: link.sequence + 1,
                    now_ms: link.now,
                    viewers: &[],
                    own: &[],
                    hot: &[],
                };
                let emitted = link.sender.encode(&frame).unwrap().unwrap();
                link.sequence += 1;
                let mut delivered = None;
                for packet in &emitted.packets {
                    delivered = link.receiver.push(packet, link.now).unwrap().or(delivered);
                }
                let delivered = delivered.unwrap();
                assert!(link.receiver.commit(delivered.sequence));
                let (latest, mask, flags) = link.receiver.ack();
                link.sender.ack(latest, mask, flags, link.now);
                if count > 0 || total > 0 {
                    total += emitted.packets.iter().map(|p| p.len() - FRAGMENT_HEADER).sum::<usize>();
                }
                count += 1;
            }
            eprintln!("peer {peer}: {} ticks, average payload {} bytes after the keyframe", count, total / (count - 1).max(1));
        }
    }

    /// Cost of one host encode plus client decode for a full-map-sized body:
    /// `cargo test --release -- --ignored --nocapture encode_cost`.
    #[test]
    #[ignore]
    fn encode_cost_for_large_mostly_static_bodies() {
        let mut rng = Lcg(59);
        let mut scene = Scene::new(&mut rng, 16);
        scene.world.objects.extend((0..6000).map(|n| (0xF000_0000 | n, [n as f32; 6])));
        let mut link = Link::new(Profile::UNLIMITED);
        link.tick(&scene, false).unwrap();
        let (body, _, _) = scene.frame();
        let started = std::time::Instant::now();
        let ticks = 300;
        for _ in 0..ticks {
            scene.step(&mut rng);
            link.tick(&scene, false).unwrap();
        }
        let per_tick = started.elapsed() / ticks;
        let stats = link.sender.stats();
        eprintln!(
            "body {} bytes: {:?} per tick (encode, decode, materialize, scene), {} wire bytes/message",
            body.len(),
            per_tick,
            stats.wire_bytes / stats.messages
        );
    }

    #[test]
    fn rejected_bodies_request_and_receive_a_reset() {
        let mut rng = Lcg(53);
        let mut scene = Scene::new(&mut rng, 4);
        let mut link = Link::new(Profile::UNLIMITED);
        link.tick(&scene, false).unwrap();
        scene.step(&mut rng);
        // Native rejection of the next delivered body.
        link.now += 33;
        let (body, floats, poses) = scene.frame();
        let frame = Frame { body: &body, floats: &floats, anchors: &[], poses: Some(poses), tick: scene.world.tick, sequence: 2, now_ms: link.now, viewers: &[], own: &[], hot: &[] };
        link.sequence = 2;
        let emitted = link.sender.encode(&frame).unwrap().unwrap();
        let mut delivered = None;
        for packet in &emitted.packets {
            delivered = link.receiver.push(packet, link.now).unwrap().or(delivered);
        }
        link.receiver.reject(delivered.unwrap().sequence);
        assert_eq!(link.receiver.ack().2, ACK_KEYFRAME);
        let (latest, mask, flags) = link.receiver.ack();
        link.sender.ack(latest, mask, flags, link.now);
        let mut recovered = false;
        for _ in 0..4 {
            scene.step(&mut rng);
            if link.tick(&scene, false).is_some() && link.receiver.ack().2 == 0 {
                recovered = true;
                break;
            }
        }
        assert!(recovered);
        assert_eq!(link.sender.stats().resets, 1);
        for _ in 0..30 {
            scene.step(&mut rng);
            assert!(link.tick(&scene, false).is_some());
        }
        assert_eq!(link.sender.stats().resets, 1);
    }
}
