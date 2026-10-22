//! Bounded datagram framing for EOS's 1170-byte packet limit.
//! EOS provides encryption and peer identity; the room supplies pairwise keys.
use std::collections::{BTreeMap, VecDeque};

pub const MAX_DATAGRAM: usize = 1404;
pub const MAX_PACKET: usize = 1170;
const HEADER: usize = 12;
const CHUNK: usize = MAX_PACKET - HEADER;

pub fn hello(source: u8, target: u8, key: &[u8; 32]) -> Vec<u8> {
    let mut value = vec![b'H', b'2', 1, 0, source, target];
    value.extend_from_slice(key);
    value
}

pub fn authenticates(packet: &[u8], source: u8, target: u8, key: &[u8; 32]) -> bool {
    authenticates_kind(packet, source, target, key, 0)
}

/// The admitted endpoint has finished validating and joining its EOS session.
pub fn session_hello(source: u8, target: u8, key: &[u8; 32]) -> Vec<u8> {
    let mut value = hello(source, target, key);
    value[3] = 2;
    value
}

pub fn authenticates_session(packet: &[u8], source: u8, target: u8, key: &[u8; 32]) -> bool {
    authenticates_kind(packet, source, target, key, 2)
}

fn authenticates_kind(packet: &[u8], source: u8, target: u8, key: &[u8; 32], kind: u8) -> bool {
    if packet.len() != 38 || packet[..6] != [b'H', b'2', 1, kind, source, target] {
        return false;
    }
    packet[6..]
        .iter()
        .zip(key)
        .fold(0u8, |d, (a, b)| d | (a ^ b))
        == 0
}

pub fn fragments(sequence: u32, packet: &[u8]) -> Vec<Vec<u8>> {
    if packet.is_empty() || packet.len() > MAX_DATAGRAM {
        return vec![];
    }
    let count = packet.len().div_ceil(CHUNK) as u8;
    packet
        .chunks(CHUNK)
        .enumerate()
        .map(|(index, chunk)| {
            let mut value = vec![b'H', b'2', 1, 1];
            value.extend_from_slice(&sequence.to_le_bytes());
            value.extend_from_slice(&(packet.len() as u16).to_le_bytes());
            value.extend_from_slice(&[index as u8, count]);
            value.extend_from_slice(chunk);
            value
        })
        .collect()
}

struct Partial {
    created: u64,
    size: usize,
    parts: [Option<Vec<u8>>; 2],
}
#[derive(Default)]
pub struct Reassembly {
    pending: BTreeMap<u32, Partial>,
    completed: VecDeque<u32>,
}
impl Reassembly {
    pub fn receive(&mut self, packet: &[u8], now_ms: u64) -> Option<Vec<u8>> {
        self.pending
            .retain(|_, part| now_ms.saturating_sub(part.created) < 250);
        if packet.len() <= HEADER || packet.len() > MAX_PACKET || packet[..4] != [b'H', b'2', 1, 1]
        {
            return None;
        }
        let sequence = u32::from_le_bytes(packet[4..8].try_into().ok()?);
        let size = u16::from_le_bytes(packet[8..10].try_into().ok()?) as usize;
        let index = packet[10] as usize;
        let count = packet[11] as usize;
        if size == 0
            || size > MAX_DATAGRAM
            || count != size.div_ceil(CHUNK)
            || index >= count
            || self.completed.contains(&sequence)
        {
            return None;
        }
        let expected = (size - index * CHUNK).min(CHUNK);
        if packet.len() != HEADER + expected {
            return None;
        }
        if !self.pending.contains_key(&sequence) && self.pending.len() >= 64 {
            return None;
        }
        let part = self.pending.entry(sequence).or_insert_with(|| Partial {
            created: now_ms,
            size,
            parts: [None, None],
        });
        if part.size != size {
            return None;
        }
        if let Some(previous) = &part.parts[index] {
            if previous != &packet[HEADER..] {
                self.pending.remove(&sequence);
            }
            return None;
        }
        part.parts[index] = Some(packet[HEADER..].to_vec());
        if part.parts[..count].iter().any(Option::is_none) {
            return None;
        }
        let part = self.pending.remove(&sequence)?;
        let result = part.parts.into_iter().flatten().flatten().collect();
        if self.completed.len() == 64 {
            self.completed.pop_front();
        }
        self.completed.push_back(sequence);
        Some(result)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn full_native_datagram_unordered_and_duplicate() {
        for size in [1, 1158, 1159, 1400, 1404] {
            let bytes: Vec<_> = (0..size).map(|i| i as u8).collect();
            let frames = fragments(19, &bytes);
            assert!(frames.iter().all(|f| f.len() <= MAX_PACKET));
            let mut reader = Reassembly::default();
            let outputs: Vec<_> = frames
                .iter()
                .rev()
                .filter_map(|p| reader.receive(p, 0))
                .collect();
            assert_eq!(outputs, vec![bytes]);
            for frame in &frames {
                assert_eq!(reader.receive(frame, 1), None);
            }
        }
    }
    #[test]
    fn malformed_lost_expired_and_bounded() {
        assert!(fragments(0, &[]).is_empty());
        assert!(fragments(0, &[0; 1405]).is_empty());
        let frames = fragments(1, &[5; 1404]);
        let mut reader = Reassembly::default();
        assert_eq!(reader.receive(&frames[0], 0), None);
        assert_eq!(reader.receive(&frames[1], 251), None);
        for size in 0..MAX_PACKET + 10 {
            assert_eq!(reader.receive(&vec![0; size], 252), None);
        }
        for seq in 2..1000 {
            let f = fragments(seq, &[0; 1404]);
            reader.receive(&f[0], 253);
        }
        assert_eq!(reader.pending.len(), 64);
        for index in [2, 3, 8, 9, 10, 11] {
            let mut bad = frames[0].clone();
            bad[index] = 255;
            assert_eq!(Reassembly::default().receive(&bad, 0), None);
        }
    }
    #[test]
    fn admission_is_pair_and_room_specific() {
        let key = [42; 32];
        let packet = hello(0, 4, &key);
        assert!(authenticates(&packet, 0, 4, &key));
        assert!(!authenticates(&packet, 4, 0, &key));
        assert!(!authenticates(&packet, 0, 4, &[41; 32]));
        assert!(!authenticates_session(&packet, 0, 4, &key));
        let joined = session_hello(0, 4, &key);
        assert!(authenticates_session(&joined, 0, 4, &key));
        assert!(!authenticates(&joined, 0, 4, &key));
        assert!(!authenticates_session(&joined, 4, 0, &key));
        assert!(!authenticates_session(&joined, 0, 4, &[41; 32]));
        for length in 0..packet.len() {
            assert!(!authenticates(&packet[..length], 0, 4, &key));
        }
    }
}
