//! Adaptive binary range coder for snapshot payloads.
//!
//! LZMA-style carry handling, but with 16-bit probabilities so that a highly
//! predictable decision (an unchanged byte, a still-static float) costs far
//! below one bit. Models start fresh for every message: no coder state ever
//! crosses a datagram, so loss of one message cannot desynchronize another.

const PROB_BITS: u32 = 16;
const PROB_ONE: u32 = 1 << PROB_BITS;
const MOVE_BITS: u32 = 4;
const TOP: u32 = 1 << 24;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Error;

/// Probability that the next bit is zero, in 1/65536 units. Adaptation keeps
/// it within [15, 65521], so neither symbol ever becomes impossible.
#[derive(Debug, Clone, Copy)]
pub struct Bit(u16);

impl Default for Bit {
    fn default() -> Self {
        Self((PROB_ONE / 2) as u16)
    }
}

impl Bit {
    fn update(&mut self, bit: bool) {
        let p = u32::from(self.0);
        self.0 = if bit { p - (p >> MOVE_BITS) } else { p + ((PROB_ONE - p) >> MOVE_BITS) } as u16;
    }
}

#[derive(Clone)]
pub struct Encoder {
    low: u64,
    range: u32,
    cache: u8,
    pending: u64,
    first: bool,
    out: Vec<u8>,
}

impl Default for Encoder {
    fn default() -> Self {
        Self { low: 0, range: u32::MAX, cache: 0, pending: 1, first: true, out: Vec::new() }
    }
}

/// Exact rollback point. Bytes already emitted are final (carries are held
/// back in `cache`), so truncation plus the scalar state restores the coder.
#[derive(Clone, Copy)]
pub struct Checkpoint {
    low: u64,
    range: u32,
    cache: u8,
    pending: u64,
    first: bool,
    len: usize,
}

impl Encoder {
    fn shift_low(&mut self) {
        if (self.low as u32) < 0xFF00_0000 || (self.low >> 32) != 0 {
            let carry = (self.low >> 32) as u8;
            let mut value = self.cache;
            loop {
                // The interval never exceeds the initial unit range, so the
                // leading byte is always zero; it is implied, not sent.
                if self.first {
                    self.first = false;
                } else {
                    self.out.push(value.wrapping_add(carry));
                }
                value = 0xFF;
                self.pending -= 1;
                if self.pending == 0 {
                    break;
                }
            }
            self.cache = (self.low >> 24) as u8;
        }
        self.pending += 1;
        self.low = (self.low & 0x00FF_FFFF) << 8;
    }

    fn normalize(&mut self) {
        while self.range < TOP {
            self.range <<= 8;
            self.shift_low();
        }
    }

    pub fn bit(&mut self, model: &mut Bit, bit: bool) {
        let bound = (self.range >> PROB_BITS) * u32::from(model.0);
        if bit {
            self.low += u64::from(bound);
            self.range -= bound;
        } else {
            self.range = bound;
        }
        model.update(bit);
        self.normalize();
    }

    /// Equiprobable bits, most significant first.
    pub fn direct(&mut self, value: u64, count: u32) {
        for shift in (0..count).rev() {
            self.range >>= 1;
            if (value >> shift) & 1 != 0 {
                self.low += u64::from(self.range);
            }
            self.normalize();
        }
    }

    pub fn checkpoint(&self) -> Checkpoint {
        Checkpoint {
            low: self.low,
            range: self.range,
            cache: self.cache,
            pending: self.pending,
            first: self.first,
            len: self.out.len(),
        }
    }

    pub fn rollback(&mut self, point: Checkpoint) {
        self.out.truncate(point.len);
        self.low = point.low;
        self.range = point.range;
        self.cache = point.cache;
        self.pending = point.pending;
        self.first = point.first;
    }

    /// Conservative byte count if the stream were finished now.
    pub fn estimate(&self) -> usize {
        self.out.len() + self.pending as usize + 4
    }

    pub fn finish(mut self) -> Vec<u8> {
        for _ in 0..5 {
            self.shift_low();
        }
        // The decoder reads zeros beyond the end, so trailing zeros are implied.
        while self.out.last() == Some(&0) {
            self.out.pop();
        }
        self.out
    }
}

pub struct Decoder<'a> {
    data: &'a [u8],
    at: usize,
    range: u32,
    code: u32,
}

