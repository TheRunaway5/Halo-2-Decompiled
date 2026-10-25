//! Lossless delta coding of serialized snapshot state against an
//! acknowledged baseline.
//!
//! The native encoder owns the schema; this codec sees bytes, the offsets of
//! 32-bit floats it wrote, and keyed anchors at list sections and entries.
//! Units are bytes, or whole floats where marked. Long runs that match the
//! aligned baseline (and did not change in it either) cost a single length.
//! Other units are coded with contexts taken from the baseline's own change
//! history, and floats as ordered-integer residuals predicted from the
//! baseline's velocity. Variable-length lists shift later bytes; at each
//! anchor whose key the baseline also holds, both sides re-align exactly.

use super::range::{Bit, Byte, Decoder, Encoder, Error, Int, Uint};
use std::cell::OnceCell;
use std::collections::HashMap;

#[derive(Clone, Default, Debug, PartialEq, Eq)]
pub struct BitSet(Vec<u64>);

impl BitSet {
    pub fn with_len(len: usize) -> Self {
        Self(vec![0; len.div_ceil(64)])
    }
    pub fn get(&self, at: usize) -> bool {
        self.0.get(at / 64).is_some_and(|word| word & (1 << (at % 64)) != 0)
    }
    pub fn set(&mut self, at: usize) {
        self.0[at / 64] |= 1 << (at % 64);
    }
    fn set_range(&mut self, at: usize, len: usize) {
        for n in at..at + len {
            self.set(n);
        }
    }
}

/// One reconstructed generic region, retained as a future baseline.
#[derive(Default)]
pub struct View {
    pub bytes: Vec<u8>,
    pub floats: BitSet,
    pub changed: BitSet,
    /// Nonzero float residuals against this view's own baseline, by offset.
    pub residuals: Vec<(u32, i64)>,
    pub tick: u32,
    /// Ticks to this view's own baseline; zero disables velocity prediction.
    pub span: u32,
    /// Strictly increasing offsets with their schema keys.
    pub anchors: Vec<(u32, u64)>,
    keys: OnceCell<HashMap<u64, u32>>,
}

impl View {
    fn byte(&self, at: i64) -> u8 {
        usize::try_from(at).ok().and_then(|at| self.bytes.get(at)).copied().unwrap_or(0)
    }
    fn word(&self, at: i64) -> u32 {
        u32::from_le_bytes([self.byte(at), self.byte(at + 1), self.byte(at + 2), self.byte(at + 3)])
    }
    fn float(&self, at: i64) -> bool {
        usize::try_from(at).is_ok_and(|at| self.floats.get(at))
    }
    fn changed(&self, at: i64) -> bool {
        usize::try_from(at).is_ok_and(|at| self.changed.get(at))
    }
    fn residual(&self, at: i64) -> Option<i64> {
        let at = u32::try_from(at).ok()?;
        self.residuals.binary_search_by_key(&at, |&(offset, _)| offset).ok().map(|n| self.residuals[n].1)
    }
    /// First offset of each anchor key.
    fn anchor(&self, key: u64) -> Option<u32> {
        self.keys
            .get_or_init(|| {
                let mut keys = HashMap::with_capacity(self.anchors.len());
                for &(offset, key) in &self.anchors {
                    keys.entry(key).or_insert(offset);
                }
                keys
            })
            .get(&key)
            .copied()
    }
}

/// Order-preserving map that keeps -0.0 distinct from +0.0.
fn ordered(bits: u32) -> i64 {
    if bits & 0x8000_0000 != 0 { -i64::from(bits & 0x7FFF_FFFF) - 1 } else { i64::from(bits) }
}

fn unordered(value: i64) -> Result<u32, Error> {
    if value >= 0 {
        u32::try_from(value).ok().filter(|v| *v <= 0x7FFF_FFFF).ok_or(Error)
    } else {
        let magnitude = u32::try_from(-(value + 1)).map_err(|_| Error)?;
        if magnitude > 0x7FFF_FFFF {
            return Err(Error);
        }
        Ok(magnitude | 0x8000_0000)
    }
}

