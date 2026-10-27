//! Complete named archive grammar. Nodes are flat, so hostile nesting cannot
//! recurse through either parsing or destruction. Native schema/materialization
//! still validates expected names/types, narrowing and actual world ownership.

use miniz_oxide::inflate::TINFLStatus;
use miniz_oxide::inflate::core::{DecompressorOxide, decompress, inflate_flags};
use std::{mem::size_of, ops::Range, sync::Mutex};

#[derive(Clone, Copy, Debug)]
pub struct Limits {
    pub encoded_bytes: u64,
    pub decoded_bytes: u64,
    pub allocated_bytes: u64,
    pub container_elements: u64,
    pub fixed_native_bytes: u64,
    pub working_bytes: u64,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    Envelope,
    Extent,
    Compression,
    Grammar,
    Budget,
    Allocation,
}
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Invalid authority archive: {self:?}")
    }
}

#[derive(Debug)]
pub struct Node {
    pub tag: u8,
    pub bits: u64,
    pub count: u64,
    pub next: usize,
    name: Range<usize>,
    data: Range<usize>,
}

#[derive(Clone, Copy, Debug)]
pub struct Stats {
    pub raw_bytes: u64,
    pub nodes: usize,
    pub metadata_bytes: u64,
    pub deepest: usize,
    pub working_limit: u64,
    pub working_reserved: u64,
    pub working_peak: u64,
    pub native_claimed: u64,
}

struct Budget {
    limit: u64,
    used: u64,
    peak: u64,
    native_left: u64,
    elements_left: u64,
    native_claimed: u64,
}
impl Budget {
    fn charge(&mut self, bytes: u64) -> Result<(), Error> {
        let next = self.used.checked_add(bytes).ok_or(Error::Budget)?;
        if next > self.limit {
            return Err(Error::Budget);
        }
        self.used = next;
        self.peak = self.peak.max(next);
        Ok(())
    }
    fn release(&mut self, bytes: u64) {
        self.used -= bytes;
    }
}

pub struct Archive {
    raw: Vec<u8>,
    nodes: Vec<Node>,
    hash: [u8; 32],
    deepest: usize,
    budget: Mutex<Budget>,
}

impl Archive {
    pub fn node(&self, index: usize) -> Option<&Node> {
        self.nodes.get(index)
    }
    pub fn name(&self, index: usize) -> &[u8] {
        self.nodes
            .get(index)
            .map_or(&[], |n| &self.raw[n.name.clone()])
    }
    pub fn data(&self, index: usize) -> &[u8] {
        self.nodes
            .get(index)
            .map_or(&[], |n| &self.raw[n.data.clone()])
    }
    pub fn raw(&self) -> &[u8] {
        &self.raw
    }
    pub fn hash(&self) -> [u8; 32] {
        self.hash
    }
    pub fn len(&self) -> usize {
        self.nodes.len()
    }
    pub fn is_empty(&self) -> bool {
        self.nodes.is_empty()
    }
    pub fn claim_native(&self, count: u64, extent: u64, elements: bool) -> bool {
        let Ok(mut b) = self.budget.lock() else {
            return false;
        };
        let Some(bytes) = count.checked_mul(extent) else {
            return false;
        };
        if bytes > b.native_left || (elements && count > b.elements_left) {
            return false;
        }
        if b.charge(bytes).is_err() {
            return false;
        }
        b.native_left -= bytes;
        b.native_claimed += bytes;
        if elements {
            b.elements_left -= count;
        }
        true
    }
    pub fn stats(&self) -> Option<Stats> {
        let b = self.budget.lock().ok()?;
        Some(Stats {
            raw_bytes: self.raw.len() as u64,
            nodes: self.nodes.len(),
            metadata_bytes: (self.nodes.capacity() * size_of::<Node>()) as u64,
            deepest: self.deepest,
            working_limit: b.limit,
            working_reserved: b.used,
            working_peak: b.peak,
            native_claimed: b.native_claimed,
        })
    }
}

