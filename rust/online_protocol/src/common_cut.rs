//! Exact physical-peer common-cut metadata and its bounded control codec.
//!
//! The caller retains native/frontier owners and explicitly reports their
//! association, application and retirement. No received control creates a
//! native owner, full migration readiness, election or role activation.

pub use crate::host_agreement::{Context, Cut, Packet};

const PEERS: usize = 16;
const TYPES: usize = 9;
const PENDING: u32 = (PEERS * TYPES) as u32;
const KIND: u8 = 11;
const PACKET_BYTES: usize = 116;
const RETRY_MS: u64 = 100;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CommonCut {
    pub context: Context,
    pub seal: Cut,
    pub recipients: u16,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct State {
    pub context: Context,
    pub recipients: u16,
    pub committed: Option<CommonCut>,
    pub pending: Option<CommonCut>,
    pub associated: u16,
    pub applied: u16,
    pub released: u16,
    pub aborted: u16,
    /// Host assertion from an explicit confirmation or authenticated Commit.
    /// This does not certify full native/presentation/input/audio readiness.
    pub native_all_validated: bool,
    pub commit: bool,
    pub release: bool,
    pub abort: bool,
    pub rejected: u64,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Invalid peer common-cut association or packet")
    }
}
impl std::error::Error for Error {}

