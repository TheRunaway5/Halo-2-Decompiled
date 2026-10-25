//! Fixed physical-electorate host/cut agreement and its bounded wire codec.
//!
//! Every decision requires the fixed survivor electorate. Each honest voter
//! locks its first value for this context, including across higher ballots.
//! Pairwise equivocation can deny availability but cannot create two decisions.
//! The session authenticates each sender and retains the actual
//! native preparation before publishing Ready. Agreement never creates that
//! readiness or owns a native world. Its caller synchronizes access.

pub use crate::migration_preparation::PreparationCut as Cut;

const PEERS: usize = 16;
const TYPES: usize = 8;
const PENDING: usize = PEERS * TYPES;
const KIND: u8 = 10;
const MAX_PACKET: usize = 134;
const RETRY_MS: u64 = 100;
const SILENCE_MS: u64 = 500;
const PROPOSAL_RETRY_MS: u64 = 1000;
const PROPOSER_SPACING_MS: u64 = 50;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Context {
    pub session: [u8; 32],
    pub epoch: u32,
    pub membership_revision: u64,
    pub peers: u16,
    pub host: u8,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct Ballot {
    pub round: u64,
    pub proposer: u8,
}
impl Default for Ballot {
    fn default() -> Self {
        Self {
            round: 0,
            proposer: 16,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Value {
    pub host: u8,
    pub cut: Cut,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Decision {
    pub previous: Context,
    pub next: Context,
    pub ballot: Ballot,
    pub value: Value,
    pub voters: u16,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct State {
    pub suspected: bool,
    pub fenced: bool,
    pub electorate: u16,
    pub silent: u16,
    pub majority: u32,
    pub ready: [Option<Cut>; PEERS],
    pub decision: Option<Decision>,
    pub rejected: u64,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Packet {
    pub peer: u8,
    pub bytes: Vec<u8>,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Invalid peer host agreement packet or context")
    }
}
impl std::error::Error for Error {}

fn need(ok: bool) -> Result<(), Error> {
    if ok { Ok(()) } else { Err(Error) }
}
fn member(context: Context, peer: u8) -> bool {
    peer < PEERS as u8 && context.peers & (1u16 << peer) != 0
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
fn write_ballot(bytes: &mut Vec<u8>, ballot: Ballot) {
    put(bytes, ballot.round, 8);
    bytes.push(ballot.proposer);
}
fn write_value(bytes: &mut Vec<u8>, value: Value) {
    bytes.push(value.host);
    put(bytes, value.cut.id, 8);
    put(bytes, value.cut.tick, 8);
    put(bytes, value.cut.bytes, 8);
    bytes.extend_from_slice(&value.cut.digest);
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
enum Type {
    Probe,
    Silence,
    Ready,
    Prepare,
    Promise,
    Accept,
    Accepted,
    Nack,
}
impl TryFrom<u8> for Type {
    type Error = Error;
    fn try_from(value: u8) -> Result<Self, Error> {
        match value {
            0 => Ok(Self::Probe),
            1 => Ok(Self::Silence),
            2 => Ok(Self::Ready),
            3 => Ok(Self::Prepare),
            4 => Ok(Self::Promise),
            5 => Ok(Self::Accept),
            6 => Ok(Self::Accepted),
            7 => Ok(Self::Nack),
            _ => Err(Error),
        }
    }
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct Vote {
    ballot: Ballot,
    value: Value,
}
struct Message {
    kind: Type,
    sender: u8,
    target: u8,
    context: Context,
    ballot: Ballot,
    prior: Option<Vote>,
    value: Option<Value>,
    silent: bool,
}
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl<'a> Reader<'a> {
    fn raw(&mut self, width: usize) -> Result<&'a [u8], Error> {
        let end = self.at.checked_add(width).ok_or(Error)?;
        let result = self.bytes.get(self.at..end).ok_or(Error)?;
        self.at = end;
        Ok(result)
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
    fn flag(&mut self) -> Result<bool, Error> {
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
    fn ballot(&mut self) -> Result<Ballot, Error> {
        Ok(Ballot {
            round: self.word(8)?,
            proposer: self.byte()?,
        })
    }
    fn value(&mut self) -> Result<Value, Error> {
        Ok(Value {
            host: self.byte()?,
            cut: Cut {
                id: self.word(8)?,
                tick: self.word(8)?,
                bytes: self.word(8)?,
                digest: self.raw(32)?.try_into().map_err(|_| Error)?,
            },
        })
    }
}
fn decode(bytes: &[u8], protocol: u16) -> Result<Message, Error> {
    need((58..=MAX_PACKET).contains(&bytes.len()))?;
    let mut r = Reader { bytes, at: 0 };
    need(
        r.word(4)? == 0x324e3248
            && r.word(2)? == u64::from(protocol)
            && r.byte()? == KIND
            && r.byte()? == 2,
    )?;
    let mut message = Message {
        kind: Type::try_from(r.byte()?)?,
        context: r.context()?,
        sender: r.byte()?,
        target: r.byte()?,
        ballot: Ballot::default(),
        prior: None,
        value: None,
        silent: false,
    };
    if message.kind == Type::Silence {
        message.silent = r.flag()?;
    }
    if message.kind == Type::Ready {
        message.value = Some(r.value()?);
    }
    if message.kind as u8 >= Type::Prepare as u8 {
        message.ballot = r.ballot()?;
    }
    if message.kind == Type::Promise && r.flag()? {
        message.prior = Some(Vote {
            ballot: r.ballot()?,
            value: r.value()?,
        });
    }
    if matches!(message.kind, Type::Accept | Type::Accepted) {
        message.value = Some(r.value()?);
    }
    need(r.at == bytes.len())?;
    Ok(message)
}

#[derive(Clone, Copy)]
struct Response {
    prior: Option<Vote>,
}
#[derive(Clone, Copy, PartialEq, Eq)]
enum Stage {
    Idle,
    Preparing,
    Accepting,
}

pub struct Agreement {
    self_peer: u8,
    context: Context,
    protocol: u16,
    electorate: u16,
    majority: u32,
    host_at: u64,
    clock: u64,
    retry_at: u64,
    rejected: u64,
    emitted: bool,
    promised: Ballot,
    accepted: Option<Vote>,
    ready: [Option<Cut>; PEERS],
    learned: [Option<Vote>; PEERS],
    responses: [Option<Response>; PEERS],
    silent_at: [u64; PEERS],
    silent_seen: u16,
    pending: [[Option<Vec<u8>>; TYPES]; PEERS],
    cursor: usize,
    highest_round: u64,
    stage: Stage,
    proposal: Ballot,
    preferred: Option<Value>,
    selected: Option<Value>,
    decision: Option<Decision>,
    automatic_after: Option<u64>,
}

impl Agreement {
    pub fn new(
        self_peer: u8,
        context: Context,
        protocol: u16,
        last_host_ms: u64,
    ) -> Result<Self, Error> {
        need(
            member(context, self_peer)
                && member(context, context.host)
                && context.session.iter().any(|&b| b != 0)
                && context.peers.count_ones() >= 2
                && context.epoch < u32::MAX
                && context.membership_revision < u64::MAX,
        )?;
        let electorate = context.peers & !(1u16 << context.host);
        Ok(Self {
            self_peer,
            context,
            protocol,
            electorate,
            majority: electorate.count_ones() / 2 + 1,
            host_at: last_host_ms,
            clock: last_host_ms,
            retry_at: 0,
            rejected: 0,
            emitted: false,
            promised: Ballot::default(),
            accepted: None,
            ready: [None; PEERS],
            learned: [None; PEERS],
            responses: [None; PEERS],
            silent_at: [0; PEERS],
            silent_seen: 0,
            pending: std::array::from_fn(|_| std::array::from_fn(|_| None)),
            cursor: 0,
            highest_round: 0,
            stage: Stage::Idle,
            proposal: Ballot::default(),
            preferred: None,
            selected: None,
            decision: None,
            automatic_after: None,
        })
    }
    fn voter(&self, peer: u8) -> bool {
        peer < PEERS as u8 && self.electorate & (1u16 << peer) != 0
    }
    fn tick(&mut self, now: u64) {
        self.clock = self.clock.max(now);
    }
    fn proposer_order(&self) -> u32 {
        (self.electorate & ((1u16 << self.self_peer) - 1)).count_ones()
    }
    fn progress_seen(&mut self) {
        self.automatic_after = Some(self.clock.saturating_add(
            PROPOSAL_RETRY_MS + u64::from(self.proposer_order()) * PROPOSER_SPACING_MS,
        ));
    }
    fn silent(&self, now: u64) -> bool {
        self.self_peer != self.context.host
            && (self.accepted.is_some()
                || self.decision.is_some()
                || self.clock.max(now) - self.host_at > SILENCE_MS)
    }
    fn valid_ballot(&self, ballot: Ballot) -> Result<(), Error> {
        need(ballot.round != 0 && self.voter(ballot.proposer))
    }
    fn valid_value(&self, value: Value) -> Result<(), Error> {
        need(
            self.voter(value.host)
                && value.cut.id != 0
                && value.cut.bytes != 0
                && value.cut.bytes <= 64 * 1024 * 1024
                && value.cut.digest.iter().any(|&b| b != 0),
        )
    }
    fn known(&self, value: Value) -> bool {
        self.voter(value.host) && self.ready[value.host as usize] == Some(value.cut)
    }
    fn send(&mut self, to: u8, kind: Type, append: impl Fn(&mut Vec<u8>)) {
        if to == self.self_peer || !member(self.context, to) {
            return;
        }
        let mut bytes = Vec::with_capacity(MAX_PACKET);
        bytes.extend_from_slice(b"H2N2");
        put(&mut bytes, u64::from(self.protocol), 2);
        bytes.extend_from_slice(&[KIND, 2, kind as u8]);
        write_context(&mut bytes, self.context);
        bytes.extend_from_slice(&[self.self_peer, to]);
        append(&mut bytes);
        // Every private append is one of the eight fixed codec layouts; the
        // longest is the 134-byte Promise with its prior accepted vote.
        self.pending[to as usize][kind as usize] = Some(bytes);
    }
    fn broadcast(&mut self, kind: Type, append: impl Fn(&mut Vec<u8>)) {
        for to in 0..PEERS as u8 {
            self.send(to, kind, &append);
        }
    }
    fn learn(&mut self, peer: u8, vote: Vote) -> Result<(), Error> {
        if let Some(previous) = self.learned[peer as usize] {
            if vote.ballot < previous.ballot {
                return Ok(());
            }
            need(vote.ballot != previous.ballot || vote.value == previous.value)?;
        }
        need(self.decision.is_none_or(|d| vote.value == d.value))?;
        if self.learned[peer as usize] != Some(vote) {
            self.progress_seen();
        }
        self.learned[peer as usize] = Some(vote);
        let mut votes = 0u16;
        for p in 0..PEERS as u8 {
            if self.voter(p) && self.learned[p as usize] == Some(vote) {
                votes |= 1u16 << p;
            }
        }
        if votes == self.electorate && self.decision.is_none() {
            let mut next = self.context;
            next.epoch += 1;
            next.membership_revision += 1;
            next.peers = self.electorate;
            next.host = vote.value.host;
            self.decision = Some(Decision {
                previous: self.context,
                next,
                ballot: vote.ballot,
                value: vote.value,
                voters: votes,
            });
        }
        Ok(())
    }
    fn accept(&mut self, ballot: Ballot, value: Value) -> Result<(), Error> {
        need(
            self.voter(self.self_peer)
                && self.silent(self.clock)
                && ballot >= self.promised
                && self.known(value)
                && self.decision.is_none_or(|d| d.value == value),
        )?;
        if let Some(previous) = self.accepted {
            need(previous.value == value)?;
        }
        if self.accepted.is_none_or(|v| v.ballot != ballot) {
            self.progress_seen();
        }
        self.promised = ballot;
        self.highest_round = self.highest_round.max(ballot.round);
        let vote = Vote { ballot, value };
        self.accepted = Some(vote);
        self.learn(self.self_peer, vote)?;
        self.broadcast(Type::Accepted, |b| {
            write_ballot(b, ballot);
            write_value(b, value);
        });
        Ok(())
    }
    fn progress(&mut self) -> Result<(), Error> {
        if self.stage != Stage::Preparing {
            return Ok(());
        }
        let mut count = 0;
        let mut highest: Option<Vote> = None;
        for response in self.responses.iter().flatten() {
            count += 1;
            if let Some(prior) = response.prior {
                if let Some(previous) = highest
                    && previous.ballot == prior.ballot
                {
                    need(previous.value == prior.value)?;
                }
                if highest.is_none_or(|v| prior.ballot > v.ballot) {
                    highest = Some(prior);
                }
            }
        }
        if count < self.majority {
            return Ok(());
        }
        let value = if let Some(highest) = highest {
            highest.value
        } else {
            self.preferred.ok_or(Error)?
        };
        if !self.known(value) {
            return Ok(());
        }
        self.selected = Some(value);
        self.stage = Stage::Accepting;
        self.progress_seen();
        let proposal = self.proposal;
        if proposal >= self.promised && self.silent(self.clock) {
            self.accept(proposal, value)?;
        }
        self.broadcast(Type::Accept, |b| {
            write_ballot(b, proposal);
            write_value(b, value);
        });
        Ok(())
    }
    fn promise(&mut self, to: u8, ballot: Ballot) -> Result<(), Error> {
        if ballot < self.promised {
            let promised = self.promised;
            self.send(to, Type::Nack, |b| write_ballot(b, promised));
            return Ok(());
        }
        if ballot > self.promised {
            self.progress_seen();
        }
        self.promised = ballot;
        self.highest_round = self.highest_round.max(ballot.round);
        if to == self.self_peer {
            self.responses[self.self_peer as usize] = Some(Response {
                prior: self.accepted,
            });
            self.progress()?;
        } else {
            let accepted = self.accepted;
            self.send(to, Type::Promise, |b| {
                write_ballot(b, ballot);
                b.push(u8::from(accepted.is_some()));
                if let Some(vote) = accepted {
                    write_ballot(b, vote.ballot);
                    write_value(b, vote.value);
                }
            });
        }
        Ok(())
    }
    fn start_proposal(&mut self, candidate: u8) -> Result<bool, Error> {
        if !self.voter(self.self_peer)
            || !self.silent(self.clock)
            || self.decision.is_some()
            || !self.voter(candidate)
            || self.ready[candidate as usize].is_none()
            || self.highest_round == u64::MAX
        {
            return Ok(false);
        }
        self.highest_round += 1;
        self.proposal = Ballot {
            round: self.highest_round,
            proposer: self.self_peer,
        };
        self.preferred = Some(Value {
            host: candidate,
            cut: self.ready[candidate as usize].ok_or(Error)?,
        });
        self.selected = None;
        self.responses.fill(None);
        self.stage = Stage::Preparing;
        self.progress_seen();
        let proposal = self.proposal;
        self.promise(self.self_peer, proposal)?;
        self.broadcast(Type::Prepare, |b| write_ballot(b, proposal));
        Ok(true)
    }
    pub fn observe_host(&mut self, now: u64) {
        self.tick(now);
        self.host_at = self.clock;
        if self.accepted.is_none() && self.decision.is_none() {
            self.automatic_after = None;
        }
    }
    pub fn publish_ready(&mut self, peer: u8, context: Context, cut: Cut) -> Result<bool, Error> {
        if !self.voter(self.self_peer) || peer != self.self_peer || context != self.context {
            return Ok(false);
        }
        let value = Value {
            host: self.self_peer,
            cut,
        };
        self.valid_value(value)?;
        if let Some(previous) = self.ready[self.self_peer as usize] {
            return Ok(previous == cut);
        }
        self.ready[self.self_peer as usize] = Some(cut);
        self.broadcast(Type::Ready, |b| write_value(b, value));
        self.progress()?;
        Ok(true)
    }
    pub fn propose(&mut self, candidate: u8, now: u64) -> Result<bool, Error> {
        self.tick(now);
        self.start_proposal(candidate)
    }
    pub fn drive(&mut self, candidate: u8, now: u64) -> Result<bool, Error> {
        self.tick(now);
        if !self.voter(self.self_peer)
            || !self.silent(self.clock)
            || self.decision.is_some()
            || !self.voter(candidate)
            || self.ready[candidate as usize].is_none()
        {
            return Ok(false);
        }
        let delay = u64::from(self.proposer_order()) * PROPOSER_SPACING_MS;
        let after = *self
            .automatic_after
            .get_or_insert_with(|| self.clock.saturating_add(delay));
        if self.clock < after {
            return Ok(false);
        }
        self.start_proposal(candidate)
    }
    pub fn poll(&mut self, now: u64, budget: u32) -> Vec<Packet> {
        self.tick(now);
        if !self.emitted || self.clock - self.retry_at >= RETRY_MS {
            self.emitted = true;
            self.retry_at = self.clock;
            if let Some(cut) = self.ready[self.self_peer as usize] {
                let value = Value {
                    host: self.self_peer,
                    cut,
                };
                self.broadcast(Type::Ready, |b| write_value(b, value));
            }
            if self.voter(self.self_peer) {
                let silent = self.silent(self.clock);
                self.broadcast(Type::Silence, |b| b.push(u8::from(silent)));
                if silent {
                    self.broadcast(Type::Probe, |_| {});
                }
            }
            if self.stage == Stage::Preparing {
                let proposal = self.proposal;
                self.broadcast(Type::Prepare, |b| write_ballot(b, proposal));
            }
            if self.stage == Stage::Accepting
                && let Some(value) = self.selected
            {
                let proposal = self.proposal;
                self.broadcast(Type::Accept, |b| {
                    write_ballot(b, proposal);
                    write_value(b, value);
                });
            }
            if let Some(vote) = self.accepted {
                self.broadcast(Type::Accepted, |b| {
                    write_ballot(b, vote.ballot);
                    write_value(b, vote.value);
                });
            }
        }
        let budget = budget.min(PENDING as u32) as usize;
        let mut result = Vec::new();
        for _ in 0..PENDING {
            if result.len() >= budget {
                break;
            }
            let index = self.cursor;
            self.cursor = (self.cursor + 1) % PENDING;
            if let Some(bytes) = self.pending[index / TYPES][index % TYPES].take() {
                result.push(Packet {
                    peer: (index / TYPES) as u8,
                    bytes,
                });
            }
        }
        result
    }
    fn apply(&mut self, peer: u8, bytes: &[u8]) -> Result<(), Error> {
        let message = decode(bytes, self.protocol)?;
        need(
            message.context == self.context
                && message.sender == peer
                && message.target == self.self_peer
                && peer != self.self_peer
                && self.voter(peer),
        )?;
        if message.kind as u8 >= Type::Prepare as u8 {
            self.valid_ballot(message.ballot)?;
        }
        if let Some(value) = message.value {
            self.valid_value(value)?;
        }
        if let Some(prior) = message.prior {
            self.valid_ballot(prior.ballot)?;
            self.valid_value(prior.value)?;
            need(prior.ballot <= message.ballot)?;
        }
        if matches!(message.kind, Type::Prepare | Type::Accept) {
            need(message.ballot.proposer == peer)?;
        }
        match message.kind {
            Type::Probe => {
                if self.voter(self.self_peer) {
                    let silent = self.silent(self.clock);
                    self.send(peer, Type::Silence, |b| b.push(u8::from(silent)));
                }
            }
            Type::Silence => {
                self.silent_seen |= 1u16 << peer;
                self.silent_at[peer as usize] = self.clock;
                if !message.silent {
                    self.silent_seen &= !(1u16 << peer);
                }
            }
            Type::Ready => {
                let value = message.value.ok_or(Error)?;
                need(
                    value.host == peer
                        && self.ready[peer as usize].is_none_or(|cut| cut == value.cut),
                )?;
                self.ready[peer as usize] = Some(value.cut);
                self.progress()?;
            }
            Type::Prepare => {
                if self.voter(self.self_peer) && self.silent(self.clock) {
                    self.promise(peer, message.ballot)?;
                }
            }
            Type::Promise => {
                need(self.voter(self.self_peer))?;
                if self.stage == Stage::Preparing && message.ballot == self.proposal {
                    if self.responses[peer as usize].is_none() {
                        self.responses[peer as usize] = Some(Response {
                            prior: message.prior,
                        });
                        self.progress_seen();
                    }
                    self.progress()?;
                }
            }
            Type::Accept => {
                if self.voter(self.self_peer) && self.silent(self.clock) {
                    if message.ballot < self.promised {
                        let promised = self.promised;
                        self.send(peer, Type::Nack, |b| write_ballot(b, promised));
                    } else {
                        let value = message.value.ok_or(Error)?;
                        if self.known(value) {
                            self.accept(message.ballot, value)?;
                        }
                    }
                }
            }
            Type::Accepted => self.learn(
                peer,
                Vote {
                    ballot: message.ballot,
                    value: message.value.ok_or(Error)?,
                },
            )?,
            Type::Nack => {
                if message.ballot.round > self.highest_round {
                    self.progress_seen();
                }
                self.highest_round = self.highest_round.max(message.ballot.round);
            }
        }
        Ok(())
    }
    pub fn receive(&mut self, peer: u8, bytes: &[u8], now: u64) -> bool {
        if !is_packet(bytes) {
            return false;
        }
        self.tick(now);
        if self.apply(peer, bytes).is_err() {
            self.rejected = self.rejected.saturating_add(1);
        }
        true
    }
    pub fn state(&self, now: u64) -> State {
        let clock = self.clock.max(now);
        let mut silent = 0u16;
        for peer in 0..PEERS as u8 {
            if self.voter(peer)
                && self.silent_seen & (1u16 << peer) != 0
                && clock - self.silent_at[peer as usize] <= RETRY_MS * 3
            {
                silent |= 1u16 << peer;
            }
        }
        if self.voter(self.self_peer) && self.silent(clock) {
            silent |= 1u16 << self.self_peer;
        }
        State {
            suspected: self.silent(clock),
            fenced: self.accepted.is_some() || self.decision.is_some(),
            electorate: self.electorate,
            silent,
            majority: self.majority,
            ready: self.ready,
            decision: self.decision,
            rejected: self.rejected,
        }
    }
}

pub fn is_packet(bytes: &[u8]) -> bool {
    bytes.len() >= 7 && bytes.get(..4) == Some(b"H2N2") && bytes[6] == KIND
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::VecDeque;

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
    fn cut(peer: u8) -> Cut {
        let mut digest = [0; 32];
        digest[0] = peer + 1;
        Cut {
            id: u64::from(peer) + 1,
            tick: 123,
            bytes: 4096,
            digest,
        }
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
        dropped: [[[bool; TYPES]; PEERS]; PEERS],
    }
    impl Mesh {
        fn new(members: u16, host: u8) -> Self {
            let context = context(members, host);
            Self {
                context,
                peers: std::array::from_fn(|peer| {
                    member(context, peer as u8)
                        .then(|| Agreement::new(peer as u8, context, 97, 0).unwrap())
                }),
                now: 501,
                serial: 0,
                dropped: [[[false; TYPES]; PEERS]; PEERS],
            }
        }
        fn ready(&mut self, peer: u8) {
            assert!(
                self.peers[peer as usize]
                    .as_mut()
                    .unwrap()
                    .publish_ready(peer, self.context, cut(peer))
                    .unwrap()
            );
        }
        fn collect(&mut self, budget: u32) -> Vec<Wire> {
            let mut packets = vec![];
            for (from, peer) in self.peers.iter_mut().enumerate() {
                if let Some(peer) = peer {
                    packets.extend(peer.poll(self.now, budget).into_iter().map(|p| {
                        assert!(p.bytes.len() <= MAX_PACKET);
                        Wire {
                            from: from as u8,
                            to: p.peer,
                            bytes: p.bytes,
                        }
                    }));
                }
            }
            packets
        }
        fn step(&mut self, lossy: bool, budget: u32, allowed: impl Fn(&Wire) -> bool) {
            self.now += 1;
            let mut wire = VecDeque::new();
            for packet in self.collect(budget) {
                if !allowed(&packet) {
                    continue;
                }
                self.serial += 1;
                let lost = &mut self.dropped[packet.from as usize][packet.to as usize]
                    [packet.bytes[8] as usize];
                let lose = lossy && self.serial.is_multiple_of(5) && !*lost;
                *lost = lose;
                if lose {
                    continue;
                }
                if lossy && self.serial.is_multiple_of(7) {
                    wire.push_front(packet.clone());
                } else {
                    wire.push_back(packet.clone());
                }
                if lossy && self.serial.is_multiple_of(11) {
                    wire.push_back(packet);
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
                self.step(true, 128, |_| true);
            }
        }
        fn same(&self, host: u8) {
            for peer in self.peers.iter().flatten() {
                let state = peer.state(self.now);
                let decision = state.decision.unwrap();
                assert_eq!(
                    decision.value,
                    Value {
                        host,
                        cut: cut(host)
                    }
                );
                assert_eq!(decision.previous, self.context);
                assert_eq!(
                    decision.next,
                    Context {
                        epoch: self.context.epoch + 1,
                        membership_revision: self.context.membership_revision + 1,
                        peers: self.context.peers & !(1u16 << self.context.host),
                        host,
                        ..self.context
                    }
                );
                assert_eq!(decision.voters, self.context.peers & !(1 << self.context.host));
                assert_eq!(decision.voters & (1u16 << self.context.host), 0);
            }
        }
    }

    #[test]
    fn literal_ready_wire_and_dispatch_admission_are_unchanged() {
        let c = context(0x1111, 0);
        let mut source = Agreement::new(8, c, 97, 0).unwrap();
        assert!(source.publish_ready(8, c, cut(8)).unwrap());
        let packet = source
            .poll(501, 128)
            .into_iter()
            .find(|p| p.peer == 4 && p.bytes[8] == 2)
            .unwrap();
        let mut literal = vec![0x48, 0x32, 0x4e, 0x32, 97, 0, 10, 2, 2];
        literal.push(0x15);
        literal.extend_from_slice(&[0; 31]);
        literal.extend_from_slice(&[2, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0x11, 0x11, 0, 8, 4]);
        literal.extend_from_slice(&[
            8, 9, 0, 0, 0, 0, 0, 0, 0, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0x10, 0, 0, 0, 0, 0, 0,
        ]);
        literal.push(9);
        literal.extend_from_slice(&[0; 31]);
        assert_eq!(literal.len(), 115);
        assert_eq!(packet.bytes, literal);
        let mut target = Agreement::new(4, c, 97, 0).unwrap();
        for n in 0..literal.len() {
            assert_eq!(target.receive(8, &literal[..n], 501), n >= 7);
        }
        assert_eq!(target.state(501).rejected, literal.len() as u64 - 7);
        for offset in [4, 9, 41, 45, 53, 55, 56, 57] {
            let mut bad = literal.clone();
            bad[offset] ^= 1;
            target.receive(8, &bad, 501);
        }
        for peer in [4, 12, 16, 255] {
            target.receive(peer, &literal, 501);
        }
        let mut extended = literal.clone();
        extended.push(0);
        target.receive(8, &extended, 501);
        assert!(target.state(501).ready.iter().all(Option::is_none));
        assert!(target.receive(8, &literal, 501));
        assert_eq!(target.state(501).ready[8], Some(cut(8)));
        assert!(!target.state(501).fenced);
        let mut future = literal;
        future[4] = 98;
        assert!(is_packet(&future));
        assert!(target.receive(8, &future, 501));
    }

    #[test]
    fn constructor_ready_once_and_monotonic_saturated_clock_bounds() {
        let c = context(0x1111, 0);
        for (self_peer, invalid) in [
            (16, c),
            (255, c),
            (1, c),
            (4, Context { host: 16, ..c }),
            (
                4,
                Context {
                    session: [0; 32],
                    ..c
                },
            ),
            (0, context(1, 0)),
            (
                4,
                Context {
                    epoch: u32::MAX,
                    ..c
                },
            ),
            (
                4,
                Context {
                    membership_revision: u64::MAX,
                    ..c
                },
            ),
        ] {
            assert!(Agreement::new(self_peer, invalid, 97, 0).is_err());
        }
        let mut peer = Agreement::new(4, c, 97, 1000).unwrap();
        assert!(!peer.state(1500).suspected && peer.state(1501).suspected);
        assert!(!peer.propose(4, 1501).unwrap());
        assert!(!peer.publish_ready(8, c, cut(4)).unwrap());
        assert!(
            !peer
                .publish_ready(4, Context { epoch: 3, ..c }, cut(4))
                .unwrap()
        );
        assert!(peer.publish_ready(4, c, Cut { id: 0, ..cut(4) }).is_err());
        assert!(peer.publish_ready(4, c, cut(4)).unwrap());
        assert!(peer.publish_ready(4, c, cut(4)).unwrap());
        assert!(
            !peer
                .publish_ready(
                    4,
                    c,
                    Cut {
                        tick: 124,
                        ..cut(4)
                    }
                )
                .unwrap()
        );
        peer.observe_host(1501);
        peer.observe_host(1000);
        assert!(!peer.state(2001).suspected && peer.state(2002).suspected);
        assert!(!peer.propose(16, 2002).unwrap() && !peer.drive(255, 2002).unwrap());
        assert_eq!(peer.state(999999).electorate, 0x1110);
        let mut solo = Agreement::new(4, context(0x11, 0), 97, 0).unwrap();
        solo.publish_ready(4, context(0x11, 0), cut(4)).unwrap();
        assert!(solo.propose(4, 501).unwrap());
        assert_eq!(solo.state(501).decision.unwrap().voters, 0x10);
        solo.observe_host(u64::MAX);
        assert!(solo.state(0).fenced && solo.state(0).decision.is_some());
        let mut saturated = Agreement::new(4, c, 97, u64::MAX - 600).unwrap();
        saturated.publish_ready(4, c, cut(4)).unwrap();
        assert!(saturated.drive(4, u64::MAX - 99).unwrap());
        assert_eq!(saturated.automatic_after, Some(u64::MAX));
        assert!(!saturated.drive(4, u64::MAX - 100).unwrap());
        saturated.rejected = u64::MAX;
        assert!(saturated.receive(255, b"H2N2\0\0\x0a", 0));
        assert_eq!(saturated.state(0).rejected, u64::MAX);
        assert!(saturated.poll(u64::MAX, 0).is_empty());
        assert!(saturated.poll(0, u32::MAX).len() <= 128);
    }

    #[test]
    fn loss_reordering_old_host_learner_and_fixed_partition_majority() {
        for host in [0, 4, 8, 12] {
            let chosen = (host + 4) % 16;
            let mut mesh = Mesh::new(0x1111, host);
            mesh.ready(chosen);
            mesh.pump(300);
            assert!(
                mesh.peers[chosen as usize]
                    .as_mut()
                    .unwrap()
                    .propose(chosen, mesh.now)
                    .unwrap()
            );
            mesh.pump(300);
            mesh.same(chosen);
            for peer in mesh.peers.iter_mut().flatten() {
                peer.observe_host(mesh.now);
            }
            mesh.same(chosen);
            assert!(
                !mesh.peers[host as usize]
                    .as_ref()
                    .unwrap()
                    .state(mesh.now)
                    .suspected
            );
        }
        let mut mesh = Mesh::new(0x1111, 0);
        mesh.ready(4);
        mesh.ready(12);
        mesh.pump(300);
        mesh.peers[4]
            .as_mut()
            .unwrap()
            .propose(4, mesh.now)
            .unwrap();
        mesh.peers[12]
            .as_mut()
            .unwrap()
            .propose(12, mesh.now)
            .unwrap();
        for _ in 0..300 {
            mesh.step(true, 128, |p| (p.from == 4) == (p.to == 4));
        }
        assert!(
            mesh.peers[4]
                .as_ref()
                .unwrap()
                .state(mesh.now)
                .decision
                .is_none()
        );
        assert_eq!(mesh.peers[4].as_ref().unwrap().state(u64::MAX).majority, 2);
        assert!(
            mesh.peers[8]
                .as_ref()
                .unwrap()
                .state(mesh.now)
                .decision
                .is_none()
        );
        mesh.pump(300);
        mesh.same(12);
        let mut even = Mesh::new(0x1f, 0);
        for peer in 1..5 {
            even.ready(peer);
        }
        even.pump(300);
        even.peers[1]
            .as_mut()
            .unwrap()
            .propose(1, even.now)
            .unwrap();
        even.peers[3]
            .as_mut()
            .unwrap()
            .propose(3, even.now)
            .unwrap();
        for _ in 0..1000 {
            even.step(true, 128, |p| (p.from <= 2) == (p.to <= 2));
        }
        for peer in even.peers.iter().flatten() {
            assert_eq!(peer.state(even.now).majority, 3);
            assert!(peer.state(even.now).decision.is_none());
        }
        even.peers[4]
            .as_mut()
            .unwrap()
            .propose(4, even.now)
            .unwrap();
        even.pump(1500);
        even.same(4);
    }

    #[test]
    fn prior_accepted_value_survives_failed_proposer_and_full_codec_corpus() {
        let mut mesh = Mesh::new(0x1111, 0);
        mesh.ready(8);
        mesh.ready(12);
        mesh.pump(300);
        mesh.peers[4]
            .as_mut()
            .unwrap()
            .propose(8, mesh.now)
            .unwrap();
        let mut corpus: [Option<Wire>; TYPES] = std::array::from_fn(|_| None);
        for _ in 0..300 {
            mesh.now += 1;
            for packet in mesh.collect(128) {
                let kind = packet.bytes[8] as usize;
                if corpus[kind]
                    .as_ref()
                    .is_none_or(|p| p.bytes.len() < packet.bytes.len())
                {
                    corpus[kind] = Some(packet.clone());
                }
                if kind != 6 {
                    mesh.peers[packet.to as usize].as_mut().unwrap().receive(
                        packet.from,
                        &packet.bytes,
                        mesh.now,
                    );
                }
            }
        }
        assert!(
            mesh.peers
                .iter()
                .flatten()
                .all(|p| p.state(mesh.now).decision.is_none())
        );
        mesh.peers[12]
            .as_mut()
            .unwrap()
            .propose(12, mesh.now)
            .unwrap();
        for _ in 0..1000 {
            mesh.now += 1;
            for packet in mesh.collect(128) {
                let kind = packet.bytes[8] as usize;
                if corpus[kind]
                    .as_ref()
                    .is_none_or(|p| p.bytes.len() < packet.bytes.len())
                {
                    corpus[kind] = Some(packet.clone());
                }
                if kind != 6 {
                    mesh.peers[packet.to as usize].as_mut().unwrap().receive(
                        packet.from,
                        &packet.bytes,
                        mesh.now,
                    );
                }
            }
        }
        assert_eq!(corpus[4].as_ref().unwrap().bytes.len(), 134);
        for sample in corpus {
            let sample = sample.unwrap();
            let mut recipient = Agreement::new(sample.to, mesh.context, 97, 0).unwrap();
            for n in 0..sample.bytes.len() {
                recipient.receive(sample.from, &sample.bytes[..n], mesh.now);
            }
            let mut extra = sample.bytes.clone();
            extra.push(0);
            recipient.receive(sample.from, &extra, mesh.now);
            for offset in [7, 8] {
                let mut bad = sample.bytes.clone();
                bad[offset] = 255;
                recipient.receive(sample.from, &bad, mesh.now);
            }
            let state = recipient.state(mesh.now);
            assert!(
                !state.fenced
                    && state.decision.is_none()
                    && state.ready.iter().all(Option::is_none)
            );
            assert_eq!(state.rejected, sample.bytes.len() as u64 - 4);
        }
        mesh.peers[4] = None;
        mesh.peers[12]
            .as_mut()
            .unwrap()
            .propose(12, mesh.now)
            .unwrap();
        for _ in 0..1000 {
            mesh.step(true, 128, |p| p.from != 4 && p.to != 4);
        }
        assert!(mesh.peers.iter().flatten().all(|p| p.state(mesh.now).decision.is_none()));
    }

    #[test]
    fn duplicate_authenticated_votes_count_once() {
        let mut mesh = Mesh::new(0x1111, 0);
        mesh.ready(8);
        mesh.pump(300);
        mesh.peers[4]
            .as_mut()
            .unwrap()
            .propose(8, mesh.now)
            .unwrap();
        let mut receipts: [Option<Vec<u8>>; PEERS] = std::array::from_fn(|_| None);
        for _ in 0..300 {
            mesh.now += 1;
            for packet in mesh.collect(128) {
                if packet.to == 0 && packet.bytes[8] == 6 {
                    receipts[packet.from as usize] = Some(packet.bytes);
                } else {
                    mesh.peers[packet.to as usize].as_mut().unwrap().receive(
                        packet.from,
                        &packet.bytes,
                        mesh.now,
                    );
                }
            }
        }
        let old_host = mesh.peers[0].as_mut().unwrap();
        assert!(old_host.state(mesh.now).decision.is_none());
        for _ in 0..100 {
            old_host.receive(4, receipts[4].as_ref().unwrap(), mesh.now);
            old_host.receive(8, receipts[4].as_ref().unwrap(), mesh.now);
        }
        assert!(old_host.state(mesh.now).decision.is_none());
        old_host.receive(8, receipts[8].as_ref().unwrap(), mesh.now);
        assert!(old_host.state(mesh.now).decision.is_none());
        old_host.receive(12, receipts[12].as_ref().unwrap(), mesh.now);
        assert!(old_host.state(mesh.now).decision.is_some());
    }

    #[test]
    fn equivocating_survivor_cannot_split_decisions_or_unlock_an_honest_vote() {
        let c = context(0xf, 0);
        let mut a = Agreement::new(1,c,101,0).unwrap();
        let mut b = Agreement::new(2,c,101,0).unwrap();
        let one = Value {host:1,cut:cut(1)};
        let two = Value {host:2,cut:cut(2)};
        let ballot = Ballot {round:1,proposer:3};
        for peer in [&mut a,&mut b] {
            peer.tick(1000);peer.ready[1]=Some(one.cut);peer.ready[2]=Some(two.cut);
        }
        a.accept(ballot,one).unwrap();b.accept(ballot,two).unwrap();
        // Authenticated survivor 3 equivocates to two honest learners. Each
        // previously had a crash-majority of two; neither has unanimity.
        a.learn(3,Vote{ballot,value:one}).unwrap();b.learn(3,Vote{ballot,value:two}).unwrap();
        assert!(a.state(1000).decision.is_none());assert!(b.state(1000).decision.is_none());
        assert!(a.accept(Ballot{round:2,proposer:2},two).is_err());
        assert!(b.accept(Ballot{round:2,proposer:1},one).is_err());
        a.learn(2,Vote{ballot,value:two}).unwrap();b.learn(1,Vote{ballot,value:one}).unwrap();
        assert!(a.state(1000).decision.is_none());assert!(b.state(1000).decision.is_none());
    }

    #[test]
    fn automatic_driver_preserves_threshold_pacing_and_healthy_majority_veto() {
        let c = context(0x1111, 0);
        let mut peer = Agreement::new(4, c, 97, 1000).unwrap();
        peer.publish_ready(4, c, cut(4)).unwrap();
        assert!(!peer.drive(4, 1500).unwrap() && peer.drive(4, 1501).unwrap());
        for now in 1502..2000 {
            assert!(!peer.drive(4, now).unwrap());
        }
        peer.observe_host(2000);
        assert!(!peer.drive(4, 2500).unwrap() && peer.drive(4, 2501).unwrap());
        let mut healthy = Mesh::new(0x1111, 0);
        healthy.ready(8);
        healthy.pump(300);
        for _ in 0..2500 {
            for peer in [8, 12] {
                healthy.peers[peer]
                    .as_mut()
                    .unwrap()
                    .observe_host(healthy.now);
            }
            for peer in [4, 8, 12] {
                healthy.peers[peer]
                    .as_mut()
                    .unwrap()
                    .drive(8, healthy.now)
                    .unwrap();
            }
            healthy.step(true, 4, |_| true);
        }
        assert!(
            healthy
                .peers
                .iter()
                .flatten()
                .all(|p| !p.state(healthy.now).fenced)
        );
        for _ in 0..2500 {
            for peer in [4, 8, 12] {
                healthy.peers[peer]
                    .as_mut()
                    .unwrap()
                    .drive(8, healthy.now)
                    .unwrap();
            }
            healthy.step(true, 4, |_| true);
        }
        healthy.same(8);
        let mut full = Mesh::new(u16::MAX, 0);
        full.ready(15);
        full.pump(1000);
        for _ in 0..4000 {
            for peer in 1..16 {
                full.peers[peer]
                    .as_mut()
                    .unwrap()
                    .drive(15, full.now)
                    .unwrap();
            }
            full.step(true, 4, |_| true);
        }
        full.same(15);
    }
}