struct Cursor<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl Cursor<'_> {
    fn range(&mut self, count: u64) -> Result<Range<usize>, Error> {
        let count = usize::try_from(count).map_err(|_| Error::Extent)?;
        let end = self.at.checked_add(count).ok_or(Error::Extent)?;
        if end > self.bytes.len() {
            return Err(Error::Extent);
        }
        let range = self.at..end;
        self.at = end;
        Ok(range)
    }
    fn word(&mut self, width: usize) -> Result<u64, Error> {
        let range = self.range(width as u64)?;
        let mut result = 0;
        for (i, b) in self.bytes[range].iter().enumerate() {
            result |= u64::from(*b) << (8 * i);
        }
        Ok(result)
    }
    fn boolean(&mut self) -> Result<u64, Error> {
        let value = self.word(1)?;
        if value > 1 {
            return Err(Error::Grammar);
        }
        Ok(value)
    }
    fn node(&mut self, name: Range<usize>, index: usize) -> Result<(Node, u64), Error> {
        let tag = self.word(1)? as u8;
        let mut n = Node {
            tag,
            bits: 0,
            count: 0,
            next: index + 1,
            name,
            data: 0..0,
        };
        let children = match tag {
            0 => {
                n.bits = self.boolean()?;
                0
            }
            1 | 2 | 4 | 14 => {
                n.bits = self.word(8)?;
                0
            }
            3 => {
                n.bits = self.word(4)?;
                0
            }
            5 | 6 => {
                n.count = self.word(8)?;
                n.data = self.range(n.count)?;
                0
            }
            7..=10 => {
                n.count = self.word(8)?;
                if tag == 9 {
                    n.count.checked_mul(2).ok_or(Error::Extent)?
                } else {
                    n.count
                }
            }
            11 => {
                n.bits = self.boolean()?;
                n.bits
            }
            12 => {
                if self.boolean()? != 0 {
                    return Err(Error::Grammar);
                }
                0
            }
            13 => {
                n.count = self.word(4)?;
                n.count
            }
            _ => return Err(Error::Grammar),
        };
        // Every child consumes at least a tag, before names or scalar payload.
        if children > (self.bytes.len() - self.at) as u64 {
            return Err(Error::Extent);
        }
        Ok((n, children))
    }
}

fn reserve<T>(v: &mut Vec<T>, budget: &mut Budget) -> Result<(), Error> {
    if v.len() < v.capacity() {
        return Ok(());
    }
    let old = v.capacity();
    let desired = old.saturating_mul(2).max(16);
    let available = usize::try_from((budget.limit - budget.used) / (size_of::<T>() as u64))
        .unwrap_or(usize::MAX);
    let capacity = desired.min(old.saturating_add(available));
    if capacity <= old {
        return Err(Error::Budget);
    }
    // A realloc may retain the old buffer while allocating/copying the new
    // one. Reserve both lifetimes at the peak, then retire the old capacity.
    budget.charge((capacity * size_of::<T>()) as u64)?;
    v.try_reserve_exact(capacity - v.len())
        .map_err(|_| Error::Allocation)?;
    if v.capacity() > capacity {
        budget.charge(((v.capacity() - capacity) * size_of::<T>()) as u64)?;
    }
    budget.release((old * size_of::<T>()) as u64);
    Ok(())
}

struct Frame {
    parent: usize,
    remaining: u64,
    record: bool,
}
fn tree(raw: &[u8], budget: &mut Budget) -> Result<(Vec<Node>, usize), Error> {
    let mut cursor = Cursor { bytes: raw, at: 0 };
    let mut nodes = Vec::new();
    let mut frames = Vec::new();
    let mut deepest = 0;
    let (root, children) = cursor.node(0..0, 0)?;
    if root.tag != 13 {
        return Err(Error::Grammar);
    }
    reserve(&mut nodes, budget)?;
    nodes.push(root);
    if children != 0 {
        reserve(&mut frames, budget)?;
        frames.push(Frame {
            parent: 0,
            remaining: children,
            record: true,
        });
        deepest = 1;
    }
    while let Some(frame) = frames.last_mut() {
        if frame.remaining == 0 {
            nodes[frame.parent].next = nodes.len();
            frames.pop();
            continue;
        }
        frame.remaining -= 1;
        let name = if frame.record {
            let n = cursor.word(2)?;
            cursor.range(n)?
        } else {
            0..0
        };
        let index = nodes.len();
        let (node, children) = cursor.node(name, index)?;
        let record = node.tag == 13;
        reserve(&mut nodes, budget)?;
        nodes.push(node);
        if children != 0 {
            reserve(&mut frames, budget)?;
            frames.push(Frame {
                parent: index,
                remaining: children,
                record,
            });
            deepest = deepest.max(frames.len());
        }
    }
    if cursor.at != raw.len() {
        return Err(Error::Grammar);
    }
    budget.release((frames.capacity() * size_of::<Frame>()) as u64);
    Ok((nodes, deepest))
}

