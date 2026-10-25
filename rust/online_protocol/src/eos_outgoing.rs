//! Bounded, per-peer FIFO admission ahead of the unreliable EOS packet queue.
//! A queue-full SDK result retains the exact unsent fragment until a later tick.
use crate::eos_wire;
use std::collections::VecDeque;

pub const MAX_DATAGRAMS: usize = 512;
pub const MAX_BYTES: usize = 768 * 1024;
pub const MAX_ATTEMPTS: usize = 32;
const MAX_AGE_MS: u64 = 750;
const MAX_PARTIAL_AGE_MS: u64 = 200;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SendResult {
    Sent,
    Backpressure,
    Failed,
}
#[derive(Clone, Copy, Default, Debug)]
pub struct Statistics {
    pub admitted: u64,
    pub invalid_refused: u64,
    pub capacity_refused: u64,
    pub sent: u64,
    pub expired: u64,
    pub cancelled: u64,
    pub permanent_failed: u64,
    pub backpressure: u64,
    pub depth_max: usize,
    pub bytes_max: usize,
}
struct Datagram {
    frames: Vec<Vec<u8>>,
    next: usize,
    bytes: usize,
    admitted: u64,
    first_sent: Option<u64>,
}
impl Datagram {
    fn expired(&self, now: u64) -> bool {
        now.saturating_sub(self.admitted) >= MAX_AGE_MS
            || self
                .first_sent
                .is_some_and(|first| now.saturating_sub(first) >= MAX_PARTIAL_AGE_MS)
    }
}
pub struct Outgoing {
    peers: [VecDeque<Datagram>; 16],
    next_peer: u8,
    sequence: u32,
    depth: usize,
    bytes: usize,
    statistics: Statistics,
}
impl Default for Outgoing {
    fn default() -> Self {
        Self {
            peers: std::array::from_fn(|_| VecDeque::new()),
            next_peer: 0,
            sequence: 0,
            depth: 0,
            bytes: 0,
            statistics: Statistics::default(),
        }
    }
}
impl Outgoing {
    pub fn statistics(&self) -> Statistics {
        self.statistics
    }
    pub fn depth(&self) -> usize {
        self.depth
    }
    pub fn bytes(&self) -> usize {
        self.bytes
    }
    /// Admission is atomic: no SDK callback occurs here, and refusal retains no bytes.
    pub fn admit(&mut self, peer: u8, data: &[u8], now: u64) -> bool {
        if peer >= 16 || data.is_empty() || data.len() > eos_wire::MAX_DATAGRAM {
            self.statistics.invalid_refused = self.statistics.invalid_refused.saturating_add(1);
            return false;
        }
        if self.depth == MAX_DATAGRAMS {
            self.statistics.capacity_refused = self.statistics.capacity_refused.saturating_add(1);
            return false;
        }
        let frames = eos_wire::fragments(self.sequence, data);
        let bytes = frames.iter().map(Vec::len).sum::<usize>();
        if self.bytes + bytes > MAX_BYTES {
            self.statistics.capacity_refused = self.statistics.capacity_refused.saturating_add(1);
            return false;
        }
        self.peers[usize::from(peer)].push_back(Datagram {
            frames,
            next: 0,
            bytes,
            admitted: now,
            first_sent: None,
        });
        self.sequence = self.sequence.wrapping_add(1);
        self.depth += 1;
        self.bytes += bytes;
        self.statistics.admitted = self.statistics.admitted.saturating_add(1);
        self.statistics.depth_max = self.statistics.depth_max.max(self.depth);
        self.statistics.bytes_max = self.statistics.bytes_max.max(self.bytes);
        true
    }
    fn remove(&mut self, peer: usize) {
        let packet = self.peers[peer]
            .pop_front()
            .expect("queue front was inspected");
        // All encoded buffers remain resident until this owning row is removed,
        // including fragments whose SDK send has already succeeded.
        self.bytes -= packet.bytes;
        self.depth -= 1;
    }
    pub fn cancel_peer(&mut self, peer: u8) {
        if peer >= 16 {
            return;
        }
        let peer = usize::from(peer);
        while !self.peers[peer].is_empty() {
            self.remove(peer);
            self.statistics.cancelled = self.statistics.cancelled.saturating_add(1);
        }
    }
    pub fn clear(&mut self) {
        for peer in 0..16 {
            self.cancel_peer(peer);
        }
    }
    /// At most512 expiry removals and32 SDK attempts. Failed attempts also
    /// advance the service cursor, so an errored peer cannot monopolize budget1.
    pub fn poll(
        &mut self,
        now: u64,
        budget: usize,
        mut send: impl FnMut(u8, &[u8]) -> SendResult,
    ) -> usize {
        for peer in 0..16 {
            while self.peers[peer]
                .front()
                .is_some_and(|packet| packet.expired(now))
            {
                self.remove(peer);
                self.statistics.expired = self.statistics.expired.saturating_add(1);
            }
        }
        let mut attempts = 0;
        while attempts < budget.min(MAX_ATTEMPTS) {
            let Some(peer) = (0..16)
                .map(|offset| (usize::from(self.next_peer) + offset) % 16)
                .find(|&peer| !self.peers[peer].is_empty())
            else {
                break;
            };
            self.next_peer = ((peer + 1) % 16) as u8;
            let packet = self.peers[peer]
                .front_mut()
                .expect("selected nonempty peer");
            let result = send(peer as u8, &packet.frames[packet.next]);
            attempts += 1;
            match result {
                SendResult::Sent => {
                    packet.first_sent.get_or_insert(now);
                    packet.next += 1;
                    if packet.next == packet.frames.len() {
                        self.remove(peer);
                        self.statistics.sent = self.statistics.sent.saturating_add(1);
                    }
                }
                SendResult::Backpressure => {
                    self.statistics.backpressure = self.statistics.backpressure.saturating_add(1);
                    break;
                }
                SendResult::Failed => {
                    self.remove(peer);
                    self.statistics.permanent_failed =
                        self.statistics.permanent_failed.saturating_add(1);
                }
            }
        }
        attempts
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn full_burst_admission_is_atomic_bounded_and_keeps_partial_buffers_counted() {
        let mut queue = Outgoing::default();
        for _ in 0..MAX_DATAGRAMS {
            assert!(queue.admit(0, &[1; 1404], 0));
        }
        assert!(!queue.admit(1, &[2; 1], 0));
        assert_eq!((queue.depth(), queue.bytes()), (512, 512 * 1428));
        assert_eq!(queue.statistics().capacity_refused, 1);
        assert_eq!(queue.poll(1, 1, |_, _| SendResult::Sent), 1);
        assert_eq!((queue.depth(), queue.bytes()), (512, 512 * 1428));
        assert!(!queue.admit(16, &[1], 1));
        assert!(!queue.admit(0, &[], 1));
        assert!(!queue.admit(0, &[1; 1405], 1));
        assert_eq!(queue.statistics().invalid_refused, 3);
    }
    #[test]
    fn queue_full_retries_exact_fragment_and_preserves_peer_fifo() {
        let mut queue = Outgoing::default();
        assert!(queue.admit(3, &[1; 1404], 0));
        assert!(queue.admit(3, &[2; 1404], 0));
        let mut calls = 0;
        let mut blocked = Vec::new();
        let mut received = eos_wire::Reassembly::default();
        assert_eq!(
            queue.poll(0, 32, |peer, bytes| {
                assert_eq!(peer, 3);
                calls += 1;
                if calls == 2 {
                    blocked = bytes.to_vec();
                    SendResult::Backpressure
                } else {
                    assert!(received.receive(bytes, 0).is_none());
                    SendResult::Sent
                }
            }),
            2
        );
        assert_eq!(queue.depth(), 2);
        let mut values = Vec::new();
        let mut first = true;
        queue.poll(1, 32, |_, bytes| {
            if first {
                assert_eq!(bytes, blocked);
                first = false;
            }
            if let Some(value) = received.receive(bytes, 1) {
                values.push(value);
            }
            SendResult::Sent
        });
        assert_eq!(values, vec![vec![1; 1404], vec![2; 1404]]);
        assert_eq!((queue.depth(), queue.bytes()), (0, 0));
        assert_eq!(
            (queue.statistics().sent, queue.statistics().backpressure),
            (2, 1)
        );
    }
    #[test]
    fn failed_or_backpressured_peer_does_not_monopolize_next_tick() {
        for failure in [SendResult::Failed, SendResult::Backpressure] {
            let mut queue = Outgoing::default();
            for peer in 0..3 {
                for _ in 0..2 {
                    assert!(queue.admit(peer, &[peer; 10], 0));
                }
            }
            queue.poll(0, 1, |peer, _| {
                assert_eq!(peer, 0);
                failure
            });
            queue.poll(1, 1, |peer, _| {
                assert_eq!(peer, 1);
                SendResult::Sent
            });
            queue.poll(2, 1, |peer, _| {
                assert_eq!(peer, 2);
                SendResult::Sent
            });
        }
    }
    #[test]
    fn admission_and_first_fragment_deadlines_never_reset_on_retry() {
        let mut queue = Outgoing::default();
        assert!(queue.admit(0, &[1; 1404], 0));
        queue.poll(0, 1, |_, _| SendResult::Sent);
        queue.poll(199, 32, |_, _| SendResult::Backpressure);
        assert_eq!(queue.depth(), 1);
        assert_eq!(
            queue.poll(200, 32, |_, _| panic!("expired partial sent")),
            0
        );
        assert_eq!(queue.statistics().expired, 1);
        assert!(queue.admit(0, &[1; 1404], 200));
        queue.poll(949, 32, |_, _| SendResult::Backpressure);
        assert_eq!(queue.depth(), 1);
        assert_eq!(
            queue.poll(950, 32, |_, _| panic!("expired admission sent")),
            0
        );
        assert_eq!(queue.statistics().expired, 2);
    }
    #[test]
    fn poll_budget_and_cancellation_are_finite() {
        let mut queue = Outgoing::default();
        for _ in 0..512 {
            assert!(queue.admit(4, &[1; 1404], 0));
        }
        assert_eq!(queue.poll(0, usize::MAX, |_, _| SendResult::Sent), 32);
        assert_eq!(queue.statistics().sent, 16);
        queue.cancel_peer(16);
        queue.cancel_peer(4);
        queue.clear();
        assert_eq!((queue.depth(), queue.bytes()), (0, 0));
        assert_eq!(queue.statistics().cancelled, 496);
        assert_eq!(
            queue.poll(750, 32, |_, _| panic!("cancelled packet sent")),
            0
        );
    }
}
