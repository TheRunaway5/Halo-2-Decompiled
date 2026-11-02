//! Complete reliable authority byte exchange over authenticated physical peers.
//! This is the existing replacement wire, not recovered original Live/XNet.
//! Checksum/byte completion never grants native validation or prepares a world.
pub use crate::peer_source_admissions::Context;
use crate::{migration_preparation::PreparationCut as Cut, peer_pacing::PeerPollCursor};
use sha2::{Digest, Sha256};
use std::{
    collections::{BTreeSet, VecDeque},
    sync::{Arc, Mutex, MutexGuard},
};

const CHUNK: usize = 1024;
const HEADER: usize = 65;
const PIPELINE: u32 = 128;
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Invalid peer authority transfer")
    }
}
impl std::error::Error for Error {}
pub type Result<T> = std::result::Result<T, Error>;
fn need(v: bool) -> Result<()> {
    if v { Ok(()) } else { Err(Error) }
}
#[derive(Clone, Copy, Debug)]
pub struct Limits {
    pub maximum_bytes: u64,
    pub bytes_per_second: u64,
    pub burst_bytes: u64,
    pub retry_ms: u64,
    pub offer_ms: u64,
    pub idle_ms: u64,
    pub lifetime_ms: u64,
    pub window_chunks: u32,
    pub features: u32,
}
impl Default for Limits {
    fn default() -> Self {
        Self {
            maximum_bytes: 32 * 1024 * 1024,
            bytes_per_second: 512 * 1024,
            burst_bytes: 16 * 1024,
            retry_ms: 250,
            offer_ms: 500,
            idle_ms: 15000,
            lifetime_ms: 300000,
            window_chunks: 64,
            features: 0,
        }
    }
}
#[derive(Debug, PartialEq, Eq)]
pub struct Packet {
    pub peer: u32,
    pub bytes: Vec<u8>,
}
#[derive(Debug, PartialEq, Eq)]
pub struct Received {
    pub peer: u32,
    pub context: Context,
    pub seal: Cut,
    pub bytes: Vec<u8>,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Validated {
    pub peer: u32,
    pub context: Context,
    pub seal: Cut,
}
#[derive(Clone, Copy)]
enum Kind {
    Hello = 0,
    HelloAck,
    Offer,
    Chunk,
    Ack,
    Ready,
    Reject,
}
fn member(c: Context, p: u32) -> bool {
    p < 16 && c.peers & (1u16 << p) != 0
}
fn valid(self_peer: u32, c: Context) -> Result<()> {
    need(
        member(c, self_peer)
            && member(c, c.host)
            && c.peers.count_ones() >= 1
            && c.session.iter().any(|b| *b != 0),
    )
}
fn elapsed(now: u64, then: u64) -> u64 {
    now.saturating_sub(then)
}
fn chunks(bytes: u64) -> usize {
    bytes.div_ceil(CHUNK as u64) as usize
}
fn put(b: &mut Vec<u8>, v: u64, n: usize) {
    b.extend_from_slice(&v.to_le_bytes()[..n]);
}
fn header(protocol: u16, c: Context, target: u32, kind: Kind, id: u64) -> Vec<u8> {
    let mut b = Vec::with_capacity(1100);
    b.extend_from_slice(b"H2N2");
    put(&mut b, protocol.into(), 2);
    b.extend_from_slice(&[7, 1, kind as u8]);
    b.extend_from_slice(&c.session);
    put(&mut b, c.epoch.into(), 4);
    put(&mut b, c.membership_revision, 8);
    put(&mut b, c.peers.into(), 2);
    put(&mut b, c.host.into(), 1);
    put(&mut b, target.into(), 1);
    put(&mut b, id, 8);
    b
}
fn put_seal(b: &mut Vec<u8>, s: Cut) {
    put(b, s.tick, 8);
    put(b, s.bytes, 8);
    b.extend_from_slice(&s.digest);
}
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl<'a> Reader<'a> {
    fn raw(&mut self, n: usize) -> Result<&'a [u8]> {
        let end = self.at.checked_add(n).ok_or(Error)?;
        let out = self.bytes.get(self.at..end).ok_or(Error)?;
        self.at = end;
        Ok(out)
    }
    fn get(&mut self, n: usize) -> Result<u64> {
        let b = self.raw(n)?;
        let mut v = [0; 8];
        v[..n].copy_from_slice(b);
        Ok(u64::from_le_bytes(v))
    }
    fn done(&self) -> bool {
        self.at == self.bytes.len()
    }
    fn seal(&mut self, id: u64) -> Result<Cut> {
        Ok(Cut {
            id,
            tick: self.get(8)?,
            bytes: self.get(8)?,
            digest: self.raw(32)?.try_into().map_err(|_| Error)?,
        })
    }
}
fn zeros<T: Default + Clone>(n: usize) -> Result<Vec<T>> {
    let mut v = Vec::new();
    v.try_reserve_exact(n).map_err(|_| Error)?;
    v.resize(n, T::default());
    Ok(v)
}
struct Sender {
    seal: Cut,
    bytes: Option<Arc<Vec<u8>>>,
    sent: Vec<u8>,
    acked: Vec<u8>,
    sent_at: Vec<u64>,
    begin: usize,
    scan: usize,
    sent_count: usize,
    started: u64,
    progress: u64,
    offered_at: u64,
    offered: bool,
    validated: bool,
    failed: bool,
}
impl Sender {
    fn new(seal: Cut, bytes: Arc<Vec<u8>>, now: u64) -> Result<Self> {
        let n = chunks(seal.bytes);
        Ok(Self {
            seal,
            bytes: Some(bytes),
            sent: zeros(n)?,
            acked: zeros(n)?,
            sent_at: zeros(n)?,
            begin: 0,
            scan: 0,
            sent_count: 0,
            started: now,
            progress: now,
            offered_at: 0,
            offered: false,
            validated: false,
            failed: false,
        })
    }
}
struct Retired {
    seal: Cut,
    started: u64,
    progress: u64,
    failed: bool,
}
struct Incoming {
    seal: Cut,
    bytes: Vec<u8>,
    received: Vec<u8>,
    dirty: BTreeSet<usize>,
    count: usize,
    started: u64,
    progress: u64,
    hashing: bool,
    complete: bool,
    ack_all: bool,
}
struct State {
    binding: Context,
    capabilities: [u64; 16],
    features: [u32; 16],
    hello_at: [u64; 16],
    hello_sent: [bool; 16],
    hello_reply: [bool; 16],
    next_id: u64,
    last_tick: u64,
    senders: [Option<Sender>; 16],
    retiring: [Option<Retired>; 16],
    incoming: Option<Incoming>,
    highest_id: u64,
    highest_tick: u64,
    offer_at: u64,
    accepted_offer: bool,
    completed: Option<Received>,
    delivered: VecDeque<Cut>,
    validated: Option<Cut>,
    ready_sent: bool,
    ready_at: u64,
    rejected: Option<u64>,
    receipts: VecDeque<Validated>,
    cursor: PeerPollCursor,
    credit: f64,
    credit_at: u64,
    credit_started: bool,
}
impl State {
    fn new(binding: Context, burst: u64) -> Self {
        Self {
            binding,
            capabilities: [0; 16],
            features: [0; 16],
            hello_at: [0; 16],
            hello_sent: [false; 16],
            hello_reply: [false; 16],
            next_id: 1,
            last_tick: 0,
            senders: std::array::from_fn(|_| None),
            retiring: std::array::from_fn(|_| None),
            incoming: None,
            highest_id: 0,
            highest_tick: 0,
            offer_at: 0,
            accepted_offer: false,
            completed: None,
            delivered: VecDeque::new(),
            validated: None,
            ready_sent: false,
            ready_at: 0,
            rejected: None,
            receipts: VecDeque::new(),
            cursor: PeerPollCursor::default(),
            credit: burst as f64,
            credit_at: 0,
            credit_started: false,
        }
    }
    fn reset(&mut self, c: Context, burst: u64) {
        self.binding = c;
        self.capabilities.fill(0);
        self.features.fill(0);
        self.hello_sent.fill(false);
        self.hello_reply.fill(false);
        self.senders = std::array::from_fn(|_| None);
        self.retiring = std::array::from_fn(|_| None);
        self.incoming = None;
        self.completed = None;
        self.delivered.clear();
        self.validated = None;
        self.rejected = None;
        self.receipts.clear();
        self.next_id = 1;
        self.last_tick = 0;
        self.highest_id = 0;
        self.highest_tick = 0;
        self.accepted_offer = false;
        self.ready_sent = false;
        self.credit_started = false;
        self.credit = burst as f64;
        // Preserve the exact native poll cursor and unused clock fields.
    }
    fn expire(&mut self, now: u64, l: Limits) {
        if self.incoming.as_ref().is_some_and(|v| {
            !v.complete
                && !v.hashing
                && (elapsed(now, v.progress) > l.idle_ms || elapsed(now, v.started) > l.lifetime_ms)
        }) {
            self.incoming = None;
        }
        for v in self.senders.iter_mut().flatten() {
            if !v.validated
                && !v.failed
                && (elapsed(now, v.progress) > l.idle_ms || elapsed(now, v.started) > l.lifetime_ms)
            {
                v.failed = true;
                v.bytes = None;
            }
        }
        for v in self.retiring.iter_mut().flatten() {
            if elapsed(now, v.progress) > l.idle_ms || elapsed(now, v.started) > l.lifetime_ms {
                v.failed = true;
            }
        }
    }
    fn receipt(&mut self, v: Validated) {
        if self.receipts.len() == 32 {
            self.receipts.pop_front();
        }
        self.receipts.push_back(v);
    }
    fn emit(&mut self, out: &mut Vec<Packet>, peer: u32, bytes: Vec<u8>) {
        out.push(Packet { peer, bytes });
        assert!(self.cursor.record_served(peer as u8));
    }
}
/// All mutable protocol state is synchronized here. Publication and completion
/// hashing run outside this mutex, with exact context/seal rechecks afterward.
pub struct Transfer {
    self_peer: u32,
    protocol: u16,
    limits: Limits,
    state: Mutex<State>,
}
impl Transfer {
    pub fn new(self_peer: u32, protocol: u16, c: Context, l: Limits) -> Result<Self> {
        valid(self_peer, c)?;
        need(
            l.maximum_bytes > 0
                && l.maximum_bytes <= 512 * 1024 * 1024
                && l.bytes_per_second > 0
                && l.bytes_per_second <= 1024 * 1024 * 1024
                && l.burst_bytes >= 1100
                && l.burst_bytes <= 1024 * 1024
                && l.retry_ms >= 10
                && l.offer_ms >= l.retry_ms
                && l.idle_ms >= l.offer_ms
                && l.lifetime_ms >= l.idle_ms
                && l.window_chunks > 0
                && l.window_chunks <= 256,
        )?;
        Ok(Self {
            self_peer,
            protocol,
            limits: l,
            state: Mutex::new(State::new(c, l.burst_bytes)),
        })
    }
    fn lock(&self) -> Result<MutexGuard<'_, State>> {
        self.state.lock().map_err(|_| Error)
    }
    pub fn context(&self) -> Result<Context> {
        Ok(self.lock()?.binding)
    }
    pub fn replace_context(&self, expected: Context, next: Context) -> Result<bool> {
        valid(self.self_peer, next)?;
        let mut s = self.lock()?;
        if s.binding != expected
            || next.session != expected.session
            || next.epoch < expected.epoch
            || (next.epoch == expected.epoch
                && (next.host != expected.host
                    || next.membership_revision <= expected.membership_revision))
        {
            return Ok(false);
        }
        s.reset(next, self.limits.burst_bytes);
        Ok(true)
    }
    pub fn supported(&self, peer: u32) -> Result<bool> {
        let s = self.lock()?;
        Ok(peer < 16 && s.capabilities[peer as usize] != 0)
    }
    pub fn feature_supported(&self, peer: u32, feature: u32) -> Result<bool> {
        let s = self.lock()?;
        Ok(peer < 16
            && feature != 0
            && s.capabilities[peer as usize] != 0
            && s.features[peer as usize] & feature == feature)
    }
    pub fn feature_snapshot(&self) -> Result<(Context, [u32; 16])> {
        let s = self.lock()?;
        Ok((
            s.binding,
            std::array::from_fn(|p| {
                if s.capabilities[p] != 0 {
                    s.features[p]
                } else {
                    0
                }
            }),
        ))
    }
    pub fn publish(
        &self,
        expected: Context,
        tick: u64,
        archive: Vec<u8>,
        targets: &[u32],
        now: u64,
    ) -> Result<Cut> {
        need(
            !archive.is_empty()
                && archive.len() as u64 <= self.limits.maximum_bytes
                && !targets.is_empty()
                && targets.len() <= 15,
        )?;
        let digest = Sha256::digest(&archive).into();
        let owned = Arc::new(archive);
        let mut s = self.lock()?;
        need(
            s.binding == expected
                && self.self_peer == s.binding.host
                && tick >= s.last_tick
                && s.next_id != u64::MAX,
        )?;
        let mut selected = 0u16;
        for &peer in targets {
            need(
                peer != self.self_peer
                    && member(s.binding, peer)
                    && s.capabilities[peer as usize] >= owned.len() as u64
                    && selected & (1u16 << peer) == 0,
            )?;
            selected |= 1u16 << peer;
        }
        let seal = Cut {
            id: s.next_id,
            tick,
            bytes: owned.len() as u64,
            digest,
        };
        s.next_id += 1;
        s.last_tick = tick;
        s.senders = std::array::from_fn(|_| None);
        s.retiring = std::array::from_fn(|_| None);
        for &peer in targets {
            s.senders[peer as usize] = Some(Sender::new(seal, owned.clone(), now)?);
        }
        Ok(seal)
    }
    pub fn delivered_to(&self, c: Context, seal: Cut, peer: u32) -> Result<bool> {
        let s = self.lock()?;
        if s.binding != c || self.self_peer != c.host || peer >= 16 {
            return Ok(false);
        }
        Ok(s.senders[peer as usize]
            .as_ref()
            .is_some_and(|v| !v.failed && v.seal == seal && v.begin == v.acked.len()))
    }
    pub fn failed_to(&self, c: Context, seal: Cut, peer: u32, now: u64) -> Result<bool> {
        let mut s = self.lock()?;
        if s.binding != c
            || self.self_peer != c.host
            || peer >= 16
            || peer == self.self_peer
            || !member(c, peer)
        {
            return Ok(false);
        }
        let p = peer as usize;
        let exact_sender = s.senders[p].as_ref().is_some_and(|v| v.seal == seal);
        let exact_retiring = s.retiring[p].as_ref().is_some_and(|v| v.seal == seal);
        if !exact_sender && !exact_retiring {
            return Ok(false);
        }
        s.expire(now, self.limits);
        Ok((exact_sender
            && s.senders[p]
                .as_ref()
                .is_some_and(|v| v.failed && !v.validated))
            || (exact_retiring && s.retiring[p].as_ref().is_some_and(|v| v.failed)))
    }
    pub fn note_native_preparation(
        &self,
        c: Context,
        seal: Cut,
        peer: u32,
        now: u64,
    ) -> Result<bool> {
        let mut s = self.lock()?;
        s.expire(now, self.limits);
        if s.binding != c
            || self.self_peer != c.host
            || peer >= 16
            || peer == self.self_peer
            || !member(c, peer)
        {
            return Ok(false);
        }
        let p = peer as usize;
        if let Some(v) = s.senders[p].as_mut() {
            if !v.failed
                && !v.validated
                && v.seal == seal
                && v.sent_count == v.sent.len()
                && v.begin == v.acked.len()
            {
                v.progress = now;
                return Ok(true);
            }
        }
        if let Some(v) = s.retiring[p].as_mut() {
            if !v.failed && v.seal == seal {
                v.progress = now;
                return Ok(true);
            }
        }
        Ok(false)
    }
    pub fn publish_ahead(
        &self,
        c: Context,
        previous: Cut,
        tick: u64,
        archive: Vec<u8>,
        peer: u32,
        now: u64,
    ) -> Result<Cut> {
        need(!archive.is_empty() && archive.len() as u64 <= self.limits.maximum_bytes)?;
        let digest = Sha256::digest(&archive).into();
        let owned = Arc::new(archive);
        let mut next = Sender::new(
            Cut {
                id: 0,
                tick,
                bytes: owned.len() as u64,
                digest,
            },
            owned,
            now,
        )?;
        let mut s = self.lock()?;
        s.expire(now, self.limits);
        need(
            s.binding == c
                && self.self_peer == c.host
                && peer < 16
                && peer != self.self_peer
                && member(c, peer)
                && self.limits.features & PIPELINE != 0
                && s.features[peer as usize] & PIPELINE != 0
                && s.capabilities[peer as usize] >= next.seal.bytes
                && tick >= s.last_tick
                && s.next_id != u64::MAX
                && s.retiring[peer as usize].is_none(),
        )?;
        let p = peer as usize;
        let old = s.senders[p].as_ref().ok_or(Error)?;
        need(!old.failed && old.seal == previous && old.begin == old.acked.len())?;
        let retired = (!old.validated).then_some(Retired {
            seal: old.seal,
            started: old.started,
            progress: old.progress,
            failed: false,
        });
        next.seal.id = s.next_id;
        s.next_id += 1;
        s.last_tick = tick;
        let seal = next.seal;
        if retired.is_some() {
            s.retiring[p] = retired;
        }
        s.senders[p] = Some(next);
        Ok(seal)
    }
    pub fn poll(&self, now: u64, budget: u32) -> Result<Vec<Packet>> {
        let mut s = self.lock()?;
        s.expire(now, self.limits);
        let budget = budget.min(64) as usize;
        let l = self.limits;
        if s.credit_started {
            s.credit = (l.burst_bytes as f64).min(
                s.credit
                    + elapsed(now, s.credit_at).min(1000) as f64 * l.bytes_per_second as f64
                        / 1000.0,
            );
        }
        s.credit_started = true;
        s.credit_at = now;
        let mut out = Vec::new();
        while out.len() < budget {
            let mut progress = false;
            let order = s.cursor.order();
            for raw_peer in order {
                if out.len() >= budget {
                    break;
                }
                let peer = u32::from(raw_peer);
                let p = peer as usize;
                if peer == self.self_peer || !member(s.binding, peer) {
                    continue;
                }
                if s.hello_reply[p] {
                    let mut b = header(self.protocol, s.binding, peer, Kind::HelloAck, 0);
                    put(&mut b, l.maximum_bytes, 8);
                    if l.features != 0 {
                        put(&mut b, l.features.into(), 4);
                    }
                    s.emit(&mut out, peer, b);
                    s.hello_reply[p] = false;
                    progress = true;
                    continue;
                }
                if s.capabilities[p] == 0
                    && (!s.hello_sent[p] || elapsed(now, s.hello_at[p]) >= 1000)
                {
                    let mut b = header(self.protocol, s.binding, peer, Kind::Hello, 0);
                    put(&mut b, l.maximum_bytes, 8);
                    if l.features != 0 {
                        put(&mut b, l.features.into(), 4);
                    }
                    s.emit(&mut out, peer, b);
                    s.hello_sent[p] = true;
                    s.hello_at[p] = now;
                    progress = true;
                    continue;
                }
                if peer == s.binding.host && self.self_peer != peer {
                    if let Some(id) = s.rejected {
                        let b = header(self.protocol, s.binding, peer, Kind::Reject, id);
                        s.emit(&mut out, peer, b);
                        s.rejected = None;
                        progress = true;
                        continue;
                    }
                    if s.incoming.as_ref().is_some_and(|v| v.ack_all) {
                        let id = s.incoming.as_ref().unwrap().seal.id;
                        let mut b = header(self.protocol, s.binding, peer, Kind::Ack, id);
                        put(&mut b, u32::MAX.into(), 4);
                        put(&mut b, 0, 8);
                        s.emit(&mut out, peer, b);
                        s.incoming.as_mut().unwrap().ack_all = false;
                        progress = true;
                        continue;
                    }
                    if s.incoming.as_ref().is_some_and(|v| !v.dirty.is_empty()) {
                        let incoming = s.incoming.as_mut().unwrap();
                        let base = incoming.dirty.pop_first().unwrap();
                        let mut bits = 0u64;
                        for i in 0..64 {
                            if base + i < incoming.received.len()
                                && incoming.received[base + i] != 0
                            {
                                bits |= 1u64 << i;
                            }
                        }
                        let id = incoming.seal.id;
                        let mut b = header(self.protocol, s.binding, peer, Kind::Ack, id);
                        put(&mut b, base as u64, 4);
                        put(&mut b, bits, 8);
                        s.emit(&mut out, peer, b);
                        progress = true;
                        continue;
                    }
                    if s.validated.is_some()
                        && (!s.ready_sent || elapsed(now, s.ready_at) >= l.offer_ms)
                    {
                        let seal = s.validated.unwrap();
                        let mut b = header(self.protocol, s.binding, peer, Kind::Ready, seal.id);
                        put_seal(&mut b, seal);
                        s.emit(&mut out, peer, b);
                        s.ready_sent = true;
                        s.ready_at = now;
                        progress = true;
                        continue;
                    }
                }
                let c = s.binding;
                let credit = s.credit;
                let Some(sender) = s.senders[p].as_mut() else {
                    continue;
                };
                if sender.failed || sender.validated {
                    continue;
                }
                if !sender.offered || elapsed(now, sender.offered_at) >= l.offer_ms {
                    let mut b = header(self.protocol, c, peer, Kind::Offer, sender.seal.id);
                    put_seal(&mut b, sender.seal);
                    put(&mut b, sender.sent.len() as u64, 4);
                    sender.offered = true;
                    sender.offered_at = now;
                    s.emit(&mut out, peer, b);
                    progress = true;
                    continue;
                }
                let end = sender
                    .sent
                    .len()
                    .min(sender.begin + l.window_chunks as usize);
                for _ in 0..end - sender.begin {
                    if sender.scan < sender.begin || sender.scan >= end {
                        sender.scan = sender.begin;
                    }
                    let index = sender.scan;
                    sender.scan += 1;
                    if sender.acked[index] != 0
                        || (sender.sent[index] != 0
                            && elapsed(now, sender.sent_at[index]) < l.retry_ms)
                    {
                        continue;
                    }
                    let start = index * CHUNK;
                    let n = CHUNK.min(sender.seal.bytes as usize - start);
                    if credit < (HEADER + 4 + n) as f64 {
                        break;
                    }
                    let mut b = header(self.protocol, c, peer, Kind::Chunk, sender.seal.id);
                    put(&mut b, index as u64, 4);
                    b.extend_from_slice(&sender.bytes.as_ref().ok_or(Error)?[start..start + n]);
                    if sender.sent[index] == 0 {
                        sender.sent_count += 1;
                    }
                    sender.sent[index] = 1;
                    sender.sent_at[index] = now;
                    s.credit -= b.len() as f64;
                    s.emit(&mut out, peer, b);
                    progress = true;
                    break;
                }
            }
            if !progress {
                break;
            }
        }
        Ok(out)
    }
    fn receive_locked(&self, peer: u32, bytes: &[u8], now: u64) -> Result<Option<Received>> {
        need(bytes.len() >= HEADER && bytes.len() <= 1400)?;
        let mut r = Reader { bytes, at: 0 };
        need(
            r.get(4)? == 0x324e3248
                && r.get(2)? == u64::from(self.protocol)
                && r.get(1)? == 7
                && r.get(1)? == 1,
        )?;
        let kind = r.get(1)?;
        need(kind <= Kind::Reject as u64)?;
        let c = Context {
            session: r.raw(32)?.try_into().map_err(|_| Error)?,
            epoch: r.get(4)? as u32,
            membership_revision: r.get(8)?,
            peers: r.get(2)? as u16,
            host: r.get(1)? as u32,
        };
        let target = r.get(1)?;
        let id = r.get(8)?;
        let mut s = self.lock()?;
        need(
            c == s.binding
                && target == u64::from(self.self_peer)
                && peer != self.self_peer
                && member(c, peer),
        )?;
        s.expire(now, self.limits);
        let p = peer as usize;
        if kind <= Kind::HelloAck as u64 {
            let maximum = r.get(8)?;
            let features = if r.done() { 0 } else { r.get(4)? as u32 };
            need(id == 0 && r.done() && maximum > 0 && maximum <= 512 * 1024 * 1024)?;
            s.capabilities[p] = maximum.min(self.limits.maximum_bytes);
            s.features[p] = features;
            if kind == Kind::Hello as u64 {
                s.hello_reply[p] = true;
            }
            return Ok(None);
        }
        need(id != 0 && s.capabilities[p] != 0)?;
        match kind {
            2 => {
                need(peer == c.host && self.self_peer != c.host)?;
                let seal = r.seal(id)?;
                let count = r.get(4)?;
                need(
                    r.done()
                        && seal.bytes > 0
                        && seal.bytes <= self.limits.maximum_bytes
                        && count == chunks(seal.bytes) as u64,
                )?;
                if let Some(v) = s.incoming.as_mut() {
                    if v.seal.id == id {
                        need(v.seal == seal)?;
                        if v.complete {
                            v.ack_all = true;
                        }
                        return Ok(None);
                    }
                }
                if s.completed.is_some()
                    || (self.limits.features & PIPELINE != 0 && s.delivered.len() >= 2)
                {
                    return Ok(None);
                }
                if id <= s.highest_id
                    || seal.tick < s.highest_tick
                    || (s.accepted_offer && elapsed(now, s.offer_at) < 250)
                {
                    return Ok(None);
                }
                let incoming = Incoming {
                    seal,
                    bytes: zeros(seal.bytes as usize)?,
                    received: zeros(count as usize)?,
                    dirty: BTreeSet::new(),
                    count: 0,
                    started: now,
                    progress: now,
                    hashing: false,
                    complete: false,
                    ack_all: false,
                };
                s.incoming = Some(incoming);
                s.highest_id = id;
                s.highest_tick = seal.tick;
                s.offer_at = now;
                s.accepted_offer = true;
            }
            3 => {
                need(peer == c.host && self.self_peer != c.host)?;
                let Some(v) = s.incoming.as_mut().filter(|v| v.seal.id == id) else {
                    return Ok(None);
                };
                let index = r.get(4)? as usize;
                need(index < v.received.len())?;
                let start = index * CHUNK;
                let n = CHUNK.min(v.seal.bytes as usize - start);
                let data = r.raw(n)?;
                need(r.done())?;
                if v.complete {
                    v.ack_all = true;
                    return Ok(None);
                }
                if v.hashing {
                    return Ok(None);
                }
                if v.received[index] != 0 {
                    need(data == &v.bytes[start..start + n])?;
                } else {
                    v.bytes[start..start + n].copy_from_slice(data);
                    v.received[index] = 1;
                    v.count += 1;
                    v.progress = now;
                }
                v.dirty.insert(index / 64 * 64);
                if v.count == v.received.len() {
                    v.hashing = true;
                    return Ok(Some(Received {
                        peer,
                        context: c,
                        seal: v.seal,
                        bytes: std::mem::take(&mut v.bytes),
                    }));
                }
            }
            4 => {
                need(self.self_peer == c.host)?;
                let Some(v) = s.senders[p].as_mut().filter(|v| v.seal.id == id) else {
                    return Ok(None);
                };
                let base = r.get(4)?;
                let bits = r.get(8)?;
                need(r.done())?;
                if base == u64::from(u32::MAX) {
                    need(bits == 0 && v.sent_count == v.sent.len())?;
                    v.acked.fill(1);
                } else {
                    need(base % 64 == 0 && base < v.sent.len() as u64)?;
                    for i in 0..64 {
                        if bits & (1u64 << i) != 0 {
                            need(
                                base + i < v.sent.len() as u64 && v.sent[(base + i) as usize] != 0,
                            )?;
                        }
                    }
                    for i in 0..64 {
                        if base + i < v.sent.len() as u64 && bits & (1u64 << i) != 0 {
                            v.acked[(base + i) as usize] = 1;
                        }
                    }
                }
                let before = v.begin;
                while v.begin < v.acked.len() && v.acked[v.begin] != 0 {
                    v.begin += 1;
                }
                if v.begin != before {
                    v.progress = now;
                }
            }
            5 => {
                need(self.self_peer == c.host)?;
                let seal = r.seal(id)?;
                need(r.done())?;
                if let Some(v) = s.retiring[p].as_ref().filter(|v| v.seal.id == id) {
                    need(v.seal == seal)?;
                    if !v.failed {
                        s.receipt(Validated {
                            peer,
                            context: c,
                            seal,
                        });
                        s.retiring[p] = None;
                    }
                    return Ok(None);
                }
                let Some(v) = s.senders[p].as_ref().filter(|v| v.seal.id == id) else {
                    return Ok(None);
                };
                need(v.seal == seal && v.sent_count == v.sent.len())?;
                if !v.validated && !v.failed {
                    if let Some(v) = s.retiring[p].as_ref() {
                        need(!v.failed)?;
                        let prior = v.seal;
                        s.receipt(Validated {
                            peer,
                            context: c,
                            seal: prior,
                        });
                        s.retiring[p] = None;
                    }
                    let v = s.senders[p].as_mut().unwrap();
                    v.validated = true;
                    v.bytes = None;
                    s.receipt(Validated {
                        peer,
                        context: c,
                        seal,
                    });
                }
            }
            6 => {
                need(self.self_peer == c.host && r.done())?;
                if let Some(v) = s.retiring[p].as_mut().filter(|v| v.seal.id == id) {
                    v.failed = true;
                }
                if let Some(v) = s.senders[p].as_mut().filter(|v| v.seal.id == id) {
                    v.failed = true;
                    v.bytes = None;
                }
            }
            _ => return Err(Error),
        }
        Ok(None)
    }
    pub fn receive(&self, peer: u32, bytes: &[u8], now: u64) -> bool {
        if !is_packet(bytes) {
            return false;
        }
        let _ = self.receive_recognized(peer, bytes, now);
        true
    }
    fn receive_recognized(&self, peer: u32, bytes: &[u8], now: u64) -> Result<()> {
        if let Some(finished) = self.receive_locked(peer, bytes, now)? {
            self.finish_received(finished)?;
        }
        Ok(())
    }
    fn finish_received(&self, finished: Received) -> Result<()> {
        // No state mutex spans the large checksum. A concurrent reset or
        // replacement may retire it; only the exact observed cut publishes.
        let actual: [u8; 32] = Sha256::digest(&finished.bytes).into();
        let mut s = self.lock()?;
        if s.binding != finished.context
            || s.incoming.as_ref().is_none_or(|v| v.seal != finished.seal)
        {
            return Ok(());
        }
        if actual != finished.seal.digest {
            s.rejected = Some(finished.seal.id);
            s.incoming = None;
            return Ok(());
        }
        let v = s.incoming.as_mut().unwrap();
        v.complete = true;
        v.hashing = false;
        v.ack_all = true;
        v.dirty.clear();
        s.completed = Some(finished);
        Ok(())
    }
    pub fn take_completed(&self) -> Result<Option<Received>> {
        let mut s = self.lock()?;
        let value = s.completed.take();
        if let Some(v) = &value {
            if self.limits.features & PIPELINE == 0 {
                s.delivered.clear();
            }
            s.delivered.push_back(v.seal);
        }
        Ok(value)
    }
    pub fn acknowledge_validated(&self, c: Context, seal: Cut) -> Result<bool> {
        let mut s = self.lock()?;
        if s.binding != c || self.self_peer == c.host || s.validated.is_some_and(|v| v.id > seal.id)
        {
            return Ok(false);
        }
        if s.validated == Some(seal) {
            return Ok(true);
        }
        let Some(at) = s.delivered.iter().position(|v| *v == seal) else {
            return Ok(false);
        };
        s.delivered.drain(..=at);
        s.validated = Some(seal);
        s.ready_sent = false;
        Ok(true)
    }
    pub fn reject_delivered(&self, c: Context, seal: Cut) -> Result<bool> {
        let mut s = self.lock()?;
        if s.binding != c
            || self.self_peer == c.host
            || s.validated.is_some_and(|v| v.id >= seal.id)
        {
            return Ok(false);
        }
        let Some(at) = s.delivered.iter().position(|v| *v == seal) else {
            return Ok(false);
        };
        s.delivered.remove(at);
        s.rejected = Some(seal.id);
        Ok(true)
    }
    pub fn validated_cuts(&self) -> Result<Vec<Validated>> {
        Ok(self.lock()?.receipts.drain(..).collect())
    }
}
pub fn is_packet(bytes: &[u8]) -> bool {
    bytes.len() >= 7
        && &bytes[..4] == b"H2N2"
        && u16::from_le_bytes([bytes[4], bytes[5]]) > 0
        && bytes[6] == 7
}