impl<'a> Decoder<'a> {
    pub fn new(data: &'a [u8]) -> Self {
        let mut decoder = Self { data, at: 0, range: u32::MAX, code: 0 };
        for _ in 0..4 {
            decoder.code = (decoder.code << 8) | u32::from(decoder.next());
        }
        decoder
    }

    fn next(&mut self) -> u8 {
        let value = self.data.get(self.at).copied().unwrap_or(0);
        self.at = self.at.saturating_add(1);
        value
    }

    fn normalize(&mut self) {
        while self.range < TOP {
            self.range <<= 8;
            self.code = (self.code << 8) | u32::from(self.next());
        }
    }

    pub fn bit(&mut self, model: &mut Bit) -> bool {
        let bound = (self.range >> PROB_BITS) * u32::from(model.0);
        let bit = self.code >= bound;
        if bit {
            self.code -= bound;
            self.range -= bound;
        } else {
            self.range = bound;
        }
        model.update(bit);
        self.normalize();
        bit
    }

    pub fn direct(&mut self, count: u32) -> u64 {
        let mut value = 0u64;
        for _ in 0..count {
            self.range >>= 1;
            let bit = self.code >= self.range;
            if bit {
                self.code -= self.range;
            }
            value = (value << 1) | u64::from(bit);
            self.normalize();
        }
        value
    }

    /// Implied trailing zeros make reads past the end legitimate; callers
    /// bound work by declared lengths and verify the reconstructed digest.
    pub fn consumed(&self) -> usize {
        self.at
    }
}

/// Adaptive Exp-Golomb code for unsigned values below 2^63.
#[derive(Clone)]
pub struct Uint {
    prefix: [Bit; 40],
    mantissa: [[Bit; 4]; 40],
}

impl Default for Uint {
    fn default() -> Self {
        Self { prefix: [Bit::default(); 40], mantissa: [[Bit::default(); 4]; 40] }
    }
}

const ADAPTIVE_MANTISSA: u32 = 2;

impl Uint {
    pub fn encode(&mut self, e: &mut Encoder, value: u64) {
        debug_assert!(value < 1 << 63);
        let shifted = value + 1;
        let width = 63 - shifted.leading_zeros();
        for k in 0..width {
            e.bit(&mut self.prefix[k.min(39) as usize], true);
        }
        if width < 63 {
            e.bit(&mut self.prefix[width.min(39) as usize], false);
        }
        let adaptive = width.min(ADAPTIVE_MANTISSA);
        let tree = &mut self.mantissa[width.min(39) as usize];
        let mut node = 1usize;
        for k in 0..adaptive {
            let bit = (shifted >> (width - 1 - k)) & 1 != 0;
            e.bit(&mut tree[node], bit);
            node = node * 2 + usize::from(bit);
        }
        let rest = width - adaptive;
        e.direct(shifted & ((1u64 << rest) - 1), rest);
    }

    pub fn decode(&mut self, d: &mut Decoder<'_>) -> Result<u64, Error> {
        let mut width = 0u32;
        while width < 63 && d.bit(&mut self.prefix[width.min(39) as usize]) {
            width += 1;
        }
        let adaptive = width.min(ADAPTIVE_MANTISSA);
        let tree = &mut self.mantissa[width.min(39) as usize];
        let mut node = 1usize;
        let mut shifted = 1u64;
        for _ in 0..adaptive {
            let bit = d.bit(&mut tree[node]);
            node = node * 2 + usize::from(bit);
            shifted = (shifted << 1) | u64::from(bit);
        }
        let rest = width - adaptive;
        shifted = (shifted << rest) | d.direct(rest);
        Ok(shifted - 1)
    }
}

/// Zero flag, sign and adaptive magnitude.
#[derive(Clone, Default)]
pub struct Int {
    zero: Bit,
    sign: Bit,
    magnitude: Uint,
}

impl Int {
    pub fn encode(&mut self, e: &mut Encoder, value: i64) {
        e.bit(&mut self.zero, value != 0);
        if value != 0 {
            e.bit(&mut self.sign, value < 0);
            self.magnitude.encode(e, value.unsigned_abs() - 1);
        }
    }