/// Constant-velocity extrapolation of the baseline's own residual. Integer
/// arithmetic keeps encoder and decoder bit-identical.
fn predict(base: &View, at: i64, tick: u32) -> i64 {
    if base.span == 0 || !base.changed(at) {
        return 0;
    }
    let Some(residual) = base.residual(at) else { return 0 };
    let elapsed = tick.wrapping_sub(base.tick);
    if elapsed == 0 || elapsed > 1 << 16 {
        return 0;
    }
    let elapsed = elapsed.min(base.span.saturating_mul(4));
    let scaled = i128::from(residual) * i128::from(elapsed) / i128::from(base.span);
    i64::try_from(scaled).unwrap_or(0).clamp(-(1 << 40), 1 << 40)
}

#[derive(Default)]
struct Models {
    anchor_count: Int,
    anchor_run: Uint,
    anchor_same: Bit,
    anchor_skip: Bit,
    anchor_skip_len: Uint,
    anchor_shift: Int,
    anchor_gap: Uint,
    run: [Uint; 2],
    is_float: [[Bit; 2]; 2],
    float_changed: [[Bit; 2]; 2],
    byte_changed: [[[Bit; 2]; 2]; 2],
    residual: [Int; 2],
    xor: Byte,
}

/// Bytes, float offsets and anchors supplied by the native schema encoder.
pub struct Input<'a> {
    pub bytes: &'a [u8],
    /// Sorted, non-overlapping offsets; invalid entries are ignored.
    pub floats: &'a [u32],
    /// Sorted offsets with keys; out-of-order or out-of-range entries are ignored.
    pub anchors: &'a [(u32, u64)],
}

fn sanitize_anchors(input: &Input<'_>) -> Vec<(u32, u64)> {
    let mut anchors: Vec<(u32, u64)> = Vec::with_capacity(input.anchors.len());
    for &(offset, key) in input.anchors {
        if (offset as usize) < input.bytes.len() && anchors.last().is_none_or(|&(last, _)| offset > last) {
            anchors.push((offset, key));
        }
    }
    anchors
}

/// Anchors that keep the baseline's key order and the running shift cost
/// one run length; only exceptions are coded individually.
fn encode_anchors(e: &mut Encoder, m: &mut Models, anchors: &[(u32, u64)], base: &View) {
    m.anchor_count.encode(e, anchors.len() as i64 - base.anchors.len() as i64);
    let positions: HashMap<u64, usize> = base.anchors.iter().enumerate().rev().map(|(n, &(_, k))| (k, n)).collect();
    let (mut cursor, mut shift, mut previous) = (0usize, 0i64, 0u32);
    let mut k = 0usize;
    while k < anchors.len() {
        let mut run = 0usize;
        while let (Some(&(offset, key)), Some(&(base_offset, base_key))) = (anchors.get(k + run), base.anchors.get(cursor + run)) {
            if key != base_key || i64::from(offset) != i64::from(base_offset) + shift {
                break;
            }
            run += 1;
        }
        m.anchor_run.encode(e, run as u64);
        if run > 0 {
            previous = anchors[k + run - 1].0;
            cursor += run;
            k += run;
        }
        let Some(&(offset, key)) = anchors.get(k) else { break };
        let same = base.anchors.get(cursor).is_some_and(|&(_, b)| b == key);
        e.bit(&mut m.anchor_same, same);
        let matched = if same {
            Some(cursor)
        } else {
            let ahead = positions.get(&key).copied().filter(|&at| at > cursor);
            e.bit(&mut m.anchor_skip, ahead.is_some());
            if let Some(at) = ahead {
                m.anchor_skip_len.encode(e, (at - cursor - 1) as u64);
            }
            ahead
        };
        match matched {
            Some(at) => {
                let predicted = i64::from(base.anchors[at].0) + shift;
                m.anchor_shift.encode(e, i64::from(offset) - predicted);
                shift = i64::from(offset) - i64::from(base.anchors[at].0);
                cursor = at + 1;
            }
            None => {
                e.direct(key, 64);
                m.anchor_gap.encode(e, u64::from(offset - previous));
            }
        }
        previous = offset;
        k += 1;
    }
}