/// The checksum is an owned expected value. The existing vetted SHA-256 consumer
/// must compare it to `raw()` before native materialization; it is not a credential.
fn budget(limits: Limits, retained: u64) -> Result<Budget, Error> {
    let fixed = limits
        .fixed_native_bytes
        .saturating_add(size_of::<Archive>() as u64);
    let additive = limits
        .encoded_bytes
        .max(retained)
        .saturating_add(limits.decoded_bytes)
        .saturating_add(limits.allocated_bytes)
        .saturating_add(fixed);
    let mut result = Budget {
        limit: if limits.working_bytes == 0 {
            additive
        } else {
            limits.working_bytes
        },
        used: 0,
        peak: 0,
        native_left: limits.allocated_bytes,
        elements_left: limits.container_elements,
        native_claimed: 0,
    };
    result.charge(retained.checked_add(fixed).ok_or(Error::Budget)?)?;
    Ok(result)
}

fn finish(raw: Vec<u8>, hash: [u8; 32], mut budget: Budget) -> Result<Archive, Error> {
    let (nodes, deepest) = tree(&raw, &mut budget)?;
    Ok(Archive {
        raw,
        nodes,
        hash,
        deepest,
        budget: Mutex::new(budget),
    })
}

/// Canonical bytes from the independently bounded delta owner. The caller's
/// backing and the owned Rust copy are both charged while parsing/materializing.
pub fn decode_raw(bytes: &[u8], hash: [u8; 32], limits: Limits) -> Result<Archive, Error> {
    if bytes.is_empty() || bytes.len() as u64 > limits.decoded_bytes {
        return Err(Error::Extent);
    }
    let mut budget = budget(limits, bytes.len() as u64)?;
    budget.charge(bytes.len() as u64)?;
    let mut raw = Vec::new();
    raw.try_reserve_exact(bytes.len())
        .map_err(|_| Error::Allocation)?;
    if raw.capacity() > bytes.len() {
        budget.charge((raw.capacity() - bytes.len()) as u64)?;
    }
    raw.extend_from_slice(bytes);
    finish(raw, hash, budget)
}