    pub fn decode(&mut self, d: &mut Decoder<'_>) -> Result<i64, Error> {
        if !d.bit(&mut self.zero) {
            return Ok(0);
        }
        let negative = d.bit(&mut self.sign);
        let magnitude = i64::try_from(self.magnitude.decode(d)?).map_err(|_| Error)? + 1;
        Ok(if negative { -magnitude } else { magnitude })
    }
}

/// Adaptive eight-bit symbol tree.
#[derive(Clone)]
pub struct Byte([Bit; 256]);

impl Default for Byte {
    fn default() -> Self {
        Self([Bit::default(); 256])
    }
}

impl Byte {
    pub fn encode(&mut self, e: &mut Encoder, value: u8) {
        let mut node = 1usize;
        for shift in (0..8).rev() {
            let bit = (value >> shift) & 1 != 0;
            e.bit(&mut self.0[node], bit);
            node = node * 2 + usize::from(bit);
        }
    }

    pub fn decode(&mut self, d: &mut Decoder<'_>) -> u8 {
        let mut node = 1usize;
        for _ in 0..8 {
            node = node * 2 + usize::from(d.bit(&mut self.0[node]));
        }
        (node - 256) as u8
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Lcg(u64);
    impl Lcg {
        fn next(&mut self) -> u64 {
            self.0 = self.0.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
            self.0 >> 11
        }
    }

    #[test]
    fn mixed_symbols_round_trip_exactly() {
        let mut rng = Lcg(7);
        let values: Vec<(u8, u64, i64, u8)> = (0..5000)
            .map(|n| {
                let width = (rng.next() % 64) as u32;
                let magnitude = rng.next() & ((1u64 << width.min(62)) - 1);
                let signed = if n % 3 == 0 { -(magnitude as i64) } else { magnitude as i64 };
                ((rng.next() % 16 == 0) as u8, magnitude, signed, rng.next() as u8)
            })
            .collect();
        let (mut flag, mut uint, mut int, mut byte) = (Bit::default(), Uint::default(), Int::default(), Byte::default());
        let mut e = Encoder::default();
        for &(f, u, i, b) in &values {
            e.bit(&mut flag, f != 0);
            uint.encode(&mut e, u);
            int.encode(&mut e, i);
            byte.encode(&mut e, b);
            e.direct(u64::from(b), 8);
        }
        let bytes = e.finish();
        let (mut flag, mut uint, mut int, mut byte) = (Bit::default(), Uint::default(), Int::default(), Byte::default());
        let mut d = Decoder::new(&bytes);
        for &(f, u, i, b) in &values {
            assert_eq!(d.bit(&mut flag), f != 0);
            assert_eq!(uint.decode(&mut d).unwrap(), u);
            assert_eq!(int.decode(&mut d).unwrap(), i);
            assert_eq!(byte.decode(&mut d), b);
            assert_eq!(d.direct(8), u64::from(b));
        }
    }

    #[test]
    fn predictable_decisions_cost_far_below_one_bit() {
        let mut model = Bit::default();
        let mut e = Encoder::default();
        for _ in 0..100_000 {
            e.bit(&mut model, false);
        }
        assert!(e.finish().len() < 12);
    }

    #[test]
    fn rollback_restores_an_exact_prefix() {
        let mut uint = Uint::default();
        let mut e = Encoder::default();
        for n in 0..200 {
            uint.encode(&mut e, n * 7);
        }
        let saved_model = uint.clone();
        let point = e.checkpoint();
        for n in 0..300 {
            uint.encode(&mut e, n * 1_000_003);
        }
        e.rollback(point);
        uint = saved_model;
        uint.encode(&mut e, 42);
        let bytes = e.finish();
        let mut uint = Uint::default();
        let mut d = Decoder::new(&bytes);
        for n in 0..200 {
            assert_eq!(uint.decode(&mut d).unwrap(), n * 7);
        }
        assert_eq!(uint.decode(&mut d).unwrap(), 42);
    }

    #[test]
    fn hostile_input_terminates_without_panicking() {
        let mut rng = Lcg(99);
        for _ in 0..200 {
            let bytes: Vec<u8> = (0..(rng.next() % 64)).map(|_| rng.next() as u8).collect();
            let mut d = Decoder::new(&bytes);
            let (mut uint, mut int, mut byte) = (Uint::default(), Int::default(), Byte::default());
            for _ in 0..64 {
                let _ = uint.decode(&mut d);
                let _ = int.decode(&mut d);
                byte.decode(&mut d);
            }
        }
    }
}