fn decode_anchors(d: &mut Decoder<'_>, m: &mut Models, len: usize, base: &View) -> Result<Vec<(u32, u64)>, Error> {
    let count = i64::try_from(base.anchors.len())
        .ok()
        .and_then(|n| n.checked_add(m.anchor_count.decode(d).ok()?))
        .and_then(|n| usize::try_from(n).ok())
        .ok_or(Error)?;
    if count > len {
        return Err(Error);
    }
    let mut anchors: Vec<(u32, u64)> = Vec::with_capacity(count);
    let (mut cursor, mut shift, mut previous) = (0usize, 0i64, 0u32);
    let push = |anchors: &mut Vec<(u32, u64)>, offset: u32, key: u64| -> Result<(), Error> {
        if offset as usize >= len || anchors.last().is_some_and(|&(last, _)| offset <= last) {
            return Err(Error);
        }
        anchors.push((offset, key));
        Ok(())
    };
    while anchors.len() < count {
        let run = usize::try_from(m.anchor_run.decode(d)?).map_err(|_| Error)?;
        if run > count - anchors.len() {
            return Err(Error);
        }
        for _ in 0..run {
            let &(base_offset, key) = base.anchors.get(cursor).ok_or(Error)?;
            let offset = u32::try_from(i64::from(base_offset) + shift).map_err(|_| Error)?;
            push(&mut anchors, offset, key)?;
            previous = offset;
            cursor += 1;
        }
        if anchors.len() == count {
            break;
        }
        let matched = if d.bit(&mut m.anchor_same) {
            Some(cursor)
        } else if d.bit(&mut m.anchor_skip) {
            Some(
                cursor
                    .checked_add(usize::try_from(m.anchor_skip_len.decode(d)?).map_err(|_| Error)?)
                    .and_then(|v| v.checked_add(1))
                    .ok_or(Error)?,
            )
        } else {
            None
        };
        let (offset, key) = match matched {
            Some(at) => {
                let &(base_offset, key) = base.anchors.get(at).ok_or(Error)?;
                let offset = i64::from(base_offset)
                    .checked_add(shift)
                    .and_then(|v| v.checked_add(m.anchor_shift.decode(d).ok()?))
                    .ok_or(Error)?;
                let offset = u32::try_from(offset).map_err(|_| Error)?;
                shift = i64::from(offset) - i64::from(base_offset);
                cursor = at + 1;
                (offset, key)
            }
            None => {
                let key = d.direct(64);
                let gap = u32::try_from(m.anchor_gap.decode(d)?).map_err(|_| Error)?;
                (previous.checked_add(gap).ok_or(Error)?, key)
            }
        };
        push(&mut anchors, offset, key)?;
        previous = offset;
    }
    Ok(anchors)
}

/// Re-aligns to the baseline at every anchor this unit boundary reached.
struct Aligner<'a> {
    anchors: &'a [(u32, u64)],
    next: usize,
}

/// Anchor keys carry a section in their top bits; this id marks a list header.
const SECTION_HEADER: u64 = 0xFFFF_FFFF;
const SECTION_SHIFT: u32 = 40;

impl Aligner<'_> {
    fn advance(&mut self, i: usize, base: &View, d: &mut i64) {
        while let Some(&(offset, key)) = self.anchors.get(self.next) {
            if offset as usize > i {
                break;
            }
            if let Some(base_offset) = base.anchor(key).or_else(|| self.sibling(base, key)) {
                *d = i64::from(base_offset) - i64::from(offset);
            }
            self.next += 1;
        }
    }

    /// A new list entry (journal event, joined player, spawned object) is
    /// compared against the baseline copy of its preceding sibling, or the
    /// section's first baseline entry: the same record layout with nearby
    /// serials, ticks and positions codes far smaller than raw bytes.
    fn sibling(&self, base: &View, key: u64) -> Option<u32> {
        let section = key >> SECTION_SHIFT;
        if key & SECTION_HEADER == SECTION_HEADER {
            return None;
        }
        let &(_, previous) = self.anchors.get(self.next.checked_sub(1)?)?;
        if previous >> SECTION_SHIFT != section {
            return None;
        }
        let previous_offset = base.anchor(previous)?;
        if previous & SECTION_HEADER != SECTION_HEADER {
            return Some(previous_offset);
        }
        let first = base.anchors.partition_point(|&(offset, _)| offset <= previous_offset);
        base.anchors.get(first).filter(|&&(_, k)| k >> SECTION_SHIFT == section && k & SECTION_HEADER != SECTION_HEADER).map(|&(offset, _)| offset)
    }
}