#[cfg(test)]
mod tests {
    // Real codec/state transitions with deterministic clocks and byte inputs;
    // native validation calls are declared fixtures, never world preparation.
    use super::*;
    fn context() -> Context {
        Context {
            session: [0x42; 32],
            epoch: 3,
            membership_revision: 5,
            peers: 17,
            host: 0,
        }
    }
    fn limits() -> Limits {
        Limits {
            maximum_bytes: 1 << 20,
            bytes_per_second: 1 << 20,
            burst_bytes: 4096,
            retry_ms: 10,
            offer_ms: 20,
            idle_ms: 5000,
            lifetime_ms: 30000,
            window_chunks: 32,
            features: PIPELINE,
        }
    }
    fn pair() -> (Transfer, Transfer) {
        let a = Transfer::new(0, 124, context(), limits()).unwrap();
        let b = Transfer::new(4, 124, context(), limits()).unwrap();
        step(&a, &b, 1000);
        step(&a, &b, 1001);
        assert!(a.supported(4).unwrap() && b.supported(0).unwrap());
        (a, b)
    }
    fn step(a: &Transfer, b: &Transfer, now: u64) {
        for p in a.poll(now, 16).unwrap() {
            assert!(b.receive(0, &p.bytes, now));
        }
        for p in b.poll(now, 16).unwrap() {
            assert!(a.receive(4, &p.bytes, now));
        }
    }
    fn deliver(a: &Transfer, b: &Transfer, tick: u64, start: u64) -> (Cut, Received, u64) {
        let bytes = (0..5003).map(|n| (n * 37) as u8).collect::<Vec<_>>();
        let seal = a
            .publish(context(), tick, bytes.clone(), &[4], start)
            .unwrap();
        let mut value = None;
        for now in start..start + 5000 {
            step(a, b, now);
            if value.is_none() {
                value = b.take_completed().unwrap();
            }
            if value.is_some() && a.delivered_to(context(), seal, 4).unwrap() {
                let value = value.unwrap();
                assert_eq!(value.bytes, bytes);
                assert_eq!(value.seal, seal);
                return (seal, value, now);
            }
        }
        panic!("fixture byte delivery timed out")
    }
    #[test]
    fn raw_constructor_context_and_limits_are_checked_without_aliases() {
        for p in [16, 256, 257, u32::MAX] {
            assert!(Transfer::new(p, 124, context(), limits()).is_err());
            let mut c = context();
            c.host = p;
            assert!(Transfer::new(0, 124, c, limits()).is_err());
        }
        for i in 0..8 {
            let mut l = limits();
            match i {
                0 => l.maximum_bytes = 0,
                1 => l.maximum_bytes = 1 << 30,
                2 => l.bytes_per_second = 0,
                3 => l.burst_bytes = 1099,
                4 => l.retry_ms = 9,
                5 => l.offer_ms = 9,
                6 => l.window_chunks = 0,
                _ => l.window_chunks = 65537,
            };
            assert!(Transfer::new(0, 124, context(), l).is_err());
        }
    }
    #[test]
    fn actual_hello_legacy_capabilities_and_generation_reset_are_exact() {
        let a = Transfer::new(0, 124, context(), limits()).unwrap();
        let b = Transfer::new(
            4,
            124,
            context(),
            Limits {
                features: 0,
                ..limits()
            },
        )
        .unwrap();
        step(&a, &b, 1000);
        step(&a, &b, 1001);
        assert!(a.supported(4).unwrap());
        assert!(!a.feature_supported(4, PIPELINE).unwrap());
        assert!(b.feature_supported(0, PIPELINE).unwrap());
        assert_eq!(a.feature_snapshot().unwrap().1, [0; 16]);
        for p in [16, 256, u32::MAX] {
            assert!(!a.supported(p).unwrap());
            assert!(!a.feature_supported(p, PIPELINE).unwrap());
        }
        let next = Context {
            epoch: 4,
            ..context()
        };
        assert!(a.replace_context(context(), next).unwrap());
        assert!(!a.supported(4).unwrap());
        assert_eq!(a.feature_snapshot().unwrap(), (next, [0; 16]));
    }
    #[test]
    fn completed_bank_requires_explicit_native_ack_and_preserves_real_bytes() {
        let (a, b) = pair();
        let (seal, bank, now) = deliver(&a, &b, 10, 2000);
        assert!(a.validated_cuts().unwrap().is_empty());
        assert!(b.take_completed().unwrap().is_none());
        let mut wrong = seal;
        wrong.digest[31] ^= 1;
        assert!(!b.acknowledge_validated(context(), wrong).unwrap());
        assert!(b.acknowledge_validated(context(), seal).unwrap());
        step(&a, &b, now + 1);
        assert_eq!(
            a.validated_cuts().unwrap(),
            vec![Validated {
                peer: 4,
                context: context(),
                seal
            }]
        );
        assert!(!a.failed_to(context(), seal, 4, now + 100000).unwrap());
        assert_eq!(bank.bytes.len(), 5003);
    }
    #[test]
    fn exact_failure_observation_has_strict_deadlines_and_never_revives() {
        let (a, b) = pair();
        let (seal, _, now) = deliver(&a, &b, 10, 2000);
        let foreign = Context {
            membership_revision: 6,
            ..context()
        };
        assert!(!a.failed_to(foreign, seal, 4, now + 6000).unwrap());
        assert!(!a.failed_to(context(), seal, 4, now + 4999).unwrap());
        assert!(a.failed_to(context(), seal, 4, now + 6000).unwrap());
        assert!(
            !a.note_native_preparation(context(), seal, 4, now + 6000)
                .unwrap()
        );
        assert!(b.acknowledge_validated(context(), seal).unwrap());
        step(&a, &b, now + 6001);
        assert!(a.validated_cuts().unwrap().is_empty());
    }
    #[test]
    fn preparation_progress_renews_only_delivered_idle_and_not_total_lifetime() {
        let (a, b) = pair();
        let (seal, _, now) = deliver(&a, &b, 10, 2000);
        for i in 1..=7 {
            assert!(
                a.note_native_preparation(context(), seal, 4, now + i * 4000)
                    .unwrap()
            );
        }
        assert!(
            !a.note_native_preparation(context(), seal, 4, now + 30001)
                .unwrap()
        );
        assert!(a.validated_cuts().unwrap().is_empty());
    }
    #[test]
    fn pipeline_newer_ready_recovers_lost_predecessor_receipt_in_order() {
        let (a, b) = pair();
        let (first, _, now) = deliver(&a, &b, 10, 2000);
        let bytes = vec![0x73; 3001];
        let second = a
            .publish_ahead(context(), first, 11, bytes.clone(), 4, now + 300)
            .unwrap();
        let mut bank = None;
        let mut finished = now + 300;
        for n in now + 300..now + 3000 {
            step(&a, &b, n);
            if bank.is_none() {
                bank = b.take_completed().unwrap();
            }
            if bank.is_some() && a.delivered_to(context(), second, 4).unwrap() {
                finished = n;
                break;
            }
        }
        assert_eq!(bank.unwrap().bytes, bytes);
        assert!(a.validated_cuts().unwrap().is_empty());
        assert!(b.acknowledge_validated(context(), second).unwrap());
        step(&a, &b, finished + 1);
        assert_eq!(
            a.validated_cuts().unwrap(),
            vec![
                Validated {
                    peer: 4,
                    context: context(),
                    seal: first
                },
                Validated {
                    peer: 4,
                    context: context(),
                    seal: second
                }
            ]
        );
    }
    #[test]
    fn committed_reset_retains_the_native_cursor_and_clears_old_protocol_state() {
        let c = Context {
            peers: 273,
            ..context()
        };
        let a = Transfer::new(0, 124, c, limits()).unwrap();
        assert_eq!(a.poll(0, 1).unwrap()[0].peer, 4);
        let next = Context {
            membership_revision: c.membership_revision + 1,
            ..c
        };
        assert!(a.replace_context(c, next).unwrap());
        assert_eq!(a.poll(1, 1).unwrap()[0].peer, 8);
        assert!(!a.replace_context(c, next).unwrap());
        let foreign = Context {
            session: [0; 32],
            ..next
        };
        assert!(a.replace_context(next, foreign).is_err());
        assert_eq!(a.context().unwrap(), next);
    }
    #[test]
    fn forged_all_ack_and_corrupt_hash_never_create_delivery_or_ready() {
        let (a, b) = pair();
        let seal = a
            .publish(context(), 10, vec![0x51; 8193], &[4], 2000)
            .unwrap();
        let mut ack = header(124, context(), 0, Kind::Ack, seal.id);
        put(&mut ack, u32::MAX.into(), 4);
        put(&mut ack, 0, 8);
        assert!(a.receive(4, &ack, 2000));
        assert!(!a.delivered_to(context(), seal, 4).unwrap());
        for now in 2000..2600 {
            for mut p in a.poll(now, 16).unwrap() {
                if p.bytes[8] == 3 {
                    p.bytes[69] ^= 1;
                }
                b.receive(0, &p.bytes, now);
            }
            for p in b.poll(now, 16).unwrap() {
                a.receive(4, &p.bytes, now);
            }
        }
        assert!(b.take_completed().unwrap().is_none());
        assert!(a.failed_to(context(), seal, 4, 2600).unwrap());
        assert!(a.validated_cuts().unwrap().is_empty());
    }
    #[test]
    fn checksum_completion_rechecks_the_exact_generation_after_unlocked_hash_work() {
        let (a, b) = pair();
        let seal = a
            .publish(context(), 10, vec![0x41; 100], &[4], 2000)
            .unwrap();
        let mut staged = None;
        for now in 2000..2100 {
            for p in a.poll(now, 16).unwrap() {
                if let Some(v) = b.receive_locked(0, &p.bytes, now).unwrap() {
                    staged = Some(v);
                }
            }
            if staged.is_some() {
                break;
            }
            for p in b.poll(now, 16).unwrap() {
                a.receive(4, &p.bytes, now);
            }
        }
        let staged = staged.unwrap();
        assert_eq!(staged.seal, seal);
        let next = Context {
            epoch: 4,
            ..context()
        };
        assert!(b.replace_context(context(), next).unwrap());
        b.finish_received(staged).unwrap();
        assert!(b.take_completed().unwrap().is_none());
        assert!(!b.acknowledge_validated(context(), seal).unwrap());
        assert_eq!(b.context().unwrap(), next);
    }
    #[test]
    fn synchronized_owner_is_send_sync_and_recognizes_only_the_prior_packet_family() {
        fn shared<T: Send + Sync>() {}
        shared::<Transfer>();
        let (a, _) = pair();
        for n in 0..7 {
            assert!(!a.receive(4, &vec![0; n], 0));
        }
        let mut bytes = header(123, context(), 0, Kind::Ack, 1);
        assert!(a.receive(4, &bytes, 0));
        bytes[6] = 8;
        assert!(!a.receive(4, &bytes, 0));
    }
}