pub fn decode(bytes: &[u8], limits: Limits) -> Result<Archive, Error> {
    if bytes.len() < 64 || bytes.len() as u64 > limits.encoded_bytes {
        return Err(Error::Envelope);
    }
    let mut envelope = Cursor { bytes, at: 0 };
    if &bytes[envelope.range(8)?] != b"H2AUTH01" || envelope.word(4)? != 1 || envelope.word(4)? != 0
    {
        return Err(Error::Envelope);
    }
    let raw_size = envelope.word(8)?;
    let compressed_size = envelope.word(8)?;
    if raw_size > limits.decoded_bytes || compressed_size != (bytes.len() - 64) as u64 {
        return Err(Error::Extent);
    }
    if compressed_size == 0 || raw_size > compressed_size.saturating_mul(2048).saturating_add(65536)
    {
        return Err(Error::Extent);
    }
    let hash = bytes[envelope.range(32)?]
        .try_into()
        .map_err(|_| Error::Extent)?;
    let mut budget = budget(limits, bytes.len() as u64)?;
    let size = usize::try_from(raw_size).map_err(|_| Error::Extent)?;
    budget.charge(raw_size)?;
    let mut raw = Vec::new();
    raw.try_reserve_exact(size).map_err(|_| Error::Allocation)?;
    if raw.capacity() > size {
        budget.charge((raw.capacity() - size) as u64)?;
    }
    raw.resize(size, 0);
    budget.charge(size_of::<DecompressorOxide>() as u64)?;
    let (status, consumed, written) = {
        let mut state = DecompressorOxide::new();
        decompress(
            &mut state,
            &bytes[64..],
            &mut raw,
            0,
            inflate_flags::TINFL_FLAG_PARSE_ZLIB_HEADER
                | inflate_flags::TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF,
        )
    };
    budget.release(size_of::<DecompressorOxide>() as u64);
    if status != TINFLStatus::Done || consumed != bytes.len() - 64 || written != size {
        return Err(Error::Compression);
    }
    finish(raw, hash, budget)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn raw_images_share_the_grammar_and_cumulative_materialization_budget() {
        let raw = [13, 0, 0, 0, 0];
        let hash = [0x15; 32];
        let archive = decode_raw(&raw, hash, limits()).expect("Canonical root");
        assert_eq!(archive.raw(), raw);
        assert_eq!(archive.hash(), hash);
        assert_eq!(archive.len(), 1);
        let mut bounded = limits();
        bounded.working_bytes = raw.len() as u64;
        assert!(decode_raw(&raw, hash, bounded).is_err());
        bounded = limits();
        bounded.decoded_bytes = raw.len() as u64 - 1;
        assert!(decode_raw(&raw, hash, bounded).is_err());
    }
    #[test]
    fn explicit_working_budget_reserves_metadata_without_relaxing_components() {
        let mut raw = vec![13, 1, 0, 0, 0, 0, 0];
        for _ in 0..1000 {
            raw.extend_from_slice(&[11, 1]);
        }
        raw.extend_from_slice(&[0, 1]);
        let mut l = limits();
        l.encoded_bytes = 1;
        l.decoded_bytes = raw.len() as u64;
        l.allocated_bytes = 0;
        l.fixed_native_bytes = 0;
        l.working_bytes = 1 << 20;
        let parsed = decode_raw(&raw, [0; 32], l).expect("Explicit metadata reservation");
        assert!(parsed.stats().unwrap().metadata_bytes > 2 * raw.len() as u64);
        assert!(!parsed.claim_native(1, 1, false));
        let mut additive = l;
        additive.working_bytes = 0;
        assert!(decode_raw(&raw, [0; 32], additive).is_err());
        l.decoded_bytes -= 1;
        assert!(decode_raw(&raw, [0; 32], l).is_err());
        l.decoded_bytes += 1;
        assert!(decode(&frame(&raw), l).is_err());
    }
    fn limits() -> Limits {
        Limits {
            encoded_bytes: 1 << 20,
            decoded_bytes: 16 << 20,
            allocated_bytes: 16 << 20,
            container_elements: 1_000_000,
            fixed_native_bytes: 128,
            working_bytes: 0,
        }
    }
    fn record(value: &[u8]) -> Vec<u8> {
        let mut b = vec![13, 1, 0, 0, 0, 4, 0, b'd', b'a', b't', b'a'];
        b.extend_from_slice(value);
        b
    }
    fn frame(raw: &[u8]) -> Vec<u8> {
        let compressed = miniz_oxide::deflate::compress_to_vec_zlib(raw, 6);
        let mut b = b"H2AUTH01".to_vec();
        b.extend_from_slice(&1u32.to_le_bytes());
        b.extend_from_slice(&0u32.to_le_bytes());
        b.extend_from_slice(&(raw.len() as u64).to_le_bytes());
        b.extend_from_slice(&(compressed.len() as u64).to_le_bytes());
        b.extend_from_slice(&[0x15; 32]);
        b.extend_from_slice(&compressed);
        b
    }
    #[test]
    fn archive_owns_backing_and_exact_scalar_bits() {
        for (tag, bits, width) in [
            (0, 1, 1),
            (1, u64::MAX, 8),
            (2, u64::MAX, 8),
            (3, 0x7fc00001, 4),
            (4, 0x8000000000000000, 8),
            (14, 255, 8),
        ] {
            let mut field = vec![tag];
            field.extend_from_slice(&bits.to_le_bytes()[..width]);
            let raw = record(&field);
            let mut encoded = frame(&raw);
            let parsed = decode(&encoded, limits()).unwrap();
            encoded.fill(0);
            assert_eq!(parsed.raw(), raw);
            assert_eq!(parsed.hash(), [0x15; 32]);
            assert_eq!(parsed.name(1), b"data");
            let n = parsed.node(1).unwrap();
            assert_eq!((n.tag, n.bits, n.next), (tag, bits, 2));
            assert_eq!(parsed.node(0).unwrap().next, 2);
            assert!(parsed.node(2).is_none());
        }
    }
    #[test]
    fn all_container_shapes_and_binary_strings_remain_owned() {
        for tag in [5, 6] {
            let mut field = vec![tag];
            field.extend_from_slice(&3u64.to_le_bytes());
            field.extend_from_slice(&[0, 255, 128]);
            let parsed = decode(&frame(&record(&field)), limits()).unwrap();
            assert_eq!(parsed.data(1), [0, 255, 128]);
        }
        for tag in [7, 8, 9, 10] {
            let mut field = vec![tag];
            field.extend_from_slice(&1u64.to_le_bytes());
            field.extend_from_slice(&[0, 1]);
            if tag == 9 {
                field.extend_from_slice(&[0, 0]);
            }
            let parsed = decode(&frame(&record(&field)), limits()).unwrap();
            assert_eq!(parsed.len(), if tag == 9 { 4 } else { 3 });
            assert_eq!(parsed.node(1).unwrap().count, 1);
        }
        for field in [
            vec![11, 0],
            vec![11, 1, 0, 1],
            vec![12, 0],
            vec![13, 0, 0, 0, 0],
        ] {
            assert!(decode(&frame(&record(&field)), limits()).is_ok());
        }
    }
    #[test]
    fn deep_untrusted_trees_parse_and_drop_without_recursion() {
        let mut field = Vec::new();
        for _ in 0..60_000 {
            field.extend_from_slice(&[11, 1]);
        }
        field.extend_from_slice(&[0, 1]);
        let parsed = decode(&frame(&record(&field)), limits()).unwrap();
        let s = parsed.stats().unwrap();
        assert_eq!((s.nodes, s.deepest), (60_002, 60_001));
        assert!(s.metadata_bytes > 0 && s.working_peak <= s.working_limit);
        drop(parsed);
    }
    #[test]
    fn corrupt_trailing_pointer_and_count_expansion_are_rejected() {
        let valid = frame(&record(&[0, 1]));
        for n in 0..valid.len() {
            assert!(decode(&valid[..n], limits()).is_err());
        }
        for mut field in [
            vec![0, 2],
            vec![11, 2],
            vec![12, 1, 0, 1],
            vec![255],
            vec![13, 255, 255, 255, 255],
            vec![9, 255, 255, 255, 255, 255, 255, 255, 255],
        ] {
            assert!(decode(&frame(&record(&field)), limits()).is_err());
            field.clear();
        }
        let mut extra = valid.clone();
        extra.push(0);
        let n = (extra.len() - 64) as u64;
        extra[24..32].copy_from_slice(&n.to_le_bytes());
        assert!(matches!(decode(&extra, limits()), Err(Error::Compression)));
        let mut corrupt = valid.clone();
        let n = corrupt.len() - 1;
        corrupt[n] ^= 1;
        assert!(matches!(
            decode(&corrupt, limits()),
            Err(Error::Compression)
        ));
        let mut trailing = record(&[0, 1]);
        trailing.push(0);
        assert!(decode(&frame(&trailing), limits()).is_err());
        let mut l = limits();
        l.decoded_bytes = 1;
        assert!(matches!(decode(&valid, l), Err(Error::Extent)));
    }
    #[test]
    fn native_claims_are_aggregate_checked_and_thread_safe() {
        let mut l = limits();
        l.allocated_bytes = 37;
        l.container_elements = 37;
        let archive = decode(&frame(&record(&[0, 1])), l).unwrap();
        std::thread::scope(|scope| {
            for _ in 0..4 {
                scope.spawn(|| {
                    for _ in 0..100 {
                        archive.claim_native(1, 1, true);
                    }
                });
            }
        });
        assert_eq!(archive.stats().unwrap().native_claimed, 37);
        assert!(!archive.claim_native(1, 1, false));
        assert!(!archive.claim_native(u64::MAX, 2, true));
        assert_eq!(archive.stats().unwrap().native_claimed, 37);
    }
}
