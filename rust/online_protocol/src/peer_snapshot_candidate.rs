//! Complete modern snapshot candidate protocol and delivery lifetime policy.
//! Native storage and private delivery identity remain genuine typed IO. Arm,
//! Captured and Retired are explicit executor assertions, never world proof.
use crate::peer_source_admissions::Context;
use sha2::{Digest, Sha256};
use std::sync::{Mutex, MutexGuard};

const CHUNK: usize = 1024;
const HEADER: usize = 67;
const TYPES: usize = 11;
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Invalid peer snapshot candidate")
    }
}
impl std::error::Error for Error {}
pub type Result<T> = std::result::Result<T, Error>;
fn need(v: bool) -> Result<()> {
    if v { Ok(()) } else { Err(Error) }
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Token {
    pub context: Context,
    pub recipients: u16,
    pub nonce: u64,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Close {
    Release,
    Abort,
}
#[derive(Clone, Copy, Debug)]
pub struct Limits {
    pub maximum_archive_bytes: u64,
    pub retry_ms: u64,
    pub idle_ms: u64,
    pub window_chunks: u32,
}
impl Default for Limits {
    fn default() -> Self {
        Self {
            maximum_archive_bytes: 9 * 1024 * 1024,
            retry_ms: 100,
            idle_ms: 15000,
            window_chunks: 32,
        }
    }
}
/// Byte storage only. Production uses uniquely owned native C++ vector banks;
/// component controls may use Vec banks. It supplies no protocol or native ACK.
pub trait Buffer: Send {
    fn bytes(&self) -> &[u8];
    fn write(&mut self, offset: usize, data: &[u8]) -> Result<()>;
}
pub struct Archive<B: Buffer> {
    pub peer: u32,
    pub bytes: B,
}
#[derive(Clone, Copy, Debug, Default)]
pub struct Witness {
    pub present: bool,
    pub owner_matches: bool,
    pub recipient: u32,
    pub token: Option<Token>,
    pub bytes_present: bool,
}
/// Called under the internal state mutex. Implementations must only operate on
/// native storage/private delivery identity and never reenter the snapshot owner.
pub trait LeaseIo<B: Buffer> {
    fn witness(&self) -> Witness;
    fn issue(&self, token: Token, bytes: B);
    fn destroy(&self);
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct View {
    pub context: Context,
    pub recipients: u16,
    pub pending: Option<Token>,
    pub armed: u16,
    pub delivered: u16,
    pub captured: u16,
    pub retired: u16,
    pub rejected: u16,
    pub closing: Option<Close>,
    pub published: bool,
    pub delivery_outstanding: bool,
    pub stalled: bool,
    pub buffered_bytes: u64,
    pub maximum_buffered_bytes: u64,
    pub rejected_packets: u64,
}
#[derive(Debug, PartialEq, Eq)]
pub struct Packet {
    pub peer: u32,
    pub bytes: Vec<u8>,
}
#[derive(Clone, Copy, PartialEq, Eq)]
enum Kind {
    Arm = 0,
    Armed,
    Offer,
    Chunk,
    Ack,
    Captured,
    Release,
    Abort,
    Released,
    Aborted,
    Reject,
}
fn member(c: Context, p: u32) -> bool {
    p < 16 && c.peers & (1u16 << p) != 0
}
fn bit(p: u32) -> u16 {
    1u16 << p
}
fn chunks(n: u64) -> usize {
    n.div_ceil(CHUNK as u64) as usize
}
fn put(b: &mut Vec<u8>, v: u64, n: usize) {
    b.extend_from_slice(&v.to_le_bytes()[..n]);
}
fn header(protocol: u16, t: Token, target: u32, kind: Kind) -> Vec<u8> {
    let mut b = Vec::with_capacity(HEADER + 4 + CHUNK);
    b.extend_from_slice(b"H2N2");
    put(&mut b, protocol.into(), 2);
    b.extend_from_slice(&[12, 1, kind as u8]);
    b.extend_from_slice(&t.context.session);
    put(&mut b, t.context.epoch.into(), 4);
    put(&mut b, t.context.membership_revision, 8);
    put(&mut b, t.context.peers.into(), 2);
    put(&mut b, t.context.host.into(), 1);
    put(&mut b, target.into(), 1);
    put(&mut b, t.recipients.into(), 2);
    put(&mut b, t.nonce, 8);
    b
}
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl<'a> Reader<'a> {
    fn raw(&mut self, n: usize) -> Result<&'a [u8]> {
        let end = self.at.checked_add(n).ok_or(Error)?;
        let b = self.bytes.get(self.at..end).ok_or(Error)?;
        self.at = end;
        Ok(b)
    }
    fn get(&mut self, n: usize) -> Result<u64> {
        let mut b = [0; 8];
        b[..n].copy_from_slice(self.raw(n)?);
        Ok(u64::from_le_bytes(b))
    }
    fn done(&self) -> bool {
        self.at == self.bytes.len()
    }
    fn message(&mut self, protocol: u16) -> Result<(Kind, u32, Token)> {
        need(self.bytes.len() >= HEADER && self.bytes.len() <= 1400)?;
        need(
            self.get(4)? == 0x324e3248
                && self.get(2)? == u64::from(protocol)
                && self.get(1)? == 12
                && self.get(1)? == 1,
        )?;
        let kind = match self.get(1)? {
            0 => Kind::Arm,
            1 => Kind::Armed,
            2 => Kind::Offer,
            3 => Kind::Chunk,
            4 => Kind::Ack,
            5 => Kind::Captured,
            6 => Kind::Release,
            7 => Kind::Abort,
            8 => Kind::Released,
            9 => Kind::Aborted,
            10 => Kind::Reject,
            _ => return Err(Error),
        };
        let context = Context {
            session: self.raw(32)?.try_into().map_err(|_| Error)?,
            epoch: self.get(4)? as u32,
            membership_revision: self.get(8)?,
            peers: self.get(2)? as u16,
            host: self.get(1)? as u32,
        };
        let target = self.get(1)? as u32;
        let recipients = self.get(2)? as u16;
        let nonce = self.get(8)?;
        Ok((
            kind,
            target,
            Token {
                context,
                recipients,
                nonce,
            },
        ))
    }
}
fn zeros<T: Default + Clone>(n: usize) -> Result<Vec<T>> {
    let mut v = Vec::new();
    v.try_reserve_exact(n).map_err(|_| Error)?;
    v.resize(n, T::default());
    Ok(v)
}
struct Sender<B: Buffer> {
    bytes: Option<B>,
    digest: [u8; 32],
    extent: u64,
    sent: Vec<u8>,
    acked: Vec<u8>,
    sent_at: Vec<u64>,
    begin: usize,
    scan: usize,
    sent_count: usize,
    offered: bool,
}
struct Incoming<B: Buffer> {
    digest: [u8; 32],
    extent: u64,
    bytes: Option<B>,
    received: Vec<u8>,
    dirty: Vec<u8>,
    count: usize,
    dirty_cursor: u32,
    hashing: bool,
    complete: bool,
    ack_all: bool,
}
struct State<B: Buffer> {
    value: View,
    highest_nonce: u64,
    clock: u64,
    progress: u64,
    publishing_bytes: u64,
    local_bytes: u64,
    publishing: bool,
    checked_out: bool,
    cursor: u32,
    emitted_at: [[u64; TYPES]; 16],
    emitted: [[bool; TYPES]; 16],
    senders: [Option<Sender<B>>; 16],
    incoming: Option<Incoming<B>>,
    completed: Option<B>,
}
pub struct Candidate<B: Buffer> {
    self_peer: u32,
    protocol: u16,
    binding: Context,
    recipients: u16,
    limits: Limits,
    allocate: fn(usize) -> Result<B>,
    state: Mutex<State<B>>,
}
impl<B: Buffer> Candidate<B> {
    pub fn new(
        self_peer: u32,
        protocol: u16,
        c: Context,
        l: Limits,
        allocate: fn(usize) -> Result<B>,
    ) -> Result<Self> {
        need(
            member(c, self_peer)
                && member(c, c.host)
                && c.peers.count_ones() >= 2
                && c.session.iter().any(|b| *b != 0),
        )?;
        need(
            l.maximum_archive_bytes > 0
                && l.maximum_archive_bytes <= 9 * 1024 * 1024
                && l.retry_ms > 0
                && l.retry_ms <= 60000
                && l.idle_ms >= l.retry_ms
                && l.idle_ms <= 86400000
                && l.window_chunks > 0
                && l.window_chunks <= 256,
        )?;
        let recipients = c.peers & !bit(c.host);
        let maximum = l.maximum_archive_bytes
            * if self_peer == c.host {
                u64::from(recipients.count_ones())
            } else {
                1
            };
        let value = View {
            context: c,
            recipients,
            pending: None,
            armed: 0,
            delivered: 0,
            captured: 0,
            retired: 0,
            rejected: 0,
            closing: None,
            published: false,
            delivery_outstanding: false,
            stalled: false,
            buffered_bytes: 0,
            maximum_buffered_bytes: maximum,
            rejected_packets: 0,
        };
        Ok(Self {
            self_peer,
            protocol,
            binding: c,
            recipients,
            limits: l,
            allocate,
            state: Mutex::new(State {
                value,
                highest_nonce: 0,
                clock: 0,
                progress: 0,
                publishing_bytes: 0,
                local_bytes: 0,
                publishing: false,
                checked_out: false,
                cursor: 0,
                emitted_at: [[0; TYPES]; 16],
                emitted: [[false; TYPES]; 16],
                senders: std::array::from_fn(|_| None),
                incoming: None,
                completed: None,
            }),
        })
    }
    fn lock(&self) -> Result<MutexGuard<'_, State<B>>> {
        self.state.lock().map_err(|_| Error)
    }
    pub fn self_peer(&self) -> u32 {
        self.self_peer
    }
    fn recipient(&self, p: u32) -> bool {
        p < 16 && self.recipients & bit(p) != 0
    }
    fn exact(&self, t: Token) -> bool {
        t.context == self.binding && t.recipients == self.recipients && t.nonce != 0
    }
    fn matches(&self, s: &State<B>, t: Token) -> bool {
        self.exact(t) && s.value.pending == Some(t)
    }
    fn ended(&self, s: &State<B>) -> bool {
        s.value.pending.is_some()
            && s.value.closing.is_some()
            && if self.self_peer == self.binding.host {
                s.value.retired == self.recipients
            } else {
                s.value.retired & bit(self.self_peer) != 0
            }
    }
    fn observe(&self, s: &mut State<B>, now: u64) {
        s.clock = s.clock.max(now);
        s.value.stalled = s.value.pending.is_some()
            && !self.ended(s)
            && s.clock - s.progress >= self.limits.idle_ms;
    }
    fn changed(s: &mut State<B>) {
        s.progress = s.clock;
        s.value.stalled = false;
    }
    fn install(&self, s: &mut State<B>, t: Token) {
        let rejected = s.value.rejected_packets;
        let maximum = s.value.maximum_buffered_bytes;
        s.value = View {
            context: self.binding,
            recipients: self.recipients,
            pending: Some(t),
            armed: 0,
            delivered: 0,
            captured: 0,
            retired: 0,
            rejected: 0,
            closing: None,
            published: false,
            delivery_outstanding: false,
            stalled: false,
            buffered_bytes: 0,
            maximum_buffered_bytes: maximum,
            rejected_packets: rejected,
        };
        s.highest_nonce = t.nonce;
        s.senders = std::array::from_fn(|_| None);
        s.incoming = None;
        s.completed = None;
        s.local_bytes = 0;
        s.checked_out = false;
        s.emitted = [[false; TYPES]; 16];
        s.emitted_at = [[0; TYPES]; 16];
        Self::changed(s);
    }
    fn close_locked(s: &mut State<B>, mode: Close) -> bool {
        if s.value.pending.is_none() {
            return false;
        }
        if let Some(before) = s.value.closing {
            return before == mode;
        }
        if mode == Close::Release && s.value.captured != s.value.recipients {
            return false;
        }
        s.value.closing = Some(mode);
        for v in s.senders.iter_mut().flatten() {
            v.bytes = None;
        }
        if s.incoming.as_ref().is_some_and(|v| !v.hashing) && !s.checked_out {
            s.incoming = None;
            s.completed = None;
            s.local_bytes = 0;
        }
        Self::changed(s);
        true
    }
    pub fn begin(&self, now: u64) -> Result<Option<Token>> {
        let mut s = self.lock()?;
        self.observe(&mut s, now);
        if self.self_peer != self.binding.host
            || s.publishing
            || (s.value.pending.is_some() && !self.ended(&s))
            || s.highest_nonce == u64::MAX
        {
            return Ok(None);
        }
        let t = Token {
            context: self.binding,
            recipients: self.recipients,
            nonce: s.highest_nonce + 1,
        };
        self.install(&mut s, t);
        Ok(Some(t))
    }
    pub fn acknowledge_arm(&self, t: Token) -> Result<bool> {
        let mut s = self.lock()?;
        let own = bit(self.self_peer);
        if !self.recipient(self.self_peer)
            || !self.matches(&s, t)
            || s.value.closing.is_some()
            || s.value.rejected & own != 0
        {
            return Ok(false);
        }
        if s.value.armed & own == 0 {
            s.value.armed |= own;
            Self::changed(&mut s);
        }
        Ok(true)
    }
    pub fn publish(&self, t: Token, archives: Vec<Archive<B>>, now: u64) -> Result<bool> {
        let mut selected = 0u16;
        let mut extent = 0u64;
        for a in &archives {
            if a.peer >= 16
                || !self.recipient(a.peer)
                || selected & bit(a.peer) != 0
                || a.bytes.bytes().is_empty()
                || a.bytes.bytes().len() as u64 > self.limits.maximum_archive_bytes
            {
                return Ok(false);
            }
            selected |= bit(a.peer);
            extent += a.bytes.bytes().len() as u64;
        }
        if selected != self.recipients || archives.len() != self.recipients.count_ones() as usize {
            return Ok(false);
        }
        {
            let mut s = self.lock()?;
            self.observe(&mut s, now);
            if self.self_peer != self.binding.host
                || !self.matches(&s, t)
                || s.value.closing.is_some()
                || s.value.published
                || s.publishing
                || s.value.armed != self.recipients
            {
                return Ok(false);
            }
            s.publishing = true;
            s.publishing_bytes = extent;
        }
        let mut prepared: [Option<Sender<B>>; 16] = std::array::from_fn(|_| None);
        let mut archives = archives.into_iter();
        let result = (|| -> Result<()> {
            for a in archives.by_ref() {
                let extent = a.bytes.bytes().len() as u64;
                let digest = Sha256::digest(a.bytes.bytes()).into();
                let count = chunks(extent);
                let sender = Sender {
                    bytes: Some(a.bytes),
                    digest,
                    extent,
                    sent: zeros(count)?,
                    acked: zeros(count)?,
                    sent_at: zeros(count)?,
                    begin: 0,
                    scan: 0,
                    sent_count: 0,
                    offered: false,
                };
                prepared[a.peer as usize] = Some(sender);
            }
            Ok(())
        })();
        if let Err(e) = result {
            drop(prepared);
            drop(archives);
            let mut s = self.lock()?;
            s.publishing = false;
            s.publishing_bytes = 0;
            return Err(e);
        }
        let mut s = self.lock()?;
        if !self.matches(&s, t) || s.value.closing.is_some() {
            drop(s);
            drop(prepared);
            drop(archives);
            let mut s = self.lock()?;
            s.publishing = false;
            s.publishing_bytes = 0;
            return Ok(false);
        }
        s.publishing = false;
        s.publishing_bytes = 0;
        s.senders = prepared;
        s.value.published = true;
        Self::changed(&mut s);
        Ok(true)
    }
    pub fn take_delivery(&self, io: &impl LeaseIo<B>) -> Result<bool> {
        let mut s = self.lock()?;
        if s.value.pending.is_none()
            || s.value.closing.is_some()
            || s.checked_out
            || s.completed.is_none()
        {
            return Ok(false);
        }
        let token = s.value.pending.unwrap();
        let bytes = s.completed.take().unwrap();
        io.issue(token, bytes);
        s.checked_out = true;
        Ok(true)
    }
    pub fn acknowledge_captured(&self, io: &impl LeaseIo<B>) -> Result<bool> {
        let mut s = self.lock()?;
        let own = bit(self.self_peer);
        let w = io.witness();
        if !self.recipient(self.self_peer)
            || w.token.is_none_or(|t| !self.matches(&s, t))
            || s.value.closing.is_some()
            || s.value.rejected & own != 0
            || !s.checked_out
            || !w.present
            || !w.owner_matches
            || w.recipient != self.self_peer
            || !w.bytes_present
            || s.incoming.as_ref().is_none_or(|v| !v.complete)
        {
            return Ok(false);
        }
        io.destroy();
        s.checked_out = false;
        s.local_bytes = 0;
        s.value.captured |= own;
        Self::changed(&mut s);
        Ok(true)
    }
    pub fn close(&self, t: Token, mode: Close) -> Result<bool> {
        let mut s = self.lock()?;
        Ok(self.self_peer == self.binding.host
            && self.matches(&s, t)
            && Self::close_locked(&mut s, mode))
    }
    pub fn reject(&self, t: Token) -> Result<bool> {
        let mut s = self.lock()?;
        if !self.recipient(self.self_peer) || !self.matches(&s, t) || s.value.closing.is_some() {
            return Ok(false);
        }
        let own = bit(self.self_peer);
        if s.value.rejected & own == 0 {
            s.value.rejected |= own;
            Self::changed(&mut s);
        }
        Ok(true)
    }
    fn can_retired(&self, s: &State<B>, t: Token, w: Witness) -> bool {
        if !self.recipient(self.self_peer)
            || !self.matches(s, t)
            || s.value.closing.is_none()
            || s.incoming.as_ref().is_some_and(|v| v.hashing)
        {
            return false;
        }
        if s.checked_out {
            w.present
                && w.owner_matches
                && w.recipient == self.self_peer
                && w.token == Some(t)
                && w.bytes_present
        } else {
            !w.present
        }
    }
    pub fn can_acknowledge_retired(&self, t: Token, io: &impl LeaseIo<B>) -> Result<bool> {
        let s = self.lock()?;
        Ok(self.can_retired(&s, t, io.witness()))
    }
    pub fn acknowledge_retired(&self, t: Token, io: &impl LeaseIo<B>) -> Result<bool> {
        let mut s = self.lock()?;
        if !self.can_retired(&s, t, io.witness()) {
            return Ok(false);
        }
        io.destroy();
        s.completed = None;
        s.incoming = None;
        s.checked_out = false;
        s.local_bytes = 0;
        let own = bit(self.self_peer);
        if s.value.retired & own == 0 {
            s.value.retired |= own;
            Self::changed(&mut s);
        }
        Ok(true)
    }
    pub fn state(&self) -> Result<View> {
        let s = self.lock()?;
        let mut out = s.value.clone();
        out.delivery_outstanding = s.checked_out;
        out.buffered_bytes = s.local_bytes
            + s.publishing_bytes
            + s.senders
                .iter()
                .flatten()
                .filter(|v| v.bytes.is_some())
                .map(|v| v.extent)
                .sum::<u64>();
        Ok(out)
    }
    fn due(&self, s: &State<B>, p: usize, k: Kind) -> bool {
        !s.emitted[p][k as usize] || s.clock - s.emitted_at[p][k as usize] >= self.limits.retry_ms
    }
    fn mark(s: &mut State<B>, p: usize, k: Kind) {
        s.emitted[p][k as usize] = true;
        s.emitted_at[p][k as usize] = s.clock;
    }
    fn make(&self, s: &mut State<B>, peer: u32, kind: Kind) -> Result<Option<Vec<u8>>> {
        let Some(token) = s.value.pending else {
            return Ok(None);
        };
        if peer == self.self_peer || !member(self.binding, peer) {
            return Ok(None);
        }
        let p = peer as usize;
        if self.self_peer == self.binding.host {
            if !self.recipient(peer) {
                return Ok(None);
            }
            match kind {
                Kind::Arm => {
                    if s.value.retired & bit(peer) != 0
                        || s.value.armed & bit(peer) != 0
                        || !self.due(s, p, kind)
                    {
                        return Ok(None);
                    }
                }
                Kind::Release | Kind::Abort => {
                    if s.value.closing.is_none()
                        || (s.value.closing == Some(Close::Release)) != (kind == Kind::Release)
                        || s.value.retired & bit(peer) != 0
                        || !self.due(s, p, kind)
                    {
                        return Ok(None);
                    }
                }
                Kind::Offer => {
                    if s.value.closing.is_some()
                        || s.senders[p].as_ref().is_none_or(|v| v.bytes.is_none())
                        || s.value.captured & bit(peer) != 0
                        || !self.due(s, p, kind)
                    {
                        return Ok(None);
                    }
                    let v = s.senders[p].as_mut().unwrap();
                    let mut out = header(self.protocol, token, peer, kind);
                    put(&mut out, v.extent, 8);
                    out.extend_from_slice(&v.digest);
                    put(&mut out, v.sent.len() as u64, 4);
                    v.offered = true;
                    Self::mark(s, p, kind);
                    return Ok(Some(out));
                }
                Kind::Chunk => {
                    if s.value.closing.is_some()
                        || s.senders[p]
                            .as_ref()
                            .is_none_or(|v| v.bytes.is_none() || !v.offered)
                        || s.value.captured & bit(peer) != 0
                    {
                        return Ok(None);
                    }
                    let v = s.senders[p].as_mut().unwrap();
                    let end = v
                        .sent
                        .len()
                        .min(v.begin + self.limits.window_chunks as usize);
                    for _ in 0..end - v.begin {
                        if v.scan < v.begin || v.scan >= end {
                            v.scan = v.begin;
                        }
                        let index = v.scan;
                        v.scan += 1;
                        if v.acked[index] != 0
                            || (v.sent[index] != 0
                                && s.clock - v.sent_at[index] < self.limits.retry_ms)
                        {
                            continue;
                        }
                        let mut out = header(self.protocol, token, peer, kind);
                        put(&mut out, index as u64, 4);
                        let start = index * CHUNK;
                        let extent = CHUNK.min(v.extent as usize - start);
                        out.extend_from_slice(
                            &v.bytes.as_ref().ok_or(Error)?.bytes()[start..start + extent],
                        );
                        if v.sent[index] == 0 {
                            v.sent_count += 1;
                        }
                        v.sent[index] = 1;
                        v.sent_at[index] = s.clock;
                        return Ok(Some(out));
                    }
                    return Ok(None);
                }
                _ => return Ok(None),
            }
        } else {
            if peer != self.binding.host {
                return Ok(None);
            }
            let own = bit(self.self_peer);
            match kind {
                Kind::Armed => {
                    if s.value.closing.is_some()
                        || s.value.armed & own == 0
                        || s.value.rejected & own != 0
                        || !self.due(s, p, kind)
                    {
                        return Ok(None);
                    }
                }
                Kind::Captured => {
                    if s.value.closing.is_some()
                        || s.value.captured & own == 0
                        || s.incoming.is_none()
                        || !self.due(s, p, kind)
                    {
                        return Ok(None);
                    }
                    let v = s.incoming.as_ref().unwrap();
                    let mut out = header(self.protocol, token, peer, kind);
                    put(&mut out, v.extent, 8);
                    out.extend_from_slice(&v.digest);
                    Self::mark(s, p, kind);
                    return Ok(Some(out));
                }
                Kind::Released | Kind::Aborted => {
                    if s.value.closing.is_none()
                        || s.value.retired & own == 0
                        || (s.value.closing == Some(Close::Release)) != (kind == Kind::Released)
                        || !self.due(s, p, kind)
                    {
                        return Ok(None);
                    }
                }
                Kind::Reject => {
                    if s.value.closing.is_some()
                        || s.value.rejected & own == 0
                        || !self.due(s, p, kind)
                    {
                        return Ok(None);
                    }
                }
                Kind::Ack => {
                    if s.value.closing.is_some()
                        || s.value.rejected & own != 0
                        || s.incoming.as_ref().is_none_or(|v| v.hashing)
                    {
                        return Ok(None);
                    }
                    let v = s.incoming.as_mut().unwrap();
                    let mut out = header(self.protocol, token, peer, kind);
                    if v.ack_all {
                        put(&mut out, u32::MAX.into(), 4);
                        put(&mut out, 0, 8);
                        v.ack_all = false;
                        return Ok(Some(out));
                    }
                    for _ in 0..v.dirty.len() {
                        let group = v.dirty_cursor as usize % v.dirty.len();
                        v.dirty_cursor = v.dirty_cursor.wrapping_add(1);
                        if v.dirty[group] == 0 {
                            continue;
                        }
                        v.dirty[group] = 0;
                        let base = group * 64;
                        let mut bits = 0u64;
                        for i in 0..64 {
                            if base + i < v.received.len() && v.received[base + i] != 0 {
                                bits |= 1u64 << i;
                            }
                        }
                        put(&mut out, base as u64, 4);
                        put(&mut out, bits, 8);
                        return Ok(Some(out));
                    }
                    return Ok(None);
                }
                _ => return Ok(None),
            }
        }
        let out = header(self.protocol, token, peer, kind);
        Self::mark(s, p, kind);
        Ok(Some(out))
    }
    pub fn poll(&self, now: u64, budget: u32) -> Result<Vec<Packet>> {
        let mut s = self.lock()?;
        self.observe(&mut s, now);
        let budget = budget.min(64) as usize;
        let mut out = Vec::with_capacity(budget);
        while out.len() < budget {
            let mut progress = false;
            for _ in 0..16 * TYPES {
                if out.len() >= budget {
                    break;
                }
                let index = s.cursor as usize % (16 * TYPES);
                s.cursor = s.cursor.wrapping_add(1);
                let kind = match index % TYPES {
                    0 => Kind::Arm,
                    1 => Kind::Armed,
                    2 => Kind::Offer,
                    3 => Kind::Chunk,
                    4 => Kind::Ack,
                    5 => Kind::Captured,
                    6 => Kind::Release,
                    7 => Kind::Abort,
                    8 => Kind::Released,
                    9 => Kind::Aborted,
                    _ => Kind::Reject,
                };
                if let Some(bytes) = self.make(&mut s, (index / TYPES) as u32, kind)? {
                    out.push(Packet {
                        peer: (index / TYPES) as u32,
                        bytes,
                    });
                    progress = true;
                }
            }
            if !progress {
                break;
            }
        }
        Ok(out)
    }
    fn receive_locked(
        &self,
        peer: u32,
        packet: &[u8],
        now: u64,
    ) -> Result<Option<(Token, [u8; 32], B)>> {
        let mut r = Reader {
            bytes: packet,
            at: 0,
        };
        let (kind, target, token) = r.message(self.protocol)?;
        let mut s = self.lock()?;
        self.observe(&mut s, now);
        need(
            peer != self.self_peer
                && member(self.binding, peer)
                && target == self.self_peer
                && self.exact(token),
        )?;
        let host_control = matches!(
            kind,
            Kind::Arm | Kind::Offer | Kind::Chunk | Kind::Release | Kind::Abort
        );
        need(if host_control {
            peer == self.binding.host
        } else {
            self.self_peer == self.binding.host && self.recipient(peer)
        })?;
        if kind == Kind::Arm {
            need(r.done())?;
            if self.matches(&s, token) {
                return Ok(None);
            }
            need(
                token.nonce > s.highest_nonce
                    && (s.value.pending.is_none() || self.ended(&s))
                    && !s.checked_out,
            )?;
            self.install(&mut s, token);
            return Ok(None);
        }
        need(self.matches(&s, token))?;
        if kind == Kind::Release || kind == Kind::Abort {
            need(r.done())?;
            if kind == Kind::Release {
                need(s.value.captured & bit(self.self_peer) != 0)?;
                if s.value.closing.is_none() {
                    s.value.closing = Some(Close::Release);
                    Self::changed(&mut s);
                } else {
                    need(s.value.closing == Some(Close::Release))?;
                }
            } else {
                need(Self::close_locked(&mut s, Close::Abort))?;
            }
            return Ok(None);
        }
        if kind == Kind::Released || kind == Kind::Aborted {
            need(
                r.done()
                    && s.value.closing.is_some()
                    && (s.value.closing == Some(Close::Release)) == (kind == Kind::Released),
            )?;
            if s.value.retired & bit(peer) == 0 {
                s.value.retired |= bit(peer);
                Self::changed(&mut s);
            }
            return Ok(None);
        }
        if kind == Kind::Reject {
            need(r.done() && (s.value.closing.is_none() || s.value.closing == Some(Close::Abort)))?;
            s.value.rejected |= bit(peer);
            need(Self::close_locked(&mut s, Close::Abort))?;
            return Ok(None);
        }
        need(s.value.closing.is_none())?;
        let p = peer as usize;
        match kind {
            Kind::Armed => {
                need(r.done())?;
                if s.value.armed & bit(peer) == 0 {
                    s.value.armed |= bit(peer);
                    Self::changed(&mut s);
                }
            }
            Kind::Offer => {
                let extent = r.get(8)?;
                let digest: [u8; 32] = r.raw(32)?.try_into().map_err(|_| Error)?;
                let count = r.get(4)?;
                need(
                    r.done()
                        && s.value.armed & bit(self.self_peer) != 0
                        && s.value.rejected & bit(self.self_peer) == 0
                        && extent > 0
                        && extent <= self.limits.maximum_archive_bytes
                        && count == chunks(extent) as u64,
                )?;
                if let Some(v) = s.incoming.as_mut() {
                    need(v.extent == extent && v.digest == digest)?;
                    if v.complete {
                        v.ack_all = true;
                    }
                    return Ok(None);
                }
                let incoming = Incoming {
                    digest,
                    extent,
                    bytes: Some((self.allocate)(extent as usize)?),
                    received: zeros(count as usize)?,
                    dirty: zeros(count.div_ceil(64) as usize)?,
                    count: 0,
                    dirty_cursor: 0,
                    hashing: false,
                    complete: false,
                    ack_all: false,
                };
                s.incoming = Some(incoming);
                s.local_bytes = extent;
                s.value.published = true;
                Self::changed(&mut s);
            }
            Kind::Chunk => {
                need(s.value.armed & bit(self.self_peer) != 0)?;
                if s.incoming.is_none() || s.value.rejected & bit(self.self_peer) != 0 {
                    return Ok(None);
                }
                let v = s.incoming.as_mut().unwrap();
                let index = r.get(4)? as usize;
                need(index < v.received.len())?;
                let start = index * CHUNK;
                let extent = CHUNK.min(v.extent as usize - start);
                let data = r.raw(extent)?;
                need(r.done())?;
                if v.hashing {
                    return Ok(None);
                }
                if v.complete {
                    v.ack_all = true;
                    return Ok(None);
                }
                let mut changed = false;
                if v.received[index] != 0 {
                    need(data == &v.bytes.as_ref().ok_or(Error)?.bytes()[start..start + extent])?;
                } else {
                    v.bytes.as_mut().ok_or(Error)?.write(start, data)?;
                    v.received[index] = 1;
                    v.count += 1;
                    changed = true;
                }
                v.dirty[index / 64] = 1;
                let finished = if v.count == v.received.len() {
                    let bytes = v.bytes.take().ok_or(Error)?;
                    v.hashing = true;
                    Some((token, v.digest, bytes))
                } else {
                    None
                };
                if changed {
                    Self::changed(&mut s);
                }
                if finished.is_some() {
                    return Ok(finished);
                }
            }
            Kind::Ack => {
                let base = r.get(4)?;
                let bits = r.get(8)?;
                need(r.done() && s.senders[p].is_some())?;
                let v = s.senders[p].as_mut().unwrap();
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
                let changed = v.begin != before;
                let delivered = v.begin == v.acked.len();
                if changed {
                    Self::changed(&mut s);
                }
                if delivered {
                    s.value.delivered |= bit(peer);
                }
            }
            Kind::Captured => {
                let extent = r.get(8)?;
                let digest: [u8; 32] = r.raw(32)?.try_into().map_err(|_| Error)?;
                let v = s.senders[p].as_ref().ok_or(Error)?;
                need(
                    r.done()
                        && v.extent == extent
                        && v.digest == digest
                        && v.sent_count == v.sent.len(),
                )?;
                if s.value.captured & bit(peer) == 0 {
                    s.value.delivered |= bit(peer);
                    s.value.captured |= bit(peer);
                    s.senders[p].as_mut().unwrap().bytes = None;
                    Self::changed(&mut s);
                }
            }
            _ => {}
        }
        Ok(None)
    }
    fn finish_received(&self, token: Token, expected: [u8; 32], bytes: B) -> Result<()> {
        let actual: [u8; 32] = Sha256::digest(bytes.bytes()).into();
        let mut s = self.lock()?;
        if !self.matches(&s, token) || s.incoming.as_ref().is_none_or(|v| !v.hashing) {
            return Ok(());
        }
        if s.value.closing.is_some() {
            drop(bytes);
            s.incoming.as_mut().unwrap().hashing = false;
            s.local_bytes = 0;
            return Ok(());
        }
        if actual != expected {
            drop(bytes);
            s.incoming.as_mut().unwrap().hashing = false;
            s.local_bytes = 0;
            s.value.rejected |= bit(self.self_peer);
            Self::changed(&mut s);
            return Ok(());
        }
        let v = s.incoming.as_mut().unwrap();
        v.hashing = false;
        v.complete = true;
        v.ack_all = true;
        v.dirty.fill(0);
        s.completed = Some(bytes);
        s.value.delivered |= bit(self.self_peer);
        Self::changed(&mut s);
        Ok(())
    }
    pub fn receive(&self, peer: u32, packet: &[u8], now: u64) -> bool {
        if !is_packet(packet) {
            return false;
        }
        let result = (|| -> Result<()> {
            if let Some((token, digest, bytes)) = self.receive_locked(peer, packet, now)? {
                self.finish_received(token, digest, bytes)?;
            }
            Ok(())
        })();
        if result.is_err() {
            if let Ok(mut s) = self.lock() {
                s.value.rejected_packets = s.value.rejected_packets.saturating_add(1);
            }
        }
        true
    }
}
pub fn is_packet(bytes: &[u8]) -> bool {
    bytes.len() >= 7 && &bytes[..4] == b"H2N2" && bytes[6] == 12
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::cell::{Cell, RefCell};
    use std::sync::{
        Arc, Condvar,
        atomic::{AtomicUsize, Ordering},
    };
    use std::time::Duration;

    #[derive(Default)]
    struct Gate {
        state: Mutex<(bool, bool)>,
        changed: Condvar,
    }
    impl Gate {
        fn block(&self) {
            let mut s = self.state.lock().unwrap();
            s.0 = true;
            self.changed.notify_all();
            let (s, _) = self
                .changed
                .wait_timeout_while(s, Duration::from_secs(5), |s| !s.1)
                .unwrap();
            assert!(s.1, "bounded test gate was not released");
        }
        fn entered(&self) {
            let s = self.state.lock().unwrap();
            let (s, _) = self
                .changed
                .wait_timeout_while(s, Duration::from_secs(5), |s| !s.0)
                .unwrap();
            assert!(s.0, "bounded test gate was not reached");
        }
        fn release(&self) {
            self.state.lock().unwrap().1 = true;
            self.changed.notify_all();
        }
    }
    struct Bank {
        data: Vec<u8>,
        reads: AtomicUsize,
        read_gate: Option<Arc<Gate>>,
        drop_gate: Option<Arc<Gate>>,
        drops: Option<Arc<AtomicUsize>>,
    }
    impl Bank {
        fn new(data: Vec<u8>) -> Self {
            Self {
                data,
                reads: AtomicUsize::new(0),
                read_gate: None,
                drop_gate: None,
                drops: None,
            }
        }
    }
    impl Drop for Bank {
        fn drop(&mut self) {
            if let Some(g) = &self.drop_gate {
                g.block();
            }
            if let Some(d) = &self.drops {
                d.fetch_add(1, Ordering::SeqCst);
            }
        }
    }
    impl Buffer for Bank {
        fn bytes(&self) -> &[u8] {
            if self.reads.fetch_add(1, Ordering::SeqCst) == 3 {
                if let Some(g) = &self.read_gate {
                    g.block();
                }
            }
            &self.data
        }
        fn write(&mut self, offset: usize, data: &[u8]) -> Result<()> {
            let end = offset.checked_add(data.len()).ok_or(Error)?;
            self.data
                .get_mut(offset..end)
                .ok_or(Error)?
                .copy_from_slice(data);
            Ok(())
        }
    }
    fn allocate(n: usize) -> Result<Bank> {
        Ok(Bank::new(vec![0; n]))
    }
    #[derive(Default)]
    struct Lease {
        witness: Cell<Witness>,
        bank: RefCell<Option<Bank>>,
        destroys: Cell<usize>,
    }
    impl LeaseIo<Bank> for Lease {
        fn witness(&self) -> Witness {
            self.witness.get()
        }
        fn issue(&self, t: Token, b: Bank) {
            assert!(self.bank.borrow().is_none());
            *self.bank.borrow_mut() = Some(b);
            self.witness.set(Witness {
                present: true,
                owner_matches: true,
                recipient: 4,
                token: Some(t),
                bytes_present: true,
            });
        }
        fn destroy(&self) {
            self.bank.borrow_mut().take();
            self.witness.set(Witness::default());
            self.destroys.set(self.destroys.get() + 1);
        }
    }
    fn context() -> Context {
        Context {
            session: [7; 32],
            epoch: 3,
            membership_revision: 5,
            peers: 0x11,
            host: 0,
        }
    }
    fn limits() -> Limits {
        Limits {
            maximum_archive_bytes: 8192,
            retry_ms: 10,
            idle_ms: 100,
            window_chunks: 8,
        }
    }
    fn node(p: u32) -> Candidate<Bank> {
        Candidate::new(p, 124, context(), limits(), allocate).unwrap()
    }
    fn armed() -> (Candidate<Bank>, Candidate<Bank>, Token) {
        let (host, client) = (node(0), node(4));
        let t = host.begin(0).unwrap().unwrap();
        assert!(client.receive(0, &header(124, t, 4, Kind::Arm), 0));
        assert!(client.acknowledge_arm(t).unwrap());
        assert!(host.receive(4, &header(124, t, 0, Kind::Armed), 0));
        (host, client, t)
    }
    fn offer(t: Token, data: &[u8]) -> Vec<u8> {
        let mut b = header(124, t, 4, Kind::Offer);
        put(&mut b, data.len() as u64, 8);
        b.extend_from_slice(&Sha256::digest(data));
        put(&mut b, chunks(data.len() as u64) as u64, 4);
        b
    }
    fn chunk(t: Token, i: usize, data: &[u8]) -> Vec<u8> {
        let mut b = header(124, t, 4, Kind::Chunk);
        put(&mut b, i as u64, 4);
        b.extend_from_slice(data);
        b
    }
    fn bytes_ready(client: &Candidate<Bank>, t: Token, data: &[u8]) {
        assert!(client.receive(0, &offer(t, data), 0));
        for (i, row) in data.chunks(CHUNK).enumerate() {
            assert!(client.receive(0, &chunk(t, i, row), 0));
        }
    }
    fn batch(data: &[u8]) -> Vec<Archive<Bank>> {
        vec![Archive {
            peer: 4,
            bytes: Bank::new(data.to_vec()),
        }]
    }

    #[test]
    fn full_width_roles_and_original_limit_guards() {
        for p in [1, 16, 256, 257, u32::MAX] {
            assert!(Candidate::new(p, 124, context(), limits(), allocate).is_err());
        }
        for p in [16, 256, 257, u32::MAX] {
            let mut c = context();
            c.host = p;
            assert!(Candidate::new(0, 124, c, limits(), allocate).is_err());
        }
        for window in [0, 257, 65537, u32::MAX] {
            let mut l = limits();
            l.window_chunks = window;
            assert!(Candidate::new(0, 124, context(), l, allocate).is_err());
        }
        for size in [0, 9 * 1024 * 1024 + 1, u64::MAX] {
            let mut l = limits();
            l.maximum_archive_bytes = size;
            assert!(Candidate::new(0, 124, context(), l, allocate).is_err());
        }
        for retry in [0, 60001, u64::MAX] {
            let mut l = limits();
            l.retry_ms = retry;
            assert!(Candidate::new(0, 124, context(), l, allocate).is_err());
        }
        let c = node(0);
        assert_eq!(c.state().unwrap().maximum_buffered_bytes, 8192);
    }
    #[test]
    fn arm_receipt_is_not_foreground_arm_and_publication_is_atomic() {
        let (h, c) = (node(0), node(4));
        let t = h.begin(0).unwrap().unwrap();
        c.receive(0, &header(124, t, 4, Kind::Arm), 0);
        assert_eq!(c.state().unwrap().armed, 0);
        assert!(!h.publish(t, batch(&[1]), 0).unwrap());
        assert!(!h.acknowledge_arm(t).unwrap());
        assert!(c.acknowledge_arm(t).unwrap());
        h.receive(4, &header(124, t, 0, Kind::Armed), 0);
        let before = h.state().unwrap();
        assert!(!h.publish(t, vec![], 9999).unwrap());
        assert!(!h.publish(t, batch(&[]), 9999).unwrap());
        let mut duplicate = batch(&[1]);
        duplicate.extend(batch(&[1]));
        assert!(!h.publish(t, duplicate, 9999).unwrap());
        assert_eq!(h.state().unwrap(), before);
        assert!(h.publish(t, batch(&[1, 2]), 0).unwrap());
        assert!(!h.publish(t, batch(&[3]), 0).unwrap());
        assert_eq!(h.state().unwrap().buffered_bytes, 2);
    }
    #[test]
    fn capture_consumes_only_an_exact_issued_bank_and_does_not_retire() {
        let (h, c, t) = armed();
        let data = vec![91; 2051];
        bytes_ready(&c, t, &data);
        let lease = Lease::default();
        assert!(c.take_delivery(&lease).unwrap());
        assert!(!c.take_delivery(&Lease::default()).unwrap());
        let original = lease.witness.get();
        for wrong in [
            Witness {
                owner_matches: false,
                ..original
            },
            Witness {
                recipient: 256,
                ..original
            },
            Witness {
                token: Some(Token { nonce: 2, ..t }),
                ..original
            },
            Witness {
                bytes_present: false,
                ..original
            },
        ] {
            lease.witness.set(wrong);
            assert!(!c.acknowledge_captured(&lease).unwrap());
            assert_eq!(lease.destroys.get(), 0);
            assert_eq!(lease.bank.borrow().as_ref().unwrap().data, data);
        }
        lease.witness.set(original);
        assert!(!h.close(t, Close::Release).unwrap());
        assert!(c.acknowledge_captured(&lease).unwrap());
        assert_eq!(lease.destroys.get(), 1);
        assert!(!c.acknowledge_captured(&lease).unwrap());
        let s = c.state().unwrap();
        assert_eq!(s.captured, 0x10);
        assert_eq!(s.retired, 0);
        assert_eq!(s.buffered_bytes, 0);
        assert!(!s.delivery_outstanding);
        assert_eq!(h.state().unwrap().captured, 0);
        c.receive(0, &header(124, t, 4, Kind::Release), 0);
        assert!(c.can_acknowledge_retired(t, &lease).unwrap());
        assert!(c.acknowledge_retired(t, &lease).unwrap());
        assert!(c.acknowledge_retired(t, &lease).unwrap());
        assert_eq!(c.state().unwrap().retired, 0x10);
    }
    #[test]
    fn abort_rechecks_observation_and_preserves_refused_native_lease() {
        let (h, c, t) = armed();
        bytes_ready(&c, t, &[4; 1025]);
        let lease = Lease::default();
        c.take_delivery(&lease).unwrap();
        assert!(!c.can_acknowledge_retired(t, &lease).unwrap());
        assert_eq!(lease.destroys.get(), 0);
        assert!(h.close(t, Close::Abort).unwrap());
        c.receive(0, &header(124, t, 4, Kind::Abort), 0);
        assert!(!c.acknowledge_captured(&lease).unwrap());
        assert!(!c.acknowledge_retired(t, &Lease::default()).unwrap());
        assert!(c.can_acknowledge_retired(t, &lease).unwrap());
        let witness = lease.witness.get();
        lease.witness.set(Witness {
            owner_matches: false,
            ..witness
        });
        assert!(!c.acknowledge_retired(t, &lease).unwrap());
        assert_eq!(lease.destroys.get(), 0);
        assert_eq!(lease.bank.borrow().as_ref().unwrap().data.len(), 1025);
        lease.witness.set(witness);
        assert!(c.acknowledge_retired(t, &lease).unwrap());
        assert!(lease.bank.borrow().is_none());
        assert_eq!(lease.destroys.get(), 1);
        assert!(h.begin(1000000).unwrap().is_none());
        assert!(h.state().unwrap().stalled);
        h.receive(4, &header(124, t, 0, Kind::Aborted), 1000000);
        assert_eq!(h.begin(1000000).unwrap().unwrap().nonce, t.nonce + 1);
    }
    #[test]
    fn early_abort_retries_token_without_fabricating_foreground_acknowledgments() {
        let (h, c) = (node(0), node(4));
        let t = h.begin(0).unwrap().unwrap();
        h.close(t, Close::Abort).unwrap();
        let packets = h.poll(0, 64).unwrap();
        assert!(packets.iter().any(|p| p.bytes[8] == 0));
        assert!(packets.iter().any(|p| p.bytes[8] == 7));
        c.receive(0, &header(124, t, 4, Kind::Abort), 0);
        assert!(c.state().unwrap().pending.is_none());
        c.receive(0, &header(124, t, 4, Kind::Arm), 0);
        c.receive(0, &header(124, t, 4, Kind::Abort), 0);
        assert_eq!(c.state().unwrap().armed, 0);
        assert_eq!(c.state().unwrap().retired, 0);
        assert!(h.begin(9999).unwrap().is_none());
        assert_eq!(h.state().unwrap().recipients, 0x10);
        c.acknowledge_retired(t, &Lease::default()).unwrap();
        h.receive(4, &header(124, t, 0, Kind::Aborted), 9999);
        assert_eq!(h.begin(9999).unwrap().unwrap().nonce, 2);
    }
    #[test]
    fn hashing_holds_retirement_until_the_actual_bank_is_destroyed() {
        for abort in [false, true] {
            let (_, c, t) = armed();
            let data = vec![5; 13];
            c.receive(0, &offer(t, &data), 0);
            let (token, digest, mut bank) = c
                .receive_locked(0, &chunk(t, 0, &data), 0)
                .unwrap()
                .unwrap();
            let drops = Arc::new(AtomicUsize::new(0));
            bank.drops = Some(drops.clone());
            if abort {
                c.receive(0, &header(124, t, 4, Kind::Abort), 0);
            } else {
                bank.data[0] ^= 1;
            }
            assert!(!c.can_acknowledge_retired(t, &Lease::default()).unwrap());
            assert_eq!(drops.load(Ordering::SeqCst), 0);
            assert_eq!(c.state().unwrap().buffered_bytes, 13);
            c.finish_received(token, digest, bank).unwrap();
            assert_eq!(drops.load(Ordering::SeqCst), 1);
            assert_eq!(c.state().unwrap().buffered_bytes, 0);
            assert!(!c.take_delivery(&Lease::default()).unwrap());
            if !abort {
                assert_eq!(c.state().unwrap().rejected, 0x10);
                c.receive(0, &header(124, t, 4, Kind::Abort), 0);
            }
            assert!(c.acknowledge_retired(t, &Lease::default()).unwrap());
        }
    }
    #[test]
    fn aborted_publication_reservation_lasts_through_bank_destruction_outside_mutex() {
        let (h, _, t) = armed();
        let h = Arc::new(h);
        let reads = Arc::new(Gate::default());
        let destruction = Arc::new(Gate::default());
        let mut bank = Bank::new(vec![7; 128]);
        bank.read_gate = Some(reads.clone());
        bank.drop_gate = Some(destruction.clone());
        let sender = h.clone();
        let thread = std::thread::spawn(move || {
            sender.publish(
                t,
                vec![Archive {
                    peer: 4,
                    bytes: bank,
                }],
                0,
            )
        });
        reads.entered();
        assert_eq!(h.state().unwrap().buffered_bytes, 128);
        h.close(t, Close::Abort).unwrap();
        h.receive(4, &header(124, t, 0, Kind::Aborted), 0);
        assert!(h.begin(0).unwrap().is_none());
        reads.release();
        destruction.entered();
        // These observations must finish while the bank destructor is blocked.
        assert_eq!(h.state().unwrap().buffered_bytes, 128);
        assert!(h.begin(0).unwrap().is_none());
        destruction.release();
        assert!(!thread.join().unwrap().unwrap());
        assert_eq!(h.state().unwrap().buffered_bytes, 0);
        assert_eq!(h.begin(0).unwrap().unwrap().nonce, 2);
    }
    #[test]
    fn recognition_rejection_saturation_and_clock_rollback_match_snapshot_contract() {
        let h = node(0);
        let t = h.begin(0).unwrap().unwrap();
        assert!(!h.receive(4, b"other", u64::MAX));
        let mut zero_protocol = header(0, t, 0, Kind::Armed);
        assert!(is_packet(&zero_protocol));
        assert!(h.receive(4, &zero_protocol, u64::MAX));
        assert_eq!(h.lock().unwrap().clock, 0);
        zero_protocol[4] = 124;
        h.receive(16, &zero_protocol, 100);
        assert_eq!(h.lock().unwrap().clock, 100);
        assert!(h.state().unwrap().stalled);
        h.poll(0, 0).unwrap();
        assert!(h.state().unwrap().stalled);
        h.lock().unwrap().value.rejected_packets = u64::MAX;
        h.receive(256, &zero_protocol, 0);
        assert_eq!(h.state().unwrap().rejected_packets, u64::MAX);
        assert_eq!(h.state().unwrap().armed, 0);
    }
}