fn sanitize(input: &Input<'_>) -> BitSet {
    let mut floats = BitSet::with_len(input.bytes.len());
    let mut next = 0usize;
    for &offset in input.floats {
        let at = offset as usize;
        if at >= next && at + 4 <= input.bytes.len() {
            floats.set(at);
            next = at + 4;
        }
    }
    floats
}

/// True when the unit at `i` equals the aligned baseline unit, has the same
/// float framing, and the baseline did not change it either.
fn is_static(n: &[u8], n_floats: &BitSet, base: &View, i: usize, j: i64) -> bool {
    let Ok(ju) = usize::try_from(j) else { return false };
    if ju >= base.bytes.len() || base.changed.get(ju) {
        return false;
    }
    let float = base.floats.get(ju);
    if n_floats.get(i) != float {
        return false;
    }
    let len = if float { 4 } else { 1 };
    i + len <= n.len() && ju + len <= base.bytes.len() && n[i..i + len] == base.bytes[ju..ju + len]
}

/// Encodes `input` against `base` (an empty view for a keyframe) and returns
/// the view the decoder will reconstruct.
pub fn encode(e: &mut Encoder, input: &Input<'_>, base: &View, tick: u32) -> View {
    let n = input.bytes;
    let n_floats = sanitize(input);
    let anchors = sanitize_anchors(input);
    let mut m = Models::default();
    encode_anchors(e, &mut m, &anchors, base);
    let mut view = View {
        bytes: n.to_vec(),
        floats: BitSet::with_len(n.len()),
        changed: BitSet::with_len(n.len()),
        residuals: Vec::new(),
        tick,
        span: if base.bytes.is_empty() { 0 } else { tick.wrapping_sub(base.tick) },
        anchors: Vec::new(),
        keys: OnceCell::new(),
    };
    // A keyframe's bytes all differ from the empty baseline; recording that
    // as change history would disable static runs in the next message.
    let history = !base.bytes.is_empty();
    let mut aligner = Aligner { anchors: &anchors, next: 0 };
    let (mut i, mut d) = (0usize, 0i64);
    let (mut prev_changed, mut prev_float) = (false, false);
    while i < n.len() {
        aligner.advance(i, base, &mut d);
        let mut run = 0u64;
        while i < n.len() && is_static(n, &n_floats, base, i, i as i64 + d) {
            let float = n_floats.get(i);
            if float {
                view.floats.set(i);
            }
            i += if float { 4 } else { 1 };
            run += 1;
            aligner.advance(i, base, &mut d);
        }
        m.run[usize::from(prev_changed)].encode(e, run);
        if i == n.len() {
            break;
        }
        let j = i as i64 + d;
        let float = n_floats.get(i);
        e.bit(&mut m.is_float[usize::from(base.float(j))][usize::from(prev_float)], float);
        let base_changed = base.changed(j);
        let changed;
        if float {
            let old = base.word(j);
            let new = u32::from_le_bytes(n[i..i + 4].try_into().expect("float"));
            changed = old != new;
            e.bit(&mut m.float_changed[usize::from(base_changed)][usize::from(prev_changed)], changed);
            if changed {
                let residual = ordered(new) - ordered(old);
                let prediction = predict(base, j, tick);
                m.residual[usize::from(prediction != 0)].encode(e, residual - prediction);
                if history {
                    view.residuals.push((i as u32, residual));
                    view.changed.set_range(i, 4);
                }
            }
            view.floats.set(i);
            i += 4;
        } else {
            let old = base.byte(j);
            changed = old != n[i];
            e.bit(
                &mut m.byte_changed[usize::from(base_changed)][usize::from(prev_changed)][usize::from(prev_float)],
                changed,
            );
            if changed {
                m.xor.encode(e, (old ^ n[i]) - 1);
                if history {
                    view.changed.set(i);
                }
            }
            i += 1;
        }
        prev_float = float;
        prev_changed = changed;
    }
    view.anchors = anchors;
    view
}