fn need(ok: bool) -> Result<(), Error> {
    if ok { Ok(()) } else { Err(Error) }
}
fn member(context: Context, peer: u8) -> bool {
    peer < PEERS as u8 && context.peers & (1u16 << peer) != 0
}
fn valid(seal: Cut) -> bool {
    seal.id != 0
        && seal.bytes != 0
        && seal.bytes <= 64 * 1024 * 1024
        && seal.digest.iter().any(|&byte| byte != 0)
}
fn put(bytes: &mut Vec<u8>, value: u64, width: usize) {
    bytes.extend_from_slice(&value.to_le_bytes()[..width]);
}
fn write_context(bytes: &mut Vec<u8>, context: Context) {
    bytes.extend_from_slice(&context.session);
    put(bytes, u64::from(context.epoch), 4);
    put(bytes, context.membership_revision, 8);
    put(bytes, u64::from(context.peers), 2);
    bytes.push(context.host);
}
fn write_cut(bytes: &mut Vec<u8>, cut: CommonCut) {
    put(bytes, u64::from(cut.recipients), 2);
    put(bytes, cut.seal.id, 8);
    put(bytes, cut.seal.tick, 8);
    put(bytes, cut.seal.bytes, 8);
    bytes.extend_from_slice(&cut.seal.digest);
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
enum Type {
    Offer,
    Associated,
    Commit,
    Applied,
    Release,
    Released,
    Reject,
    Abort,
    Aborted,
}
impl TryFrom<u8> for Type {
    type Error = Error;
    fn try_from(value: u8) -> Result<Self, Error> {
        match value {
            0 => Ok(Self::Offer),
            1 => Ok(Self::Associated),
            2 => Ok(Self::Commit),
            3 => Ok(Self::Applied),
            4 => Ok(Self::Release),
            5 => Ok(Self::Released),
            6 => Ok(Self::Reject),
            7 => Ok(Self::Abort),
            8 => Ok(Self::Aborted),
            _ => Err(Error),
        }
    }
}
struct Message {
    kind: Type,
    sender: u8,
    target: u8,
    cut: CommonCut,
}
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl<'a> Reader<'a> {
    fn raw(&mut self, width: usize) -> Result<&'a [u8], Error> {
        let end = self.at.checked_add(width).ok_or(Error)?;
        let bytes = self.bytes.get(self.at..end).ok_or(Error)?;
        self.at = end;
        Ok(bytes)
    }
    fn word(&mut self, width: usize) -> Result<u64, Error> {
        let mut value = [0; 8];
        value
            .get_mut(..width)
            .ok_or(Error)?
            .copy_from_slice(self.raw(width)?);
        Ok(u64::from_le_bytes(value))
    }
    fn byte(&mut self) -> Result<u8, Error> {
        Ok(self.word(1)? as u8)
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
}
fn decode(packet: &[u8], protocol: u16) -> Result<Message, Error> {
    need(packet.len() == PACKET_BYTES)?;
    let mut reader = Reader {
        bytes: packet,
        at: 0,
    };
    need(
        reader.word(4)? == 0x324e3248
            && reader.word(2)? == u64::from(protocol)
            && reader.byte()? == KIND
            && reader.byte()? == 1,
    )?;
    let kind = Type::try_from(reader.byte()?)?;
    let context = reader.context()?;
    let sender = reader.byte()?;
    let target = reader.byte()?;
    let recipients = reader.word(2)? as u16;
    let seal = Cut {
        id: reader.word(8)?,
        tick: reader.word(8)?,
        bytes: reader.word(8)?,
        digest: reader.raw(32)?.try_into().map_err(|_| Error)?,
    };
    need(reader.at == packet.len())?;
    Ok(Message {
        kind,
        sender,
        target,
        cut: CommonCut {
            context,
            seal,
            recipients,
        },
    })
}

pub struct Agreement {
    self_peer: u8,
    binding: Context,
    protocol: u16,
    recipients: u16,
    value: State,
    local_rejected: bool,
    dirty: bool,
    emitted: bool,
    clock: u64,
    emitted_at: u64,
    highest_id: u64,
    highest_tick: u64,
    packets: [[Option<Vec<u8>>; TYPES]; PEERS],
    cursor: u32,
}
impl Agreement {
    pub fn new(self_peer: u8, context: Context, protocol: u16) -> Result<Self, Error> {
        need(
            member(context, self_peer)
                && member(context, context.host)
                && context.session.iter().any(|&byte| byte != 0)
                && context.peers.count_ones() >= 2,
        )?;
        let recipients = context.peers & !(1u16 << context.host);
        Ok(Self {
            self_peer,
            binding: context,
            protocol,
            recipients,
            value: State {
                context,
                recipients,
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
            },
            local_rejected: false,
            dirty: true,
            emitted: false,
            clock: 0,
            emitted_at: 0,
            highest_id: 0,
            highest_tick: 0,
            packets: std::array::from_fn(|_| std::array::from_fn(|_| None)),
            cursor: 0,
        })
    }
    pub fn context(&self) -> Context {
        self.binding
    }
    pub fn state(&self) -> State {
        self.value
    }
    fn recipient(&self, peer: u8) -> bool {
        peer < PEERS as u8 && self.recipients & (1u16 << peer) != 0
    }
    fn exact(&self, cut: CommonCut) -> bool {
        cut.context == self.binding && cut.recipients == self.recipients && valid(cut.seal)
    }
    fn matches(&self, cut: CommonCut) -> bool {
        self.exact(cut) && self.value.pending == Some(cut)
    }
    fn own(&self) -> u16 {
        1u16 << self.self_peer
    }
    fn ended(&self) -> bool {
        if self.value.pending.is_none() {
            return true;
        }
        if self.self_peer == self.binding.host {
            if self.value.abort {
                self.value.aborted == self.recipients
            } else {
                self.value.release && self.value.released == self.recipients
            }
        } else if self.value.abort {
            self.value.aborted & self.own() != 0
        } else {
            self.value.release && self.value.released & self.own() != 0
        }
    }
    fn progress(&mut self) {
        if self.self_peer != self.binding.host || self.value.pending.is_none() || self.value.abort {
            return;
        }
        if !self.value.commit
            && self.value.native_all_validated
            && self.value.associated == self.recipients
        {
            self.value.commit = true;
            self.dirty = true;
        }
        if self.value.commit && !self.value.release && self.value.applied == self.recipients {
            self.value.release = true;
            self.dirty = true;
        }
        if self.value.release && self.value.released == self.recipients {
            self.value.committed = self.value.pending;
        }
    }
    fn clear_packets(&mut self) {
        for peer in &mut self.packets {
            peer.fill(None);
        }
    }
    fn offer_impl(&mut self, cut: CommonCut) -> bool {
        if !self.exact(cut) {
            return false;
        }
        if self.value.pending == Some(cut) {
            return true;
        }
        if !self.ended() || cut.seal.id <= self.highest_id || cut.seal.tick < self.highest_tick {
            return false;
        }
        self.highest_id = cut.seal.id;
        self.highest_tick = cut.seal.tick;
        self.value.pending = Some(cut);
        self.value.associated = 0;
        self.value.applied = 0;
        self.value.released = 0;
        self.value.aborted = 0;
        self.value.native_all_validated = false;
        self.value.commit = false;
        self.value.release = false;
        self.value.abort = false;
        self.local_rejected = false;
        self.clear_packets();
        self.dirty = true;
        true
    }
    fn abort_impl(&mut self, cut: CommonCut) -> bool {
        if !self.matches(cut) || self.value.commit || self.value.release {
            return false;
        }
        if self.value.abort {
            return true;
        }
        self.value.abort = true;
        self.clear_packets();
        self.dirty = true;
        true
    }
    fn send(&mut self, target: u8, kind: Type) {
        if target == self.self_peer || !member(self.binding, target) {
            return;
        }
        let Some(cut) = self.value.pending else {
            return;
        };
        let mut bytes = Vec::with_capacity(PACKET_BYTES);
        bytes.extend_from_slice(b"H2N2");
        put(&mut bytes, u64::from(self.protocol), 2);
        bytes.extend_from_slice(&[KIND, 1, kind as u8]);
        write_context(&mut bytes, self.binding);
        bytes.extend_from_slice(&[self.self_peer, target]);
        write_cut(&mut bytes, cut);
        self.packets[target as usize][kind as usize] = Some(bytes);
    }
    fn broadcast(&mut self, kind: Type) {
        for peer in 0..PEERS as u8 {
            self.send(peer, kind);
        }
    }
    fn refresh(&mut self) {
        if self.value.pending.is_none() {
            return;
        }
        if self.self_peer == self.binding.host {
            self.broadcast(Type::Offer);
            if self.value.abort {
                self.broadcast(Type::Abort);
            } else {
                if self.value.commit {
                    self.broadcast(Type::Commit);
                }
                if self.value.release {
                    self.broadcast(Type::Release);
                }
            }
        } else {
            let own = self.own();
            if !self.value.abort {
                if self.value.associated & own != 0 {
                    self.broadcast(Type::Associated);
                }
                if self.value.applied & own != 0 {
                    self.broadcast(Type::Applied);
                }
                if self.value.released & own != 0 {
                    self.broadcast(Type::Released);
                }
            }
            if self.local_rejected && !self.value.abort {
                self.send(self.binding.host, Type::Reject);
            }
            if self.value.aborted & own != 0 {
                self.broadcast(Type::Aborted);
            }
        }
    }
    pub fn offer(&mut self, cut: CommonCut) -> bool {
        self.self_peer == self.binding.host && self.offer_impl(cut)
    }
    pub fn confirm_native(&mut self, cut: CommonCut) -> bool {
        if self.self_peer != self.binding.host || !self.matches(cut) || self.value.abort {
            return false;
        }
        if !self.value.native_all_validated {
            self.value.native_all_validated = true;
            self.dirty = true;
            self.progress();
        }
        true
    }
    pub fn associate(&mut self, cut: CommonCut) -> bool {
        if !self.recipient(self.self_peer)
            || !self.matches(cut)
            || self.value.abort
            || self.local_rejected
        {
            return false;
        }
        if self.value.associated & self.own() == 0 {
            self.value.associated |= self.own();
            self.dirty = true;
        }
        true
    }
    pub fn apply(&mut self, cut: CommonCut) -> bool {
        if !self.recipient(self.self_peer)
            || !self.matches(cut)
            || !self.value.commit
            || self.value.abort
            || self.value.associated != self.recipients
        {
            return false;
        }
        if self.value.applied & self.own() == 0 {
            self.value.applied |= self.own();
            self.dirty = true;
        }
        true
    }
    pub fn release(&mut self, cut: CommonCut) -> bool {
        if !self.recipient(self.self_peer)
            || !self.matches(cut)
            || !self.value.release
            || self.value.abort
            || self.value.applied != self.recipients
        {
            return false;
        }
        if self.value.released & self.own() == 0 {
            self.value.released |= self.own();
            self.value.committed = self.value.pending;
            self.dirty = true;
        }
        true
    }
    pub fn reject(&mut self, cut: CommonCut) -> bool {
        if !self.recipient(self.self_peer)
            || !self.matches(cut)
            || self.value.commit
            || self.value.abort
            || self.value.associated & self.own() != 0
        {
            return false;
        }
        if !self.local_rejected {
            self.local_rejected = true;
            self.dirty = true;
        }
        true
    }
    pub fn abort(&mut self, cut: CommonCut) -> bool {
        self.self_peer == self.binding.host && self.abort_impl(cut)
    }
    pub fn acknowledge_abort(&mut self, cut: CommonCut) -> bool {
        if !self.recipient(self.self_peer)
            || !self.matches(cut)
            || !self.value.abort
            || self.value.commit
        {
            return false;
        }
        if self.value.aborted & self.own() == 0 {
            self.value.aborted |= self.own();
            self.dirty = true;
        }
        true
    }
    pub fn poll(&mut self, now: u64, budget: u32) -> Vec<Packet> {
        self.clock = self.clock.max(now);
        if self.dirty || !self.emitted || self.clock - self.emitted_at >= RETRY_MS {
            self.refresh();
            self.dirty = false;
            self.emitted = true;
            self.emitted_at = self.clock;
        }
        let budget = budget.min(128) as usize;
        let mut result = Vec::new();
        for _ in 0..PENDING {
            if result.len() >= budget {
                break;
            }
            // Legacy unsigned cursor wraps before modulo144. Keeping only a
            // reduced cursor changes the scan at wrap (2^32 mod144 is112).
            let index = (self.cursor % PENDING) as usize;
            self.cursor = self.cursor.wrapping_add(1);
            if let Some(bytes) = self.packets[index / TYPES][index % TYPES].take() {
                result.push(Packet {
                    peer: (index / TYPES) as u8,
                    bytes,
                });
            }
        }
        result
    }
    fn apply_packet(&mut self, peer: u8, packet: &[u8]) -> Result<(), Error> {
        let message = decode(packet, self.protocol)?;
        need(
            message.sender == peer
                && message.target == self.self_peer
                && peer != self.self_peer
                && member(self.binding, peer)
                && self.exact(message.cut),
        )?;
        let host_control = matches!(
            message.kind,
            Type::Offer | Type::Commit | Type::Release | Type::Abort
        );
        need(if host_control {
            peer == self.binding.host
        } else {
            self.recipient(peer)
        })?;
        if message.kind == Type::Offer {
            return need(self.offer_impl(message.cut));
        }
        need(self.matches(message.cut))?;
        if message.kind == Type::Abort {
            return need(self.abort_impl(message.cut));
        }
        let bit = 1u16 << peer;
        if message.kind == Type::Aborted {
            need(self.value.abort && !self.value.commit)?;
            self.value.aborted |= bit;
            return Ok(());
        }
        if message.kind == Type::Reject {
            need(
                self.self_peer == self.binding.host
                    && !self.value.commit
                    && self.value.associated & bit == 0,
            )?;
            return need(self.abort_impl(message.cut));
        }
        need(!self.value.abort)?;
        match message.kind {
            Type::Associated => {
                self.value.associated |= bit;
                self.progress();
            }
            Type::Commit => {
                need(
                    self.recipient(self.self_peer)
                        && !self.local_rejected
                        && self.value.associated & self.own() != 0,
                )?;
                if !self.value.commit {
                    self.value.native_all_validated = true;
                    self.value.commit = true;
                    self.dirty = true;
                }
            }
            Type::Applied => {
                need(self.value.commit && self.value.associated & bit != 0)?;
                self.value.applied |= bit;
                self.progress();
            }
            Type::Release => {
                need(
                    self.recipient(self.self_peer)
                        && self.value.commit
                        && self.value.applied & self.own() != 0,
                )?;
                if !self.value.release {
                    self.value.release = true;
                    self.dirty = true;
                }
            }
            Type::Released => {
                need(self.value.release && self.value.applied & bit != 0)?;
                self.value.released |= bit;
                self.progress();
            }
            // These were handled above, in original receive order.
            Type::Offer | Type::Abort | Type::Aborted | Type::Reject => {}
        }
        Ok(())
    }
    pub fn receive(&mut self, peer: u8, packet: &[u8], now: u64) -> bool {
        if !is_packet(packet) {
            return false;
        }
        self.clock = self.clock.max(now);
        if self.apply_packet(peer, packet).is_err() {
            self.value.rejected = self.value.rejected.saturating_add(1);
        }
        true
    }
}

pub fn is_packet(packet: &[u8]) -> bool {
    packet.len() >= 7 && packet.get(..4) == Some(b"H2N2") && packet[6] == KIND
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::{BTreeMap, BTreeSet, VecDeque};

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
    fn cut(context: Context, id: u64, tick: u64) -> CommonCut {
        let mut digest = [0; 32];
        digest[0] = id as u8;
        CommonCut {
            context,
            recipients: context.peers & !(1u16 << context.host),
            seal: Cut {
                id,
                tick,
                bytes: 4096,
                digest,
            },
        }
    }
    fn safety(mut state: State) -> State {
        state.rejected = 0;
        state
    }
    #[derive(Clone)]
    struct Wire {
        from: u8,
        to: u8,
        bytes: Vec<u8>,
    }
    struct Mesh {
        context: Context,
        peers: [Option<Agreement>; PEERS],
        now: u64,
        serial: usize,
        samples: BTreeMap<(u8, u8, u8), Wire>,
        dropped: BTreeSet<(u8, u8, Vec<u8>)>,
        losses: usize,
        reorders: usize,
        duplicates: usize,
    }
    impl Mesh {
        fn new(members: u16, host: u8) -> Self {
            let context = context(members, host);
            Self {
                context,
                peers: std::array::from_fn(|peer| {
                    member(context, peer as u8)
                        .then(|| Agreement::new(peer as u8, context, 97).unwrap())
                }),
                now: 0,
                serial: 0,
                samples: BTreeMap::new(),
                dropped: BTreeSet::new(),
                losses: 0,
                reorders: 0,
                duplicates: 0,
            }
        }
        fn host(&mut self) -> &mut Agreement {
            self.peers[self.context.host as usize].as_mut().unwrap()
        }
        fn recipients(&mut self, mut action: impl FnMut(u8, &mut Agreement)) {
            for (peer, owner) in self.peers.iter_mut().enumerate() {
                if peer != self.context.host as usize
                    && let Some(owner) = owner
                {
                    action(peer as u8, owner);
                }
            }
        }
        fn step(&mut self, lossy: bool, budget: u32, allowed: impl Fn(&Wire) -> bool) {
            self.now += 101;
            let mut wire = VecDeque::new();
            for (from, peer) in self.peers.iter_mut().enumerate() {
                let Some(peer) = peer else {
                    continue;
                };
                for packet in peer.poll(self.now, budget) {
                    assert_eq!(packet.bytes.len(), 116);
                    assert!(is_packet(&packet.bytes));
                    assert!(packet.peer as usize != from && member(self.context, packet.peer));
                    let packet = Wire {
                        from: from as u8,
                        to: packet.peer,
                        bytes: packet.bytes,
                    };
                    self.samples
                        .entry((packet.bytes[8], packet.from, packet.to))
                        .or_insert_with(|| packet.clone());
                    if !allowed(&packet) {
                        continue;
                    }
                    if lossy
                        && self
                            .dropped
                            .insert((packet.from, packet.to, packet.bytes.clone()))
                    {
                        self.losses += 1;
                        continue;
                    }
                    self.serial += 1;
                    if lossy && self.serial.is_multiple_of(2) {
                        wire.push_front(packet.clone());
                        self.reorders += 1;
                    } else {
                        wire.push_back(packet.clone());
                    }
                    if lossy && self.serial.is_multiple_of(3) {
                        wire.push_back(packet);
                        self.duplicates += 1;
                    }
                }
            }
            for packet in wire {
                if allowed(&packet)
                    && let Some(peer) = &mut self.peers[packet.to as usize]
                {
                    assert!(peer.receive(packet.from, &packet.bytes, self.now));
                }
            }
        }
        fn pump(&mut self, steps: usize) {
            for _ in 0..steps {
                self.step(true, 4, |_| true);
            }
        }
        fn filtered(&mut self, steps: usize, allowed: impl Fn(&Wire) -> bool) {
            for _ in 0..steps {
                self.step(true, 4, &allowed);
            }
        }
        fn offer(&mut self, value: CommonCut) {
            assert!(self.host().offer(value));
            self.pump(64);
            self.recipients(|_, peer| assert_eq!(peer.state().pending, Some(value)));
        }
        fn complete(&mut self, value: CommonCut) {
            self.offer(value);
            self.recipients(|_, peer| assert!(peer.associate(value)));
            self.pump(64);
            assert!(!self.host().state().commit);
            assert!(self.host().confirm_native(value));
            self.pump(64);
            self.recipients(|_, peer| assert!(peer.state().commit && peer.apply(value)));
            self.pump(64);
            assert!(self.host().state().release);
            self.recipients(|_, peer| assert!(peer.state().release && peer.release(value)));
            self.pump(64);
            assert_eq!(self.host().state().committed, Some(value));
            assert_eq!(self.host().state().released, value.recipients);
            self.recipients(|_, peer| assert_eq!(peer.state().committed, Some(value)));
        }
    }

    #[test]
    fn literal_offer_and_constructor_fixed_physical_bounds() {
        let context = context(0x111, 0);
        for (peer, bad) in [
            (16, context),
            (255, context),
            (1, context),
            (
                4,
                Context {
                    host: 16,
                    ..context
                },
            ),
            (4, Context { host: 1, ..context }),
            (
                4,
                Context {
                    session: [0; 32],
                    ..context
                },
            ),
            (
                0,
                Context {
                    peers: 1,
                    ..context
                },
            ),
        ] {
            assert!(Agreement::new(peer, bad, 97).is_err());
        }
        let mut host = Agreement::new(0, context, 97).unwrap();
        let value = cut(context, 1, 123);
        for seal in [
            Cut {
                id: 0,
                ..value.seal
            },
            Cut {
                bytes: 0,
                ..value.seal
            },
            Cut {
                bytes: 64 * 1024 * 1024 + 1,
                ..value.seal
            },
            Cut {
                digest: [0; 32],
                ..value.seal
            },
        ] {
            assert!(!host.offer(CommonCut { seal, ..value }));
        }
        for recipients in [0x10, 0x111, 0x112] {
            assert!(!host.offer(CommonCut {
                recipients,
                ..value
            }));
        }
        assert!(host.offer(value));
        let packet = host.poll(0, 128).into_iter().find(|p| p.peer == 4).unwrap();
        let mut literal = vec![0x48, 0x32, 0x4e, 0x32, 97, 0, 11, 1, 0];
        literal.push(0x15);
        literal.extend_from_slice(&[0; 31]);
        literal.extend_from_slice(&[
            2, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0x11, 1, 0, 0, 4, 0x10, 1,
        ]);
        literal.extend_from_slice(&[
            1, 0, 0, 0, 0, 0, 0, 0, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0x10, 0, 0, 0, 0, 0, 0,
        ]);
        literal.push(1);
        literal.extend_from_slice(&[0; 31]);
        assert_eq!(literal.len(), 116);
        assert_eq!(packet.bytes, literal);
        let maximal = Context {
            epoch: u32::MAX,
            membership_revision: u64::MAX,
            ..context
        };
        let mut maximal_host = Agreement::new(0, maximal, 97).unwrap();
        let seal = Cut {
            id: u64::MAX,
            tick: u64::MAX,
            bytes: 64 * 1024 * 1024,
            digest: [1; 32],
        };
        assert!(maximal_host.offer(CommonCut {
            context: maximal,
            seal,
            recipients: value.recipients
        }));
        assert_eq!(maximal_host.context(), maximal);
    }

    #[test]
    fn explicit_barriers_latch_controls_without_inventing_missing_matrices() {
        let mut mesh = Mesh::new(0x111, 0);
        let old = cut(mesh.context, 1, 123);
        let pending = cut(mesh.context, 2, 123);
        mesh.complete(old);
        mesh.offer(pending);
        let next = cut(mesh.context, 3, 123);
        assert!(!mesh.peers[4].as_mut().unwrap().offer(pending));
        assert!(
            !mesh.host().associate(pending)
                && !mesh.host().apply(pending)
                && !mesh.host().release(pending)
        );
        assert!(!mesh.peers[4].as_mut().unwrap().apply(pending));
        assert!(!mesh.peers[4].as_mut().unwrap().release(pending));
        mesh.recipients(|_, peer| {
            assert!(peer.associate(pending));
            assert!(!peer.reject(pending));
        });
        assert!(mesh.host().confirm_native(pending));
        mesh.filtered(64, |p| {
            !(p.from == 4 && p.to == 8 && p.bytes[8] == Type::Associated as u8)
        });
        assert!(mesh.host().state().commit);
        let last = mesh.peers[8].as_mut().unwrap();
        assert!(last.state().commit);
        assert_eq!(last.state().associated, 0x100);
        assert!(!last.apply(pending));
        assert!(!mesh.host().offer(next) && !mesh.host().abort(pending));
        mesh.pump(64);
        mesh.recipients(|_, peer| assert!(peer.apply(pending)));
        mesh.filtered(64, |p| {
            !(p.from == 4 && p.to == 8 && p.bytes[8] == Type::Applied as u8)
        });
        let last = mesh.peers[8].as_mut().unwrap();
        assert!(last.state().release);
        assert_eq!(last.state().applied, 0x100);
        assert!(!last.release(pending));
        mesh.pump(64);
        mesh.recipients(|_, peer| {
            assert_eq!(peer.state().committed, Some(old));
            assert!(peer.release(pending));
        });
        mesh.filtered(64, |p| !(p.from == 8 && p.bytes[8] == Type::Released as u8));
        assert_eq!(mesh.host().state().released, 0x10);
        assert_eq!(mesh.host().state().committed, Some(old));
        assert!(!mesh.host().offer(next));
        mesh.pump(64);
        assert_eq!(mesh.host().state().committed, Some(pending));
        let regressed = cut(mesh.context, 3, 122);
        assert!(!mesh.host().offer(regressed));
        assert!(!mesh.host().offer(old));
        mesh.complete(next);
    }

    #[test]
    fn abort_requires_explicit_each_recipient_retirement_and_preserves_old_cut() {
        let mut mesh = Mesh::new(0x111, 0);
        let old = cut(mesh.context, 1, 123);
        let pending = cut(mesh.context, 2, 123);
        let repair = cut(mesh.context, 3, 123);
        mesh.complete(old);
        mesh.offer(pending);
        assert!(mesh.peers[4].as_mut().unwrap().associate(pending));
        assert!(mesh.host().confirm_native(pending));
        mesh.pump(64);
        assert!(!mesh.host().state().commit);
        assert!(mesh.peers[8].as_mut().unwrap().reject(pending));
        assert!(!mesh.peers[8].as_mut().unwrap().associate(pending));
        mesh.pump(64);
        assert!(mesh.host().state().abort);
        mesh.recipients(|_, peer| {
            assert!(peer.state().abort);
            assert_eq!(peer.state().committed, Some(old));
            assert!(!peer.apply(pending) && !peer.release(pending));
        });
        assert!(!mesh.host().confirm_native(pending) && !mesh.host().offer(repair));
        assert!(mesh.peers[4].as_mut().unwrap().acknowledge_abort(pending));
        mesh.pump(64);
        assert_eq!(mesh.host().state().aborted, 0x10);
        assert!(!mesh.host().offer(repair));
        assert!(mesh.peers[8].as_mut().unwrap().acknowledge_abort(pending));
        mesh.pump(64);
        assert_eq!(mesh.host().state().aborted, pending.recipients);
        assert_eq!(mesh.host().state().committed, Some(old));
        mesh.complete(repair);
    }

    #[test]
    fn all_nine_control_types_reject_full_malformed_corpus_and_foreign_ack_claims() {
        let mut mesh = Mesh::new(0x111, 0);
        let old = cut(mesh.context, 1, 123);
        let pending = cut(mesh.context, 2, 123);
        mesh.complete(old);
        mesh.offer(pending);
        assert!(mesh.peers[4].as_mut().unwrap().associate(pending));
        mesh.pump(64);
        let old_association = mesh.samples[&(Type::Associated as u8, 4, 0)].bytes.clone();
        let before = safety(mesh.host().state());
        for _ in 0..32 {
            let now = mesh.now;
            mesh.host().receive(4, &old_association, now);
            mesh.host().receive(8, &old_association, now);
        }
        assert_eq!(before, safety(mesh.host().state()));
        let exact = mesh.peers[4]
            .as_mut()
            .unwrap()
            .poll(mesh.now + 101, 128)
            .into_iter()
            .find(|p| p.peer == 0 && p.bytes[8] == Type::Associated as u8)
            .unwrap()
            .bytes;
        for _ in 0..32 {
            let now = mesh.now;
            mesh.host().receive(4, &exact, now);
            mesh.host().receive(8, &exact, now);
        }
        assert_eq!(mesh.host().state().associated, 0x10);
        assert!(!mesh.host().state().commit);
        for offset in [
            4, 5, 7, 8, 9, 41, 45, 53, 55, 56, 57, 58, 60, 68, 76, 84, 115,
        ] {
            let mut bad = exact.clone();
            bad[offset] ^= 0x80;
            let before = safety(mesh.host().state());
            let now = mesh.now;
            mesh.host().receive(4, &bad, now);
            assert_eq!(before, safety(mesh.host().state()));
        }
        assert!(mesh.peers[8].as_mut().unwrap().reject(pending));
        mesh.pump(64);
        mesh.recipients(|_, peer| assert!(peer.acknowledge_abort(pending)));
        mesh.pump(64);
        for kind in 0..TYPES as u8 {
            let sample = mesh
                .samples
                .iter()
                .find(|((k, _, _), _)| *k == kind)
                .unwrap()
                .1;
            let mut recipient = Agreement::new(sample.to, mesh.context, 97).unwrap();
            for n in 0..sample.bytes.len() {
                assert_eq!(
                    recipient.receive(sample.from, &sample.bytes[..n], mesh.now),
                    n >= 7
                );
            }
            let mut extra = sample.bytes.clone();
            extra.push(0);
            recipient.receive(sample.from, &extra, mesh.now);
            for offset in [7, 8] {
                let mut bad = sample.bytes.clone();
                bad[offset] = 255;
                recipient.receive(sample.from, &bad, mesh.now);
            }
            let mut oversized = sample.bytes.clone();
            oversized.resize(10000, 0);
            recipient.receive(sample.from, &oversized, mesh.now);
            assert!(
                recipient.state().pending.is_none()
                    && recipient.state().committed.is_none()
                    && !recipient.state().commit
                    && !recipient.state().release
            );
            assert_eq!(recipient.state().rejected, 113);
            for peer in [16, 255] {
                recipient.receive(peer, &sample.bytes, mesh.now);
            }
            assert!(recipient.state().pending.is_none());
        }
    }

    #[test]
    fn host_loss_at_every_phase_never_fabricates_commit_release_abort_or_membership() {
        for phase in 0..9 {
            let mut mesh = Mesh::new(0x111, 0);
            let old = cut(mesh.context, 1, 123);
            let pending = cut(mesh.context, 2, 123);
            mesh.complete(old);
            if phase > 0 {
                mesh.offer(pending);
                if phase >= 2 {
                    assert!(mesh.peers[4].as_mut().unwrap().associate(pending));
                    mesh.pump(64);
                }
                if phase >= 3 {
                    assert!(mesh.peers[8].as_mut().unwrap().associate(pending));
                    mesh.pump(64);
                }
                if phase >= 4 && phase != 8 {
                    assert!(mesh.host().confirm_native(pending));
                    mesh.pump(64);
                }
                if phase >= 5 && phase != 8 {
                    assert!(mesh.peers[4].as_mut().unwrap().apply(pending));
                    mesh.pump(64);
                }
                if phase >= 6 && phase != 8 {
                    assert!(mesh.peers[8].as_mut().unwrap().apply(pending));
                    mesh.pump(64);
                }
                if phase == 7 {
                    assert!(mesh.peers[4].as_mut().unwrap().release(pending));
                    mesh.pump(64);
                }
                if phase == 8 {
                    assert!(mesh.host().abort(pending));
                    mesh.pump(64);
                    assert!(mesh.peers[4].as_mut().unwrap().acknowledge_abort(pending));
                    mesh.pump(64);
                }
            }
            let saved = mesh
                .peers
                .each_ref()
                .map(|p| p.as_ref().map(|p| safety(p.state())));
            mesh.peers[0] = None;
            mesh.now += 600000;
            mesh.pump(1000);
            mesh.recipients(|peer, owner| {
                assert_eq!(safety(owner.state()), saved[peer as usize].unwrap());
                assert_eq!(
                    owner.state().committed,
                    Some(if phase == 7 && peer == 4 {
                        pending
                    } else {
                        old
                    })
                );
            });
        }
    }

    #[test]
    fn real_lossy_sparse_full_room_and_delayed_owner_schedules_complete() {
        for host in [0, 4, 8, 12] {
            let mut mesh = Mesh::new(0x1111, host);
            let c = mesh.context;
            mesh.complete(cut(c, 1, 123));
            mesh.complete(cut(c, 2, 123));
            assert!(mesh.losses > 0 && mesh.reorders > 0 && mesh.duplicates > 0);
        }
        let mut full = Mesh::new(u16::MAX, 15);
        let c = full.context;
        full.complete(cut(c, 1, 123));
        full.complete(cut(c, 2, 123));
        assert_eq!(full.host().state().recipients, 0x7fff);
        assert_eq!(full.host().state().released.count_ones(), 15);
        for seed in 0..32 {
            let mut random = seed as u64 + 1;
            let mut next = || {
                random = random.wrapping_mul(6364136223846793005).wrapping_add(1);
                random >> 32
            };
            let mut mesh = Mesh::new(0x111, 0);
            for round in 1..=2 {
                let value = cut(mesh.context, round, 123);
                assert!(mesh.host().offer(value));
                let mut released = 0u16;
                let mut finished = false;
                for _ in 0..600 {
                    if next() % 5 == 0 {
                        mesh.host().confirm_native(value);
                    }
                    mesh.recipients(|peer, owner| {
                        if owner.state().pending != Some(value) {
                            return;
                        }
                        if owner.state().associated & (1u16 << peer) == 0 && next() % 4 == 0 {
                            assert!(owner.associate(value));
                        }
                        let state = owner.state();
                        if state.commit
                            && state.associated == value.recipients
                            && state.applied & (1u16 << peer) == 0
                            && next() % 4 == 0
                        {
                            assert!(owner.apply(value));
                        }
                        let state = owner.state();
                        if state.release
                            && state.applied == value.recipients
                            && state.released & (1u16 << peer) == 0
                            && next() % 4 == 0
                        {
                            assert!(owner.release(value));
                            released |= 1u16 << peer;
                        }
                    });
                    mesh.step(true, 4, |_| true);
                    mesh.recipients(|peer, owner| {
                        if owner.state().committed == Some(value) {
                            assert_ne!(released & (1u16 << peer), 0);
                        }
                    });
                    finished = mesh.host().state().committed == Some(value);
                    if finished {
                        break;
                    }
                    let successor = cut(mesh.context, round + 1, 123);
                    assert!(!mesh.host().offer(successor));
                }
                assert!(finished);
                assert_eq!(mesh.host().state().released, value.recipients);
            }
        }
    }

    #[test]
    fn monotonic_retry_clock_dispatch_saturation_and_legacy_u32_cursor_wrap() {
        let context = context(u16::MAX, 0);
        let value = cut(context, 1, 0);
        let mut host = Agreement::new(0, context, 97).unwrap();
        assert!(host.offer(value));
        assert!(!host.receive(255, b"foreign", u64::MAX));
        assert_eq!(host.clock, 0);
        assert!(host.receive(255, b"H2N2\xff\xff\x0b", 500));
        assert_eq!(host.clock, 500);
        assert_eq!(host.state().rejected, 1);
        assert_eq!(host.poll(499, 128).len(), 15);
        assert_eq!(host.clock, 500);
        assert!(host.poll(599, 128).is_empty());
        assert_eq!(host.poll(600, 128).len(), 15);
        assert!(host.poll(700, 0).is_empty());
        assert_eq!(host.cursor, 432);
        host.cursor = u32::MAX;
        let emitted = host.poll(700, 2);
        assert_eq!(
            emitted.iter().map(|p| p.peer).collect::<Vec<_>>(),
            vec![1, 2]
        );
        assert_eq!(host.cursor, 19);
        host.value.rejected = u64::MAX;
        host.receive(255, b"H2N2\xff\xff\x0b", u64::MAX);
        assert_eq!(host.state().rejected, u64::MAX);
        assert_eq!(host.clock, u64::MAX);
        assert!(host.poll(0, u32::MAX).len() <= 128);
        assert_eq!(host.emitted_at, u64::MAX);
        assert!(host.poll(u64::MAX, u32::MAX).is_empty());
    }
}
