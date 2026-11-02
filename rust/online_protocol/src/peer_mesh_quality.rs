//! Complete direct-link measurement and reciprocal host scoring owner.
//!
//! This is the existing replacement H2QO protocol, not recovered Xbox XNet.
//! Receipts measure acknowledged offered application bytes, not ISP bandwidth.
//! No observation elects a host, shrinks membership or creates a native world.
pub use crate::host_agreement::Context;
use std::collections::VecDeque;

const PEERS: usize = 16;
const HEADER: usize = 62;
const PROBE_BYTES: usize = 1200;
const PROBE_PACKETS: u32 = 64;
const MAX_UPLOAD: u32 = 64 * 1024 * 1024;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Invalid direct mesh quality packet or context")
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn context() -> Context {
        Context {
            session: [7; 32],
            epoch: 3,
            membership_revision: 5,
            peers: 0x11,
            host: 0,
        }
    }
    fn mesh() -> Mesh {
        Mesh::new(0, context()).unwrap()
    }
    #[test]
    fn physical_membership_and_pregame_sentinel_are_exact() {
        assert!(
            Mesh::new(
                4,
                Context {
                    host: 16,
                    ..context()
                }
            )
            .is_ok()
        );
        for peer in [1, 16, 255] {
            assert!(Mesh::new(peer, context()).is_err());
        }
        for host in [1, 17, 255] {
            assert!(Mesh::new(0, Context { host, ..context() }).is_err());
        }
        assert!(
            Mesh::new(
                0,
                Context {
                    peers: 1,
                    ..context()
                }
            )
            .is_err()
        );
    }
    #[test]
    fn reply_budget_zero_does_not_acknowledge_or_discard_the_queue() {
        let mut m = mesh();
        let ping = header(context(), Kind::Ping, 1, 0);
        assert!(m.receive(4, &ping, 1000));
        assert!(m.poll(0, 1001, 0).is_empty());
        let out = m.poll(0, 1002, 1);
        assert_eq!(out.len(), 1);
        assert_eq!(out[0].peer, 4);
        assert_eq!(out[0].bytes, header(context(), Kind::Echo, 1, 0));
        assert!(m.candidates(1002).is_empty());
    }
    #[test]
    fn outstanding_echo_replay_and_ring_overwrite_do_not_refresh_measurements() {
        let mut m = mesh();
        let ping = m.poll(0x10, 0, 1).remove(0);
        let mut echo = ping.bytes;
        echo[6] = 2;
        m.receive(4, &echo, 4);
        assert_eq!(m.links[4].ping_samples, 1);
        assert_eq!(m.links[4].echo_at, 4);
        m.receive(4, &echo, 100);
        assert_eq!(m.links[4].ping_samples, 1);
        assert_eq!(m.links[4].echo_at, 4);
        for now in (250..=2250).step_by(250) {
            m.poll(0x10, now, 1);
        }
        m.receive(4, &echo, 2251);
        assert_eq!(m.links[4].ping_samples, 1);
        assert_eq!(m.links[4].echo_at, 4);
    }
    #[test]
    fn late_duplicate_receipt_cannot_inflate_the_offered_upload_span() {
        let mut m = mesh();
        let l = &mut m.links[4];
        l.probing = true;
        l.round = 1;
        l.next_packet = 64;
        l.ping_samples = 3;
        l.probe_started = 1000;
        for (index, elapsed, now) in [(0, 5, 1001), (1, 20, 1002), (1, 9000, 1003)] {
            let mut packet = header(context(), Kind::Receipt, 1, index);
            put(&mut packet, elapsed, 4);
            m.receive(4, &packet, now);
        }
        m.links[4].finish(2000);
        let q = m.links[4].quality;
        assert_eq!(q.upload_bytes_per_second, 60000);
        assert_eq!(q.loss_per_mille, 968);
        assert!(q.measured);
    }
    #[test]
    fn authenticated_reply_queue_is_bounded_and_unrelated_bytes_fall_through() {
        let mut m = mesh();
        for n in 1..=1024 {
            assert!(m.receive(4, &header(context(), Kind::Ping, n, 0), 1000));
        }
        assert_eq!(m.replies.len(), 256);
        assert!(m.poll(0, 1001, u32::MAX).len() <= 64);
        assert!(!m.receive(4, b"H2N2", 1001));
        assert!(m.receive(16, b"H2QO", 1001));
        assert!(m.candidates(1001).is_empty());
    }
    #[test]
    fn whole_report_admission_is_atomic_and_duplicate_rows_cannot_extend_freshness() {
        let make = |sequence| {
            let mut bytes = header(context(), Kind::Report, sequence, 0);
            for _ in 0..16 {
                put(&mut bytes, u32::MAX as u64, 4);
                put(&mut bytes, 0, 4);
                put(&mut bytes, 1000, 2);
                bytes.push(0);
            }
            bytes
        };
        let mut m = mesh();
        let first = make(1);
        m.receive(4, &first, 1000);
        m.receive(4, &first, 4000);
        assert_eq!(m.matrix(4000)[4].unwrap().received_ms, 1000);
        assert!(m.matrix(6001)[4].is_none());
        let next = make(2);
        let mut bad = next.clone();
        bad.push(0);
        m.receive(4, &bad, 6002);
        assert!(m.matrix(6002)[4].is_none());
        m.receive(4, &next, 6003);
        assert_eq!(m.matrix(6003)[4].unwrap().received_ms, 6003);
    }
    #[test]
    fn ranking_uses_reciprocal_loss_then_directional_upload_then_mean_rtt() {
        let mut m = Mesh::new(
            0,
            Context {
                peers: 0x111,
                ..context()
            },
        )
        .unwrap();
        for p in [0, 4, 8] {
            let mut row = Row {
                links: [Quality::default(); 16],
                received_ms: 1000,
            };
            for other in [0, 4, 8] {
                if p != other {
                    let rtt = match (p.min(other), p.max(other)) {
                        (0, 4) => 10,
                        (0, 8) => 20,
                        _ => 30,
                    };
                    row.links[other] = Quality {
                        rtt_ms: rtt,
                        upload_bytes_per_second: 10000,
                        loss_per_mille: 0,
                        measured: true,
                    };
                }
            }
            m.rows[p] = Some(row);
        }
        let order = |m: &Mesh| {
            m.candidates(1000)
                .iter()
                .map(|q| q.peer)
                .collect::<Vec<_>>()
        };
        assert_eq!(order(&m), vec![0, 4, 8]);
        assert_eq!(m.candidates(1000)[0].mean_rtt_ms, 15);
        for other in [4, 8] {
            m.rows[0].as_mut().unwrap().links[other].upload_bytes_per_second = 5000;
        }
        assert_eq!(order(&m), vec![4, 8, 0]);
        m.rows[4].as_mut().unwrap().links[8].loss_per_mille = 2;
        assert_eq!(order(&m), vec![0, 4, 8]);
        m.rows[0].as_mut().unwrap().received_ms = 0;
        assert!(m.candidates(6000).is_empty());
        for p in [4, 8] {
            m.rows[p].as_mut().unwrap().received_ms = 6000;
        }
        assert_eq!(m.migration_candidates(6000).len(), 2);
        m.rows[8] = None;
        assert!(m.migration_candidates(6000).is_empty());
        assert_eq!(m.context().peers, 0x111);
    }
    #[test]
    fn exhausted_counters_and_rewound_clock_remain_bounded_without_a_new_round() {
        let mut m = mesh();
        m.links[4].ping_sequence = u32::MAX;
        m.cursor = u32::MAX;
        m.report_sequence = u32::MAX;
        assert!(m.poll(0x10, 1000, 64).is_empty());
        assert!(m.cursor < 1024);
        let l = &mut m.links[4];
        l.ping_sequence = 1;
        l.ping_at = 61_000;
        l.ping_samples = 3;
        l.round = u32::MAX;
        assert!(m.poll(0x10, 61_001, 64).is_empty());
        assert!(!m.links[4].probing);
        assert!(m.matrix(999).iter().all(Option::is_none));
        assert!(!m.local(61_001).unwrap().links[4].measured);
    }
}
impl std::error::Error for Error {}
type Result<T> = std::result::Result<T, Error>;
fn need(value: bool) -> Result<()> {
    if value { Ok(()) } else { Err(Error) }
}
fn member(c: Context, p: u8) -> bool {
    p < 16 && c.peers & (1u16 << p) != 0
}
fn age(now: u64, before: u64) -> u64 {
    now.checked_sub(before).unwrap_or(u64::MAX)
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Quality {
    pub rtt_ms: u32,
    pub upload_bytes_per_second: u32,
    pub loss_per_mille: u16,
    pub measured: bool,
}
impl Default for Quality {
    fn default() -> Self {
        Self {
            rtt_ms: u32::MAX,
            upload_bytes_per_second: 0,
            loss_per_mille: 1000,
            measured: false,
        }
    }
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Row {
    pub links: [Quality; 16],
    pub received_ms: u64,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct HostQuality {
    pub peer: u8,
    pub mean_rtt_ms: u32,
    pub minimum_upload_bytes_per_second: u32,
    pub worst_loss_per_mille: u16,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Packet {
    pub peer: u8,
    pub bytes: Vec<u8>,
}
#[derive(Clone, Copy)]
#[repr(u8)]
enum Kind {
    Ping = 1,
    Echo = 2,
    Upload = 3,
    Receipt = 4,
    Report = 5,
}
fn put(bytes: &mut Vec<u8>, n: u64, width: usize) {
    bytes.extend_from_slice(&n.to_le_bytes()[..width]);
}
fn header(c: Context, kind: Kind, round: u32, index: u16) -> Vec<u8> {
    let mut out = Vec::with_capacity(HEADER);
    out.extend_from_slice(b"H2QO");
    put(&mut out, 1, 2);
    out.extend_from_slice(&[kind as u8, 0]);
    out.extend_from_slice(&c.session);
    put(&mut out, c.epoch as u64, 4);
    put(&mut out, c.membership_revision, 8);
    put(&mut out, c.peers as u64, 2);
    out.extend_from_slice(&[c.host, 0]);
    put(&mut out, round as u64, 4);
    put(&mut out, index as u64, 2);
    out
}
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl Reader<'_> {
    fn get(&mut self, width: usize) -> Result<u64> {
        let part = self
            .bytes
            .get(self.at..self.at.checked_add(width).ok_or(Error)?)
            .ok_or(Error)?;
        let mut result = [0u8; 8];
        need(width <= 8)?;
        result[..width].copy_from_slice(part);
        self.at += width;
        Ok(u64::from_le_bytes(result))
    }
    fn end(&self) -> Result<()> {
        need(self.at == self.bytes.len())
    }
}
#[derive(Clone, Default)]
struct Link {
    ping_sequence: u32,
    round: u32,
    report_sequence: u32,
    pings: [(u32, u64); 8],
    ping_samples: u32,
    next_packet: u32,
    ping_at: u64,
    echo_at: u64,
    probe_at: u64,
    probe_started: u64,
    acked: u64,
    received_span: u64,
    incoming_round: u64,
    incoming_packets: u64,
    incoming_started: u64,
    quality: Quality,
    probing: bool,
}
impl Link {
    fn finish(&mut self, now: u64) {
        if !self.probing || self.next_packet != PROBE_PACKETS || age(now, self.probe_started) < 1000
        {
            return;
        }
        let count = self.acked.count_ones();
        let elapsed = self.received_span.max(1);
        self.quality.upload_bytes_per_second = if count != 0 {
            (((count - 1) as u64 * PROBE_BYTES as u64 * 1000) / elapsed).min(MAX_UPLOAD as u64)
                as u32
        } else {
            0
        };
        self.quality.loss_per_mille = ((PROBE_PACKETS - count) * 1000 / PROBE_PACKETS) as u16;
        self.quality.measured =
            self.ping_samples >= 3 && count > 1 && self.quality.upload_bytes_per_second != 0;
        self.probing = false;
        self.probe_at = now;
    }
}
pub struct Mesh {
    self_peer: u8,
    context: Context,
    links: [Link; 16],
    rows: [Option<Row>; 16],
    replies: VecDeque<Packet>,
    report_at: [u64; 16],
    cursor: u32,
    report_cursor: u32,
    report_sequence: u32,
}
pub fn is_packet(bytes: &[u8]) -> bool {
    bytes.starts_with(b"H2QO")
}
impl Mesh {
    pub fn new(self_peer: u8, context: Context) -> Result<Self> {
        need(
            member(context, self_peer)
                && (context.host == 16 || member(context, context.host))
                && context.peers.count_ones() >= 2,
        )?;
        Ok(Self {
            self_peer,
            context,
            links: std::array::from_fn(|_| Link::default()),
            rows: [None; 16],
            replies: VecDeque::new(),
            report_at: [0; 16],
            cursor: 0,
            report_cursor: 0,
            report_sequence: 0,
        })
    }
    pub fn context(&self) -> Context {
        self.context
    }
    pub fn poll(&mut self, ready: u16, now: u64, budget: u32) -> Vec<Packet> {
        let budget = budget.min(64) as usize;
        let mut out = Vec::new();
        let reply_budget = if budget > 1 { budget / 2 } else { budget };
        while !self.replies.is_empty() && out.len() < reply_budget {
            out.push(self.replies.pop_front().expect("nonempty bounded queue"));
        }
        for _ in 0..16 * budget {
            if out.len() >= budget {
                break;
            }
            let p = (self.cursor % 16) as u8;
            self.cursor = self.cursor.wrapping_add(1);
            if p == self.self_peer || !member(self.context, p) {
                continue;
            }
            let link = &mut self.links[p as usize];
            link.finish(now);
            if ready & (1u16 << p) == 0 {
                link.quality.measured = false;
                continue;
            }
            link.quality.measured = link.ping_samples >= 3
                && link.quality.upload_bytes_per_second != 0
                && age(now, link.echo_at) <= 2000
                && age(now, link.probe_at) <= 90_000;
            if link.ping_sequence == 0 || age(now, link.ping_at) >= 250 {
                if link.ping_sequence == u32::MAX {
                    continue;
                }
                link.ping_sequence += 1;
                let sequence = link.ping_sequence;
                link.ping_at = now;
                link.pings[sequence as usize % 8] = (sequence, now);
                out.push(Packet {
                    peer: p,
                    bytes: header(self.context, Kind::Ping, sequence, 0),
                });
                continue;
            }
            if !link.probing
                && link.ping_samples >= 3
                && (link.round == 0 || age(now, link.probe_at) >= 60_000)
            {
                if link.round == u32::MAX {
                    continue;
                }
                link.round += 1;
                link.probing = true;
                link.next_packet = 0;
                link.acked = 0;
                link.probe_started = now;
                link.received_span = 0;
            }
            if link.probing && link.next_packet < PROBE_PACKETS {
                let mut bytes = header(
                    self.context,
                    Kind::Upload,
                    link.round,
                    link.next_packet as u16,
                );
                link.next_packet += 1;
                bytes.resize(PROBE_BYTES, 0);
                out.push(Packet { peer: p, bytes });
            }
        }
        let mut local = Row {
            links: [Quality::default(); 16],
            received_ms: now,
        };
        for (p, link) in self.links.iter().enumerate() {
            local.links[p] = link.quality;
            local.links[p].measured = link.quality.measured
                && ready & (1u16 << p) != 0
                && age(now, link.echo_at) <= 2000
                && age(now, link.probe_at) <= 90_000;
        }
        self.rows[self.self_peer as usize] = Some(local);
        if out.len() < budget {
            for _ in 0..16 {
                let p = (self.report_cursor % 16) as u8;
                self.report_cursor = self.report_cursor.wrapping_add(1);
                if p == self.self_peer
                    || !member(self.context, p)
                    || ready & (1u16 << p) == 0
                    || age(now, self.report_at[p as usize]) < 500
                {
                    continue;
                }
                if self.report_sequence == u32::MAX {
                    break;
                }
                self.report_sequence += 1;
                let mut bytes = header(self.context, Kind::Report, self.report_sequence, 0);
                for q in local.links {
                    put(&mut bytes, q.rtt_ms as u64, 4);
                    put(&mut bytes, q.upload_bytes_per_second as u64, 4);
                    put(&mut bytes, q.loss_per_mille as u64, 2);
                    bytes.push(u8::from(q.measured));
                }
                out.push(Packet { peer: p, bytes });
                self.report_at[p as usize] = now;
                break;
            }
        }
        out
    }
    pub fn receive(&mut self, peer: u8, bytes: &[u8], now: u64) -> bool {
        if !is_packet(bytes) {
            return false;
        }
        let _ = self.receive_inner(peer, bytes, now);
        true
    }
    fn receive_inner(&mut self, peer: u8, bytes: &[u8], now: u64) -> Result<()> {
        need(
            peer != self.self_peer
                && member(self.context, peer)
                && (HEADER..=PROBE_BYTES).contains(&bytes.len()),
        )?;
        let mut input = Reader { bytes, at: 0 };
        need(input.get(4)? == 0x4f513248 && input.get(2)? == 1)?;
        let kind = input.get(1)?;
        need(input.get(1)? == 0)?;
        for byte in self.context.session {
            need(input.get(1)? == byte as u64)?;
        }
        need(
            input.get(4)? == self.context.epoch as u64
                && input.get(8)? == self.context.membership_revision
                && input.get(2)? == self.context.peers as u64
                && input.get(1)? == self.context.host as u64
                && input.get(1)? == 0,
        )?;
        let round = input.get(4)? as u32;
        let index = input.get(2)? as u16;
        let link = &mut self.links[peer as usize];
        match kind {
            1 => {
                need(round != 0 && index == 0)?;
                input.end()?;
                if self.replies.len() < 256 {
                    self.replies.push_back(Packet {
                        peer,
                        bytes: header(self.context, Kind::Echo, round, index),
                    });
                }
            }
            2 => {
                need(round != 0 && index == 0)?;
                input.end()?;
                let sent = &mut link.pings[round as usize % 8];
                need(sent.0 == round && age(now, sent.1) <= 60_000)?;
                let rtt = age(now, sent.1) as u32;
                link.quality.rtt_ms = if link.ping_samples != 0 {
                    link.quality.rtt_ms.wrapping_mul(3).wrapping_add(rtt) / 4
                } else {
                    rtt
                };
                link.ping_samples = (link.ping_samples + 1).min(1000);
                link.echo_at = now;
                sent.0 = 0;
            }
            3 => {
                need(round != 0 && u32::from(index) < PROBE_PACKETS && bytes.len() == PROBE_BYTES)?;
                if round as u64 > link.incoming_round {
                    link.incoming_round = round as u64;
                    link.incoming_packets = 0;
                    link.incoming_started = now;
                }
                need(
                    round as u64 == link.incoming_round
                        && age(now, link.incoming_started) <= 10_000,
                )?;
                let bit = 1u64 << index;
                if link.incoming_packets & bit == 0 && self.replies.len() < 256 {
                    let mut receipt = header(self.context, Kind::Receipt, round, index);
                    put(&mut receipt, age(now, link.incoming_started), 4);
                    self.replies.push_back(Packet {
                        peer,
                        bytes: receipt,
                    });
                    link.incoming_packets |= bit;
                }
            }
            4 => {
                need(round == link.round && link.probing && u32::from(index) < link.next_packet)?;
                let elapsed = input.get(4)?;
                need(elapsed <= 10_000)?;
                input.end()?;
                let bit = 1u64 << index;
                if link.acked & bit == 0 {
                    link.acked |= bit;
                    link.received_span = link.received_span.max(elapsed);
                }
                link.finish(now);
            }
            5 => {
                need(round > link.report_sequence && index == 0)?;
                let mut row = Row {
                    links: [Quality::default(); 16],
                    received_ms: now,
                };
                for (n, q) in row.links.iter_mut().enumerate() {
                    q.rtt_ms = input.get(4)? as u32;
                    q.upload_bytes_per_second = input.get(4)? as u32;
                    q.loss_per_mille = input.get(2)? as u16;
                    let measured = input.get(1)?;
                    need(
                        measured <= 1
                            && q.loss_per_mille <= 1000
                            && q.upload_bytes_per_second <= MAX_UPLOAD
                            && (q.rtt_ms == u32::MAX || q.rtt_ms <= 60_000),
                    )?;
                    q.measured = measured != 0;
                    if !member(self.context, n as u8) || n == peer as usize {
                        need(
                            !q.measured
                                && q.upload_bytes_per_second == 0
                                && q.rtt_ms == u32::MAX
                                && q.loss_per_mille == 1000,
                        )?;
                    }
                }
                input.end()?;
                self.rows[peer as usize] = Some(row);
                link.report_sequence = round;
            }
            _ => return Err(Error),
        }
        Ok(())
    }
    pub fn matrix(&self, now: u64) -> [Option<Row>; 16] {
        self.rows
            .map(|row| row.filter(|row| age(now, row.received_ms) <= 5000))
    }
    pub fn local(&self, now: u64) -> Option<Row> {
        self.matrix(now)[self.self_peer as usize]
    }
    pub fn candidates(&self, now: u64) -> Vec<HostQuality> {
        self.rank(now, self.context.peers)
    }
    pub fn migration_candidates(&self, now: u64) -> Vec<HostQuality> {
        if self.context.host >= 16 {
            Vec::new()
        } else {
            self.rank(now, self.context.peers & !(1u16 << self.context.host))
        }
    }
    fn rank(&self, now: u64, members: u16) -> Vec<HostQuality> {
        let rows = self.matrix(now);
        let mut out = Vec::new();
        for p in 0..PEERS {
            if members & (1u16 << p) == 0 || rows[p].is_none() {
                continue;
            }
            let mut quality = HostQuality {
                peer: p as u8,
                mean_rtt_ms: 0,
                minimum_upload_bytes_per_second: u32::MAX,
                worst_loss_per_mille: 0,
            };
            let (mut total, mut count) = (0u64, 0u32);
            let mut eligible = true;
            for other in 0..PEERS {
                if other == p || members & (1u16 << other) == 0 {
                    continue;
                }
                let Some(row) = rows[other] else {
                    eligible = false;
                    break;
                };
                let a = rows[p].expect("admitted row").links[other];
                let b = row.links[p];
                if !a.measured
                    || !b.measured
                    || a.upload_bytes_per_second == 0
                    || b.upload_bytes_per_second == 0
                    || a.rtt_ms == u32::MAX
                    || b.rtt_ms == u32::MAX
                {
                    eligible = false;
                    break;
                }
                total += a.rtt_ms.max(b.rtt_ms) as u64;
                count += 1;
                quality.minimum_upload_bytes_per_second = quality
                    .minimum_upload_bytes_per_second
                    .min(a.upload_bytes_per_second);
                quality.worst_loss_per_mille = quality
                    .worst_loss_per_mille
                    .max(a.loss_per_mille)
                    .max(b.loss_per_mille);
            }
            if eligible && count != 0 {
                quality.mean_rtt_ms = (total / count as u64) as u32;
                out.push(quality);
            }
        }
        out.sort_by_key(|q| {
            (
                q.worst_loss_per_mille,
                u32::MAX - q.minimum_upload_bytes_per_second,
                q.mean_rtt_ms,
                q.peer,
            )
        });
        out
    }
}