pub fn decode(d_in: &mut Decoder<'_>, len: usize, base: &View, tick: u32) -> Result<View, Error> {
    let mut m = Models::default();
    let anchors = decode_anchors(d_in, &mut m, len, base)?;
    let mut view = View {
        bytes: Vec::with_capacity(len),
        floats: BitSet::with_len(len),
        changed: BitSet::with_len(len),
        residuals: Vec::new(),
        tick,
        span: if base.bytes.is_empty() { 0 } else { tick.wrapping_sub(base.tick) },
        anchors: Vec::new(),
        keys: OnceCell::new(),
    };
    let history = !base.bytes.is_empty();
    let mut aligner = Aligner { anchors: &anchors, next: 0 };
    let mut d = 0i64;
    let (mut prev_changed, mut prev_float) = (false, false);
    while view.bytes.len() < len {
        aligner.advance(view.bytes.len(), base, &mut d);
        let run = m.run[usize::from(prev_changed)].decode(d_in)?;
        for _ in 0..run {
            if view.bytes.len() >= len {
                return Err(Error);
            }
            let i = view.bytes.len();
            let j = usize::try_from(i as i64 + d).map_err(|_| Error)?;
            if j >= base.bytes.len() || base.changed.get(j) {
                return Err(Error);
            }
            let unit = if base.floats.get(j) { 4 } else { 1 };
            if i + unit > len || j + unit > base.bytes.len() {
                return Err(Error);
            }
            if unit == 4 {
                view.floats.set(i);
            }
            view.bytes.extend_from_slice(&base.bytes[j..j + unit]);
            aligner.advance(view.bytes.len(), base, &mut d);
        }
        let i = view.bytes.len();
        if i == len {
            break;
        }
        let j = i as i64 + d;
        let float = d_in.bit(&mut m.is_float[usize::from(base.float(j))][usize::from(prev_float)]);
        let base_changed = base.changed(j);
        let changed;
        if float {
            if i + 4 > len {
                return Err(Error);
            }
            let old = base.word(j);
            changed = d_in.bit(&mut m.float_changed[usize::from(base_changed)][usize::from(prev_changed)]);
            let new = if changed {
                let prediction = predict(base, j, tick);
                let residual = m.residual[usize::from(prediction != 0)]
                    .decode(d_in)?
                    .checked_add(prediction)
                    .ok_or(Error)?;
                if history {
                    view.residuals.push((i as u32, residual));
                    view.changed.set_range(i, 4);
                }
                unordered(ordered(old).checked_add(residual).ok_or(Error)?)?
            } else {
                old
            };
            view.floats.set(i);
            view.bytes.extend_from_slice(&new.to_le_bytes());
        } else {
            let old = base.byte(j);
            changed = d_in.bit(
                &mut m.byte_changed[usize::from(base_changed)][usize::from(prev_changed)][usize::from(prev_float)],
            );
            let new = if changed {
                let xor = m.xor.decode(d_in).checked_add(1).ok_or(Error)?;
                if history {
                    view.changed.set(i);
                }
                old ^ xor
            } else {
                old
            };
            view.bytes.push(new);
        }
        prev_float = float;
        prev_changed = changed;
    }
    view.anchors = anchors;
    Ok(view)
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;

    pub struct Lcg(pub u64);
    impl Lcg {
        pub fn next(&mut self) -> u64 {
            self.0 = self.0.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
            self.0 >> 11
        }
        pub fn unit(&mut self) -> f32 {
            (self.next() % 1_000_000) as f32 / 1_000_000.0
        }
    }

    /// Synthetic schema shaped like a native snapshot: a header, a roster of
    /// player records with floats, and variable-length object lists.
    pub struct World {
        pub tick: u32,
        pub players: Vec<[f32; 8]>,
        pub velocity: Vec<[f32; 3]>,
        pub objects: Vec<(u32, [f32; 6])>,
        pub score: u16,
    }

    impl World {
        pub fn new(rng: &mut Lcg, players: usize, objects: usize) -> Self {
            Self {
                tick: 100,
                players: (0..players).map(|_| std::array::from_fn(|_| rng.unit() * 40.0 - 20.0)).collect(),
                velocity: (0..players).map(|_| std::array::from_fn(|_| rng.unit() * 0.2 - 0.1)).collect(),
                objects: (0..objects).map(|n| (0xE000_0000 | n as u32, std::array::from_fn(|_| rng.unit() * 50.0))).collect(),
                score: 0,
            }
        }
        pub fn step(&mut self, rng: &mut Lcg) {
            self.tick += 1;
            for (n, (player, velocity)) in self.players.iter_mut().zip(&mut self.velocity).enumerate() {
                // Half the roster turns, so prediction is imperfect.
                if n % 2 == 1 {
                    let (sin, cos) = 0.08f32.sin_cos();
                    *velocity = [velocity[0] * cos - velocity[1] * sin, velocity[0] * sin + velocity[1] * cos, velocity[2]];
                    player[3] += 0.08;
                }
                for axis in 0..3 {
                    player[axis] += velocity[axis];
                }
                player[3] += 0.01;
            }
            if rng.next() % 50 == 0 {
                self.score += 1;
            }
            if rng.next() % 40 == 0 {
                let handle = 0xE100_0000 | (rng.next() as u32 & 0xFFFF);
                let at = (rng.next() as usize) % (self.objects.len() + 1);
                self.objects.insert(at, (handle, [rng.unit(); 6]));
            }
            if rng.next() % 45 == 0 && !self.objects.is_empty() {
                let at = (rng.next() as usize) % self.objects.len();
                self.objects.remove(at);
            }
        }
        pub fn encode_anchored(&self) -> (Vec<u8>, Vec<u32>, Vec<(u32, u64)>) {
            let (mut bytes, mut floats, mut anchors) = (Vec::new(), Vec::new(), Vec::new());
            bytes.extend_from_slice(&self.tick.to_le_bytes());
            bytes.extend_from_slice(&self.score.to_le_bytes());
            bytes.extend_from_slice(b"Variant name\0\0\0\0");
            for (n, player) in self.players.iter().enumerate() {
                anchors.push((bytes.len() as u32, 1 << 40 | n as u64));
                bytes.push(1);
                for &value in player {
                    floats.push(bytes.len() as u32);
                    bytes.extend_from_slice(&value.to_le_bytes());
                }
                bytes.extend_from_slice(&[0xFF; 12]);
                bytes.extend_from_slice(b"Profile and emblem data that never changes");
            }
            anchors.push((bytes.len() as u32, 2 << 40 | 0xFFFF_FFFF));
            bytes.extend_from_slice(&(self.objects.len() as u16).to_le_bytes());
            for (handle, values) in &self.objects {
                anchors.push((bytes.len() as u32, 2 << 40 | u64::from(*handle)));
                bytes.extend_from_slice(&handle.to_le_bytes());
                for &value in values {
                    floats.push(bytes.len() as u32);
                    bytes.extend_from_slice(&value.to_le_bytes());
                }
            }
            (bytes, floats, anchors)
        }
    }

    fn round_trip(input: &Input<'_>, base: &View, tick: u32) -> (View, usize) {
        let mut e = Encoder::default();
        let sent = encode(&mut e, input, base, tick);
        let bytes = e.finish();
        let mut d = Decoder::new(&bytes);
        let received = decode(&mut d, input.bytes.len(), base, tick).unwrap();
        assert_eq!(received.bytes, input.bytes);
        assert_eq!(received.bytes, sent.bytes);
        assert_eq!(received.floats, sent.floats);
        assert_eq!(received.changed, sent.changed);
        assert_eq!(received.residuals, sent.residuals);
        assert_eq!(received.anchors, sent.anchors);
        (received, bytes.len())
    }

    #[test]
    fn ordered_floats_are_monotone_and_exact() {
        for bits in [0u32, 0x8000_0000, 1, 0x8000_0001, 0x3F80_0000, 0xBF80_0000, 0x7F7F_FFFF, 0xFF7F_FFFF, 0x7FC0_0000] {
            assert_eq!(unordered(ordered(bits)).unwrap(), bits);
        }
        assert!(ordered((-1.0f32).to_bits()) < ordered((-0.0f32).to_bits()));
        assert!(ordered((-0.0f32).to_bits()) < ordered(0.0f32.to_bits()));
        assert!(ordered(1.0f32.to_bits()) < ordered(1.5f32.to_bits()));
    }

    #[test]
    fn moving_world_round_trips_and_steady_motion_is_cheap() {
        let mut rng = Lcg(3);
        let mut world = World::new(&mut rng, 16, 80);
        let (bytes, floats, anchors) = world.encode_anchored();
        let input = Input { bytes: &bytes, floats: &floats, anchors: &anchors };
        let (mut base, keyframe) = round_trip(&input, &View::default(), world.tick);
        let mut total = 0;
        for _ in 0..120 {
            world.step(&mut rng);
            let (bytes, floats, anchors) = world.encode_anchored();
            let (next, size) = round_trip(&Input { bytes: &bytes, floats: &floats, anchors: &anchors }, &base, world.tick);
            total += size;
            base = next;
        }
        let average = total / 120;
        eprintln!("moving world: keyframe {keyframe} bytes, average delta {average} bytes");
        // Sixteen players moving every tick plus list insertions/removals.
        assert!(average < keyframe / 8, "average {average} keyframe {keyframe}");
        assert!(average < 200, "average {average}");
    }

    #[test]
    fn anchors_keep_list_insertions_local() {
        let mut rng = Lcg(17);
        let mut world = World::new(&mut rng, 8, 300);
        let (bytes, floats, anchors) = world.encode_anchored();
        let (base, _) = round_trip(&Input { bytes: &bytes, floats: &floats, anchors: &anchors }, &View::default(), 1);
        world.objects.insert(3, (0xE200_0000, [1.0; 6]));
        world.objects.remove(150);
        let (bytes, floats, anchors) = world.encode_anchored();
        let (_, anchored) = round_trip(&Input { bytes: &bytes, floats: &floats, anchors: &anchors }, &base, 2);
        let (_, unanchored) = round_trip(&Input { bytes: &bytes, floats: &floats, anchors: &[] }, &base, 2);
        assert!(anchored < 80, "anchored {anchored}");
        assert!(unanchored > anchored * 10, "anchored {anchored} unanchored {unanchored}");
    }

    #[test]
    fn unchanged_state_costs_a_few_bytes() {
        let mut rng = Lcg(5);
        let world = World::new(&mut rng, 16, 200);
        let (bytes, floats, anchors) = world.encode_anchored();
        let input = Input { bytes: &bytes, floats: &floats, anchors: &anchors };
        let (base, _) = round_trip(&input, &View::default(), 1);
        let (base, _) = round_trip(&input, &base, 2);
        let (_, size) = round_trip(&input, &base, 3);
        // Range-coder flush plus one run length and one anchor run.
        assert!(size <= 6, "static size {size}");
    }

    #[test]
    fn older_baselines_and_arbitrary_markers_remain_exact() {
        let mut rng = Lcg(11);
        let mut world = World::new(&mut rng, 8, 30);
        let mut history = Vec::new();
        let (bytes, floats, anchors) = world.encode_anchored();
        history.push(round_trip(&Input { bytes: &bytes, floats: &floats, anchors: &anchors }, &View::default(), world.tick).0);
        for n in 0..60 {
            world.step(&mut rng);
            let (mut bytes, mut floats, mut anchors) = world.encode_anchored();
            if n % 7 == 0 {
                // Hostile framing: overlapping/out-of-range marks, odd tail,
                // duplicate/unsorted anchors and anchors without a baseline.
                floats.extend([1, 2, bytes.len() as u32 - 2, u32::MAX]);
                floats.sort_unstable();
                anchors.extend([(3, 7), (2, 7), (bytes.len() as u32 + 9, 1), (u32::MAX, 0)]);
                anchors.insert(0, (0, 2 << 40 | 0xFFFF_FFFF));
                bytes.push(0x5A);
            }
            if n % 11 == 0 {
                anchors.clear();
            }
            let base = &history[(rng.next() as usize) % history.len()];
            let view = round_trip(&Input { bytes: &bytes, floats: &floats, anchors: &anchors }, base, world.tick).0;
            history.push(view);
        }
    }

    #[test]
    fn corrupt_streams_fail_or_terminate() {
        let mut rng = Lcg(13);
        let world = World::new(&mut rng, 4, 10);
        let (bytes, floats, anchors) = world.encode_anchored();
        let base = round_trip(&Input { bytes: &bytes, floats: &floats, anchors: &anchors }, &View::default(), 1).0;
        for _ in 0..200 {
            let garbage: Vec<u8> = (0..(rng.next() % 40)).map(|_| rng.next() as u8).collect();
            let mut d = Decoder::new(&garbage);
            let _ = decode(&mut d, (rng.next() % 4096) as usize, &base, 2);
        }
    }
}
